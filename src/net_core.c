/**
 * KSNet mesh core module
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config/config.h"

#include <ev.h>
#ifdef HAVE_MINGW
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
typedef int socklen_t;
#else
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

#include "ev_mgr.h"
#include "utils/utils.h"

// Constants
const char *localhost = "127.0.0.1";
#define PACKET_HEADER_ADD_SIZE 2    // Sizeof from length + Sizeof command

// Local functions
void host_cb(EV_P_ ev_io *w, int revents);
int ksnCoreBind(ksnCoreClass *kc);
void *ksnCoreCreatePacket(ksnCoreClass *kc, uint8_t cmd, const void *data, size_t data_len, size_t *packet_len);
int ksnCoreParsePacket(void *packet, size_t packet_len, ksnCorePacketData *recv_data);
int send_cmd_connected_cb(ksnetArpClass *ka, char *name, ksnet_arp_data *arp_data, void *data);
int send_cmd_disconnect_cb(ksnetArpClass *ka, char *name, ksnet_arp_data *arp_data, void *data);

/**
 * Encrypt packet and sent to
 *
 * @param ke
 * @param name
 * @param port
 * @param addr
 * @return
 */
#if KSNET_CRYPT
#define sendto_encrypt(kc, DATA, D_LEN) \
    { \
        if(((ksnetEvMgrClass*)kc->ke)->ksn_cfg.crypt_f) { \
            size_t data_len; \
            char *buffer = NULL; /*[KSN_BUFFER_DB_SIZE];*/ \
            void *data = ksnEncryptPackage(kc->kcr, DATA, D_LEN, buffer, &data_len); \
            retval = sendto(kc->fd, data, data_len, 0, \
                                (struct sockaddr *)&remaddr, addrlen); \
            free(data); \
        } \
        else { \
            retval = sendto(kc->fd, DATA, D_LEN, 0, \
                                (struct sockaddr *)&remaddr, addrlen); \
        } \
    }
#else
#define sendto_encrypt(kc, DATA, D_LEN) \
    retval = sendto(kc->fd, DATA, D_LEN, 0, \
                        (struct sockaddr *)&remaddr, addrlen);
#endif

/**
 * Initialize ksnet core. Create socket FD and Bind ksnet UDP client/server
 *
 * @param name Host name
 * @param port Host port. 0 - create system dependent port number
 * @param addr Host IP. NULL - listen at all IPs
 *
 * @return Pointer to ksnCoreClass or NULL if error
 */
ksnCoreClass *ksnCoreInit(void* ke, char *name, int port, char* addr) {

    ksnCoreClass *kc = malloc(sizeof(ksnCoreClass));

    kc->ke = ke;
    kc->name = strdup(name);
    kc->name_len = strlen(name) + 1;
    kc->port = port;
    if(addr != NULL) kc->addr = strdup(addr);
    else kc->addr = addr;
    kc->last_check_event = 0;

    kc->ka = ksnetArpInit(ke);
    kc->kco = ksnCommandInit(kc);
    #if KSNET_CRYPT
    kc->kcr = ksnCryptInit();
    #endif

    // Create and bind host socket
    if(ksnCoreBind(kc)) {

        ksnCoreDestroy(kc);
        return NULL;
    }

    // Add host socket to the event manager
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wstrict-aliasing"
    ev_io_init(&kc->host_w, host_cb, kc->fd, EV_READ);
    kc->host_w.data = kc;
    ev_io_start(EV_DEFAULT_ &kc->host_w);
    #pragma GCC diagnostic pop

    return kc;
}

/**
 * Close socket and free memory
 *
 * @param kc Pointer to ksnCore class object
 */
void ksnCoreDestroy(ksnCoreClass *kc) {

    if(kc != NULL) {

        ksnetEvMgrClass *ke = kc->ke;

        // Send disconnect to all
        ksnetArpGetAll((kc)->ka, send_cmd_disconnect_cb, NULL);

        close((kc)->fd);
        free((kc)->name);
        if((kc)->addr != NULL) free((kc)->addr);
        ksnetArpDestroy((kc)->ka);
        ksnCommandDestroy((kc)->kco);
        #if KSNET_CRYPT
        ksnCryptDestroy((kc)->kcr);
        #endif
        free(kc);
        ke->kc = NULL;

        #ifdef HAVE_MINGW
        WSACleanup();
        #endif
    }
}

