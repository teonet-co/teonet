/**
 * File:   ev_mgr.h
 * Author: Kirill Scherba
 *
 * Created on April 11, 2015, 2:13 AM
 */

#ifndef EV_MGR_H
#define	EV_MGR_H

#include <ev.h>

#include "config/opt.h"
#include "config/conf.h"
#include "utils/utils.h"

#include "hotkeys.h"
//#include "modules.h"
#include "modules/vpn.h"

//#include "net_tcp.h"
//#include "net_tun.h"
//#include "net_core.h"
//#include "net_term.h"

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
    EV_K_IDLE,          ///< Idle check host events (after 11.5 after last host send or receive data)
    EV_K_TIMER,         ///< Timer event
    EV_K_ASYNC          ///< Async event

} ksnetEvMgrEvents;

/**
 * KSNet event manager functions data
 */
typedef struct ksnetEvMgrClass {

    // Pointers to Modules classes
    ksnCoreClass *kc;  ///< KSNet core class
    ksnetHotkeysClass *kh; ///< Hotkeys class
//    ksnModulesClass *km; ///< Modules class
    ksnVpnClass *kvpn; ///< VPN class
//    ksnTcpClass *kt; /// TCP Client/Server class
//    ksnTermClass *kter; // Terminal class
//    ksnTunClass *ktun; // Tunnel class
//
    ksnet_cfg ksn_cfg; ///< KSNet configuration

    int runEventMgr; ///< Run even manages (stop if 0)
    uint32_t timer_val; ///< Event loop timer value
    uint32_t idle_count; ///< Idle callback count
    uint32_t idle_activity_count; ///< Idle activity callback count
    void (*event_cb)(struct ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data, size_t data_len, void *user_data);
    struct ev_loop *ev_loop; ///< Event loop

    // Event Manager Watchers
    ev_idle idle_w;         ///< Idle TIMER watcher
    ev_idle idle_stdin_w;   ///< Idle STDIN watcher
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
    int port
);
int ksnetEvMgrRun(ksnetEvMgrClass *ke);
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
