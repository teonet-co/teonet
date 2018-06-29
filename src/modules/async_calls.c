#include "ev_mgr.h"
#include "async_calls.h"

#define MODULE "async_module"
#define kev ((ksnetEvMgrClass*)(ke))

void teoAsyncTest(void *ke);

// Logging Async call label
static const uint32_t ASYNC_LABEL = 0x77AA77AA;

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
                    int ptr = 0;
                    const uint8_t f_type = *(uint8_t*)data; ptr++;
                    const uint8_t cmd = *(uint8_t*)(data + ptr); ptr++;
                    switch(f_type) {
                        
                        // ksnCoreSendCmdtoA (sendCmdToBinaryA)
                        case 1: {
                            // Parse buffer: { f_type, cmd, peer_length, peer, data }
                            const uint8_t peer_length = *(uint8_t*)(data + ptr); ptr++;
                            const char *peer = (const char *)(data + ptr); ptr += peer_length;
                            void *d = data + ptr;
                            const size_t d_length = data_length - ptr;
                            if(kev->ta->test)
                                ksn_printf(kev, MODULE, DEBUG /*DEBUG_VV*/, // \TODO set DEBUG_VV
                                        "teoSScrSendA Test: %s %d %s %d\n", peer, cmd, d, d_length);
                            else ksnCoreSendCmdto(kev->kc, (char*)peer, cmd, d, d_length);
                            processed = 1;
                            
                        } break;
                        
                        // teoSScrSend (sendToSscrA)
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
                        
                        // sendCmdAnswerToBinaryA
                        case 3: {
                            //int retval = 0;
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
                            if (l0_f) /*retval =*/ ksnLNullSendToL0(ke, (char*)addr, port, (char*)from, from_length, cmd, d, d_length);
                            else /*retval =*/ ksnCoreSendto(kev->kc, (char*)addr, port, cmd, d, d_length);
                            processed = 1;
                        } break;
                        
                        // teoSScrSubscribeA (subscribeA)
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
    kev->event_cb = event_cb;
    ta->test = 0;

    #ifdef DEBUG_KSNET
    ksn_puts(kev, MODULE, DEBUG /*DEBUG_VV*/, // \TODO set DEBUG_VV
            "have been initialized");
    #endif

    return ta;
}

void teoAsyncDestroy(teoAsyncClass *ta) {

    ksnetEvMgrClass *ke = ta->ke;
    ke->event_cb = ta->event_cb;
    free(ta);
    ke->ta = NULL;

    #ifdef DEBUG_KSNET
    ksn_puts(ke, MODULE, DEBUG /*DEBUG_VV*/, // \TODO set DEBUG_VV
            "have been de-initialized");
    #endif
}

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
    ksnetEvMgrAsync(kev, buf, buf_length, (void*)&ASYNC_LABEL); 
    free(buf);
}

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
    ksnetEvMgrAsync(kev, buf, buf_length, (void*)&ASYNC_LABEL); 
    free(buf);
}

void sendCmdAnswerToBinaryA(void *ke, void *rdp, uint8_t cmd, void *data,
        size_t data_length) {
    
    ksnCorePacketData *rd = rdp;
    
    const uint8_t addr_length = strlen(rd->addr) + 1;
    
    if(kev->ta->test)
        ksn_printf(kev, MODULE, DEBUG /*DEBUG_VV*/, // \TODO set DEBUG_VV
            "sendCmdAnswerToBinaryA: %d %d %d %d %s %s %d %s %d\n", cmd, rd->l0_f, addr_length, rd->from_len, rd->addr, rd->from, rd->port, data, data_length);

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
    ksnetEvMgrAsync(kev, buf, buf_length, (void*)&ASYNC_LABEL); 
    free(buf);
}

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
    ksnetEvMgrAsync(kev, buf, buf_length, (void*)&ASYNC_LABEL); 
    free(buf);
}

void teoAsyncTest(void *ke) {
    
    #ifdef DEBUG_KSNET
    ksn_puts(kev, MODULE, DEBUG /*DEBUG_VV*/, // \TODO set DEBUG_VV
            "start test");
    #endif
        
    kev->ta->test = 1;
    
    // 1
    ksnCoreSendCmdtoA(ke, "teo-test-peer", 129, "Hello", 6);
    
    // 2
    teoSScrSendA(ke, 0x8000, "Hello", 6, 129);
    
    // 3
    ksnCorePacketData rd;
    rd.addr = "127.0.0.1";
    rd.port = 9010;
    rd.from = "teo-test-peer";
    rd.from_len = 14;
    rd.l0_f = 0;
    sendCmdAnswerToBinaryA(ke, &rd, 129, "Hello", 6);
    
    // 4
    teoSScrSubscribeA(kev->kc->kco->ksscr, "teo-test-peer", 0x8000);
}
