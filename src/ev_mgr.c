/**
 * File:   ev_mgr.c
 * Author: Kirill Scherba
 *
 * Created on April 11, 2015, 2:13 AM
 *
 * Event manager module
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>    // For mutex and TEO_TREAD

#include "ev_mgr.h"
#include "utils/utils.h"
#include "utils/rlutil.h"

// Constants
#define KSNET_EVENT_MGR_TIMER 0.5
#define CHECK_EVENTS_AFTER 11.5

const char *null_str = "";

// Local functions
void idle_cb (EV_P_ ev_idle *w, int revents); // Timer idle callback
void idle_stdin_cb (EV_P_ ev_idle *w, int revents); // STDIN idle callback
void idle_activity_cb(EV_P_ ev_idle *w, int revents); // Idle activity callback
void timer_cb (EV_P_ ev_timer *w, int revents); // Timer callback
void host_cb (EV_P_ ev_io *w, int revents); // Host callback
void sig_async_cb (EV_P_ ev_async *w, int revents); // Async signal callback
void sigint_cb (struct ev_loop *loop, ev_signal *w, int revents); // SIGINT callback
void stdin_cb (EV_P_ ev_io *w, int revents); // STDIN callback
int modules_init(ksnetEvMgrClass *ke); // Initialize modules
void modules_destroy(ksnetEvMgrClass *ke); // Deinitialize modules

/**
 * Initialize KSNet Event Manager and network
 *
 * @param argc Number of applications arguments (from main)
 * @param argv Applications arguments array (from main)
 * @param event_cb Events callback function called when an event happens
 * @param options Options set: \n
 *                READ_OPTIONS - read options from command line parameters; \n
 *                READ_CONFIGURATION - read options from configuration file
 * @return Pointer to created ksnetEvMgrClass
 */
ksnetEvMgrClass *ksnetEvMgrInit(

  int argc, char** argv,
  void (*event_cb)(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data, size_t data_len, void *user_data),
  int options
  //void (*read_cl_params)(ksnetEvMgrClass *ke, int argc, char** argv)
  //void (*read_config)(ksnet_cfg *conf, int port_param)
    ) {

    ksnetEvMgrClass *ke = malloc(sizeof(ksnetEvMgrClass));
    ke->event_cb = event_cb;
    ke->custom_timer_interval = 0.0;
    ke->last_custom_timer = 0.0;
    
    // Initialize async mutex
    pthread_mutex_init(&ke->async_mutex, NULL);

    // KSNet parameters
    const int app_argc = 1;             // number of application arguments
    char *app_argv[app_argc];           // array for argument names
    app_argv[0] = (char*)"peer_name";   // peer name argument name
    //app_argv[1] = (char*)"file_name";   // file name argument name

    // Initial configuration, set defaults, read defaults from command line
    ksnet_configInit(&ke->ksn_cfg, ke); // Set configuration default
    if(options&READ_OPTIONS) ksnet_optRead(argc, argv, &ke->ksn_cfg, app_argc, app_argv, 1); // Read command line parameters (to use it as default)
    if(options&READ_CONFIGURATION) read_config(&ke->ksn_cfg, ke->ksn_cfg.port); // Read configuration file parameters
    if(options&READ_OPTIONS) ksnet_optRead(argc, argv, &ke->ksn_cfg, app_argc, app_argv, 0); // Read command line parameters (to replace configuration file)
    
    return ke;
}

/**
 * Stop event manager
 *
 * @param ke
 */
