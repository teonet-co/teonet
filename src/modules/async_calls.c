#include "ev_mgr.h"
#include "async_calls.h"

#define MODULE "async_module"
#define kev ((ksnetEvMgrClass*)(ke))
#define MULTITHREADED() (multithread == MULTITHREAD)
// #define NO_MULTITHREADED() (kev->ta->f_multi_thread == NO_MULTITHREAD)
// \TODO Think about CHSQ_WRONG_PEER - it used if we send to peer by it type, 
// but in this contend the _check_send_queue function does not check real send 
// queue channel
//
// \TODO we use the !MULTITHREADED() condition with this condition in one if
// and the !MULTITHREADED() mode application do not consider real send queue 
// channel size returned by _check_send_queue function, so we can to overload 
// send queue and break peers connection
#define SEND_CONDITION() ((ud->rv >= 0 || ud->rv == CHSQ_WRONG_PEER) && ud->rv < SEND_IF)

void teoAsyncTest(void *ke);

// Logging Async call label
static const uint32_t ASYNC_FUNC_LABEL = 0x77AA77AA;

static const int SEND_IF = 64;
static int ud_count = 0, ud_count_total = 0;

typedef struct async_data {
    uint32_t label;
    int sq_length;
    int rv;
    int multithread;
} async_data;

enum CHECK_SEND_QUEUE {
    CHSQ_WRONG_REQUEST = -2,   //   -2 at wrong request
    CHSQ_WRONG_PEER    = -3,   //   -3 wrong peer
    CHSQ_WRONG_ADDRESS = -4,   //   -4 wrong address
    CHSQ_TIMEOUT = -5,         //   -5 timeout
};

enum MULTITHREAD {
    NOT_DEFINED_MULTITHREAD = 0,
    MULTITHREAD = 1,
    NO_MULTITHREAD = 2
};

enum ASYNC_FUNC {
    KSN_CORE_SEND_CMD_TO = 1,   // ksnCoreSendCmdto
    TEO_SSCR_SEND = 2,          // teoSScrSend
    SEND_CMD_ANSWER_TO = 3,     // sendCmdAnswerTo, ksnLNullSendToL0 & ksnCoreSendto
    TEO_SSCR_SUBSCRIBE = 4      // teoSScrSubscribe
};

