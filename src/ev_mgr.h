/**
 * File:   ev_mgr.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on April 11, 2015, 2:13 AM
 */

#ifndef EV_MGR_H
#define	EV_MGR_H

#include <unistd.h>
#include <pthread.h>

#include <ev.h>

#include "config/opt.h"
#include "config/conf.h"
#include "utils/utils.h"

#include "hotkeys.h"
#include "modules/vpn.h"
#include "modules/cque.h"
#include "modules/teodb.h"
#include "modules/stream.h"
#include "modules/net_tcp.h"
#include "modules/net_tun.h"
#include "modules/net_term.h"
#include "modules/tcp_proxy.h"
#include "modules/l0-server.h"

extern const char *null_str;
#define NULL_STR (void*) null_str

#define CHECK_EVENTS_AFTER 11.5

/**
 * KSNet event manager events
 */
typedef enum ksnetEvMgrEvents {

    /**
     * #0 Calls immediately after event manager starts
     * 
     * Parameters of Teonet Events callback function:
     * 
     * @param ke Pointer to ksnetEvMgrClass
     * @param event This event
     * @param data NULL
     * @param data_len 0
     * @param user_data NULL
     */
    EV_K_STARTED,       // #0  Calls immediately after event manager starts
            
    /**
     * #1 Calls before event manager stopped
     * 
     * Parameters of Teonet Events callback function:
     * 
     * @param ke Pointer to ksnetEvMgrClass
     * @param event This event
     * @param data NULL
     * @param data_len 0
     * @param user_data NULL
     */            
    EV_K_STOPPED_BEFORE,// #1  Calls before event manager stopped
            
    /**
     * #2  Calls after event manager stopped
     * 
     * Parameters of Teonet Events callback function:
     * 
     * @param ke Pointer to ksnetEvMgrClass
     * @param event This event
     * @param data NULL
     * @param data_len 0
     * @param user_data NULL
     */                        
    EV_K_STOPPED,       // #2  Calls after event manager stopped
            
    /**
     * #3 New peer connected to host event
     * 
     * Parameters of Teonet Events callback function:
     * 
     * @param ke Pointer to ksnetEvMgrClass
     * @param event This event
     * @param data Pointer to ksnCorePacketData
     * @param data_len Size of ksnCorePacketData
     * @param user_data NULL
     */
    EV_K_CONNECTED,     // #3  New peer connected to host
            
    /**
     * #4  A peer was disconnected from host
     * 
     * Parameters of Teonet Events callback function:
     * 
     * @param ke Pointer to ksnetEvMgrClass
     * @param event This event
     * @param data Pointer to ksnCorePacketData
     * @param data_len Size of ksnCorePacketData
     * @param user_data NULL
     */
    EV_K_DISCONNECTED,  // #4  A peer was disconnected from host
            
    /**
     * #5  This host Received a data
     * 
     * Parameters of Teonet Events callback function:
     * 
     * @param ke Pointer to ksnetEvMgrClass
     * @param event This event
     * @param data Pointer to ksnCorePacketData
     * @param data_len Size of ksnCorePacketData
     * @param user_data NULL
     * 
     */
    EV_K_RECEIVED,      // #5  This host Received a data
    EV_K_RECEIVED_WRONG,///< #6  Wrong packet received
    /**
     * #7 This host Received ACK to sent data
     * 
     * Parameters of Teonet Events callback function:
     * 
     * @param ke Pointer to ksnetEvMgrClass
     * @param event This event
     * @param data Pointer to ksnCorePacketData
     * @param data_len Size of ksnCorePacketData
     * @param user_data Pointer to packet ID
     */
    EV_K_RECEIVED_ACK,  // #7  This host Received ACK to sent data
    EV_K_IDLE,          ///< #8  Idle check host events (after 11.5 after last host send or receive data)
    EV_K_TIMER,         ///< #9  Timer event
            
    /**
     * #10 Hotkey event
     * 
     * Parameters of Teonet Events callback function:
     * 
     * @param ke Pointer to ksnetEvMgrClass
     * @param event This event
     * @param data Pointer to integer hotkey
     * @param data_len Size of integer
     * @param user_data Pointer to raw keyboard input buffer
     */
    EV_K_HOTKEY,        // #10 Hotkey event
    EV_K_USER,          ///< #11 User press A hotkey
    EV_K_ASYNC,         ///< #12 Async event   
            
    /**
     * #13 After terminal started (in place to define commands 
     * 
     * Parameters of Teonet Events callback function:
     * 
     * @param ke Pointer to ksnetEvMgrClass
     * @param event This event
     * @param data NULL
     * @param data_len 0
     * @param user_data NULL
     */
    EV_K_TERM_STARTED,  // #13 After terminal started (in place to define commands 
            
    /**
     * #14 Teonet Callback QUEUE event
     * 
     * Parameters of Teonet Events callback function:
     * 
     * @param ke Pointer to ksnetEvMgrClass
     * @param event This event
     * @param data Pointer to ksnCQueData
     * @param data_len Size of ksnCQueData structure
     * @param user_data Pointer to integer with type of this event: 
     *                  1 - success; 0 - timeout
     */
    EV_K_CQUE_CALLBACK,             // #14 Teonet Callback QUEUE event
            
    EV_K_STREAM_CONNECTED,          ///< #15 After stream connected
    EV_K_STREAM_CONNECT_TIMEOUT,    ///< #16 Connection timeout
    EV_K_STREAM_DISCONNECTED,       ///< #17 After stream disconnected
    EV_K_STREAM_DATA,               ///< #18 Input stream has a data
            
    EV_K_SUBSCRIBE,                 ///< #19 Subscribe answer command received
    EV_K_SUBSCRIBED,                ///< #20 A peer subscribed to event at this host
            
    EV_K_L0_CONNECTED,              ///< #21 New L0 client connected to L0 server
    EV_K_L0_DISCONNECTED,           ///< #22 A L0 client was disconnected from L0 server
    EV_K_L0_NEW_VISIT,              ///< #23 New clients visit event to all subscribers (equal to L0_CONNECTED but send number of visits)
            
    EV_H_HWS_EVENT,                 ///< #24 HTTP WebSocket server event, data - depends of event, user data - poiter to hws_event

    EV_U_RECEIVED,                  ///< #25 UNIX socket received data event, data - data received from unix socket, user_data - pointer to the usock_class 
            
    EV_D_SET,                       ///< #26 Database updated
            
    /**
     * #27 Angular interval event happened
     * 
     * This event sends by Angular Teonet Service running in Teonet Node 
     * Application when Angular interval tick happened
     * 
     * Parameters of Teonet Events callback function:
     * 
     * @param ke Pointer to ksnetEvMgrClass
     * @param event This event
     * @param data NULL
     * @param data_len 0
     * @param user_data NULL
     */
    EV_A_INTERVAL,                  // #27 Angular interval event happened
            
    EV_K_APP_USER = 0x8000          ///< #0x8000 Teonet based Applications events

} ksnetEvMgrEvents;

