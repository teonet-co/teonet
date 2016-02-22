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
#include "net_split.h"
#include "net_multi.h"
#include "tr-udp.h"
#include "utils/utils.h"
#include "utils/rlutil.h"
#include "tr-udp_.h"

// Constants
const char *localhost = "127.0.0.1";
#define PACKET_HEADER_ADD_SIZE 2    // Sizeof from length + Sizeof command

#define MODULE _ANSI_GREEN "net_core" _ANSI_GREY
#define kev ((ksnetEvMgrClass *)ksn_cfg->ke)

// Local functions
void host_cb(EV_P_ ev_io *w, int revents);
int ksnCoreBind(ksnCoreClass *kc);
void *ksnCoreCreatePacket(ksnCoreClass *kc, uint8_t cmd, const void *data, size_t data_len, size_t *packet_len);
int send_cmd_connected_cb(ksnetArpClass *ka, char *name, ksnet_arp_data *arp_data, void *data);
int send_cmd_disconnect_cb(ksnetArpClass *ka, char *name, ksnet_arp_data *arp_data, void *data);
int send_cmd_disconnect_peer_cb(ksnetArpClass *ka, char *name, ksnet_arp_data *arp_data, void *data);

// UDP / UDT functions
#define ksn_socket(domain, type, protocol) \
            socket(domain, type, protocol)

#define ksn_bind(fd, addr, addr_len) \
            bind(fd, addr, addr_len)

#define ksn_sendto(tu, cmd, fd, data, data_len, flags, remaddr, addrlen) \
            ksnTRUDPsendto(tu, 0, 0, 0, cmd, fd, data, data_len, flags, remaddr, addrlen)

#define ksn_recvfrom(ku, fd, buf, buf_len, flags, remaddr, addrlen) \
            ksnTRUDPrecvfrom(ku, fd, buf, buf_len, flags, remaddr, addrlen)

/**
 * Encrypt packet and sent to
 *
 * @param kc
 * @param cmd
 * @param DATA
 * @param D_LEN
 * @return
 */
#if KSNET_CRYPT
#define sendto_encrypt(kc, cmd, DATA, D_LEN) \
    { \
        if(((ksnetEvMgrClass*)kc->ke)->ksn_cfg.crypt_f) { \
            size_t data_len; \
            char *buffer = NULL; /*[KSN_BUFFER_DB_SIZE];*/ \
            void *data = ksnEncryptPackage(kc->kcr, DATA, D_LEN, buffer, &data_len); \
            retval = ksn_sendto(kc->ku, cmd, kc->fd, data, data_len, 0, \
                                (struct sockaddr *)&remaddr, addrlen); \
            free(data); \
        } \
        else { \
            retval = ksn_sendto(kc->ku, cmd, kc->fd, DATA, D_LEN, 0, \
                                (struct sockaddr *)&remaddr, addrlen); \
        } \
    }
#else
#define sendto_encrypt(kc, cmd, DATA, D_LEN) \
    retval = ksn_sendto(kc->ku, cmd, kc->fd, DATA, D_LEN, 0, \
                        (struct sockaddr *)&remaddr, addrlen);
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

/**
 * Initialize ksnet core. Create socket FD and Bind ksnet UDP client/server
 *
 * @param ke Pointer to ksnetEvMgrClass
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

    ((ksnetEvMgrClass*)ke)->kc = kc;
    kc->ku = ksnTRUDPinit(kc);
    kc->ka = ksnetArpInit(ke);
    kc->kco = ksnCommandInit(kc);
    #if KSNET_CRYPT
    kc->kcr = ksnCryptInit(ke);
    #endif

    // Create and bind host socket
    if(ksnCoreBind(kc)) {

        return NULL;
    }

    // Change this host port number to port changed in ksnCoreBind function
    ksnetArpSetHostPort(kc->ka, ((ksnetEvMgrClass*)ke)->ksn_cfg.host_name, kc->port);

    // Add host socket to the event manager
    if(!((ksnetEvMgrClass*)ke)->ksn_cfg.r_tcp_f) {

        ev_io_init(&kc->host_w, host_cb, kc->fd, EV_READ);
        kc->host_w.data = kc;
        ev_io_start(((ksnetEvMgrClass*)ke)->ev_loop, &kc->host_w);
    }

    return kc;
}

#pragma GCC diagnostic pop

/**
 * Close socket and free memory
 *
 * @param kc Pointer to ksnCore class object
 */
