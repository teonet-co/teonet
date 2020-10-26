/**
 * File:   tr-udp.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Teonet Real time communications over UDP protocol (TR-UDP)
 *
 * Created on August 4, 2015, 12:16 AM
 */

#include <stdlib.h>
#include <string.h>

#include "ev_mgr.h"
#include "tr-udp.h"
#include "trudp_ev.h"

#define MODULE _ANSI_LIGHTGREEN "tr_udp" _ANSI_NONE

// Teonet TCP proxy functions
ssize_t teo_recvfrom (ksnetEvMgrClass* ke,
            int fd, void *buffer, size_t buffer_len, int flags,
            __SOCKADDR_ARG addr, socklen_t *__restrict addr_len);
ssize_t teo_sendto (ksnetEvMgrClass* ke,
            int fd, const void *buffer, size_t buffer_len, int flags,
            __CONST_SOCKADDR_ARG addr, socklen_t addr_len);


#define kev ((ksnetEvMgrClass*)(((trudpData *)td)->user_data))

/**
 * Allow or disallow send ACK event (EV_K_RECEIVED_ACK) to teonet event loop
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @param allow Allow if true or disallow (set by default)
 *
 * @return Previous state
 */
inline int ksnetAllowAckEvent(ksnetEvMgrClass* ke, int allow) {
    int rv = ke->ksn_cfg.send_ack_event_f;
    ke->ksn_cfg.send_ack_event_f = allow;
    return rv;
}

/**
 * Send to peer through TR-UDP transport
 *
 * @param td Pointer to trudpData object
 * @param resend_flg New message or resend sent before (0 - new, 1 -resend)
 * @param id ID of resend message
 * @param cmd Command to allow TR-UDP
 * @param attempt Number of attempt of this message
 * @param fd File descriptor of UDP connection
 * @param buf Buffer with data
 * @param buf_len Data length
 * @param flags Flags (always 0, reserved)
 * @param addr Peer address
 * @param addr_len Peer address length
 *
 * @return Number of bytes sent to UDP
 */
ssize_t ksnTRUDPsendto(trudpData *td, int resend_flg, uint32_t id,
        int attempt, int cmd, int fd, const void *buf, size_t buf_len,
        int flags, __CONST_SOCKADDR_ARG addr, socklen_t addr_len) {

    // Show debug messages
    #ifdef DEBUG_KSNET
    ksn_printf(kev, MODULE, DEBUG_VVV,
            "got %d bytes data to send, cmd %d\n", buf_len, cmd
    );
    #endif

    trudpChannelData *tcd = trudpGetChannelCreate(td, addr, addr_len, 0); // The trudpCheckRemoteAddr (instead of trudpGetChannel) function need to connect web socket server with l0-server

    if(tcd == (void*)-1) {
        return -1; // what to return in this case?
    }

    // TR-UDP: Check commands array
    if(CMD_TRUDP_CHECK(cmd)) {
        trudpChannelSendData(tcd, (void *)buf, buf_len);
        return buf_len;
    }

    // UDP
    ssize_t sent = trudpUdpSendto(td->fd, (void *)buf, buf_len, (__CONST_SOCKADDR_ARG)&tcd->remaddr, sizeof(tcd->remaddr));

    #ifdef DEBUG_KSNET
    addr_port_t *ap_obj = wrap_inet_ntop(addr);
    ksn_printf(kev, MODULE, DEBUG_VV, ">> skip this packet, send %d bytes direct by UDP to: %s:%d\n",
            sent, ap_obj->addr, ap_obj->port
    );
    addr_port_free(ap_obj);
    #endif

    return sent;
}

/**
 * Get data from peer through TR-UDP transport
 *
 * @param td
 * @param fd
 * @param buffer
 * @param buffer_len
 * @param flags
 * @param addr
 * @param addr_len
 *
 * @return If return 0 than the packet is processed by tu->process_packet
 *         function. In other case there is value returned by UDP recvfrom
 *         function and the buffer contain received data
 */
ssize_t ksnTRUDPrecvfrom(trudpData *td, int fd, void *buffer,
                         size_t buffer_len, int flags, __SOCKADDR_ARG addr,
                         socklen_t *addr_len) {

    trudpSendEvent(td, PROCESS_RECEIVE, buffer, buffer_len, 0);

    return 0;
}

