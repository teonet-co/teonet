/**
 * File:   ev_mgr.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on April 11, 2015, 2:13 AM
 *
 * Event manager module
 */

#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <pthread.h>    // For mutex and TEO_TREAD


#include "ev_mgr.h"
#include "utils/utils.h"
#include "utils/rlutil.h"
//#include "modules/tcp_proxy.h"

// Constants
const char *null_str = "";

int teoRestartApp = 0;

// Local functions
void idle_cb (EV_P_ ev_idle *w, int revents); // Timer idle callback
void idle_activity_cb(EV_P_ ev_idle *w, int revents); // Idle activity callback
void timer_cb (EV_P_ ev_timer *w, int revents); // Timer callback
void host_cb (EV_P_ ev_io *w, int revents); // Host callback
void sig_async_cb (EV_P_ ev_async *w, int revents); // Async signal callback
void sigint_cb (struct ev_loop *loop, ev_signal *w, int revents); // SIGINT callback
void sigsegv_cb (struct ev_loop *loop, ev_signal *w, int revents); // SIGSEGV or SIGABRT callback
void sigsegv_cb_h(int signum);
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
 * 
 * @return Pointer to created ksnetEvMgrClass
 */
ksnetEvMgrClass *ksnetEvMgrInit(

  int argc, char** argv,
  void (*event_cb)(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data, size_t data_len, void *user_data),
  int options
    ) {
    
    return ksnetEvMgrInitPort(argc, argv, event_cb, options, 0, NULL);
}
/**
 * Initialize KSNet Event Manager and network and set new default port
 *
 * @param argc Number of applications arguments (from main)
 * @param argv Applications arguments array (from main)
 * @param event_cb Events callback function called when an event happens
 * @param options Options set: \n
 *                READ_OPTIONS - read options from command line parameters; \n
 *                READ_CONFIGURATION - read options from configuration file
 * @param port Set default port number if non 0
 * @param user_data Pointer to user data or NULL if absent
 * 
 * @return Pointer to created ksnetEvMgrClass
 *
 */