// Get send queue size
// Return
//   >= 0 send queue size
//   CHSQ_WRONG_REQUEST (-2)  if wrong request
//   CHSQ_WRONG_PEER    (-3)  if wrong peer
//   CHSQ_WRONG_ADDRESS (-4)  wrong address
static inline int _check_send_queue(void *ke, const char*peer, const char*addr,
        uint32_t port) {

    int rv = 0;

    // check sendQueue
    // by peer name
    if(peer && !addr) {
        ksnet_arp_data *arp = (ksnet_arp_data *)ksnetArpGet(kev->kc->ka, (char*)peer);
        if(arp) {
            addr = arp->addr;
            port = arp->port;
        }
        else rv = CHSQ_WRONG_PEER; // wrong peer
    }
    // check address and port
    if(addr) {
        trudpChannelData *tcd = trudpGetChannelAddr(kev->kc->ku, (char*)addr, port, 0);
        if(tcd != (void*)-1) rv = trudpSendQueueSize(tcd->sendQueue);
        else rv = CHSQ_WRONG_ADDRESS; // wrong address
    }
    else if(!rv) rv = CHSQ_WRONG_REQUEST; // wrong request

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

        // Async event
        case EV_K_ASYNC:
            if(user_data && *(uint32_t*)user_data == ASYNC_FUNC_LABEL) {

                async_data *ud = user_data;
                int multithread = ud->multithread;

                // Check multi thread request
                if(!data) {
                    ksn_printf(kev, MODULE, DEBUG,
                        "Check MULTITHREAD mode rv: %d, f_multi_thread: %d\n",
                        ud->rv, kev->ta->f_multi_thread);
                    if(MULTITHREADED()) {
                        pthread_mutex_lock(&kev->ta->cv_mutex);
                        ud->rv = 0;
                        pthread_cond_signal(&kev->ta->cv_threshold);
                        pthread_mutex_unlock(&kev->ta->cv_mutex);
                    }
                    else { //if((NO_MULTITHREADED() || MULTITHREADED()) && ud->rv == CHSQ_TIMEOUT) {
                        free(ud); ud_count--;
                    }
                    processed = 1;
                }

                // Process async functions
                else if(data && data_length > 2) {
                    if(MULTITHREADED()) pthread_mutex_lock(&kev->ta->cv_mutex);

                    int ptr = 0;
                    const uint8_t f_type = *(uint8_t*)data; ptr++;
                    const uint8_t cmd = *(uint8_t*)(data + ptr); ptr++;
                    switch(f_type) {

                        // type #1
                        // ksnCoreSendCmdtoA (sendCmdToBinaryA)
                        // Send command by name to peer
                        case KSN_CORE_SEND_CMD_TO: {
                            // Parse buffer: { f_type, cmd, peer_length, peer, data }
                            const uint8_t peer_length = *(uint8_t*)(data + ptr); ptr++;
                            const char *peer = (const char *)(data + ptr); ptr += peer_length;
                            void *d = data + ptr;
                            const size_t d_length = data_length - ptr;

                            ksn_printf(kev, MODULE, DEBUG_VV, "KSN_CORE_SEND_CMD_TO, data_length: %d\n", d_length);

                            //async_data *ud = user_data;
                            ud->rv = _check_send_queue(ke, peer, NULL, 0);
                            if(!MULTITHREADED() || SEND_CONDITION()) {
                                ksnCoreSendCmdto(kev->kc, (char*)peer, cmd, d, d_length);
                            }

                            if(MULTITHREADED()) pthread_cond_signal(&kev->ta->cv_threshold);
                            else { free(ud); ud_count--; }
                            processed = 1;
                        } break;

                        // type #2
                        // teoSScrSend (teoSScrSendA)
                        // Send event and it data to all subscribers
                        case TEO_SSCR_SEND: {
                            // Parse buffer: { f_type, cmd, event, data }
                            const uint16_t event = *(uint16_t*)(data + ptr); ptr += 2;
                            void *d = data + ptr;
                            const size_t d_length = data_length - ptr;

                            //async_data *ud = user_data;
                            ud->rv = 0;

                            if(kev->ta->test)
                                ksn_printf(kev, MODULE, DEBUG /*DEBUG_VV*/, // \TODO set DEBUG_VV
                                        "teoSScrSend Test: %d %s %d %d\n", event, d, d_length, cmd);
                            else teoSScrSend(kev->kc->kco->ksscr, event, d, d_length, cmd);

                            if(MULTITHREADED()) pthread_cond_signal(&kev->ta->cv_threshold);
                            else { free(ud); ud_count--; }
                            processed = 1;
                        } break;

                        // type #3 SEND_CMD_ANSWER_TO
                        // sendCmdAnswerToBinaryA
                        // Send data to L0 client. Usually it is an answer to request from L0 client
                        // or
                        // Send data to remote peer IP:Port
                        case SEND_CMD_ANSWER_TO: {
                            // Parse buffer: { f_type, cmd, l0_f, addr_length, from_length, addr, from, port, data }
                            const uint8_t l0_f = *(uint8_t*)(data + ptr); ptr++;
                            const uint8_t addr_length = *(uint8_t*)(data + ptr); ptr++;
                            const uint8_t from_length = *(uint8_t*)(data + ptr); ptr++;
                            const char *addr = (const char *)(data + ptr); ptr += addr_length;
                            const char *from = (const char *)(data + ptr); ptr += from_length;
                            const uint32_t port = *(uint32_t*)(data + ptr); ptr += 4;
                            void *d = data + ptr;
                            const size_t d_length = data_length - ptr;

                            //async_data *ud = user_data;
                            ud->rv = _check_send_queue(ke, NULL, addr, port);

                            if(!MULTITHREADED() || SEND_CONDITION()) {
                                if(kev->ta->test)
                                    ksn_printf(kev, MODULE, DEBUG /*DEBUG_VV*/, // \TODO set DEBUG_VV
                                        "sendCmdAnswerToBinaryA Test: %d %d %d %d %s %s %d %s %d\n",
                                        cmd, l0_f, addr_length, from_length, addr, from, port, d, d_length);
                                else
                                if (l0_f) ksnLNullSendToL0(ke, (char*)addr, port, (char*)from, from_length, cmd, d, d_length);
                                else ksnCoreSendto(kev->kc, (char*)addr, port, cmd, d, d_length);
                            }

                            if(MULTITHREADED()) pthread_cond_signal(&kev->ta->cv_threshold);
                            else { free(ud); ud_count--; }
                            processed = 1;
                        } break;

                        // type #4 TEO_SSCR_SUBSCRIBE
                        // teoSScrSubscribeA (subscribeA)
                        // Send command to subscribe this host to event at remote peer
                        case TEO_SSCR_SUBSCRIBE: {
                            // Parse buffer: { f_type, cmd, peer_length, ev, peer }
                            const uint8_t peer_length = *(uint8_t*)(data + ptr); ptr++;
                            const uint16_t ev = *(uint32_t*)(data + ptr); ptr += 2;
                            const char *peer = (const char *)(data + ptr); ptr += peer_length;

                            //async_data *ud = user_data;
                            ud->rv = _check_send_queue(ke, peer, NULL, 0);

                            if(!MULTITHREADED() || SEND_CONDITION()) {
                                if(kev->ta->test)
                                    ksn_printf(kev, MODULE, DEBUG /*DEBUG_VV*/, // \TODO set DEBUG_VV
                                            "teoSScrSubscribeA Test: %d %d %d %s\n", cmd, peer_length, ev, peer);
                                else teoSScrSubscribe(kev->kc->kco->ksscr, (char*)peer, ev);
                            }

                            if(MULTITHREADED()) pthread_cond_signal(&kev->ta->cv_threshold);
                            else { free(ud); ud_count--; }
                            processed = 1;
                        } break;

                        default:
                            printf("send unknown func async to user\n");
                            break;
                    }
                    if(MULTITHREADED()) pthread_mutex_unlock(&kev->ta->cv_mutex);
                }
            }
            break;

        default:
            break;
    }

    // Call parent event loop
    if(ke->ta->event_cb != NULL && !processed) {
        //printf("send async to user\n");
        ((event_cb_t)ke->ta->event_cb)(ke, event, data, data_length, user_data);
    }
}