void ksnCoreDestroy(ksnCoreClass *kc) {

    if(kc != NULL) {

        ksnetEvMgrClass *ke = kc->ke;

        // Send disconnect from peer connected to TCP Proxy command to all
        ksnetArpGetAll(kc->ka, send_cmd_disconnect_peer_cb, NULL);
        
        // Send disconnect to all
        ksnetArpGetAll(kc->ka, send_cmd_disconnect_cb, NULL);
        
        // Stop watcher
        ev_io_stop(((ksnetEvMgrClass*)ke)->ev_loop, &kc->host_w);

        close(kc->fd);
        free(kc->name);
        if(kc->addr != NULL) free(kc->addr);
        ksnetArpDestroy(kc->ka);
        ksnCommandDestroy(kc->kco);
        ksnTRUDPDestroy(kc->ku);
        #if KSNET_CRYPT
        ksnCryptDestroy(kc->kcr);
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
 * @param[in] ksn_cfg Pointer to teonet configuration: ksnet_cfg
 * @param[out] port Pointer to Port number
 * @return File descriptor or error if return value < 0
 */
int ksnCoreBindRaw(ksnet_cfg *ksn_cfg, int *port) {
    
    int i, sd;
    struct sockaddr_in addr;	// Our address 
    
    // Create a UDP socket
    if((sd = ksn_socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("cannot create socket\n");
        return -1;
    }
    
    memset((char *)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind the socket to any valid IP address and a specific port, increment 
    // port if busy 
    for(i=0;;) {
        
        addr.sin_port = htons(*port);

        if(ksn_bind(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {

//            ksnet_printf(ksn_cfg, MESSAGE, 
//                    "%sNet core:%s Can't bind on port %d, "
//                    "try next port number ...%s\n", 
//                    ANSI_GREEN, ANSI_GREY,
//                    *port, 
//                    ANSI_NONE);
            ksn_printf(kev, MODULE, MESSAGE,
                    "Can't bind on port %d, try next port number ...\n", 
                    *port);
                    
            (*port)++;
            if(ksn_cfg->port_inc_f && i++ < NUMBER_TRY_PORTS) continue;
            else return -2;
        }
        else break;
    }
    
    return sd;
}

/**
 * Create and bind UDP socket for client/server
 *
 * @param kc Pointer to ksnCoreClass
 * @return 0 if successfully created and bind
 */
int ksnCoreBind(ksnCoreClass *kc) {

    int fd;
    ksnet_cfg *ksn_cfg = & ((ksnetEvMgrClass*)kc->ke)->ksn_cfg;
    
    #ifdef DEBUG_KSNET
//    ksnet_printf(ksn_cfg, MESSAGE, 
//                    "%sNet core:%s Create UDP client/server at port %d ...\n", 
//                    ANSI_GREEN, ANSI_NONE, kc->port);
    ksn_printf(kev, MODULE, MESSAGE, 
            "Create UDP client/server at port %d ...\n", kc->port);
    #endif
    
    if((fd = ksnCoreBindRaw(ksn_cfg, &kc->port)) > 0) {

        kc->fd = fd;
        #ifdef DEBUG_KSNET
        ksnet_printf(ksn_cfg, MESSAGE, 
                "%sNet core:%s Start listen at port %d, socket fd %d\n", 
                ANSI_GREEN, ANSI_NONE, kc->port, kc->fd);
        #endif

        // Set non block mode
        // \todo Test with "Set non block on" 
        //       and set the set_nonblock if it work correct
        // set_nonblock(fd);
    }

    return !(fd > 0);
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
 * @return Return 0 if success; -1 if data length is too lage (more than 32319)
 */
int ksnCoreSendto(ksnCoreClass *kc, char *addr, int port, uint8_t cmd,
                  void *data, size_t data_len) {

    #ifdef DEBUG_KSNET
    ksnet_printf( & ((ksnetEvMgrClass*)kc->ke)->ksn_cfg, DEBUG_VV,
                 "%sNet core:%s >> ksnCoreSendto %s:%d %d \n", 
                 ANSI_GREEN, ANSI_NONE, addr, port, data_len);
    #endif


    int retval = 0;

    if(data_len <= MAX_PACKET_LEN - MAX_DATA_LEN) {

        struct sockaddr_in remaddr;         // remote address
        socklen_t addrlen = sizeof(remaddr);// length of addresses
        make_addr(addr, port, (__SOCKADDR_ARG) &remaddr, &addrlen);

        // Split large packet
        int num_subpackets;
        void **packets = ksnSplitPacket(kc->kco->ks, cmd, data, data_len, &num_subpackets);    

        // Send large packet
        if(num_subpackets) {

            int i;

            // Encrypt and send sub-packets
            for(i = 0; i < num_subpackets; i++) {

                // Create packet
                size_t packet_len;
                size_t data_len = *(uint16_t*)(packets[i]);
                void *packet = ksnCoreCreatePacket(kc, CMD_SPLIT, 
                        packets[i] + sizeof(uint16_t), 
                        data_len + sizeof(uint16_t)*2, 
                        &packet_len);

                // Encrypt and send one spitted sub-packet
                sendto_encrypt(kc, CMD_SPLIT, packet, packet_len);

                // Free memory
                free(packet);
                free(packets[i]);
            }
            free(packets);
        } 
        
        // Send small packet
        else {

            // Create packet
            size_t packet_len;
            void *packet = ksnCoreCreatePacket(kc, cmd, data, data_len, 
                    &packet_len);

            // Encrypt and send one not spitted packet
            sendto_encrypt(kc, cmd, packet, packet_len);
            
            // Free packet
            free(packet);
        }

    }
    else retval = -1;   // Error: To lage packet

    // Set last host event time
    ksnCoreSetEventTime(kc);

    return retval;
}

/**
 * Send command by name to peer
 *
 * @param kc Pointer to ksnCoreClass
 * @param to Peer name to send to
 * @param cmd Command
 * @param data Commands data
 * @param data_len Commands data length
 * @return Pointer to ksnet_arp_data or NULL if to peer is absent
 */
ksnet_arp_data *ksnCoreSendCmdto(ksnCoreClass *kc, char *to, uint8_t cmd,
                                 void *data, size_t data_len) {

    int *fd;
    ksnet_arp_data *arp;

    // Send to peer in this network
    if((arp = ksnetArpGet(kc->ka, to)) != NULL && arp->mode != -1) {

        ksnCoreSendto(kc, arp->addr, arp->port, cmd, data, data_len);
    }
    
    // Send to peer at other network
    else if(((ksnetEvMgrClass*)(kc->ke))->km != NULL && 
        (arp = ksnMultiSendCmdTo(((ksnetEvMgrClass*)(kc->ke))->km, to, cmd, data, 
                data_len))) {
               
        #ifdef DEBUG_KSNET
        ksnet_printf(&((ksnetEvMgrClass*)(kc->ke))->ksn_cfg, DEBUG, 
                "%sNet core:%s Send to peer %s at other network\n", 
                ANSI_GREEN, ANSI_NONE, to);
        #endif
    }
    
    // Send this message to L0 client
    else if(cmd == CMD_L0 && 
            ((ksnetEvMgrClass*)(kc->ke))->ksn_cfg.l0_allow_f &&            
            (fd = ksnLNullClientIsConnected(((ksnetEvMgrClass*)(kc->ke))->kl, 
                    to)) != NULL) {
        
        #ifdef DEBUG_KSNET
        ksnet_printf(&((ksnetEvMgrClass*)(kc->ke))->ksn_cfg, DEBUG_VV, 
                "%sNet core:%s Send command to L0 client \"%s\" to r-host\n", 
                ANSI_GREEN, ANSI_NONE, 
                to);
        #endif                

        ssize_t snd;                
        ksnLNullSPacket *cmd_l0_data = data;

        const size_t buf_length = teoLNullBufferSize(
                cmd_l0_data->from_length, 
                cmd_l0_data->data_length
        );

        char *buf = malloc(buf_length);
        teoLNullPacketCreate(buf, buf_length, 
                cmd_l0_data->cmd, 
                cmd_l0_data->from, 
                cmd_l0_data->from + cmd_l0_data->from_length, 
                cmd_l0_data->data_length);
        if((snd = write(*fd, buf, buf_length)) >= 0);
        free(buf);
    }
        
    // Send to r-host
    else {
        
        // If connected to r-host
        char *r_host = ((ksnetEvMgrClass*)(kc->ke))->ksn_cfg.r_host_name;
        if(r_host[0] && (arp = ksnetArpGet(kc->ka, r_host)) != NULL) {
            
            #ifdef DEBUG_KSNET
            ksnet_printf(&((ksnetEvMgrClass*)(kc->ke))->ksn_cfg, DEBUG_VV, 
                    "%sNet core:%s Resend command to peer \"%s\" to r-host\n", 
                    ANSI_GREEN, ANSI_NONE, 
                    to);
            #endif

            // Create resend command buffer and Send command to r-host 
            // Command data format: to, cmd, data, data_len
            size_t ptr = 0;
            const size_t to_len = strlen(to) + 1;
            const size_t buf_len = to_len + sizeof(cmd) + data_len;
            char *buf = malloc(buf_len);
            memcpy(buf + ptr, to, to_len); ptr += to_len;
            memcpy(buf + ptr, &cmd, sizeof(uint8_t)); ptr += sizeof(uint8_t);
            memcpy(buf + ptr, data, data_len); ptr += data_len;
            ksnCoreSendto(kc, arp->addr, arp->port, CMD_RESEND, buf, buf_len);
            free(buf);                                           
        }
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
    *((uint8_t *)packet) = kc->name_len; ptr += sizeof(uint8_t); // Name length
    memcpy(packet + ptr, kc->name, kc->name_len); ptr += kc->name_len; // Name
    *((uint8_t *) packet +ptr) = cmd; ptr += sizeof(uint8_t); // Command
    memcpy(packet + ptr, data, data_len); ptr += data_len; // Data

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

    ksnCoreSendto(((ksnetEvMgrClass*) ka->ke)->kc, 
            arp_data->addr,
            arp_data->port, 
            CMD_DISCONNECTED, 
            data, data != NULL ? strlen(data)+1 : 0
    );

    return 0;
}

/**
 * Send command to peer to disconnect peer
 *
 * @param ka
 * @param name
 * @param arp_data
 * @param data
 */
int send_cmd_disconnect_peer_cb(ksnetArpClass *ka, char *name,
                            ksnet_arp_data *arp_data, void *data) {
    
    if(arp_data->mode == 2) {
        ksnetArpGetAll(ka, send_cmd_disconnect_cb, name);
    }
            
    return 0;
}

/**
 * Teonet host fd read event callback
 *
 * param loop
 * @param w
 * @param revents
 */
void host_cb(EV_P_ ev_io *w, int revents) {
    
    ksnCoreClass *kc = w->data;             // ksnCore Class object
    ksnetEvMgrClass *ke = kc->ke;           // ksnetEvMgr Class object

    struct sockaddr_in remaddr;             // remote address
    socklen_t addrlen = sizeof(remaddr);    // length of addresses
    unsigned char buf[KSN_BUFFER_DB_SIZE];  // Message buffer
    size_t recvlen;                         // # bytes received

    // Receive data
    recvlen = ksn_recvfrom(ke->kc->ku, 
                revents == EV_NONE ? 0 : kc->fd, (char*)buf, KSN_BUFFER_DB_SIZE, 
                0, (struct sockaddr *)&remaddr, &addrlen);

    // Process package
    ksnCoreProcessPacket(kc, buf, recvlen, (__SOCKADDR_ARG) &remaddr);

    // Set last host event time
    ksnCoreSetEventTime(kc);
}

/**
 * Check new peer
 * 
 * @param kc
 * @param rd
 * @return 
 */
void ksnCoreCheckNewPeer(ksnCoreClass *kc, ksnCorePacketData *rd) {
    
    ksnetEvMgrClass *ke = kc->ke; // ksnetEvMgr Class object
     
    // Check new peer connected
    if((rd->arp = ksnetArpGet(kc->ka, rd->from)) == NULL) {

        ksnet_arp_data arp;
    
        rd->arp = &arp;
        int mode = 0;

        // Check r-host connected
        if(!ke->ksn_cfg.r_host_name[0] &&
           !strcmp(ke->ksn_cfg.r_host_addr, rd->addr) &&
           ke->ksn_cfg.r_port == rd->port) {

            strncpy(ke->ksn_cfg.r_host_name, rd->from,
                    sizeof(ke->ksn_cfg.r_host_name));

            mode = 1;
        }

        // Add peer to ARP Table
        memset(rd->arp, 0, sizeof(*rd->arp));
        strncpy(rd->arp->addr, rd->addr, sizeof(rd->arp->addr));
        rd->arp->connected_time = ksnetEvMgrGetTime(ke);
        rd->arp->port = rd->port;
        rd->arp->mode = mode;
        ksnetArpAdd(kc->ka, rd->from, rd->arp);
        rd->arp = ksnetArpGet(kc->ka, rd->from);

        // Send child to new peer and new peer to child
        ksnetArpGetAll(kc->ka, send_cmd_connected_cb, rd);

        // Send event callback
        if(ke->event_cb != NULL)
            ke->event_cb(ke, EV_K_CONNECTED, (void*)rd, sizeof(*rd), NULL);
        
        // Send event to subscribers
        teoSScrSend(ke->kc->kco->ksscr, EV_K_CONNECTED, rd->from, rd->from_len, 0);
    }
}

/**
 * Process ksnet packet
 * 
 * @param vkc Pointer to ksnCoreClass
 * @param buf Buffer with packet
 * @param recvlen Packet length
 * @param remaddr Address
 */
void ksnCoreProcessPacket (void *vkc, void *buf, size_t recvlen, 
        __SOCKADDR_ARG remaddr) {
    
    ksnCoreClass *kc = vkc; // ksnCoreClass Class object
    ksnetEvMgrClass *ke = kc->ke; // ksnetEvMgr Class object
            
    // Data received    
    if(recvlen > 0) {
        
        char *addr = strdup(inet_ntoa(((struct sockaddr_in*)remaddr)->sin_addr)); // IP to string
        int port = ntohs(((struct sockaddr_in*)remaddr)->sin_port); // Port to integer

        #ifdef DEBUG_KSNET
        ksnet_printf(&ke->ksn_cfg, DEBUG_VV, 
                "%sNet core:%s << host_cb receive %d bytes from %s:%d\n", 
                ANSI_GREEN, ANSI_NONE, 
                recvlen, 
                addr,
                port);
        #endif

        void *data; // Decrypted packet data
        size_t data_len; // Decrypted packet data length

        // Decrypt package
        #if KSNET_CRYPT
        if(ke->ksn_cfg.crypt_f && ksnCheckEncrypted(buf, recvlen)) {
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
//        ksnet_arp_data arp;
        ksnCorePacketData rd;
        memset(&rd, 0, sizeof(rd));
        int event = EV_K_RECEIVED, command_processed = 0;

        // Remote peer address and peer
        rd.addr = addr; // IP to string
        rd.port = port; // Port to integer

        // Parse packet and check if it valid
        if(!ksnCoreParsePacket(data, data_len, &rd)) {
            event = EV_K_RECEIVED_WRONG;
        }

        // Check ARP Table and add peer if not present
        else {
            
            #ifdef DEBUG_KSNET
            ksnet_printf(&ke->ksn_cfg, DEBUG_VV, 
                "%sNet core:%s << got data %d bytes len, cmd = %d, from %s\n",
                ANSI_GREEN, ANSI_NONE, 
                rd.data_len, rd.cmd, rd.from);
            #endif
         
            // Check new peer connected
            ksnCoreCheckNewPeer(kc, &rd);

            // Set last activity time
            rd.arp->last_activity = ksnetEvMgrGetTime(ke);

            // Check & process command
            command_processed = ksnCommandCheck(kc->kco, &rd);
        }

        // Send event to User level
        if(!command_processed) {

            // Send event callback
            if(ke->event_cb != NULL)
                ke->event_cb(ke, event, (void*)&rd, sizeof(rd), NULL);

        }
        
        // Free address buffer
        free(addr);
    }

//    // Socket disconnected
//    else {
//        #ifdef DEBUG_KSNET
//        ksnet_printf(&ke->ksn_cfg, DEBUG_VV, 
//                "TR-UDP protocol data, dropped or disconnected ...\n");
//        #endif
//    }
}


