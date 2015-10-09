/**
 * File:   ev_mgr.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on April 11, 2015, 2:13 AM
 */

#ifndef EV_MGR_H
#define	EV_MGR_H

#include <ev.h>
#include <pthread.h>

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

/**
 * KSNet event manager events
 */
typedef enum ksnetEvMgrEvents {

    EV_K_STARTED,       ///< Calls immediately after event manager starts
    EV_K_STOPPED,       ///< Calls after event manager stopped
    EV_K_CONNECTED,     ///< New peer connected to host
    EV_K_DISCONNECTED,  ///< A peer was disconnected from host
    EV_K_RECEIVED,      ///< This host Received a data
    EV_K_RECEIVED_WRONG,///< Wrong packet received
    EV_K_RECEIVED_ACK,  ///< This host Received ACK to sent data
    EV_K_IDLE,          ///< Idle check host events (after 11.5 after last host send or receive data)
    EV_K_TIMER,         ///< Timer event
    EV_K_HOTKEY,        ///< Hotkey event
    EV_K_USER,          ///< User press U hotkey
    EV_K_ASYNC,         ///< Async event           
    EV_K_TERM_STARTED,  ///< After terminal started (in place to define commands 
    /**
     * Teonet Callback QUEUE event. 
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
    EV_K_CQUE_CALLBACK,
            
    EV_K_STREAM_CONNECTED,          ///< After stream connected
    EV_K_STREAM_CONNECT_TIMEOUT,    ///< Connection timeout
    EV_K_STREAM_DISCONNECTED,       ///< After stream disconnected
    EV_K_STREAM_DATA                ///< Input stream has a data

} ksnetEvMgrEvents;

/**
 * Application parameters user data
 */
typedef struct ksnetEvMgrAppParam {
    
    int app_argc;
    char **app_argv;
    
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
    #ifndef HAVE_MINGW
    ev_signal sigquit_w; ///< Signal SIGQUIT watcher
    ev_signal sigkill_w; ///< Signal SIGKILL watcher
    ev_signal sigstop_w; ///< Signal SIGSTOP watcher
    #endif

    void *user_data; ///< Pointer to user data or NULL if absent
    
    struct cli_def *cli;

} ksnetEvMgrClass;

/**
 * STDIN idle watcher data
 */
typedef struct stdin_idle_data {

    ksnetEvMgrClass *ke;
    void *data;
    ev_io *stdin_w;

} stdin_idle_data;


#ifdef	__cplusplus
extern "C" {
#endif

ksnetEvMgrClass *ksnetEvMgrInit(
    int argc, char** argv,
    void (*event_cb)(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data, size_t data_len, void *user_data),
    int options
);
ksnetEvMgrClass *ksnetEvMgrInitPort(
    int argc, char** argv,
    void (*event_cb)(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data, size_t data_len, void *user_data),
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
void ksnetEvMgrAsync(ksnetEvMgrClass *ke, void *data, size_t data_len, void *user_data);
double ksnetEvMgrGetTime(ksnetEvMgrClass *ke);
char* ksnetEvMgrGetHostName(ksnetEvMgrClass *ke);
void ksnetEvMgrSetCustomTimer(ksnetEvMgrClass *ke, double time_interval);

#ifdef	__cplusplus
}
#endif

#endif	/* EV_MGR_H */