/**
 * Send queue processing initialize
 *
 * @param td
 */
static void trudp_send_queue_init(trudpData *td) {

    if(!td->psq_data) {

        td->psq_data = malloc(sizeof(trudpProcessSendQueueData));
        trudpProcessSendQueueData *p = (trudpProcessSendQueueData*)td->psq_data;
        p->inited = 0;
        p->started = 0;
        p->loop = kev->ev_loop;
        p->td = td;     
        ev_timer_init(&p->process_send_queue_w, NULL, 0, 0.0);
    }
}

/**
 * Send queue processing destroy
 *
 * @param td
 */
static void trudp_send_queue_destroy(trudpData *td) {

    if(td->psq_data) {
        trudpProcessSendQueueData *p = (trudpProcessSendQueueData *)td->psq_data;
        if(ev_is_active(&p->process_send_queue_w)) {
                ev_timer_stop(p->loop, &p->process_send_queue_w);
        }
        free(td->psq_data);
        td->psq_data = NULL;
    }
}

/**
 * Send ACK event to teonet event loop
 *
 * @param ke
 * @param id
 * @param data
 * @param data_len
 * @param addr
 */
void trudp_send_event_ack_to_app(ksnetEvMgrClass *ke, uint32_t id,
        void *data, size_t data_length, __CONST_SOCKADDR_ARG addr) {

    // Send event to application
    if(ke->event_cb != NULL) {

        #if KSNET_CRYPT
        if(ke->ksn_cfg.crypt_f && ksnCheckEncrypted(
                data, data_length)) {

            data = ksnDecryptPackage(ke->kc->kcr, data,
                    data_length, &data_length);
        }
        #endif
        ksnCorePacketData rd;
        memset(&rd, 0, sizeof(rd));

        addr_port_t *ap_obj = wrap_inet_ntop(addr);

        // Remote peer address and port
        rd.addr = strdup(ap_obj->addr); // IP to string
        rd.port = ap_obj->port; // Port to integer

        addr_port_free(ap_obj);

        // Parse packet and check if it valid
        if(ksnCoreParsePacket(data, data_length, &rd)) {

            // Send event for CMD for Application level TR-UDP mode: 128...191
            if(rd.cmd >= 128 && rd.cmd < 192) {

                ke->event_cb(ke, EV_K_RECEIVED_ACK,
                    (void*)&rd, // Pointer to ksnCorePacketData
                    sizeof(rd), // Length of ksnCorePacketData
                    &id);       // Pointer to packet ID
            }
        }
        free(rd.addr);
    }
}

/**
 * Process read data from UDP
 *
 * @param td
 * @param data
 * @param data_length
 */
void trudp_process_receive(trudpData *td, void *data, size_t data_length) {

    struct sockaddr_storage remaddr; // remote address

    socklen_t addr_len = sizeof(remaddr);

    // Read data using teonet
    ssize_t recvlen = teo_recvfrom(kev,
            td->fd, data, data_length, 0 /* int flags*/,
            (__SOCKADDR_ARG)&remaddr, &addr_len);

    if (trudpIsPacketPing(data, recvlen) && trudpGetChannel(td, (__CONST_SOCKADDR_ARG) &remaddr, addr_len, 0) == (void *)-1) {
        trudpChannelData *tcd = trudpGetChannelCreate(td, (__CONST_SOCKADDR_ARG) &remaddr, addr_len, 0);
        trudpChannelSendRESET(tcd, NULL, 0);
        return;
    }

    // Process received packet
    if (recvlen <= 0) { return; }

    trudpChannelData *tcd =
        trudpGetChannelCreate(td, (__CONST_SOCKADDR_ARG)&remaddr, addr_len, 0);

    if (tcd == (void *)-1) {
        fprintf(stderr, "!!! can't PROCESS_RECEIVE_NO_TRUDP\n");
        return;
    }

    if (trudpChannelProcessReceivedPacket(tcd, data, recvlen) == -1) {
        trudpChannelSendEvent(tcd, PROCESS_RECEIVE_NO_TRUDP, data, recvlen, NULL);
    }
}