void ksnetEvMgrStop(ksnetEvMgrClass *ke) {

    ke->runEventMgr = 0;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
/**
 * Start KSNet Event Manager and network communication
 *
 * Start KSNet Event Manager and network communication
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @return Alway 0
 */
int ksnetEvMgrRun(ksnetEvMgrClass *ke) {

    #ifdef DEBUG_KSNET
    //ksnet_printf(&ke->ksn_cfg, DEBUG, "Event manager: started ...\n");
    printf("%sEvent manager:%s started ...\n", ANSI_CYAN, ANSI_NONE);
    #endif

    ke->runEventMgr = 1;
    ke->timer_val = 0;
    ke->idle_count = 0;
    ke->idle_activity_count = 0;

    // Event loop
    struct ev_loop *loop = ev_loop_new (0); //EV_DEFAULT;
    ke->ev_loop = loop;

    // Define watchers
    ev_io stdin_w;       // STDIN watcher
    ev_signal sigint_w;  // Signal SIGINT watcher
    ev_signal sigterm_w; // Signal SIGTERM watcher
    #ifndef HAVE_MINGW
    ev_signal sigquit_w; // Signal SIGQUIT watcher
    ev_signal sigkill_w; // Signal SIGKILL watcher
    ev_signal sigstop_w; // Signal SIGSTOP watcher
    #endif

    // Initialize modules
    if(modules_init(ke)) {

        // Initialize idle watchers
        ev_idle_init (&ke->idle_w, idle_cb);
        ke->idle_w.data = ke->kc;

        // Initialize STDIN idle watchers
        ev_idle_init (&ke->idle_stdin_w, idle_stdin_cb);

        // Initialize Check activity watcher
        ev_idle_init (&ke->idle_activity_w, idle_activity_cb);
        ke->idle_activity_w.data = ke;

        // Initialize and start main timer watcher, it is a repeated timer
        ev_timer_init (&ke->timer_w, timer_cb, 0.0, KSNET_EVENT_MGR_TIMER);
        ke->timer_w.data = ke;
        ev_timer_start (loop, &ke->timer_w);

        // Initialize and start signals watchers
        // SIGINT
        ev_signal_init (&sigint_w, sigint_cb, SIGINT);
        sigint_w.data = ke;
        ev_signal_start (loop, &sigint_w);

        // SIGQUIT
        #ifdef SIGQUIT
        ev_signal_init (&sigquit_w, sigint_cb, SIGQUIT);
        sigquit_w.data = ke;
        ev_signal_start (loop, &sigquit_w);
        #endif

        // SIGTERM
        #ifdef SIGTERM
        ev_signal_init (&sigterm_w, sigint_cb, SIGTERM);
        sigterm_w.data = ke;
        ev_signal_start (loop, &sigterm_w);
        #endif

        // SIGKILL
        #ifdef SIGKILL
        ev_signal_init (&sigkill_w, sigint_cb, SIGKILL);
        sigkill_w.data = ke;
        ev_signal_start (loop, &sigkill_w);
        #endif

        // SIGSTOP
        #ifdef SIGSTOP
        ev_signal_init (&sigstop_w, sigint_cb, SIGSTOP);
        sigstop_w.data = ke;
        ev_signal_start (loop, &sigstop_w);
        #endif

        // Initialize and start keyboard input watcher
        ev_init (&stdin_w, stdin_cb);
        ev_io_set (&stdin_w, STDIN_FILENO, EV_READ);
        stdin_w.data = ke;
        ev_io_start (loop, &stdin_w);

        // Initialize and start a async signal watcher for event add
        ke->async_queue = pblListNewArrayList();
        ev_async_init (&ke->sig_async_w, sig_async_cb);
        ke->sig_async_w.data = ke;
        ev_async_start (loop, &ke->sig_async_w);

        // Run event loop
        ev_run(loop, 0);
        
        // Free async data queue
        pblListFree(ke->async_queue);
        pthread_mutex_destroy(&ke->async_mutex);
    }

    // Destroy modules
    modules_destroy(ke);

    // Destroy event loop (and free memory)
    ev_loop_destroy(loop);

    #ifdef DEBUG_KSNET
    //ksnet_printf(&ke->ksn_cfg, DEBUG, "Event manager: stopped.\n");
    printf("%sEvent manager:%s stopped.\n", ANSI_CYAN, ANSI_NONE);
    #endif

    // Send stopped event to user level
    if(ke->event_cb != NULL) ke->event_cb(ke, EV_K_STOPPED, NULL, 0, NULL);

    // Free memory
    free(ke);

    return 0;
}

#ifdef TEO_THREAD
void *_ksnetEvMgrRunThread(void *ke) {
   
    ksnetEvMgrRun(ke);
    
    return NULL;
}

int ksnetEvMgrRunThread(ksnetEvMgrClass *ke) {
    
    // Start fossa thread
    int err = pthread_create(&ke->tid, NULL, &_ksnetEvMgrRunThread, ke);
    if (err != 0) printf("\nCan't create thread :[%s]", strerror(err));
    else printf("\nThread created successfully\n");
    
    return err;
}
#endif

/**
 * Set custom timer interval
 *
 * @param ke
 * @param time_interval
 */
void ksnetEvMgrSetCustomTimer(ksnetEvMgrClass *ke, double time_interval) {

    ke->custom_timer_interval = time_interval;
}

/**
 * Return host name
 *
 * @param ke
 * @return
 */
char* ksnetEvMgrGetHostName(ksnetEvMgrClass *ke) {

    return ke->ksn_cfg.host_name;
}

/**
 * Async call to Event manager
 *
 * @param w_accept
 */
void ksnetEvMgrAsync(ksnetEvMgrClass *ke, void *data, size_t data_len, void *user_data) {

    #ifdef DEBUG_KSNET
    ksnet_printf(&ke->ksn_cfg, DEBUG_VV, 
            "%sEvent manager:%s make Async call to Event manager\n", 
            ANSI_CYAN, ANSI_NONE);
    #endif

    // Add something to queue and send async signal to event loop
    void* element = NULL;
    if(data != NULL) {
        element = malloc(data_len + sizeof(uint16_t) + sizeof(void*));
        size_t ptr = 0;
        *(void**)element = user_data; ptr += sizeof(void**);
        *(uint16_t*)(element + ptr) = (uint16_t)data_len; ptr += sizeof(uint16_t);
        memcpy(element + ptr, data, data_len);
    }
    pthread_mutex_lock (&ke->async_mutex);
    pblListAdd(ke->async_queue, element); 
    pthread_mutex_unlock (&ke->async_mutex);
    
    // Send async signal to process queue
    ev_async_send(ke->ev_loop,/*EV_DEFAULT_*/ &ke->sig_async_w); 
}

/**
 * Get KSNet event manager time
 *
 * @return
 */
double ksnetEvMgrGetTime(ksnetEvMgrClass *ke) {

    return ke->runEventMgr ? ev_now(ke->ev_loop) : 0.0;
}

/**
 * Connect to remote host
 *
 * @param ke
 */
void connect_r_host_cb(ksnetEvMgrClass *ke) {

    if(ke->ksn_cfg.r_host_addr[0] && !ke->ksn_cfg.r_host_name[0]) {

        // Create data with list of local IPs and port
        ksnet_stringArr ips = getIPs(); // IPs array
        uint8_t len = ksnet_stringArrLength(ips); // Max number of IPs
        void *data = malloc(len*16 + sizeof(uint8_t) + sizeof(uint32_t)); // Data
        size_t ptr = sizeof(uint8_t); // Pointer (to first IP)
        uint8_t *num = (uint8_t *) data; // Real number of IPs
        *num = 0;
        // Fill data with IPs and Port
        int i, ip_len;
        for(i=0; i < len; i++) {

            if(ip_is_private(ips[i])) {

                ip_len =  strlen(ips[i]) + 1;
                memcpy(data + ptr, ips[i], ip_len); ptr += ip_len;
                (*num)++;
            }
        }
        *((uint32_t *)(data + ptr)) = ke->kc->port; ptr += sizeof(uint32_t); // Port

        // Send data to r-host
        ksnCoreSendto(ke->kc, ke->ksn_cfg.r_host_addr, ke->ksn_cfg.r_port,
                      CMD_CONNECT_R, data, ptr);

        free(data);
        ksnet_stringArrFree(&ips);
    }
}

/**
 * Send packet to local address to open private firewall
 */
void open_local_port(ksnetEvMgrClass *ke) {

    // Create data with list of local IPs and port
    ksnet_stringArr ips = getIPs(); // IPs array
    uint8_t len = ksnet_stringArrLength(ips); // Max number of IPs

    // Send to local IPs and Port
    int i;
    for(i=0; i < len; i++) {

        if(ip_is_private(ips[i])) {

            // TODO: Need to send to real local IP
            uint8_t ip_arr[4];
            ip_to_array(ips[i], ip_arr);
            char *ip_str = ksnet_formatMessage("%d.%d.%d.93", ip_arr[0],
                                               ip_arr[1], ip_arr[2]);

            // Send to IP to open port
            ksnCoreSendto(ke->kc, ip_str, ke->ksn_cfg.r_port,
                      CMD_NONE, NULL_STR, 1);

            printf("Send to: %s:%d\n", ip_str, (int)ke->ksn_cfg.r_port);

            free(ip_str);
        }
    }

    ksnet_stringArrFree(&ips);
}

/**
 * Check connected peers, send trip time request and disconnect peer at timeout
 *
 * @param ka
 * @param peer_name
 * @param arp_data
 * @param data
 */
int check_connected_cb(ksnetArpClass *ka, char *peer_name,
                        ksnet_arp_data *arp_data, void *data) {

    #define kev ((ksnetEvMgrClass*)(ka->ke))

    int retval = 0;
    double ct = ksnetEvMgrGetTime(kev);

    // Send trip time request
    if(ct - arp_data->last_triptime_send > CHECK_EVENTS_AFTER) {
        ksnCommandSendCmdEcho(kev->kc->kco, peer_name, (void*) TRIPTIME,
                              TRIPTIME_LEN);
    }

    // Disconnect dead peer
    else if(ct - arp_data->last_acrivity > CHECK_EVENTS_AFTER * 1.5) {

        ksnCorePacketData rd;
        rd.from = peer_name;
        cmd_disconnected_cb(kev->kc->kco, &rd);

        retval = 1;
    }

    return retval;

    #undef kev
}

/**
 * Idle callback (runs every 11.5 second of Idle time)
 *
 * This callback started by event manager timer after every
 * CHECK_EVENTS_AFTER = 11.5 second. Send two user event:
 * first EV_K_STARTED sent at startup (when application started)
 * and second EV_K_IDLE sent every tick.
 *
 * @param loop
 * @param w
 * @param revents
 */
void idle_cb (EV_P_ ev_idle *w, int revents) {

    #define kev ((ksnetEvMgrClass *)((ksnCoreClass *)w->data)->ke)

    #ifdef DEBUG_KSNET
    ksnet_printf(&kev->ksn_cfg, DEBUG_VV, "%sEvent manager:%s idle callback %d\n", 
            ANSI_CYAN, ANSI_NONE,            
            kev->idle_count);
    #endif

    // Stop this watcher
    ev_idle_stop(EV_A_ w);

    // Idle count startup (first time run)
    if(!kev->idle_count) {
        // TODO:       open_local_port(kev);
        if(kev->event_cb != NULL) kev->event_cb(kev, EV_K_STARTED, NULL, 0, NULL);
        connect_r_host_cb(kev);
    }
    // Idle count max value
    else if(kev->idle_count == UINT32_MAX) kev->idle_count = 0;

    // Increment count
    kev->idle_count++;

    // Check host events to send him service information
    // TODO:    host_cb(EV_A_ (ev_io*)w, revents);

    // Send idle Event
    if(kev->event_cb != NULL) {
        kev->event_cb(kev, EV_K_IDLE , NULL, 0, NULL);
    }

    // Set last host event time
    ksnCoreSetEventTime(kev->kc);

    #undef kev
}

/**
 * Timer callback (runs every KSNET_EVENT_MGR_TIMER = 0.5 sec)
 *
 * @param loop
 * @param w
 * @param revents
 */
void timer_cb(EV_P_ ev_timer *w, int revents) {

    #ifdef DEBUG_KSNET
    const int show_interval = 5 / KSNET_EVENT_MGR_TIMER /* 10 */;
    #endif
    const int activity_interval = 11.5 / KSNET_EVENT_MGR_TIMER /* 23 */;
    ksnetEvMgrClass *ke = w->data;
    double t = ksnetEvMgrGetTime(ke);

    if(ke->runEventMgr) {

        // Increment timer value
        ke->timer_val++;

        // Show timer info
        #ifdef DEBUG_KSNET
        if( !(ke->timer_val % show_interval) ) {
            ksnet_printf(&((ksnetEvMgrClass *)w->data)->ksn_cfg, DEBUG_VV,
                    "%sEvent manager:%s timer (%.1f sec of %f)\n", 
                    ANSI_CYAN, ANSI_NONE,
                    show_interval*KSNET_EVENT_MGR_TIMER, t);

        }
        #endif

        // Send custom timer Event
        if(ke->event_cb != NULL &&
           ke->custom_timer_interval &&
           (t - ke->last_custom_timer > ke->custom_timer_interval)) {

            ke->last_custom_timer = t;
            ke->event_cb(ke, EV_K_TIMER , NULL, 0, NULL);
        }

        // Start idle watcher
        if(ksnetEvMgrGetTime(ke) - ke->kc->last_check_event > CHECK_EVENTS_AFTER) {
            ev_idle_start(EV_A_ & ke->idle_w);
        }

        // Check connected and request trip time
        if( !(ke->timer_val % activity_interval) ) {
            ev_idle_start(EV_A_ & ke->idle_activity_w);
        }
    }
    // Send break to exit from event loop
    else ev_break(EV_A_ EVBREAK_ALL);
}

/**
 * Signal to stop event loop callback
 *
 * @param loop
 * @param w
 * @param revents
 */
void sigint_cb (struct ev_loop *loop, ev_signal *w, int revents) {

    #ifdef DEBUG_KSNET
    ksnet_printf(&((ksnetEvMgrClass *)w->data)->ksn_cfg, DEBUG,
            "\n%sEvent manager:%s got a signal to stop event manager ...\n", 
            ANSI_CYAN, ANSI_NONE);
    #endif

    ((ksnetEvMgrClass *)w->data)->runEventMgr = 0;
}

/**
 * Async Event callback (Signal)
 *
 * @param loop
 * @param w
 * @param revents
 */
void sig_async_cb (EV_P_ ev_async *w, int revents) {

    #define kev ((ksnetEvMgrClass*)(w->data))

    #ifdef DEBUG_KSNET
    ksnet_printf(&kev->ksn_cfg, DEBUG_VV,
            "%sEvent manager:%s async event callback\n", 
            ANSI_CYAN, ANSI_NONE);
    #endif

    // Get data from async queue and send user event with it
    pthread_mutex_lock (&kev->async_mutex);
    while(!pblListIsEmpty(kev->async_queue)) {    
        size_t ptr = 0;
        void *data = pblListPoll(kev->async_queue);
        void *user_data = *(void**)data; ptr += sizeof(void**);
        uint16_t data_len = *(uint16_t*)(data + ptr); ptr += sizeof(uint16_t);
        kev->event_cb(kev, EV_K_ASYNC , data + ptr, data_len, user_data);
        free(data);
    }
    pthread_mutex_unlock (&kev->async_mutex);
    
    // Do something ...
    
    #undef kev
}

/**
 * STDIN idle watcher data
 */
typedef struct stdin_idle_data {

    ksnetEvMgrClass *ke;
    void *data;
    ev_io *stdin_w;

} stdin_idle_data;

/**
 * STDIN (has data) callback
 *
 * @param loop
 * @param w
 * @param revents
 */
void stdin_cb (EV_P_ ev_io *w, int revents) {

    #ifdef DEBUG_KSNET
    ksnet_printf(&((ksnetEvMgrClass *)w->data)->ksn_cfg, DEBUG_VV,
            "%sEvent manager:%s STDIN (has data) callback\n", 
            ANSI_CYAN, ANSI_NONE);
    #endif

    void *data;

    // Get string
    if(((ksnetEvMgrClass *)w->data)->kh->non_blocking == 0) {

        char *buffer = malloc(KSN_BUFFER_SM_SIZE);
        fgets(buffer, KSN_BUFFER_SM_SIZE , stdin);
        data = buffer;
    }
    // Get character
    else {
        int ch = getchar();
        putchar(ch);
        putchar('\n');
        data = malloc(sizeof(int));
        *(int*)data = ch;
    }

    // Create STDIN idle watcher data
    stdin_idle_data *id = malloc(sizeof(stdin_idle_data));
    id->ke = ((ksnetEvMgrClass *)w->data);
    id->data = data;
    id->stdin_w = w;
    ((ksnetEvMgrClass *)w->data)->idle_stdin_w.data = id;

    // Stop this watcher
    ev_io_stop(EV_A_ w);

    // Start STDIN idle watcher
    ev_idle_start(EV_A_ & ((ksnetEvMgrClass *)w->data)->idle_stdin_w);
}

/**
 * STDIN Idle callback: execute STDIN callback in idle time
 *
 * @param loop
 * @param w
 * @param revents
 */
void idle_stdin_cb(EV_P_ ev_idle *w, int revents) {

    #ifdef DEBUG_KSNET
    ksnet_printf(& ((stdin_idle_data *)w->data)->ke->ksn_cfg, DEBUG_VV,
                "%sEvent manager:%s STDIN idle (process data) callback (%c)\n", 
                ANSI_CYAN, ANSI_NONE,
                *((int*)((stdin_idle_data *)w->data)->data));
    #endif

    // Stop this watcher
    ev_idle_stop(EV_A_ w);

    // Call the hot keys module callback
    hotkeys_cb(((stdin_idle_data *)w->data)->ke,
               ((stdin_idle_data *)w->data)->data);

    // Start STDIN watcher
    ev_io_start(EV_A_ ((stdin_idle_data *)w->data)->stdin_w);

    // Free watchers data
    free(((stdin_idle_data *)w->data)->data);
    free(w->data);
}

/**
 * Check activity Idle callback.
 *
 * Start(restart) connection to R-Host and check connections to all peers.
 *
 * @param loop
 * @param w
 * @param revents
 */
void idle_activity_cb(EV_P_ ev_idle *w, int revents) {

    #define kev ((ksnetEvMgrClass *)w->data)

    #ifdef DEBUG_KSNET
    ksnet_printf(& ((ksnetEvMgrClass *)w->data)->ksn_cfg, DEBUG_VV,
                "%sEvent manager:%s idle activity callback %d\n", 
                ANSI_CYAN, ANSI_NONE,
                kev->idle_activity_count);
    #endif

    // Start(restart) connection to R-Host
    connect_r_host_cb(w->data);

    // Check connected
    if(!ksnetArpGetAll(kev->kc->ka, check_connected_cb, NULL)) {

        // Stop this watcher if not checked
        ev_idle_stop(EV_A_ w);

        // Increment count
        if(kev->idle_activity_count == UINT32_MAX) kev->idle_activity_count = 0;
        kev->idle_activity_count++;
    }

    #undef kev
}

#pragma GCC diagnostic pop

/**
 * Initialize connected to Event Manager Modules
 *
 * @param ke
 * @return
 */
int modules_init(ksnetEvMgrClass *ke) {

    ke->kc = NULL;
    ke->kh = NULL;

    if((ke->kc = ksnCoreInit(ke, ke->ksn_cfg.host_name, ke->ksn_cfg.port, NULL)) == NULL) return 0;
    ke->kh = ksnetHotkeysInit(ke);

    // VPN Module
    #if M_ENAMBE_VPN
    ke->kvpn = ksnVpnInit(ke);
    #endif

//    ke->kt = ksnTcpInit(ke);
//    ke->kter = ksnTermInit(ke);
//    ke->ktun = ksnTunInit(ke);

    return 1;
}

/**
 * Destroy modules
 *
 * @param ke
 */
void modules_destroy(ksnetEvMgrClass *ke) {

//    ksnTunDestroy(ke->ktun);
//    ksnTermDestroy(ke->kter);
//    ksnTcpDestroy(ke->kt);

    #if M_ENAMBE_VPN
    ksnVpnDestroy(ke->kvpn);
    #endif
    ksnetHotkeysDestroy(ke->kh);
    ksnCoreDestroy(ke->kc);
}