teoAsyncClass *teoAsyncInit(void *ke) {

    teoAsyncClass *ta = malloc(sizeof(teoAsyncClass));
    ta->ke = ke;
    ta->f_multi_thread = NOT_DEFINED_MULTITHREAD;
    ta->event_cb = kev->event_cb;
    ta->t_id = pthread_self();
    kev->event_cb = event_cb;
    ta->test = 0;

    // Initialize conditional variable
    pthread_mutex_init(&ta->async_func_mutex, NULL);
    pthread_cond_init(&ta->cv_threshold, NULL);
    pthread_mutex_init(&ta->cv_mutex, NULL);

    #ifdef DEBUG_KSNET
    ksn_puts(kev, MODULE, DEBUG_VV,
            "have been initialized");
    #endif

    return ta;
}

void teoAsyncDestroy(teoAsyncClass *ta) {

    pthread_mutex_destroy(&ta->async_func_mutex);
    pthread_cond_destroy(&ta->cv_threshold);
    pthread_mutex_destroy(&ta->cv_mutex);

    ksnetEvMgrClass *ke = ta->ke;
    ke->event_cb = ta->event_cb;
    free(ta);
    ke->ta = NULL;

    #ifdef DEBUG_KSNET
    ksn_puts(ke, MODULE, DEBUG_VV,
            "have been de-initialized");
    #endif
}

#define SET_TIMEOUT() \
        gettimeofday(&now,NULL); \
        t = now.tv_usec + now.tv_sec * 1000000 + timeout; \
        now.tv_sec = t / 1000000; \
        now.tv_usec = t % 1000000; \
        timeToWait.tv_sec = now.tv_sec; \
        timeToWait.tv_nsec = now.tv_usec * 1000

static int check_retrives = 0, timeouts = 0;
#define CHECK_RETRIVES 10

#define LOCK_CV() pthread_mutex_lock(&kev->ta->cv_mutex);
#define UNLOCK_CV() pthread_mutex_unlock(&kev->ta->cv_mutex);

static int _check_multi_thread(void *ke);