/**
 * Application parameters user data
 */
typedef struct ksnetEvMgrAppParam {
    
    int app_argc;
    char **app_argv;
    char **app_descr;
    
} ksnetEvMgrAppParam;

/**
 * KSNet event manager functions data
 */
typedef struct ksnetEvMgrClass {

    // Pointers to Modules classes
    void *km; ///< Pointer to multi net class
    ksnCoreClass *kc;  ///< KSNet core class
    ksnetHotkeysClass *kh; ///< Hotkeys class
    ksnVpnClass *kvpn; ///< VPN class
    ksnTcpClass *kt; ///< TCP Client/Server class
    ksnLNullClass *kl; ///< L0 Server class
    ksnTCPProxyClass *tp; ///< TCP Proxy class
    ksnTunClass *ktun; ///< Tunnel class
    ksnTermClass *kter; ///< Terminal class
    ksnCQueClass *kq; ///< Callback QUEUE class
    ksnTDBClass *kf; ///< PBL KeyFile class
    ksnStreamClass *ks; ///< Stream class

    ksnet_cfg ksn_cfg; ///< KSNet configuration

    int runEventMgr; ///< Run even manages (stop if 0)
    uint32_t timer_val; ///< Event loop timer value
    uint32_t idle_count; ///< Idle callback count
    uint32_t idle_activity_count; ///< Idle activity callback count
    void (*event_cb)(struct ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data, size_t data_len, void *user_data);
    struct ev_loop *ev_loop; ///< Event loop

    // Event Manager Watchers
    ev_idle idle_w;         ///< Idle TIMER watcher
    ev_idle idle_activity_w;///< Idle Check activity watcher
    ev_timer timer_w;       ///< Timer watcher
    ev_async sig_async_w;   ///< Async signal watcher

    double custom_timer_interval;   ///< Custom timer interval
    double last_custom_timer;       ///< Last time the custom timer called

    PblList* async_queue;   ///< Async data queue
    pthread_mutex_t async_mutex; ///< Async data queue mutex

    size_t n_num; ///< Network number
    void *n_prev; ///< Previouse network
    void *n_next; ///< Next network
    size_t num_nets; ///< Number of networks

    // Define signals watchers
    ev_signal sigint_w;  ///< Signal SIGINT watcher
    ev_signal sigterm_w; ///< Signal SIGTERM watcher
    ev_signal sigsegv_w; ///< Signal SIGSEGV watcher
    ev_signal sigabrt_w; ///< Signal SIGABRT watcher
    #ifndef HAVE_MINGW
    ev_signal sigquit_w; ///< Signal SIGQUIT watcher
    ev_signal sigkill_w; ///< Signal SIGKILL watcher
    ev_signal sigstop_w; ///< Signal SIGSTOP watcher
    #endif
    
    void *user_data; ///< Pointer to user data or NULL if absent
    
    struct cli_def *cli;
    
    int argc;         ///< Applications argc
    char** argv;      ///< Applications argv  
    
    char *app_type;         ///< Application type
    char *app_version;  ///< Application version

} ksnetEvMgrClass;