/**
 * TR-UDP event callback
 *
 * @param tcd_pointer
 * @param event
 * @param data
 * @param data_length
 * @param user_data
 */
void trudp_event_cb(void *tcd_pointer, int event, void *data, size_t data_length,
        void *user_data) {

    trudpChannelData *tcd = (trudpChannelData *)tcd_pointer;

    switch(event) {

        // Initialize TR-UDP event (send after initialize)
        // @param td Pointer to trudpData
        case INITIALIZE: {

            trudpData *td = (trudpData *) tcd_pointer;
            trudp_send_queue_init(td);
            #ifdef DEBUG_KSNET
            ksn_puts(kev, MODULE, DEBUG_VV, "TR-UDP module initialized");
            #endif

        } break;

        // Destroy TR-UDP event (send before destroy)
        // @param td Pointer to trudpData
        case DESTROY: {

            trudpData *td = (trudpData *) tcd_pointer;
            trudp_send_queue_destroy(td);
            #ifdef DEBUG_KSNET
            ksn_puts(kev, MODULE, DEBUG_VV, "TR-UDP module destroying");
            #endif

        } break;

        // CONNECTED event (send after TR-UDP channel connected)
        // @param tcd Pointer to trudpChannelData
        // @param data NULL
        // @param user_data NULL
        case CONNECTED: {

            #ifdef DEBUG_KSNET
            const trudpData *td = tcd->td; // used in kev macro
            ksn_printf(kev, MODULE, DEBUG_VV, "connect channel %s, %p\n", tcd->channel_key, tcd);
            #endif

        } break;

        // DISCONNECTED event (send before TR-UDP channel disconnected)
        // @param tcd Pointer to trudpChannelData
        // @param data Last packet received
        // @param user_data NULL
        case DISCONNECTED: {

            const trudpData *td = tcd->td; // used in kev macro
            if(data_length == sizeof(uint32_t)) {

                uint32_t last_received = *(uint32_t*)data;
                #ifdef DEBUG_KSNET
                ksn_printf(kev, MODULE, DEBUG_VV, /*CONNECT,*/
                    "disconnect channel %s, last received: %.6f sec\n",
                    tcd->channel_key, last_received / 1000000.0);
                #endif

                if(tcd->fd) {
                    ksnLNullClientDisconnect(kev->kl, tcd->fd, 1);
                }
                
                // Remove peer from ARP table
                const trudpData *td = tcd->td; // used in kev macro
                if(1 != remove_peer_addr(kev, (__CONST_SOCKADDR_ARG) &tcd->remaddr)) {
                
                    // Remove TR-UDP channel
                    trudpChannelDestroyChannel(td, tcd);
                }
            }
            else {
                #ifdef DEBUG_KSNET
                ksn_printf(kev, MODULE, DEBUG_VV, /*CONNECT,*/
                    "disconnect channel %s (Channel destroyed), %p\n",
                    tcd->channel_key, tcd);
                #endif
            }

            //connected_flag = 0;

        } break;

        // GOT_RESET event
        // @param tcd Pointer to trudpChannelData
        // @param data NULL
        // @param user_data NULL
        case GOT_RESET: {

            #ifdef DEBUG_KSNET
            const trudpData *td = tcd->td; // used in kev macro
            ksn_printf(kev, MODULE, DEBUG_VV,
                "GOT_RESET event. Got TRU_RESET packet from channel %s\n",
                tcd->channel_key);
            #endif

            // When we received RESET - than that peer 
            // was restarted after crash or connection lost:
            // Remove peer from ARP table and destroy this channel
            // const trudpData *td = tcd->td; // used in kev macro
            if(1 != remove_peer_addr(kev, (__CONST_SOCKADDR_ARG) &tcd->remaddr)) {
            
                // Remove TR-UDP channel
                // trudpChannelDestroyChannel(td, tcd);
            }

        } break;

        // SEND_RESET event
        // @param tcd Pointer to trudpChannelData
        // @param data Pointer to uint32_t id or NULL (data_size == 0)
        // @param user_data NULL
        case SEND_RESET: {

            const trudpData *td = tcd->td; // used in kev macro

            if(!data) {
                
                #ifdef DEBUG_KSNET
                ksn_printf(kev, MODULE, DEBUG_VV, "send reset to channel %s\n", tcd->channel_key);
                #endif                

                // When we received id=0 from existing peer - than that peer 
                // was restarted after crash:
                // Remove peer from ARP table and destroy this channel
                // const trudpData *td = tcd->td; // used in kev macro
                if(1 != remove_peer_addr(kev, (__CONST_SOCKADDR_ARG) &tcd->remaddr)) {
                
                    // Remove TR-UDP channel
                    // trudpChannelDestroyChannel(td, tcd);
                }

            } else {

                uint32_t id = (data_length == sizeof(uint32_t)) ? *(uint32_t*)data:0;

                if(!id)
                    #ifdef DEBUG_KSNET
                    ksn_printf(kev, MODULE, DEBUG_VV,
                        "send reset: "
                        "not expected packet with id = 0 received from channel %s\n",
                        tcd->channel_key);
                    #endif
                else
                    #ifdef DEBUG_KSNET
                    ksn_printf(kev, MODULE, DEBUG_VV,
                        "send reset: "
                        "high send packet number (%d) at channel %s\n",
                        id, tcd->channel_key);
                    #endif
                }

        } break;

        // GOT_ACK_RESET event: got ACK to reset command
        // @param tcd Pointer to trudpChannelData
        // @param data NULL
        // @param user_data NULL
        case GOT_ACK_RESET: {

            #ifdef DEBUG_KSNET
            const trudpData *td = tcd->td; // used in kev macro
            ksn_printf(kev, MODULE, DEBUG_VV,
                "got ACK to RESET packet at channel %s\n", tcd->channel_key);
            #endif

        } break;

        // GOT_ACK_PING event: got ACK to ping command
        // @param tcd Pointer to trudpChannelData
        // @param data Pointer to ping data (usually it is a string)
        // @param user_data NULL
        case GOT_ACK_PING: {

            #ifdef DEBUG_KSNET
            const trudpData *td = tcd->td; // used in kev macro
            ksn_printf(kev, MODULE, DEBUG_VV,
                "got ACK to PING packet at channel %s, data: %s, %.3f(%.3f) ms\n",
                tcd->channel_key, (char*)data,
                (tcd->triptime)/1000.0, (tcd->triptimeMiddle)/1000.0);
            #endif

        } break;

        // GOT_PING event: got PING packet, data
        // @param tcd Pointer to trudpChannelData
        // @param data Pointer to ping data (usually it is a string)
        // @param user_data NULL
        case GOT_PING: {
            #ifdef DEBUG_KSNET
            const trudpData *td = tcd->td; // used in kev macro
            ksn_printf(kev, MODULE, DEBUG_VV,
                "got PING packet at channel %s, data: %s\n",
                tcd->channel_key, (char*)data);
            #endif

        } break;

        // Got ACK event
        // @param tcd Pointer to trudpChannelData
        // @param data Pointer to packet which ACK received
        // @param data_length Length of packet
        // @param user_data NULL
        case GOT_ACK: {

            const trudpData *td = tcd->td; // used in kev macro
            trudpPacket *packet = (trudpPacket *)data;

            #ifdef DEBUG_KSNET
            int net_lag = trudpGetTimestamp() - trudpPacketGetTimestamp(packet);
            ksn_printf(kev, MODULE, DEBUG_VV,
                "GOT_ACK event. Packet type = %s. Got ACK id=%u at channel %s, triptime %.3f(%.3f) ms, network lag %d ms%s\n",
                STRING_trudpPacketType(trudpPacketGetType(packet)),
                trudpPacketGetId(packet),
                tcd->channel_key, (tcd->triptime)/1000.0, (tcd->triptimeMiddle)/1000.0,
                (int)net_lag, (net_lag > 50000) ? _ANSI_RED "  Big Lag!" _ANSI_NONE : "" );
            #endif

            // Send event ACK to teonet event loop
            if(kev->ksn_cfg.send_ack_event_f)
                trudp_send_event_ack_to_app(kev, trudpPacketGetId(packet),
                    trudpPacketGetData(packet), trudpPacketGetDataLength(packet),
                    (__CONST_SOCKADDR_ARG) &tcd->remaddr);

            // Start send queue \todo test and remove it (if tested better)
            //trudp_start_send_queue_cb(td->psq_data, 0);

        } break;

        // Got DATA event
        // @param tcd Pointer to trudpChannelData
        // @param data Pointer to packets data
        // @param data_length Length of packets data
        // @param user_data NULL
        case GOT_DATA: {

            // Process package
            const trudpData *td = tcd->td; // used in kev macro
            trudpPacket * packet = (trudpPacket *)data;
            #ifdef DEBUG_KSNET
            ksn_printf(kev, MODULE, DEBUG_VV,
                "GOT_DATA event. Packet type = %s. Got %d bytes, packet ID = %d from channel %s\n",
                STRING_trudpPacketType(trudpPacketGetType(packet)),
                data_length,
                trudpPacketGetId(packet),
                tcd->channel_key
            );
            #endif

            size_t block_len = trudpPacketGetDataLength(packet);
            void* block = trudpPacketGetData(packet);
            ksnCoreProcessPacket(kev->kc, block, block_len, (__SOCKADDR_ARG) &tcd->remaddr);

        } break;

        case GOT_DATA_NO_TRUDP: {
            const trudpData *td = tcd->td; // used in kev macro

            #ifdef DEBUG_KSNET
            ksn_printf(kev, MODULE, DEBUG_VV,
                       "got %d bytes DATA packet from no trudp chan %s \n",
                       data_length, tcd->channel_key);
            #endif

            ksnCoreProcessPacket(kev->kc, data, data_length, (__SOCKADDR_ARG)&tcd->remaddr);
        } break;

        // Process received data
        // @param tcd Pointer to trudpData
        // @param data Pointer to receive buffer
        // @param data_length Receive buffer length
        // @param user_data NULL
        case PROCESS_RECEIVE: {
            trudpPacket *packet = (trudpPacket *)data;

            const trudpData *td = (trudpData *)tcd_pointer; // used in kev macro
            ksn_printf(kev, MODULE, DEBUG_VV,
                "PROCESS_RECEIVE. type packet %s\n", STRING_trudpPacketType(trudpPacketGetType(packet)));
            trudp_process_receive((trudpData *)tcd_pointer, data, data_length);
        } break;

        // Process received not TR-UDP data
        // @param tcd Pointer to trudpChannelData
        // @param data Pointer to receive buffer
        // @param data_length Receive buffer length
        // @param user_data NULL
        case PROCESS_RECEIVE_NO_TRUDP: {

            // Process package
            const trudpData *td = tcd->td; // used in kev macro
            #ifdef DEBUG_KSNET
            ksn_printf(kev, MODULE, DEBUG_VV,
                "not TR-UDP %d bytes DATA packet received from channel %s\n",
                data_length,
                tcd->channel_key);
            #endif
            ksnCoreProcessPacket(kev->kc, data, data_length,
                                 (__SOCKADDR_ARG)&tcd->remaddr);

        } break;

        // Process send data
        // @param data Pointer to send data
        // @param data_length Length of send
        // @param user_data NULL
        case PROCESS_SEND: {
            const trudpData *td = tcd->td; // used in kev macro
            trudpPacket *packet = (trudpPacket *)data;

            #ifdef DEBUG_KSNET 
            ksn_printf(kev, MODULE, DEBUG_VV,
                       "PROCESS_SEND event. Packet type = %s. Send %d bytes, packet ID = %d to channel %s, %p\n",
                        STRING_trudpPacketType(trudpPacketGetType(packet)),
                       data_length,
                       trudpPacketGetId(packet),
                       tcd->channel_key, tcd
            );
            #endif
            teo_sendto(kev, td->fd, data, data_length, 0,
                       (__CONST_SOCKADDR_ARG)&tcd->remaddr, tcd->addrlen);

            // Start send queue timer
            if (trudpPacketGetType(packet) == TRU_DATA) {
                trudpSendQueueCbStart(td->psq_data, 0);
            }

        } break;
    }
}
