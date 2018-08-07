#include "ev_mgr.h"
#include "async_calls.h"

#define MODULE "async_module"
#define kev ((ksnetEvMgrClass*)(ke))

void teoAsyncTest(void *ke);

// Logging Async call label
static const uint32_t ASYNC_LABEL = 0x77AA77AA;

static const int SEND_IF = 64;

typedef struct async_data {
    
    uint32_t label;
    int sq_length;
    int rv;
    
} async_data;

static inline int _check_send_queue(void *ke, const char*peer, size_t peer_length,
        const char*addr, size_t addr_length, uint32_t port) {
    
    int rv = 0;
    
    // check sendQueue
    // by peer name
    if(peer && !addr) {
        ksnet_arp_data *arp = ksnetArpGet(kev->kc->ka, (char*)peer);
        if(arp) {
            addr = arp->addr;
            port = arp->port;
        }
        // wrong peer
        else rv = -3;
    }
    // check address and port
    if(addr) {
        trudpChannelData *tcd = trudpGetChannelAddr(kev->kc->ku, (char*)addr, port, 0);
        if(tcd != (void*)-1) {
            // Wait sendQueue ready to get packet
            //while(trudpSendQueueSize(tcd->sendQueue) > 200) usleep(1000); // Sleep 1ms
            rv = trudpSendQueueSize(tcd->sendQueue);
        }
        // wrong address
        else rv = -4;
    }
    // wrong request
    else rv = -2;
    
    return rv;
}