ksnetEvMgrClass *ksnetEvMgrInitPort(

  int argc, char** argv,
  void (*event_cb)(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data, size_t data_len, void *user_data),
  int options,
  int port,
  void *user_data
    ) {

    ksnetEvMgrClass *ke = malloc(sizeof(ksnetEvMgrClass));
    ke->custom_timer_interval = 0.0;
    ke->last_custom_timer = 0.0;
    ke->runEventMgr = 0;
    ke->event_cb = event_cb;
    ke->kt = NULL;
    ke->km = NULL;
    ke->kf = NULL;
    ke->tp = NULL;
    ke->ks = NULL;
    ke->kl = NULL;
    ke->num_nets = 1;
    ke->n_num = 0;
    ke->n_prev = NULL;
    ke->n_next = NULL;
    ke->user_data = user_data;
    ke->argc = argc;
    ke->argv = argv;
    
    // Initialize async mutex
    pthread_mutex_init(&ke->async_mutex, NULL);
    
    // KSNet parameters
    const int app_argc = options&APP_PARAM && user_data != NULL && ((ksnetEvMgrAppParam*)user_data)->app_argc > 1 ? ((ksnetEvMgrAppParam*)user_data)->app_argc : 1; // number of application arguments
    char *app_argv[app_argc];           // array for argument names
    app_argv[0] = (char*)"host_name";   // host name argument name
    //app_argv[1] = (char*)"file_name";   // file name argument name
    if(options&APP_PARAM && user_data != NULL) {
        if(((ksnetEvMgrAppParam*)user_data)->app_argc > 1) {
            int i;
            for(i = 1; i < ((ksnetEvMgrAppParam*)user_data)->app_argc; i++) {
                app_argv[i] = ((ksnetEvMgrAppParam*)user_data)->app_argv[i];
            }
        }
    }

    // Initial configuration, set defaults, read defaults from command line
    ksnet_configInit(&ke->ksn_cfg, ke); // Set configuration default
    if(port) ke->ksn_cfg.port = port; // Set port default
    char **argv_ret = NULL;
    if(options&READ_OPTIONS) ksnet_optRead(argc, argv, &ke->ksn_cfg, app_argc, app_argv, 1); // Read command line parameters (to use it as default)
    if(options&READ_CONFIGURATION) read_config(&ke->ksn_cfg, ke->ksn_cfg.port); // Read configuration file parameters
    if(options&READ_OPTIONS) argv_ret = ksnet_optRead(argc, argv, &ke->ksn_cfg, app_argc, app_argv, 0); // Read command line parameters (to replace configuration file)

    ke->ksn_cfg.app_argc = app_argc;
    ke->ksn_cfg.app_argv = argv_ret;
//    
//    if(options&APP_PARAM && user_data != NULL && 
//       ((ksnetEvMgrAppParam*)user_data)->app_argc > 1) {
//        
//        ((ksnetEvMgrAppParam*)user_data)->app_argv_result = argv_ret;
//    }
//    
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
    ksnet_printf(&ke->ksn_cfg, MESSAGE, "%sEvent manager:%s Started ...\n", 
            ANSI_CYAN, ANSI_NONE);
    #endif

    ke->timer_val = 0;
    ke->idle_count = 0;
    ke->idle_activity_count = 0;

    // Event loop
    struct ev_loop *loop = ke->n_num && ke->km == NULL ? ev_loop_new (0) : EV_DEFAULT;
    ke->ev_loop = loop;

    // Initialize modules
    if(modules_init(ke)) {

        // Initialize idle watchers
        ev_idle_init (&ke->idle_w, idle_cb);
        ke->idle_w.data = ke->kc;

        // Initialize Check activity watcher
        ev_idle_init (&ke->idle_activity_w, idle_activity_cb);
        ke->idle_activity_w.data = ke;

        // Initialize and start main timer watcher, it is a repeated timer
        ev_timer_init (&ke->timer_w, timer_cb, 0.0, KSNET_EVENT_MGR_TIMER);
        ke->timer_w.data = ke;
        ev_timer_start (loop, &ke->timer_w);

        // Initialize signals and keyboard watcher for first net (default loop)
        if(!ke->n_num) {
            
            // Initialize and start signals watchers
            // SIGINT
            ev_signal_init (&ke->sigint_w, sigint_cb, SIGINT);
            ke->sigint_w.data = ke;
            ev_signal_start (loop, &ke->sigint_w);

            // SIGQUIT
            #ifdef SIGQUIT
            ev_signal_init (&ke->sigquit_w, sigint_cb, SIGQUIT);
            ke->sigquit_w.data = ke;
            ev_signal_start (loop, &ke->sigquit_w);
            #endif

            // SIGTERM
            #ifdef SIGTERM
            ev_signal_init (&ke->sigterm_w, sigint_cb, SIGTERM);
            ke->sigterm_w.data = ke;
            ev_signal_start (loop, &ke->sigterm_w);
            #endif

            // SIGKILL
            #ifdef SIGKILL
            ev_signal_init (&ke->sigkill_w, sigint_cb, SIGKILL);
            ke->sigkill_w.data = ke;
            ev_signal_start (loop, &ke->sigkill_w);
            #endif

            // SIGSTOP
            #ifdef SIGSTOP
            ev_signal_init (&ke->sigstop_w, sigint_cb, SIGSTOP);
            ke->sigstop_w.data = ke;
            ev_signal_start (loop, &ke->sigstop_w);
            #endif

            // SIGSEGV
            #ifdef SIGSEGV
            puts("Set signal handler");
            signal(SIGSEGV, sigsegv_cb_h);
//            ev_signal_init (&ke->sigsegv_w, sigsegv_cb, SIGSEGV);
//            ke->sigsegv_w.data = ke;
//            ev_signal_start (loop, &ke->sigsegv_w);
            #endif

            // SIGABRT
            #ifdef SIGABRT
            puts("Set signal handler");
            signal(SIGABRT, sigsegv_cb_h);
//            ev_signal_init (&ke->sigabrt_w, sigsegv_cb, SIGABRT);
//            ke->sigabrt_w.data = ke;
//            ev_signal_start (loop, &ke->sigabrt_w);
            #endif
            
            #ifdef SIGUSR2
            ev_signal_init (&ke->sigabrt_w, sigsegv_cb, SIGUSR2);
            ke->sigabrt_w.data = ke;
            ev_signal_start (loop, &ke->sigabrt_w);
            #endif
        }

        // Initialize and start a async signal watcher for event add
        ke->async_queue = pblListNewArrayList();
        ev_async_init (&ke->sig_async_w, sig_async_cb);
        ke->sig_async_w.data = ke;
        ev_async_start (loop, &ke->sig_async_w);

        // Run event loop
        ke->runEventMgr = 1;
        if(ke->km == NULL) ev_run(loop, 0);
        else return 0;
        
        ksnetEvMgrFree(ke, 1); // Free class variables and watchers after run
    }
    
    ksnetEvMgrFree(ke, 0); // Free class variables and watchers after run
            
    return 0;
}