int SEND_ASYNC(void *ke, void *buf, int buf_length) {
    int retval;
    int multithread = _check_multi_thread(ke);
    async_data *ud = malloc(sizeof(async_data));
    ud_count++; ud_count_total++;
    /*printf("\033[s\033[%d;%dH\33[2Kud_count: %d, timeouts: %d, ud_count_total: %d\n\033[u", 1, 1, ud_count, timeouts, ud_count_total);*/
    ud->multithread = multithread;
    ud->label = ASYNC_FUNC_LABEL;
    ud->rv = CHSQ_TIMEOUT;
    for(;;) {
        unsigned long t;
        struct timeval now;
        struct timespec timeToWait;
        const int timeout =  buf ? 150000 : 150000; /* 50000 : 1000 time out for data | time out for check multi thread */

        pthread_mutex_lock(&kev->ta->async_func_mutex);
        if(MULTITHREADED()) LOCK_CV();
        ksnetEvMgrAsync(kev, buf, buf_length, (void*)ud);
      /*retrive:*/
        SET_TIMEOUT();
        if(MULTITHREADED()) pthread_cond_timedwait(&kev->ta->cv_threshold, &kev->ta->cv_mutex, &timeToWait);
        if(MULTITHREADED() && ud->rv == CHSQ_TIMEOUT) {
            timeouts++;
            check_retrives++;
            kev->ta->f_multi_thread = check_retrives < CHECK_RETRIVES ? NOT_DEFINED_MULTITHREAD : NO_MULTITHREAD;
            ksn_puts(kev, MODULE, DEBUG, "MULTITHREAD timeout");
            UNLOCK_CV();
        }
        if(MULTITHREADED()) UNLOCK_CV();
        pthread_mutex_unlock(&kev->ta->async_func_mutex);

        if(MULTITHREADED() && (ud->rv >= SEND_IF || ud->rv == CHSQ_TIMEOUT) && kev->runEventMgr) usleep(5000);
        else break;
    }
    retval = ud->rv;
    if(MULTITHREADED()) { free(ud); ud_count--; }
    if(buf) free(buf);

    return retval;
}

// check thread
static inline int _check_thread(void *ke) {
    return pthread_self() != kev->ta->t_id;
}

// check multi thread
// return
// retval MULTITHREAD (1) - if multi thread run correctly;
// retval NO_MULTITHREAD (2) - if multi thread run incorrectly (no multi thread, like in nodejs);
static int _check_multi_thread(void *ke) {

    if(!kev->ksn_cfg.no_multi_thread_f && kev->ta->f_multi_thread != MULTITHREAD && check_retrives < CHECK_RETRIVES) {
        kev->ta->f_multi_thread = MULTITHREAD;
        kev->ta->f_multi_thread = SEND_ASYNC(ke, NULL, 0) == 0 ? MULTITHREAD : NO_MULTITHREAD;
        ksn_printf(kev, MODULE, DEBUG, "Set MULTITHREAD mode: %s\n",
            kev->ta->f_multi_thread == MULTITHREAD ? "MULTITHREAD" : "NO MULTITHREAD");
        if(kev->ta->f_multi_thread == MULTITHREAD) check_retrives = 0;
    }

    return kev->ta->f_multi_thread;
}

// type #1 KSN_CORE_SEND_CMD_TO
void ksnCoreSendCmdtoA(void *ke, const char *peer, uint8_t cmd, void *data,
        size_t data_length) {

    if(_check_thread(ke)) {
        if(kev->ta->test)
            ksn_printf(kev, MODULE, DEBUG /*DEBUG_VV*/, // \TODO set DEBUG_VV
                "ksnCoreSendCmdtoA: %s %d %s %d\n", peer, cmd, data, data_length);

        int ptr = 0;
        const uint8_t f_type = KSN_CORE_SEND_CMD_TO;
        const uint8_t peer_length = strlen(peer) + 1;

        // Create buffer: { f_type, cmd, peer_length, peer, data }
        size_t buf_length = sizeof(uint8_t)*3 + peer_length + data_length;
        void *buf = malloc(buf_length);
        *(uint8_t*)(buf + ptr) = f_type; ptr++;
        *(uint8_t*)(buf + ptr) = cmd; ptr++;
        *(uint8_t*)(buf + ptr) = peer_length; ptr++;
        memcpy(buf + ptr, peer, peer_length); ptr += peer_length;
        memcpy(buf + ptr, data, data_length);

        // Send sync and clear buffer
        SEND_ASYNC(ke, buf, buf_length);
    }
    else { ksnCoreSendCmdto(kev->kc, (char*)peer, cmd, data, data_length); }
}