// Event loop to gab teonet events
static void event_cb(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data,
        size_t data_length, void *user_data) {

    int processed = 0;

    switch(event) {

        // Start test
        case EV_K_STARTED:
            //teoAsyncTest(ke);
            break;

        // Async event from teoLoggingClientSend (kns_printf)
        case EV_K_ASYNC:
            if(user_data && *(uint32_t*)user_data == ASYNC_LABEL) {

                if(data && data_length > 2) {
                    pthread_mutex_lock(&kev->ta->cv_mutex);

                    int ptr = 0;
                    const uint8_t f_type = *(uint8_t*)data; ptr++;
                    const uint8_t cmd = *(uint8_t*)(data + ptr); ptr++;
                    switch(f_type) {

//                        // type #0
//                        // Wait for channel sendQueue ready
//                        //   if call made from different thread, check sendQueue
//                        //   of destination peer and send wait flag = 1, or ready flag = 2
//                        //   flag to caller
//                        case 0: {
//                            // Parse buffer: { f_type, cmd, peer_length, addr_length, peer, addr, port, flag }
//                            const uint8_t peer_length = *(uint8_t*)(data + ptr); ptr++;
//                            const uint8_t addr_length = *(uint8_t*)(data + ptr); ptr++;
//                            const char *peer = (const char *)(data + ptr); ptr += peer_length;
//                            const char *addr = (const char *)(data + ptr); ptr += addr_length;
//                            const uint32_t port = *(uint32_t*)(data + ptr); ptr += 4;
//                            const int *flag = *(int**)(data + ptr); ptr += sizeof(int*);
//                        } break;

                        // type #1
                        // ksnCoreSendCmdtoA (sendCmdToBinaryA)
                        // Send command by name to peer
                        case 1: {
                            // Parse buffer: { f_type, cmd, peer_length, peer, data }
                            const uint8_t peer_length = *(uint8_t*)(data + ptr); ptr++;
                            const char *peer = (const char *)(data + ptr); ptr += peer_length;
                            void *d = data + ptr;
                            const size_t d_length = data_length - ptr;

                            async_data *ud = user_data;
                            ud->rv = _check_send_queue(ke, peer, peer_length, NULL, 0, 0);                            
                            
                            if(ud->rv >= 0 && ud->rv < SEND_IF) {
                                ksnCoreSendCmdto(kev->kc, (char*)peer, cmd, d, d_length);
                            }
                            
//                            ksn_printf(kev, MODULE, DEBUG, "sent to peer: %s, id: %u\n", peer, pthread_self());

                            pthread_cond_signal(&kev->ta->cv_threshold);

                            processed = 1;
                        } break;

                        // type #2
                        // teoSScrSend (sendToSscrA)
                        // Send event and it data to all subscribers
                        case 2: {
                            // Parse buffer: { f_type, cmd, event, data }
                            const uint16_t event = *(uint16_t*)(data + ptr); ptr += 2;
                            void *d = data + ptr;
                            const size_t d_length = data_length - ptr;
                            if(kev->ta->test)
                                ksn_printf(kev, MODULE, DEBUG /*DEBUG_VV*/, // \TODO set DEBUG_VV
                                        "teoSScrSend Test: %d %s %d %d\n", event, d, d_length, cmd);
                            else teoSScrSend(kev->kc->kco->ksscr, event, d, d_length, cmd);
                            processed = 1;
                        } break;

                        // type #3
                        // sendCmdAnswerToBinaryA
                        // Send data to L0 client. Usually it is an answer to request from L0 client
                        // or
                        // Send data to remote peer IP:Port
                        case 3: {
                            // Parse buffer: { f_type, cmd, l0_f, addr_length, from_length, addr, from, port, data }
                            const uint8_t l0_f = *(uint8_t*)(data + ptr); ptr++;
                            const uint8_t addr_length = *(uint8_t*)(data + ptr); ptr++;
                            const uint8_t from_length = *(uint8_t*)(data + ptr); ptr++;
                            const char *addr = (const char *)(data + ptr); ptr += addr_length;
                            const char *from = (const char *)(data + ptr); ptr += from_length;
                            const uint32_t port = *(uint32_t*)(data + ptr); ptr += 4;
                            void *d = data + ptr;
                            const size_t d_length = data_length - ptr;
                            if(kev->ta->test)
                                ksn_printf(kev, MODULE, DEBUG /*DEBUG_VV*/, // \TODO set DEBUG_VV
                                        "sendCmdAnswerToBinaryA Test: %d %d %d %d %s %s %d %s %d\n", cmd, l0_f, addr_length, from_length, addr, from, port, d, d_length);
                            else
                            if (l0_f) ksnLNullSendToL0(ke, (char*)addr, port, (char*)from, from_length, cmd, d, d_length);
                            else ksnCoreSendto(kev->kc, (char*)addr, port, cmd, d, d_length);
                            processed = 1;
                        } break;

                        // type #4
                        // teoSScrSubscribeA (subscribeA)
                        // Send command to subscribe this host to event at remote peer
                        case 4: {
                            // Parse buffer: { f_type, cmd, peer_length, ev, peer }
                            const uint8_t peer_length = *(uint8_t*)(data + ptr); ptr++;
                            const uint16_t ev = *(uint32_t*)(data + ptr); ptr += 2;
                            const char *peer = (const char *)(data + ptr); ptr += peer_length;
                            if(kev->ta->test)
                                ksn_printf(kev, MODULE, DEBUG /*DEBUG_VV*/, // \TODO set DEBUG_VV
                                        "teoSScrSubscribeA Test: %d %d %d %s\n", cmd, peer_length, ev, peer);
                            else teoSScrSubscribe(kev->kc->kco->ksscr, (char*)peer, ev);
                            processed = 1;
                        } break;

                        default:
                            break;
                    }
                    pthread_mutex_unlock(&kev->ta->cv_mutex);
                }
            }
            break;

        default:
            break;
    }

    // Call parent event loop
    if(ke->ta->event_cb != NULL && !processed)
        ((event_cb_t)ke->ta->event_cb)(ke, event, data, data_length, user_data);
}

teoAsyncClass *teoAsyncInit(void *ke) {

    teoAsyncClass *ta = malloc(sizeof(teoAsyncClass));
    ta->ke = ke;
    ta->event_cb = kev->event_cb;
    ta->t_id = pthread_self();
    kev->event_cb = event_cb;
    ta->test = 0;

    // Initialize conditional variable
    pthread_cond_init(&ta->cv_threshold, NULL);
    pthread_mutex_init(&ta->cv_mutex, NULL);

    #ifdef DEBUG_KSNET
    ksn_puts(kev, MODULE, DEBUG /*DEBUG_VV*/, // \TODO set DEBUG_VV
            "have been initialized");
    #endif

    return ta;
}

void teoAsyncDestroy(teoAsyncClass *ta) {

    pthread_cond_destroy(&ta->cv_threshold);
    pthread_mutex_destroy(&ta->cv_mutex);

    ksnetEvMgrClass *ke = ta->ke;
    ke->event_cb = ta->event_cb;
    free(ta);
    ke->ta = NULL;

    #ifdef DEBUG_KSNET
    ksn_puts(ke, MODULE, DEBUG /*DEBUG_VV*/, // \TODO set DEBUG_VV
            "have been de-initialized");
    #endif
}