/**
 * Create and bind UDP socket for client/server
 *
 * @param kc
 * @return
 */
int ksnCoreBind(ksnCoreClass *kc) {

    struct sockaddr_in addr;	/* our address */

    #ifdef HAVE_MINGW
    // Request Winsock version 2.2
    int retval;
    if ((retval = WSAStartup(0x202, &kc->wsaData)) != 0) {
        fprintf(stderr,"Server: WSAStartup() failed with error %d\n", retval);
        WSACleanup();
        return -1;
    }
    #endif

    // Create a UDP socket
    if((kc->fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("cannot create socket\n");
        return -1;
    }

    // Bind the socket to any valid IP address and a specific port */
    memset((char *)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(kc->port);

    if(bind(kc->fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        return -2;
    }

    printf("Start listen at port %d\n", kc->port);

    return 0;
}

/**
 * Send data to remote peer IP:Port
 *
 * @param kc Pointer to KSNet core class object
 * @param addr IP address of remote peer
 * @param port Port of remote peer
 * @param cmd Command ID
 * @param data Pointer to data
 * @param data_len Data size
 *
 * @return
 */
int ksnCoreSendto(ksnCoreClass *kc, char *addr, int port, uint8_t cmd,
                  void *data, size_t data_len) {

    #ifdef DEBUG_KSNET
    ksnet_printf( & ((ksnetEvMgrClass*)kc->ke)->ksn_cfg, DEBUG_VV,
                 "Net core: ksnCoreSendto %s:%d %d \n", addr, port, data_len);
    #endif


    int retval;

    struct sockaddr_in remaddr;         // remote address
    const socklen_t addrlen = sizeof(remaddr);// length of addresses

    memset((char *) &remaddr, 0, addrlen);
    remaddr.sin_family = AF_INET;
    remaddr.sin_port = htons(port);
    #ifndef HAVE_MINGW
    if(inet_aton(addr, &remaddr.sin_addr) == 0) {
            //fprintf(stderr, "inet_aton() failed\n");
            return(-2);
    }
    #else
    remaddr.sin_addr.s_addr = inet_addr(addr);
    #endif

    // Create packet
    size_t packet_len;
//printf("ksnCoreSendto, data_len: %d\n", data_len);
    void *packet = ksnCoreCreatePacket(kc, cmd, data, data_len, &packet_len);

    /* Divide packet to small parts

    const int MTU = 1600;
    int MAX_UDP_DATA = MTU - 46 - sizeof(uint16_t); // 46 - UDP Header, sizeof(uint16_t) - packet length
    MAX_UDP_DATA -= MAX_UDP_DATA % 16; // The encryption bloc = 16

    !!! MSG_MORE does not work under MAC OS
    #ifndef MSG_MORE
    #define MSG_MORE 0
    #endif

    // Send packet longe than MTU with MSG_MORE flag
    if(packet_len > MAX_UDP_DATA) {

        size_t ptr = 0, len = MAX_UDP_DATA;

        // Send long data with MSG_MORE flag
        if(MSG_MORE) {

            do {
                sendto_encrypt(kc, packet + ptr, len);

                ptr += len;
                len = packet_len - ptr;
                if(len > MAX_UDP_DATA) len = MAX_UDP_DATA;
            }
            while(len);

            // Send empty data without MSG_MORE flag
            retval = sendto(kc->fd, NULL_STR, 0, 0,
                                (struct sockaddr *)&remaddr, addrlen);

            // Test of sending data with MSG_MORE flag
            //retval = sendto(kc->fd, packet + ptr, len, MSG_MORE,
            //                    (struct sockaddr *)&remaddr, addrlen);
            //retval = sendto(kc->fd, " - with add", 7, MSG_MORE,
            //                    (struct sockaddr *)&remaddr, addrlen);
            //retval = sendto(kc->fd, "", 0, 0,
            //                    (struct sockaddr *)&remaddr, addrlen);
        }

        // Split long packet to several compatible
        else {
            uint8_t buf[MAX_UDP_DATA];
            size_t header_len = packet_len - data_len;

            size_t max_data_len = MAX_UDP_DATA - header_len;
            memcpy(buf, packet, header_len);
            len = max_data_len;

            do {
                memcpy(buf + header_len, data + ptr, len);
                sendto_encrypt(kc, buf, len + header_len);

                ptr += len;
                len = data_len - ptr;
                if(len > max_data_len) len = max_data_len;
            }
            while(len);
        }
    }
    // Send packet less or equal than MTU
    else
    */

    // Encrypt and send packet
    sendto_encrypt(kc, packet, packet_len);

    // Free packet
    free(packet);

    // Set last host event time
    ksnCoreSetEventTime(kc);

    return retval;
}

/**
 * Send command by name to peer
 *
 * @param kc
 * @param to
 * @param cmd
 * @param data
 * @param data_len
 * @return
 */
ksnet_arp_data *ksnCoreSendCmdto(ksnCoreClass *kc, char *to, uint8_t cmd,
                                 void *data, size_t data_len) {

    ksnet_arp_data *arp = ksnetArpGet(kc->ka, to);

    if(arp != NULL) {

        ksnCoreSendto(kc, arp->addr, arp->port, cmd, data, data_len);
    }

    return arp;
}

/**
 * Create ksnet packet
 *
 * @param kc Pointer to ksnCore Class object
 * @param cmd Command ID
 * @param data Pointer to data
 * @param data_len Data length
 * @param [out] packet_len Pointer to packet length
 *
 * @return Pointer to packet. Should be free after use.
 */
void *ksnCoreCreatePacket(ksnCoreClass *kc, uint8_t cmd, const void *data,
                          size_t data_len, size_t *packet_len) {

    size_t ptr = 0;
    *packet_len = kc->name_len + data_len + PACKET_HEADER_ADD_SIZE;
    void *packet = malloc(*packet_len);

    // Copy packet data
    *((uint8_t *)packet) = kc->name_len; ptr += sizeof(uint8_t);
    memcpy(packet + ptr, kc->name, kc->name_len); ptr += kc->name_len;
    *((uint8_t *) packet +ptr) = cmd; ptr += sizeof(uint8_t);
    memcpy(packet + ptr, data, data_len); ptr += data_len;

    return packet;
}

/**
 * Parse received data to ksnCoreRecvData structure
 *
 * @param packet Received packet
 * @param packet_len Received packet length
 * @param rd Pointer to ksnCoreRecvData structure
 */
int ksnCoreParsePacket(void *packet, size_t packet_len, ksnCorePacketData *rd) {

    size_t ptr = 0;
    int packed_valid = 0;

    // Raw Packet buffer and length
    rd->raw_data = packet;
    rd->raw_data_len = packet_len;

    rd->from_len = *((uint8_t*)packet); ptr += sizeof(rd->from_len); // From length
    if(rd->from_len &&
       rd->from_len + PACKET_HEADER_ADD_SIZE <= packet_len &&
       *((char*)(packet + ptr + rd->from_len - 1)) == '\0') {

        rd->from = (char*)(packet + ptr); ptr += rd->from_len; // From pointer
        rd->cmd = *((uint8_t*)(packet + ptr)); ptr += sizeof(rd->cmd); // Command ID
        rd->data = packet + ptr; // Data pointer
        rd->data_len = packet_len - ptr; // Data length

        packed_valid = 1;
    }

    return packed_valid;
}

/**
 * Send command connected to peer
 *
 * @param ka Pointer to ksnetArpClass
 * @param child_peer Child peer name
 * @param arp_data Child arp data
 * @param data Pointer to ksnCoreRecvData (included new peer name)
 */
int send_cmd_connected_cb(ksnetArpClass *ka, char *child_peer,
                      ksnet_arp_data *arp_data, void *data) {

    #define rd ((ksnCorePacketData*) data)
    #define new_peer rd->from

    if(strcmp(child_peer, new_peer)) {

        // Send child to new peer
        ksnCommandSendCmdConnect( ((ksnetEvMgrClass*) ka->ke)->kc->kco, new_peer,
                child_peer, arp_data->addr, arp_data->port );

        // Send new peer to child
        ksnCommandSendCmdConnect( ((ksnetEvMgrClass*) ka->ke)->kc->kco, child_peer,
                new_peer, rd->addr, rd->port );
    }

    return 0;

    #undef new_peer
    #undef rd
}

/**
 * Send command disconnect to peer
 *
 * @param ka
 * @param name
 * @param arp_data
 * @param data
 */
int send_cmd_disconnect_cb(ksnetArpClass *ka, char *name,
                            ksnet_arp_data *arp_data, void *data) {

    ksnCoreSendto(((ksnetEvMgrClass*) ka->ke)->kc, arp_data->addr,
                  arp_data->port, CMD_DISCONNECTED, NULL, 0);

    return 0;
}

/**
 * ENet host fd read event callback
 *
 * @param loop
 * @param w
 * @param revents
 */
void host_cb(EV_P_ ev_io *w, int revents) {

    ksnCoreClass *kc = w->data;             // ksnCore Class object
    ksnetEvMgrClass *ke = kc->ke;           // ksnetEvMgr Class object

    struct sockaddr_in remaddr;             // remote address
    socklen_t addrlen = sizeof(remaddr);    // length of addresses
    unsigned char buf[KSN_BUFFER_DB_SIZE];  // Message buffer
    int recvlen;                            // # bytes received

    // Receive data
    recvlen = recvfrom(kc->fd, (char*)buf, KSN_BUFFER_DB_SIZE, 0,
            (struct sockaddr *)&remaddr, &addrlen);

    #ifdef DEBUG_KSNET
    ksnet_printf(&ke->ksn_cfg, DEBUG_VV, "Net core: host_cb %d \n", recvlen);
    #endif

    // Data received
    if(recvlen > 0) {

        void *data; // Decrypted packet data
        size_t data_len; // Decrypted packet data length

        // Decrypt package
        #if KSNET_CRYPT
        if(ke->ksn_cfg.crypt_f) {
            data = ksnDecryptPackage(kc->kcr, buf, recvlen, &data_len);
        }
        
        // Use packet without decryption
        else {
        #endif
            data = buf;
            data_len = recvlen;
        #if KSNET_CRYPT
        }
        #endif

        /****************************************************/
        /* KSNet core packet structure                      */
        /****************************************************/
        /* from length *  from         * command *   data   */
        /****************************************************/
        /* 1 byte      *  from length  * 1 byte  *  N byte  */
        /****************************************************/
        //
        // from length - uint8_t: length of from (with 0 at the end)
        // from - char[]: from name
        // data - uint8_t[]: data
        // data_length = packet_length - 1 - from_length
        //

        // Parse ksnet packet and Fill ksnet receive data structure
        ksnet_arp_data arp;
        ksnCorePacketData rd;
        memset(&rd, 0, sizeof(rd));
        int event = EV_K_RECEIVED, command_processed = 0;

        // Remote peer address and peer
        rd.addr = inet_ntoa(remaddr.sin_addr); // IP to string
        rd.port = ntohs(remaddr.sin_port); // Port to integer

        // Parse packet and check if it valid
        if(!ksnCoreParsePacket(data, data_len, &rd)) {
            event = EV_K_RECEIVED_WRONG;
        }

        // Check ARP Table and add peer if not present
        else {

            // Check new peer connected
            if((rd.arp = ksnetArpGet(kc->ka, rd.from)) == NULL) {

                rd.arp = &arp;
                int mode = 0;

                // Check r-host connected
                if(!ke->ksn_cfg.r_host_name[0] &&
                   !strcmp(ke->ksn_cfg.r_host_addr, rd.addr) &&
                   ke->ksn_cfg.r_port == rd.port) {

                    strncpy(ke->ksn_cfg.r_host_name, rd.from,
                            sizeof(ke->ksn_cfg.r_host_name));

                    mode = 1;
                }

                // Add peer to ARP Table
                memset(rd.arp, 0, sizeof(*rd.arp));
                strncpy(rd.arp->addr, rd.addr, sizeof(rd.arp->addr));
                rd.arp->port = rd.port;
                rd.arp->mode = mode;
                ksnetArpAdd(kc->ka, rd.from, rd.arp);

                // Send child to new peer and new peer to child
                ksnetArpGetAll(kc->ka, send_cmd_connected_cb, &rd);

                // Send event callback
                if(ke->event_cb != NULL)
                    ke->event_cb(ke, EV_K_CONNECTED, (void*)&rd, sizeof(rd));
            }

            // Set last activity time
            rd.arp->last_acrivity = ksnetEvMgrGetTime(ke);

            // Check & process command
            command_processed = ksnCommandCheck(kc->kco, &rd);
        }

        // Send event to User level
        if(!command_processed) {

            // Send event callback
            if(ke->event_cb != NULL)
                ke->event_cb(ke, event, (void*)&rd, sizeof(rd));

        }
    }

    // Socket disconnected
    else {
        printf("Disconnected ...\n");
    }

    // Set last host event time
    ksnCoreSetEventTime(kc);
}