/**
 * STDIN idle watcher data
 */
typedef struct stdin_idle_data {

    ksnetEvMgrClass *ke;
    void *data;
    ev_io *stdin_w;

} stdin_idle_data;

typedef void (*ksn_event_cb_type)(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data, size_t data_len, void *user_data);

#ifdef	__cplusplus
extern "C" {
#endif

    
ksnetEvMgrClass *ksnetEvMgrInit(
    int argc, char** argv,
    //void (*event_cb)(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data, size_t data_len, void *user_data),
    ksn_event_cb_type event_cb,
    int options
);
ksnetEvMgrClass *ksnetEvMgrInitPort(
    int argc, char** argv,
    //void (*event_cb)(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data, size_t data_len, void *user_data),
    ksn_event_cb_type event_cb,
    int options,
    int port,
    void *user_data
);
int ksnetEvMgrRun(ksnetEvMgrClass *ke);
int ksnetEvMgrFree(ksnetEvMgrClass *ke, int free_async);
#ifdef TEO_THREAD
int ksnetEvMgrRunThread(ksnetEvMgrClass *ke);
#endif
void ksnetEvMgrStop(ksnetEvMgrClass *ke);
int ksnetEvMgrRestart(int argc, char **argv);
void ksnetEvMgrAsync(ksnetEvMgrClass *ke, void *data, size_t data_len, void *user_data);
double ksnetEvMgrGetTime(ksnetEvMgrClass *ke);
char* ksnetEvMgrGetHostName(ksnetEvMgrClass *ke);
host_info_data *teoGetHostInfo(ksnetEvMgrClass *ke, size_t *hd_len);
void ksnetEvMgrSetCustomTimer(ksnetEvMgrClass *ke, double time_interval);

const char *teoGetLibteonetVersion();
void teoSetAppType(ksnetEvMgrClass *ke, char *type);
void teoSetAppVersion(ksnetEvMgrClass *ke, char *version);
const char *teoGetAppType(ksnetEvMgrClass *ke);
const char *teoGetAppVersion(ksnetEvMgrClass *ke);

int remove_peer_addr(ksnetEvMgrClass *ke, __CONST_SOCKADDR_ARG addr);

#ifdef	__cplusplus
}
#endif

#endif	/* EV_MGR_H */