// Wait to send queue
// Return
//   1 if call was made from thread different to teonet thread
//   0 if there is the same thread
//   -1 at timeout of sendQueue free waiting
//   -2 at wrong request
//   -3 wrong peer
//   -4 wrong address
static inline int _wait_send_queue(void *ke, const char*peer, size_t peer_length,
        const char*addr, size_t addr_length, uint32_t port) {

    int other_thread = pthread_self() != kev->ta->t_id;
    if(other_thread) {
        // check sendQueue
        // by peer name
        if(peer && !addr) {
            ksnet_arp_data *arp = ksnetArpGet(kev->kc->ka, (char*)peer);
            if(arp) {
                addr = arp->addr;
                port = arp->port;
            }
            // wrong peer
            else other_thread = -3;
        }
        // check address and port
        if(addr) {
            trudpChannelData *tcd = trudpGetChannelAddr(kev->kc->ku, (char*)addr, port, 0);
            if(tcd != (void*)-1) {
                // Wait sendQueue ready to get packet
                while(trudpSendQueueSize(tcd->sendQueue) > 200) usleep(1000); // Sleep 1ms
            }
            // wrong address
            else other_thread = -4;
        }
        // wrong request
        else other_thread = -2;
    }
    return other_thread;
}

// type #1
void ksnCoreSendCmdtoA(void *ke, const char *peer, uint8_t cmd, void *data,
        size_t data_length) {

    if(kev->ta->test)
        ksn_printf(kev, MODULE, DEBUG /*DEBUG_VV*/, // \TODO set DEBUG_VV
            "ksnCoreSendCmdtoA: %s %d %s %d\n", peer, cmd, data, data_length);

    int ptr = 0;
    const uint8_t f_type = 1;
    const uint8_t peer_length = strlen(peer) + 1;

    // Create buffer: { f_type, cmd, peer_length, peer, data }
    size_t buf_length = sizeof(uint8_t)*3 + peer_length + data_length;
    void *buf = malloc(buf_length);
    *(uint8_t*)(buf + ptr) = f_type; ptr++;
    *(uint8_t*)(buf + ptr) = cmd; ptr++;
    *(uint8_t*)(buf + ptr) = peer_length; ptr++;
    memcpy(buf + ptr, peer, peer_length); ptr += peer_length;
    memcpy(buf + ptr, data, data_length);

    // Wait for sendQueue ready
    //_wait_send_queue(ke, peer, peer_length, NULL, 0, 0);

    pthread_mutex_lock(&kev->ta->cv_mutex);

//    ksn_printf(kev, MODULE, DEBUG, "wait start, id: %u\n", pthread_self());
    async_data *ud = malloc(sizeof(async_data));
    ud->label = ASYNC_LABEL;
    ud->rv = 0;
    
    for(;;) {
//        struct timeval now;
//        struct timespec timeToWait;
//        gettimeofday(&now,NULL);
//        timeToWait.tv_sec = now.tv_sec+2;
//        timeToWait.tv_nsec = now.tv_usec;        

        ksnetEvMgrAsync(kev, buf, buf_length, (void*)ud); //&ASYNC_LABEL);
//        pthread_cond_timedwait(&kev->ta->cv_threshold, &kev->ta->cv_mutex, &timeToWait);
        pthread_cond_wait(&kev->ta->cv_threshold, &kev->ta->cv_mutex);
        //ksn_printf(kev, MODULE, DEBUG, "wait done, id: %u, rv: %d, run: %d\n\n", pthread_self(), ud->rv, kev->runEventMgr);
        if(ud->rv >= SEND_IF && kev->runEventMgr) usleep(10000);
        else break;
    }    
    
    free(ud);
    
    pthread_mutex_unlock(&kev->ta->cv_mutex);

    free(buf);
}

// type #2
void teoSScrSendA(void *ke, uint16_t event, void *data, size_t data_length,
        uint8_t cmd) {

    if(kev->ta->test)
        ksn_printf(kev, MODULE, DEBUG /*DEBUG_VV*/, // \TODO set DEBUG_VV
            "teoSScrSendA: %d %s %d %d\n", event, data, data_length, cmd);

    int ptr = 0;
    const uint8_t f_type = 2;

    // Create buffer: { f_type, cmd, event, data }
    size_t buf_length = sizeof(uint8_t)*2 + sizeof(uint16_t) + data_length;
    void *buf = malloc(buf_length);
    *(uint8_t*)(buf + ptr) = f_type; ptr++;
    *(uint8_t*)(buf + ptr) = cmd; ptr++;
    *(uint16_t*)(buf + ptr) = event; ptr += 2;
    memcpy(buf + ptr, data, data_length);

    // Wait for sendQueue ready
    //_wait_send_queue(ke, peer, peer_length, NULL, 0, 0);

    ksnetEvMgrAsync(kev, buf, buf_length, (void*)&ASYNC_LABEL);
    free(buf);
}