// type #2 TEO_SSCR_SEND
void teoSScrSendA(void *ke, uint16_t event, void *data, size_t data_length,
        uint8_t cmd) {

    if(_check_thread(ke)) {
        if(kev->ta->test)
            ksn_printf(kev, MODULE, DEBUG /*DEBUG_VV*/, // \TODO set DEBUG_VV
                "teoSScrSendA: %d %s %d %d\n", event, data, data_length, cmd);

        int ptr = 0;
        const uint8_t f_type = TEO_SSCR_SEND;

        // Create buffer: { f_type, cmd, event, data }
        size_t buf_length = sizeof(uint8_t)*2 + sizeof(uint16_t) + data_length;
        void *buf = malloc(buf_length);
        *(uint8_t*)(buf + ptr) = f_type; ptr++;
        *(uint8_t*)(buf + ptr) = cmd; ptr++;
        *(uint16_t*)(buf + ptr) = event; ptr += sizeof(uint16_t);
        memcpy(buf + ptr, data, data_length);

        // Send sync and clear buffer
        SEND_ASYNC(ke, buf, buf_length);
    }
    else teoSScrSend(kev->kc->kco->ksscr, event, data, data_length, cmd);
}

// type #3 SEND_CMD_ANSWER_TO
void sendCmdAnswerToBinaryA(void *ke, void *rdp, uint8_t cmd, void *data,
        size_t data_length) {

    ksnCorePacketData *rd = rdp;
    const uint8_t addr_length = strlen(rd->addr) + 1;

    if(_check_thread(ke)) {
        if(kev->ta->test)
            ksn_printf(kev, MODULE, DEBUG /*DEBUG_VV*/, // \TODO set DEBUG_VV
                "sendCmdAnswerToBinaryA: %d %d %d %d %s %s %d %s %d\n",
                cmd, rd->l0_f, addr_length, rd->from_len, rd->addr, rd->from,
                rd->port, data, data_length);

        int ptr = 0;
        const uint8_t f_type = SEND_CMD_ANSWER_TO;

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
        *(uint32_t*)(buf + ptr) = rd->port; ptr += sizeof(uint32_t);
        memcpy(buf + ptr, data, data_length);

        // Send sync and clear buffer
        SEND_ASYNC(ke, buf, buf_length);
    }
    else {
        if (rd->l0_f) ksnLNullSendToL0(ke, (char*)rd->addr, rd->port, (char*)rd->from, rd->from_len, cmd, data, data_length);
        else ksnCoreSendto(kev->kc, (char*)rd->addr, rd->port, cmd, data, data_length);
    }
}

// type #4 TEO_SSCR_SUBSCRIBE
void teoSScrSubscribeA(teoSScrClass *sscr, char *peer, uint16_t ev) {

    void *ke = sscr->ke;

    if(_check_thread(ke)) {
        const uint8_t peer_length = strlen(peer) + 1;

        if(kev->ta->test)
            ksn_printf(kev, MODULE, DEBUG /*DEBUG_VV*/, // \TODO set DEBUG_VV
                "teoSScrSubscribeA: %d %d %d %s\n", 0, peer_length, ev, peer);

        int ptr = 0;
        const uint8_t f_type = TEO_SSCR_SUBSCRIBE;

        // Create buffer: { f_type, cmd, peer_length, ev, peer }
        size_t buf_length = sizeof(uint8_t)*3 + sizeof(uint16_t) + peer_length;
        void *buf = malloc(buf_length);
        *(uint8_t*)(buf + ptr) = f_type; ptr++;
        *(uint8_t*)(buf + ptr) = 0; ptr++; // cmd
        *(uint8_t*)(buf + ptr) = peer_length; ptr++;
        *(uint16_t*)(buf + ptr) = ev; ptr += sizeof(uint16_t);
        memcpy(buf + ptr, peer, peer_length);

        // Send sync and clear buffer
        SEND_ASYNC(ke, buf, buf_length);
    }
    else teoSScrSubscribe(kev->kc->kco->ksscr, (char*)peer, ev);
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