/**
 * Free ksnetEvMgrClass after run
 * 
 * @param ke
 * @param free_async 1 - free async queue data and destroy queue mutex; 
 *                   0 - free basic class data;
 *                   2 - free all - async queue data and basic class data
 * 
 * @return 
 */
int ksnetEvMgrFree(ksnetEvMgrClass *ke, int free_async) {
    
    // Free async data queue and destroy queue mutex
    if(free_async) {
        
        pblListFree(ke->async_queue);
        pthread_mutex_destroy(&ke->async_mutex);        
    }
    
    // Free all other class data
    if(free_async == 0 || free_async == 2) {
        
        // Send stopped event to user level
        if(ke->event_cb != NULL) ke->event_cb(ke, EV_K_STOPPED_BEFORE, NULL, 0, NULL);
        
        // Stop watchers
        ev_async_stop(ke->ev_loop, &ke->sig_async_w);
        ev_timer_stop(ke->ev_loop, &ke->timer_w);

        // Destroy modules
        modules_destroy(ke);

        // Destroy event loop (and free memory)
        if(ke->km == NULL || !ke->n_num) ev_loop_destroy(ke->ev_loop);

        #ifdef DEBUG_KSNET
        //ksnet_printf(&ke->ksn_cfg, DEBUG, "Event manager: stopped.\n");
        printf("%sEvent manager:%s at port %d stopped.\n", ANSI_CYAN, ANSI_NONE,
               (int)ke->ksn_cfg.port );
        #endif

        // Send stopped event to user level
        if(ke->event_cb != NULL) ke->event_cb(ke, EV_K_STOPPED, NULL, 0, NULL);

        // Save application parameters to restart it
        int argc = ke->argc;
        char **argv = ke->argv;

        // Free memory
        free(ke);
        
        // Restart application if need it
        ksnetEvMgrRestart(argc, argv);        
    }
    
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

    ke->last_custom_timer = ksnetEvMgrGetTime(ke);
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
 * @param ke Pointer to ksnetEvMgrClass
 * @param data Pointer to data
 * @param data_len Data length
 * @param user_data Pointer to user data
 */
void ksnetEvMgrAsync(ksnetEvMgrClass *ke, void *data, size_t data_len, void *user_data) {

    #ifdef DEBUG_KSNET
    ksnet_printf(&ke->ksn_cfg, DEBUG_VV, 
            "%sEvent manager:%s make Async call to Event manager\n", 
            ANSI_CYAN, ANSI_NONE);
    #endif

    // Add something to queue and send async signal to event loop
    void* element = NULL;
    if(data == NULL) data_len = 0;
    element = malloc(data_len + sizeof(uint16_t) + sizeof(void*));
    size_t ptr = 0;
    *(void**)element = user_data; ptr += sizeof(void**);
    *(uint16_t*)(element + ptr) = (uint16_t)data_len; ptr += sizeof(uint16_t);
    if(data != NULL) {
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
        
        size_t ptr = 0;
        void *data = NULL;
        ksnet_stringArr ips = NULL;
        
        // Start TCP Proxy client connection if it is allowed and is not connected
        if(ke->tp != NULL && ke->ksn_cfg.r_tcp_f) { 
        
            // Start TCP proxy client
            if(!(ke->tp->fd_client > 0))
                ksnTCPProxyClientConnect(ke->tp);  
            
            // Create data with empty list of local IPs and port
            data = malloc(sizeof(uint8_t));
            uint8_t *num = (uint8_t *) data; // Pointer to number of IPs
            ptr = sizeof(uint8_t); // Pointer (to first IP)
            *num = 0; // Number of IPs
        }

        // Create data for UDP connection
        else {            
            
            // Create data with list of local IPs and port
            ips = getIPs(&ke->ksn_cfg); // IPs array
            uint8_t len = ksnet_stringArrLength(ips); // Max number of IPs
            const size_t MAX_IP_STR_LEN = 16; // Max IPs string length
            data = malloc(len * MAX_IP_STR_LEN + sizeof(uint8_t) + sizeof(uint32_t)); // Data
            ptr = sizeof(uint8_t); // Pointer (to first IP)
            uint8_t *num = (uint8_t *) data; // Pointer to number of IPs
            *num = 0; // Number of IPs
            
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
        }
        
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
    ksnet_stringArr ips = getIPs(&ke->ksn_cfg); // IPs array
    uint8_t len = ksnet_stringArrLength(ips); // Max number of IPs

    // Send to local IPs and Port
    int i;
    for(i=0; i < len; i++) {

        if(ip_is_private(ips[i])) {

            //! \todo: Need to send to real local IP
            uint8_t ip_arr[4];
            ip_to_array(ips[i], ip_arr);
            char *ip_str = ksnet_formatMessage("%d.%d.%d.93", ip_arr[0],
                                               ip_arr[1], ip_arr[2]);

            // Send to IP to open port
            ksnCoreSendto(ke->kc, ip_str, ke->ksn_cfg.r_port, CMD_NONE,  
                    NULL_STR, 1);

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

    int retval = 0; // Return value
    double ct = ksnetEvMgrGetTime(kev); //Current time

    // Send trip time request
    if(ct - arp_data->last_triptime_send > CHECK_EVENTS_AFTER) {
        ksnCommandSendCmdEcho(kev->kc->kco, peer_name, (void*) TRIPTIME,
                              TRIPTIME_LEN);
    }

    // Disconnect dead peer
    else if(ct - arp_data->last_activity > CHECK_EVENTS_AFTER * 1.5) {

        ksnCorePacketData rd;
        rd.from = peer_name;
        rd.data = NULL;
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
 * param loop
 * @param w Pointer to watcher
 * @param revents
 */
void idle_cb (EV_P_ ev_idle *w, int revents) {

    #define kev ((ksnetEvMgrClass *)((ksnCoreClass *)w->data)->ke)

    #ifdef DEBUG_KSNET
    ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
            "%sEvent manager:%s idle callback %d\n", ANSI_CYAN, ANSI_NONE,            
            kev->idle_count);
    #endif

    // Stop this watcher
    ev_idle_stop(EV_A_ w);

    // Idle count startup (first time run)
    if(!kev->idle_count) {
        //! \todo: open_local_port(kev);
        // Set statistic start time
        if(!kev->kc->ku->started) kev->kc->ku->started = ksnetEvMgrGetTime(kev);
        // Connect to R-Host
        connect_r_host_cb(kev);
        // Send event to application
        if(kev->event_cb != NULL) kev->event_cb(kev, EV_K_STARTED, NULL, 0, NULL);
    }
    // Idle count max value
    else if(kev->idle_count == UINT32_MAX) kev->idle_count = 0;

    // Increment count
    kev->idle_count++;

    // Check host events to send him service information
    //! \todo:    host_cb(EV_A_ (ev_io*)w, revents);

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
 * param loop
 * @param w
 * @param revents
 */
void timer_cb(EV_P_ ev_timer *w, int revents) {

    #ifdef DEBUG_KSNET
    const int show_interval = 5 / KSNET_EVENT_MGR_TIMER /* 10 */;
    #endif
    const int activity_interval = CHECK_EVENTS_AFTER / KSNET_EVENT_MGR_TIMER /* 23 */;
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
 * SIGSEGV or SIGABRT or SIGUSR2 signal handler
 *
 * @param loop
 * @param w
 * @param revents
 */
void sigsegv_cb (struct ev_loop *loop, ev_signal *w, int revents) {

    ksnetEvMgrClass *ke = (ksnetEvMgrClass *)w->data;
    static int attempt = 0;
    
    teoRestartApp = 1; // Set restart flag
    
    #ifdef DEBUG_KSNET
    ksnet_printf(&((ksnetEvMgrClass *)w->data)->ksn_cfg, ERROR_M,
            "\n%sEvent manager:%s Got a signal %s ...\n", 
            ANSI_RED, ANSI_NONE,
            w->signum == SIGSEGV ? "SIGSEGV" : 
            w->signum == SIGABRT ? "SIGABRT" : 
            w->signum == SIGUSR2 ? "SIGUSR2" :
                                   "?");
    #endif
    
    // If SIGSEGV repeated than we can't exit from application
    if(attempt) exit(-1);
    
    // \todo Issue #162: Initiate restart application
    else {
        
        attempt++;
        
        //ksnetEvMgrRestart(ke->argc, ke->argv);        
        ksnetEvMgrStop(ke);
        #ifdef SHOW_DFL
        signal(w->signum, SIG_DFL);
        kill(getpid(), w->signum);     
        ksnetEvMgrRestart(ke->argc, ke->argv);
        exit(0);
        #endif
    }
}

int teo_argc; 
char** teo_argv;

void sigsegv_cb_h(int signum) {
    
    printf("\n%sApplication:%s Got a signal %s ...\n",
            ANSI_RED, ANSI_NONE,
            signum == SIGSEGV ? "SIGSEGV" : 
            signum == SIGABRT ? "SIGABRT" : 
                                   "?");
    sleep(1);
    
    // Send signal SIGUSR2
    //kill(getpid(), SIGUSR2);
    
//    if(!fork()) {
        puts("starting restart process...");
        sleep(1);
//        kill(getpid(), SIGCONT);
        
        teoRestartApp = 1;
        ksnetEvMgrRestart(teo_argc, teo_argv);
//    }
//    exit(0);
    
    // Default action
    signal(signum, SIG_DFL);
    kill(getpid(), signum);
}

/**
 * Restart application 
 * 
 * @param argc
 * @param argv
 * @return 
 */
int ksnetEvMgrRestart(int argc, char **argv) {
    
    if(teoRestartApp) {
        
        // Show application path and parameters
        int i = 1;
        puts("");
        printf("Restart application: %s", argv[0]);
        for(;;i++) {
            if(argv[i] != NULL) printf(" %s", argv[i]);
            else break;
        }
        puts("\n");

        //#define USE_SYSTEM
        #ifdef USE_SYSTEM
        // Execute application
        char *app = ksnet_formatMessage("%s", argv[0]);
        for(i = 1; argv[i] != NULL; i++) {
            app = ksnet_formatMessage("%s %s", app, argv[i]);
        }            
        if(system(app));
        free(app);
        exit(0);
        #else
        // Execute application
        if(execv(argv[0], argv) == -1) {
            fprintf(stderr, "Can't execute application %s: %s\n", 
                    argv[0], strerror(errno));
            exit(-1);
        }
        exit(0); // Not need it because execv never return
        #endif
    }
    
    return teoRestartApp;
}

/**
 * Async Event callback (Signal)
 *
 * param loop
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
        if(data != NULL) {
            void *user_data = *(void**)data; ptr += sizeof(void**);
            uint16_t data_len = *(uint16_t*)(data + ptr); ptr += sizeof(uint16_t);
            kev->event_cb(kev, EV_K_ASYNC , data + ptr, data_len, user_data);
            free(data);
        }
        else  kev->event_cb(kev, EV_K_ASYNC , NULL, 0, NULL);
    }
    pthread_mutex_unlock (&kev->async_mutex);
    
    // Do something ...
    
    #undef kev
}

/**
 * Check activity Idle callback.
 *
 * Start(restart) connection to R-Host and check connections to all peers.
 *
 * param loop
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

    // Teonet core module
    if((ke->kc = ksnCoreInit(ke, ke->ksn_cfg.host_name, ke->ksn_cfg.port, NULL)) == NULL) return 0;
    
    // Hotkeys
    if(!ke->n_num) ke->kh = ksnetHotkeysInit(ke);
    
    // Callback QUEUE
    #if M_ENAMBE_CQUE
    ke->kq = ksnCQueInit(ke);
    #endif

    // Stream module
    #if M_ENAMBE_STREAM
    ke->ks = ksnStreamInit(ke);
    #endif

    // PBL KeyFile Module
    #if M_ENAMBE_PBLKF
    ke->kf = ksnTDBinit(ke);
    #endif

    // VPN Module
    #if M_ENAMBE_VPN
    ke->kvpn = ksnVpnInit(ke);
    #endif

    // TCP client/server module
    #if M_ENAMBE_TCP
    ke->kt = ksnTcpInit(ke);
    #endif

    // L0 Server
    #if M_ENAMBE_L0s
    ke->kl = ksnLNullInit(ke);
    #endif

    // TCP Proxy module
    #ifdef M_ENAMBE_TCP_P
    ke->tp = ksnTCPProxyInit(ke);
    #endif
    
    // TCP Tunnel module  
    #ifdef M_ENAMBE_TUN
    ke->ktun = ksnTunInit(ke);
    #endif

    // Terminal module
    #ifdef M_ENAMBE_TERM
    ke->kter = ksnTermInit(ke);
    #endif

    return 1;
}

/**
 * Destroy modules
 *
 * @param ke
 */
void modules_destroy(ksnetEvMgrClass *ke) {

    #ifdef M_ENAMBE_TERM
    ksnTermDestroy(ke->kter);
    #endif
    #ifdef M_ENAMBE_TUN
    ksnTunDestroy(ke->ktun);
    #endif
//    #if M_ENAMBE_TCP_P
//    ksnTCPProxyDestroy(ke->tp);
//    #endif
//    #if M_ENAMBE_TCP
//    ksnTcpDestroy(ke->kt);
//    #endif
    #if M_ENAMBE_VPN
    ksnVpnDestroy(ke->kvpn);
    #endif
    #if M_ENAMBE_CQUE
    ksnCQueDestroy(ke->kq);
    #endif
    #if M_ENAMBE_PBLKF
    ksnTDBdestroy(ke->kf);
    #endif
    #if M_ENAMBE_STREAM
    ksnStreamDestroy(ke->ks);
    #endif
    #if M_ENAMBE_L0s
    ksnLNullDestroy(ke->kl);
    #endif

    ksnetHotkeysDestroy(ke->kh);
    ksnCoreDestroy(ke->kc);
    
    #if M_ENAMBE_TCP_P
    ksnTCPProxyDestroy(ke->tp);
    #endif
    #if M_ENAMBE_TCP
    ksnTcpDestroy(ke->kt);
    #endif   
}