// type #3
void sendCmdAnswerToBinaryA(void *ke, void *rdp, uint8_t cmd, void *data,
        size_t data_length) {

    ksnCorePacketData *rd = rdp;

    const uint8_t addr_length = strlen(rd->addr) + 1;

    if(kev->ta->test)
        ksn_printf(kev, MODULE, DEBUG /*DEBUG_VV*/, // \TODO set DEBUG_VV
            "sendCmdAnswerToBinaryA: %d %d %d %d %s %s %d %s %d\n",
            cmd, rd->l0_f, addr_length, rd->from_len, rd->addr, rd->from,
            rd->port, data, data_length);

    int ptr = 0;
    const uint8_t f_type = 3;

    // Create buffer: { f_type, cmd, l0_f, addr_length, from_length, addr, from, port, data }
    size_t buf_length = sizeof(uint8_t)*5 + addr_length + rd->from_len + sizeof(uint32_t) + data_length;
    void *buf = malloc(buf_length);
    *(uint8_t*)(buf + ptr) = f_type; ptr++;
    *(uint8_t*)(buf + ptr) = cmd; ptr++;
    *(uint8_t*)(buf + ptr) = rd->l0_f; ptr++;
    *(uint8_t*)(buf + ptr) = addr_length; ptr++;
    *(uint8_t*)(buf + ptr) = rd->from_len; ptr++;
    memcpy(buf + ptr, rd->addr, addr_length); ptr += addr_length;
    memcpy(buf + ptr, rd->from, rd->from_len); ptr += rd->from_len;
    *(uint32_t*)(buf + ptr) = rd->port; ptr += 4;
    memcpy(buf + ptr, data, data_length);

    // Wait for sendQueue ready
    //_wait_send_queue(ke, NULL, 0, rd->addr, addr_length, rd->port);

    ksnetEvMgrAsync(kev, buf, buf_length, (void*)&ASYNC_LABEL);
    free(buf);
}

// type #4
void teoSScrSubscribeA(teoSScrClass *sscr, char *peer, uint16_t ev) {

    const uint8_t peer_length = strlen(peer) + 1;
    const void *ke = sscr->ke;

    if(kev->ta->test)
        ksn_printf(kev, MODULE, DEBUG /*DEBUG_VV*/, // \TODO set DEBUG_VV
            "teoSScrSubscribeA: %d %d %d %s\n", 0, peer_length, ev, peer);

    int ptr = 0;
    const uint8_t f_type = 4;

    // Create buffer: { f_type, cmd, peer_length, ev, peer }
    size_t buf_length = sizeof(uint8_t)*3 + sizeof(uint16_t) + peer_length;
    void *buf = malloc(buf_length);
    *(uint8_t*)(buf + ptr) = f_type; ptr++;
    *(uint8_t*)(buf + ptr) = 0; ptr++; // cmd
    *(uint8_t*)(buf + ptr) = peer_length; ptr++;
    *(uint16_t*)(buf + ptr) = ev; ptr += 2;
    memcpy(buf + ptr, peer, peer_length);

    // Wait for sendQueue ready
    //_wait_send_queue((void*)ke, (const char*)peer, peer_length, NULL, 0, 0);

    ksnetEvMgrAsync(kev, buf, buf_length, (void*)&ASYNC_LABEL);
    free(buf);
}

void teoAsyncTest(void *ke) {

    #ifdef DEBUG_KSNET
    ksn_puts(kev, MODULE, DEBUG /*DEBUG_VV*/, // \TODO set DEBUG_VV
            "start test");
    #endif

    kev->ta->test = 1;

    // #1
    ksnCoreSendCmdtoA(ke, "teo-test-peer", 129, "Hello", 6);

    // #2
    teoSScrSendA(ke, 0x8000, "Hello", 6, 129);

    // #3
    ksnCorePacketData rd;
    rd.addr = "127.0.0.1";
    rd.port = 9010;
    rd.from = "teo-test-peer";
    rd.from_len = 14;
    rd.l0_f = 0;
    sendCmdAnswerToBinaryA(ke, &rd, 129, "Hello", 6);

    // #4
    teoSScrSubscribeA(kev->kc->kco->ksscr, "teo-test-peer", 0x8000);
}
