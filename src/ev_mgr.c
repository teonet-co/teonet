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
#include <signal.h>
#include <pthread.h>    // For mutex and TEO_TREAD
#include <stdbool.h>

#include "ev_mgr.h"
#include "net_multi.h"
#include "utils/utils.h"
#include "utils/rlutil.h"
#include "modules/metric.h"

#include "commands_creator.h"

#define MODULE "event_manager"

// Global module variables
static int teoRestartApp_f = 0; // Restart teonet application before exit

// Global library variables to process SIGSEGV, SIGABRT signals handler
extern int teo_argc;
extern char** teo_argv;

// Global library constants
const char *null_str = "";

// Run file name and buffer
#define RUN_NAME "-teonet.run"
char run_file[KSN_BUFFER_SIZE];

ksnetEvMgrClass* __ke_from_command_class(ksnCommandClass *X){return ((ksnetEvMgrClass*)((ksnCoreClass*)X->kc)->ke); }
ksnetEvMgrClass* __ke_from_L0_class(ksnLNullClass *X){return ((ksnetEvMgrClass*)X->ke); };

ksnetArpClass* __arp_from_command_class(ksnCommandClass *X){return ((ksnetArpClass*)((ksnCoreClass*)X->kc)->ka); }

// Local functions
void idle_cb (EV_P_ ev_idle *w, int revents); // Timer idle callback
void idle_activity_cb(EV_P_ ev_idle *w, int revents); // Idle activity callback
void idle_async_cb(EV_P_ ev_idle *w, int revents); // Idle async callback
void timer_cb (EV_P_ ev_timer *w, int revents); // Timer callback
void host_cb (EV_P_ ev_io *w, int revents); // Host callback
void sig_async_cb (EV_P_ ev_async *w, int revents); // Async signal callback
void sigint_cb (struct ev_loop *loop, ev_signal *w, int revents); // SIGINT callback
void sigusr2_cb (struct ev_loop *loop, ev_signal *w, int revents); // SIGSEGV or SIGABRT callback
static void sigsegv_cb_h(int signum, siginfo_t *si, void *unused);
int modules_init(ksnetEvMgrClass *ke); // Initialize modules
void modules_destroy(ksnetEvMgrClass *ke); // Deinitialize modules
int send_cmd_disconnect_cb(ksnetArpClass *ka, char *name,
                            ksnet_arp_data *arp_data, void *data);

void metric_teonet_count(teoMetricClass *tm);

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
  //void (*event_cb)(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data, size_t data_len, void *user_data),
  ksn_event_cb_type event_cb,
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
    memset(ke, 0, sizeof(ksnetEvMgrClass));
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
    ke->net_count = 1;
    ke->net_idx = 0;
    ke->n_prev = NULL;
    ke->n_next = NULL;
    ke->user_data = user_data;
    ke->teo_class = NULL;
    ke->argc = argc;
    ke->argv = argv;
    ke->ev_loop = NULL;

    // Set Global Variables used by SIGSEGV signal handler
    teo_argc = argc;
    teo_argv = argv;

    // Initialize async and printf mutex
    pthread_mutex_init(&ke->async_mutex, NULL);
    pthread_mutex_init(&ke->printf_mutex, NULL);

    // KSNet parameters
    const int app_argc = options&APP_PARAM && user_data != NULL && ((ksnetEvMgrAppParam*)user_data)->app_argc > 1 ? ((ksnetEvMgrAppParam*)user_data)->app_argc : 1; // number of application arguments
    char *app_argv_descr[app_argc];     // array for argument names description
    char *app_argv[app_argc];           // array for argument names
    app_argv[0] = (char* )"host_name";  // host name argument name
    app_argv_descr[0] = (char*) "This host name";   // host name argument name description
    //app_argv[1] = (char*)"file_name";   // file name argument name
    if(options&APP_PARAM && user_data != NULL) {
        if(((ksnetEvMgrAppParam*)user_data)->app_argc > 1) {
            int i;
            for(i = 1; i < ((ksnetEvMgrAppParam*)user_data)->app_argc; i++) {
                app_argv[i] = (char*)((ksnetEvMgrAppParam*)user_data)->app_argv[i];

                if(((ksnetEvMgrAppParam*)user_data)->app_descr != NULL) {
                    app_argv_descr[i] = (char*)((ksnetEvMgrAppParam*)user_data)->app_descr[i];
                }
                else app_argv_descr[i] = NULL;
            }
        }
    }

    // Initial configuration, set defaults, read defaults from command line
    ksnet_configInit(&ke->teo_cfg, ke); // Set configuration default
    if(port) ke->teo_cfg.port = port; // Set port default
    char **argv_ret = NULL;
    ksnet_optSetApp(&ke->teo_cfg, basename(argv[0]), basename(argv[0]), null_str);
    if(options&READ_OPTIONS) ksnet_optRead(argc, argv, &ke->teo_cfg, app_argc, app_argv, app_argv_descr, 1); // Read command line parameters (to use it as default)
    if(options&BLOCK_CLI_INPUT) ke->teo_cfg.block_cli_input_f = 1; // Set Block CLI Input from options
    if(options&READ_CONFIGURATION) read_config(&ke->teo_cfg, ke->teo_cfg.port); // Read configuration file parameters
    if(options&READ_OPTIONS) argv_ret = ksnet_optRead(argc, argv, &ke->teo_cfg, app_argc, app_argv, app_argv_descr, 0); // Read command line parameters (to replace configuration file)

    ke->teo_cfg.app_argc = app_argc;
    ke->teo_cfg.app_argv = argv_ret;
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
 * Set Teonet application type
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @param type Application type string
 */
inline void teoSetAppType(ksnetEvMgrClass *ke, char *type) {

    ke->app_type = strdup(type);
}

/**
 * Set Teonet application version
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @param version Application version string
 */
inline void teoSetAppVersion(ksnetEvMgrClass *ke, char *version) {

    ke->app_version = strdup(version);
}

/**
 * Get current application type
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @return Application type string
 */
inline const char *teoGetAppType(ksnetEvMgrClass *ke) {

    return (const char *)ke->app_type;
}

char *teoGetFullAppTypeFromHostInfo(host_info_data *hid) {
    // Combine types
    size_t ptr = strlen(hid->string_ar) + 1;
    char *type_str = strdup(null_str);
    for(int i = 1; i < hid->string_ar_num; i++) {

        type_str = ksnet_sformatMessage(type_str, "%s%s\"%s\"",
                type_str, i > 1 ? ", " : "", hid->string_ar + ptr);
        ptr += strlen(hid->string_ar + ptr) + 1;
    }

    return type_str;
}

/**
 * Get current application version
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @return Application version string
 */
inline const char *teoGetAppVersion(ksnetEvMgrClass *ke) {

    return (const char *)ke->app_version;
}

/**
 * Get teonet library version
 *
 * @return
 */
inline const char *teoGetLibteonetVersion() {

    return VERSION;
}

/**
 * Stop event manager
 *
 * @param ke Pointer to ksnetEvMgrClass
 */
inline void ksnetEvMgrStop(ksnetEvMgrClass *ke) {
    ke->runEventMgr = 0;
}

int ksnetEvMgrStatus(ksnetEvMgrClass *ke) {
    return ke->runEventMgr;
}
/**
 * Set sigaction function typedef
 */
typedef void (*set_sigaction_h) (int, siginfo_t *, void *);

/**
 * Set system signal action
 * @param ke Pointer to ksnetEvMgrClass
 * @return
 */
static void set_sigaction(ksnetEvMgrClass *ke, int sig,
        set_sigaction_h sigsegv_cb_h) {

    if(ke->teo_cfg.sig_segv_f) {

        struct sigaction sa;
        sa.sa_flags = SA_NOMASK; //SA_SIGINFO; //
        sigemptyset(&sa.sa_mask);
        sa.sa_sigaction = sigsegv_cb_h;
        if (sigaction(sig, &sa, NULL) == -1) {
            ksn_puts(ke, MODULE, ERROR_M, "set sigaction error");
        }
    }
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
    ksn_puts(ke, MODULE, MESSAGE, "started");
    #endif

    // Create run file name
    const char *network = ke->teo_cfg.network;

    char *DataPath = getDataPath();
    strncpy(run_file, DataPath, KSN_BUFFER_SIZE - 1);
    free(DataPath);

    if (network != NULL && network[0]) {
        strncat(run_file, "/", KSN_BUFFER_SIZE - strlen(run_file) - 1);
        strncat(run_file, network, KSN_BUFFER_SIZE - strlen(run_file) - 1);
    }
    strncat(run_file, "/", KSN_BUFFER_SIZE - strlen(run_file) - 1);
    strncat(run_file, ksnetEvMgrGetHostName(ke),
                KSN_BUFFER_SIZE - strlen(run_file) - 1);
    strncat(run_file, RUN_NAME, KSN_BUFFER_SIZE - strlen(run_file) - 1);
    FILE *fp;

    // Wait other teonet application to get disconnect signal
    // if this application crash or deployed
    if(!ke->net_idx) {
        if ((fp = fopen(run_file, "r"))){
            // usleep(3500000);
            fclose(fp);
        }
        // Create run file
        else {
            fp = fopen(run_file, "w");
            fprintf(fp,"run\n");
            fclose(fp);
        }
    }

    ke->timer_val = 0;
    ke->idle_count = 0;
    ke->idle_activity_count = 0;

    // Event loop
    bool loop_already_initialised = ke->ev_loop != NULL;
    if(ke->ev_loop == NULL){
        struct ev_loop *loop = ke->net_idx && ke->km == NULL ? ev_loop_new (0) : EV_DEFAULT;
        ke->ev_loop = loop;
    }

    // \todo remove this print
    ksn_printf(ke, MODULE, DEBUG_VV,
        _ANSI_BROWN "event loop initialized as %s " _ANSI_NONE ", ke->net_idx = %d\n",
        ke->net_idx && ke->km == NULL ? "ev_loop_new (0)" : "EV_DEFAULT",
        (int)ke->net_idx
    );


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
        ev_timer_start (ke->ev_loop, &ke->timer_w);

        // Initialize signals and keyboard watcher for first net (default loop)
        if(!ke->net_idx) {

            // Initialize and start signals watchers
            // SIGINT
            ev_signal_init (&ke->sigint_w, sigint_cb, SIGINT);
            ke->sigint_w.data = ke;
            ev_signal_start (ke->ev_loop, &ke->sigint_w);

            // SIGQUIT
            #ifdef SIGQUIT
            ev_signal_init (&ke->sigquit_w, sigint_cb, SIGQUIT);
            ke->sigquit_w.data = ke;
            ev_signal_start (ke->ev_loop, &ke->sigquit_w);
            #endif

            // SIGTERM
            #ifdef SIGTERM
            ev_signal_init (&ke->sigterm_w, sigint_cb, SIGTERM);
            ke->sigterm_w.data = ke;
            ev_signal_start (ke->ev_loop, &ke->sigterm_w);
            #endif

            // SIGKILL
            #ifdef SIGKILL
            ev_signal_init (&ke->sigkill_w, sigint_cb, SIGKILL);
            ke->sigkill_w.data = ke;
            ev_signal_start (ke->ev_loop, &ke->sigkill_w);
            #endif

            // SIGSTOP
            #ifdef SIGSTOP
            ev_signal_init (&ke->sigstop_w, sigint_cb, SIGSTOP);
            ke->sigstop_w.data = ke;
            ev_signal_start (ke->ev_loop, &ke->sigstop_w);
            #endif

            // SIGABRT used to restart this application
            #ifdef SIGUSR2
            ev_signal_init (&ke->sigabrt_w, sigusr2_cb, SIGUSR2);
            ke->sigabrt_w.data = ke;
            ev_signal_start (ke->ev_loop, &ke->sigabrt_w);
            #endif

            // SIGSEGV
            #ifdef SIGSEGV
            #ifdef DEBUG_KSNET
            if(ke->teo_cfg.sig_segv_f) {
                ksn_puts(ke, MODULE, MESSAGE, "set SIGSEGV signal handler");
            }
            #endif
            // Libev can't process this signal properly
            set_sigaction(ke, SIGSEGV, sigsegv_cb_h);
            #endif

            // SIGABRT
            #ifdef SIGABRT
            #ifdef DEBUG_KSNET
            if(ke->teo_cfg.sig_segv_f)
                ksn_puts(ke, MODULE, MESSAGE, "set SIGABRT signal handler");
            #endif
            // Libev can't process this signal properly
            set_sigaction(ke, SIGABRT, sigsegv_cb_h);
            #endif
        }

        // Initialize and start a async signal watcher for event add
        ke->async_queue = pblListNewArrayList();
        ev_async_init (&ke->sig_async_w, sig_async_cb);
        ke->sig_async_w.data = ke;
        ev_async_start (ke->ev_loop, &ke->sig_async_w);

        // Initialize async idle watcher
        ev_idle_init (&ke->idle_async_w, idle_async_cb);
        ke->idle_async_w.data = ke;

        // Run event loop
        ke->runEventMgr = 1;
        ev_idle_start(ke->ev_loop, & ke->idle_w);
        if(ke->km == NULL && loop_already_initialised == false) ev_run(ke->ev_loop, 0);
        else return 0;

        ksnetEvMgrFree(ke, 1); // Free class variables and watchers after run
    }

    ksnetEvMgrFree(ke, 0); // Free class variables and watchers after run

    return 0;
}

/**
 * Free ksnetEvMgrClass after run
 *
 * @param ke Pointer to ksnetEvMgrClass
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
        ke->async_queue = NULL;
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
        if(ke->km == NULL || !ke->net_idx) ev_loop_destroy(ke->ev_loop);

        #ifdef DEBUG_KSNET
        ksn_printf(ke, MODULE, MESSAGE,
                "at port %d stopped.\n", (int)ke->teo_cfg.port);
        #endif

        // Send stopped event to user level
        if(ke->event_cb != NULL) ke->event_cb(ke, EV_K_STOPPED, NULL, 0, NULL);

        // Save application parameters to restart it
        int argc = ke->argc;
        char **argv = ke->argv;

        // Free info parameters
        if(ke->app_type != NULL) free(ke->app_type);
        if(ke->app_version != NULL) free(ke->app_version);

        int km = ke->km != NULL;

        // Destroy printf mutex
        pthread_mutex_destroy(&ke->printf_mutex);

        // Free memory
        free(ke);

        // Remove run_file
        remove(run_file);

        // Restart application if need it
        if(!km) ksnetEvMgrRestart(argc, argv);
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
 * @param ke Pointer to ksnetEvMgrClass
 * @param time_interval
 */
void ksnetEvMgrSetCustomTimer(ksnetEvMgrClass *ke, double time_interval) {

    ke->last_custom_timer = ksnetEvMgrGetTime(ke);
    ke->custom_timer_interval = time_interval;
}

/**
 * Return host name
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @return
 */
inline char* ksnetEvMgrGetHostName(ksnetEvMgrClass *ke) {

    return ke->teo_cfg.host_name;
}

/**
 * Get host info data
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @param hd_len Pointer to host_info_data length holder
 *
 * @return  Pointer to host_info_data, should be free after use
 */
host_info_data *teoGetHostInfo(ksnetEvMgrClass *ke, size_t *hd_len) {

    // String arrays members
    const uint8_t string_ar_num = 2;
    const char *name = ksnetEvMgrGetHostName(ke);
    size_t name_len = strlen(name) + 1;
    const char *app_type = teoGetAppType(ke);
    const char *app_version = teoGetAppVersion(ke);
    if(app_type == NULL) app_type = "teo-default";
    if(app_version == NULL) app_version = null_str;
    size_t type_len = strlen(app_type) + 1;

    // Create host info data
    size_t string_ar_len = name_len + type_len;
    *hd_len = sizeof(host_info_data) + string_ar_len;
    host_info_data* hd = malloc(*hd_len);
    if(hd != NULL) {

        memset(hd, 0, *hd_len);

        // Fill version array data (split string version to 3 digits array)
        {
            size_t ptr = 0;
            const char *version = VERSION;
            int i, j; for(i = 0; i < DIG_IN_TEO_VER; i++) {
                for(j = ptr;;j++) {
                    const char c = *(version + j);
                    if(c == '.' || c == 0) {
                        char dig[10];
                        if(j-ptr < 10) {
                            memcpy(dig, version + ptr, j-ptr);
                            dig[j-ptr] = 0;
                            hd->ver[i] = atoi(dig);
                        }
                        else hd->ver[i] = 0;
                        ptr = j + 1;
                        break;
                    }
                }
            }
        }

        // Fill string array data
        size_t ptr = 0;
        hd->string_ar_num = string_ar_num;
        memcpy(hd->string_ar + ptr, name, name_len); ptr += name_len;
        memcpy(hd->string_ar + ptr, app_type, type_len); ptr += type_len;

        // Add fill services
        // L0 server
        if(ke->teo_cfg.l0_allow_f) {
            const char *l0 = "teo-l0";
            size_t l0_len = strlen(l0) + 1;
            *hd_len += l0_len;
            hd = realloc(hd, *hd_len);
            memcpy(hd->string_ar + ptr, l0, l0_len); ptr += l0_len;
            hd->string_ar_num++;
        }
        // VPN
        if(ke->teo_cfg.vpn_connect_f) {
            const char *vpn = "teo-vpn";
            size_t vpn_len = strlen(vpn) + 1;
            *hd_len += vpn_len;
            hd = realloc(hd, *hd_len);
            memcpy(hd->string_ar + ptr, vpn, vpn_len);  ptr += vpn_len;
            hd->string_ar_num++;
        }
        // Application version
        if(app_version != NULL) {
            size_t app_version_len = strlen(app_version) + 1;
            *hd_len += app_version_len;
            hd = realloc(hd, *hd_len);
            memcpy(hd->string_ar + ptr, app_version, app_version_len); ptr += app_version_len;
            //hd->string_ar_num++;
        }
    }

    return hd;
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

    if(!ke->async_queue) return;

    // Add something to queue and send async signal to event loop
    void* element = NULL;
    if(data == NULL) data_len = 0;
    element = malloc(data_len + sizeof(uint32_t) + sizeof(void*));
    size_t ptr = 0;
    *(void**)element = user_data; ptr += sizeof(void**);
    *(uint32_t*)(element + ptr) = (uint32_t)data_len; ptr += sizeof(uint32_t);
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
 * @param ke Pointer to ksnetEvMgrClass
 *
 * @return
 */
double ksnetEvMgrGetTime(ksnetEvMgrClass *ke) {
    double rv = ke->runEventMgr ? ev_now(ke->ev_loop) : 0.0;
    return rv;
}

/**
 * Connect to remote host
 *
 * @param ke Pointer to ksnetEvMgrClass
 */
void connect_r_host_cb(ksnetEvMgrClass *ke) {
    // Connect to r-host
    if(ke->teo_cfg.r_host_addr[0] && !ke->teo_cfg.r_host_name[0]) {
        resolveDnsName(&ke->teo_cfg);
        #ifdef DEBUG_KSNET
        ksn_printf(ke, MODULE, DEBUG, "connect to r-host: %s\n", ke->teo_cfg.r_host_addr);
        #endif

        size_t packet_size = 0;
        uint8_t *packet = NULL;

        // Start TCP Proxy client connection if it is allowed and is not connected
        if(ke->tp != NULL && ke->teo_cfg.r_tcp_f) {
            // Start TCP proxy client
            if(ke->tp->fd_client < 0) {
                ksnTCPProxyClientConnect(ke->tp);
            }

            packet = createCmdConnectRPacketTcp(ke, &packet_size);
        } else { // Create data for UDP connection
            packet = createCmdConnectRPacketUdp(ke, &packet_size);
        }

        // Send data to r-host
        ksnCoreSendto(ke->kc, ke->teo_cfg.r_host_addr, ke->teo_cfg.r_port,
                      CMD_CONNECT_R, packet, packet_size);

        #ifdef DEBUG_KSNET
        ksn_printf(ke, MODULE, DEBUG_VV, "send CMD_CONNECT_R = %u to r-host peer by address %s:%d.\n",
            CMD_CONNECT_R, ke->teo_cfg.r_host_addr, ke->teo_cfg.r_port);
        #endif

        free(packet);
    }
}

/**
 * Send packet to local address to open private firewall
 */
void open_local_port(ksnetEvMgrClass *ke) {

    // Create data with list of local IPs and port
    ksnet_stringArr ips = getIPs(&ke->teo_cfg); // IPs array
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
            ksnCoreSendto(ke->kc, ip_str, ke->teo_cfg.r_port, CMD_NONE, NULL_STR, 1);
            #ifdef DEBUG_KSNET
            ksn_printf(ke, MODULE, DEBUG_VV, "send CMD_NONE = %u to (%s:%d).\n",
                CMD_NONE, ip_str, ke->teo_cfg.r_port);
            #endif

            free(ip_str);
        }
    }

    ksnet_stringArrFree(&ips);
}

/**
 * Remove peer by name
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @param peer_name
 */
static void remove_peer(ksnetEvMgrClass *ke, char *peer_name, ksnet_arp_data_ext *arp) {

    // Disconnect dead peer from this host
    ksnCorePacketData rd;
    memset(&rd, 0, sizeof(rd));

    rd.from = peer_name;
    rd.from_len = strlen(peer_name) + 1;
    rd.addr = arp->data.addr;
    rd.port = arp->data.port;
    rd.arp = arp;
    cmd_disconnected_cb(ke->kc->kco, &rd);
}

/**
 * Remove peer by address
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @param addr
 * @return
 * @retval 0  peer not found
 * @retval 1  peer removed
 * @retval -1 peer not removed, this is this host
 */
int remove_peer_addr(ksnetEvMgrClass *ke, __CONST_SOCKADDR_ARG addr) {

    int rv = 0;
    char *peer_name;
    ksnet_arp_data_ext *arp;
    if((arp = (ksnet_arp_data_ext *)ksnetArpFindByAddr(ke->kc->ka, (__CONST_SOCKADDR_ARG) addr, &peer_name))) {
        if(arp->data.mode >= 0) {
            remove_peer(ke, peer_name, arp);
            rv = 1;
        }
        else rv = arp->data.mode;
    }

    return rv;
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
                        ksnet_arp_data_ext *arp, void *data) {

    #define kev ((ksnetEvMgrClass*)(ka->ke))

    int retval = 0; // Return value
    double ct = ksnetEvMgrGetTime(kev); //Current time

    // Send trip time request
    if((ct - arp->data.last_activity > CHECK_EVENTS_AFTER) || // 11,5 sec
       (ct - arp->data.last_triptime_send > CHECK_EVENTS_AFTER * 2)) { // 23 sec
        ksnCommandSendCmdEcho(kev->kc->kco, peer_name, (void*) TRIPTIME,
                              TRIPTIME_LEN);
    }

    // Disconnect dead peer
    if(ct - arp->data.last_activity > CHECK_EVENTS_AFTER + 2.5) { // 14 sec

        // Send this host disconnect command to dead peer
        send_cmd_disconnect_cb(kev->kc->ka, NULL, (ksnet_arp_data *)arp, NULL);

        // Disconnect dead peer from this host
        remove_peer(kev, peer_name, arp);

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
    ksnetEvMgrClass *ke = ((ksnCoreClass *)w->data)->ke;

    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, DEBUG_VV, "idle callback %d\n", ke->idle_count);
    #endif

    // Stop this watcher
    ev_idle_stop(EV_A_ w);

    // Idle count startup (first time run)
    if(!ke->idle_count) {
        //! \todo: open_local_port(kev);
        // Connect to R-Host
        connect_r_host_cb(ke);
        // Send event to application
        if(ke->ta) ke->ta->t_id = pthread_self();
        if(ke->event_cb != NULL) ke->event_cb(ke, EV_K_STARTED, NULL, 0, NULL);
        // Start host socket in the event manager
        if(!ke->teo_cfg.r_tcp_f) {
            ev_io_start(ke->ev_loop, &ke->kc->host_w);
        }
    }
    // Idle count max value
    else if(ke->idle_count == UINT32_MAX) ke->idle_count = 0;

    // Increment count
    ke->idle_count++;

    // Check host events to send him service information
    //! \todo:    host_cb(EV_A_ (ev_io*)w, revents);

    // Send idle Event
    if(ke->event_cb != NULL) {
        ke->event_cb(ke, EV_K_IDLE , NULL, 0, NULL);
    }

    // Set last host event time
    ksnCoreSetEventTime(ke->kc);
}

/**
 * Timer callback (runs every KSNET_EVENT_MGR_TIMER = 0.25 sec)
 *
 * param loop
 * @param w
 * @param revents
 */
void timer_cb(EV_P_ ev_timer *w, int revents) {

    #ifdef DEBUG_KSNET
    const int show_interval = 5 / KSNET_EVENT_MGR_TIMER;
    #endif
    const int activity_interval = (CHECK_EVENTS_AFTER / 8) / KSNET_EVENT_MGR_TIMER;
    ksnetEvMgrClass *ke = w->data;
    double t = ksnetEvMgrGetTime(ke);
    if(ke->last_custom_timer == 0.0) ke->last_custom_timer = t;


    if(ke->runEventMgr) {

        // Increment timer value
        ke->timer_val++;

        // Show timer info and send teonet metrics
        if( !(ke->timer_val % show_interval) ) {

            // Show timer info
            #ifdef DEBUG_KSNET
            ksn_printf(((ksnetEvMgrClass *)w->data), MODULE, DEBUG_VV,
                    "timer (%.1f sec of %f)\n",
                    show_interval * KSNET_EVENT_MGR_TIMER, t);
            #endif

            // Send teonet metrics
            metric_teonet_count(ke->tm);
        }

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

    ksnetEvMgrClass *ke = (ksnetEvMgrClass *)w->data;
    #ifdef DEBUG_KSNET
    ksn_puts(ke, MODULE, DEBUG,
            "got a signal to stop event manager ...");
    #endif

    // trudpSendResetAll(ke->kc->ku);
    ((ksnetEvMgrClass *)w->data)->runEventMgr = 0;
}

/**
 * SIGUSR2 signal handler
 *
 * @param loop
 * @param w
 * @param revents
 */
void sigusr2_cb (struct ev_loop *loop, ev_signal *w, int revents) {

    ksnetEvMgrClass *ke = (ksnetEvMgrClass *)w->data;
    static int attempt = 0;
    // trudpSendResetAll(ke->kc->ku);

    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, MESSAGE,
            "got a signal %s ...\n",
            w->signum == SIGSEGV ? "SIGSEGV" :
            w->signum == SIGABRT ? "SIGABRT" :
            w->signum == SIGUSR2 ? "SIGUSR2" :
                                   "?");
    #endif

    // If SIGSEGV repeated than we can't exit from application
    if(w->signum != SIGUSR2 && attempt) exit(-1);

    // Initiate restart application
    else {

        attempt++; // Calculate attempts

        teoRestartApp_f = 1; // Set restart flag
        if(ke->km != NULL) {
            ksn_printf(ke, MODULE, DEBUG, "multi destroy %s ...\n", "");
            ksnMultiDestroy(ke->km);
        }
        else ksnetEvMgrStop(ke); // Stop event manager and restart application before exit

        #ifdef SHOW_DFL
        signal(w->signum, SIG_DFL);
        kill(getpid(), w->signum);
        ksnetEvMgrRestart(ke->argc, ke->argv);
        exit(0);
        #endif
    }
}

// Global variables to process SIGSEGV, SIGABRT signals handler
int teo_argc;
char** teo_argv;

/**
 * SIGSEGV, SIGABRT signals handler
 *
 * @param signum
 * @param si
 * @param unused
 */
static void sigsegv_cb_h(int signum, siginfo_t *si, void *unused) {

    printf("\n%sApplication:%s Got a signal %s ...\n\n",
           (char*)ANSI_RED, (char*)ANSI_NONE,
           signum == SIGSEGV ? "SIGSEGV" :
           signum == SIGABRT ? "SIGABRT" :
                                  "?");
    usleep(250000);
    puts("starting application restart process...");
    // Unbind sockets
    int i; for (i = 9000; i <= 9030; i++) {
        //if (FD_ISSET(i, &set)) {
            close(i);
        //}
    }
    usleep(250000);

    signal(signum, SIG_DFL); // Restore signal processing
    if(system("reset"));     // Reset terminal

    teoRestartApp_f = 1;
    ksnetEvMgrRestart(teo_argc, teo_argv);

    // Default action
    // (this code does not executed because ksnetEvMgrRestart never return)
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

    if(teoRestartApp_f) {

        // Show application path and parameters
        int i = 1;
        puts("");
        printf("Restart application: %s", argv[0]);
        for(;i < argc; i++) {
            printf(" %s", argv[i]);
        }
        puts("\n");

        usleep(950000);

        //#define USE_SYSTEM
        #ifdef USE_SYSTEM
        // Execute application
        char *app = ksnet_formatMessage("%s", argv[0]);
        for(i = 1; i < argc; i++) {
            app = ksnet_formatMessage("%s %s", app, argv[i]);
        }
        if(system(app));
        free(app);
        exit(0);
        #else
        // Execute application with changed parameters
        char *s = strstr(argv[0], " ");
        if(s != NULL) {
            char *app = ksnet_formatMessage("%s", argv[0]);
            for(i = 1; i < argc; i++) {
                app = ksnet_formatMessage("%s %s", app, argv[i]);
            }
            puts(app);
            ksnet_stringArr argvv = ksnet_stringArrSplit(app, " ", 0, 0);
            if(execv(argvv[0], argvv) == -1) {
                fprintf(stderr, "Can't execute application %s: %s\n",
                        argv[0], strerror(errno));
                exit(-1);
            }
        } else
        // Execute application
        if(execvp(argv[0], argv) == -1) {
            fprintf(stderr, "Can't execute application %s: %s\n",
                    argv[0], strerror(errno));
            exit(-1);
        }
        exit(0); // Not need it because execv never return
        #endif
    }

    return teoRestartApp_f;
}

/**
 * Async Idle callback.
 *
 * Process async queue messages
 *
 * param loop
 * @param w
 * @param revents
 */
void idle_async_cb(EV_P_ ev_idle *w, int revents) {

    #define kev ((ksnetEvMgrClass *)w->data)

    //static int i = 0;
    //printf("Async Idle callback %d\n", ++i);

    // Stop this watcher
    ev_idle_stop(EV_A_ w);
    ev_async_send(kev->ev_loop,/*EV_DEFAULT_*/ &kev->sig_async_w);

    #undef kev
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

    // Send async event to event loop
    #define SEND_EVENT(data, data_len, user_data) \
        kev->event_cb(kev, EV_K_ASYNC , data, data_len, user_data)

//        pthread_mutex_unlock (&kev->async_mutex);
//        kev->event_cb(kev, EV_K_ASYNC , data, data_len, user_data);
//        pthread_mutex_lock (&kev->async_mutex)


    #ifdef DEBUG_KSNET
    ksn_puts(kev, MODULE, DEBUG_VV, "async event callback");
    #endif

    // Get data from async queue and send user event with it
//    pthread_mutex_lock (&kev->async_mutex);
    //int i = 0;
    for(;;) {
        pthread_mutex_lock(&kev->async_mutex);
        int isEmpty = pblListIsEmpty(kev->async_queue);
        pthread_mutex_unlock(&kev->async_mutex);
        if(isEmpty) break;

        if(!ev_is_active(EV_A_ &kev->idle_async_w)) {
            size_t ptr = 0;
            pthread_mutex_lock(&kev->async_mutex);
            void *data = pblListPoll(kev->async_queue);
            pthread_mutex_unlock(&kev->async_mutex);
            if(data != NULL) {
                void *user_data = *(void**)data; ptr += sizeof(void*);
                uint32_t data_len = *(uint32_t*)(data + ptr); ptr += sizeof(uint32_t);
                SEND_EVENT(data_len ? data + ptr : NULL, data_len, user_data);
                free(data);
            }
            else {
                SEND_EVENT(NULL, 0, NULL);
            }
            //if(++i <= 2) continue; // Send 3 records without Idle event
            ev_idle_start(EV_A_ &kev->idle_async_w); // Send idle event to continue queue processing
        }
        break;
    }
//    pthread_mutex_unlock (&kev->async_mutex);

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
    ksn_printf(((ksnetEvMgrClass *)w->data), MODULE, DEBUG_VV,
                "idle activity callback %d\n", kev->idle_activity_count);
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

    // Check TR-UDP activity
    //trudpProcessKeepConnection(kev->kc->ku);

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
    if((ke->kc = ksnCoreInit(ke, ke->teo_cfg.host_name, ke->teo_cfg.port, NULL)) == NULL) return 0;

    // Hotkeys
    if(!ke->teo_cfg.block_cli_input_f && !ke->teo_cfg.dflag) {
        if(!ke->net_idx) ke->kh = ksnetHotkeysInit(ke);
        // Set filter from parameters
        if(ke->teo_cfg.filter[0]) teoHotkeySetFilter(ke->kh, ke->teo_cfg.filter);
    }

    // Callback QUEUE
    #if M_ENAMBE_CQUE
    ke->kq = ksnCQueInit(ke, 0);
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

    // Logging server module
    #ifdef M_ENAMBE_LOGGING_SERVER
    ke->ls = teoLoggingServerInit(ke);
    #endif

    // Logging client module
    #ifdef M_ENAMBE_LOGGING_CLIENT
    ke->lc = teoLoggingClientInit(ke);
    #endif

    // Log reader module
    #ifdef M_ENAMBE_LOG_READER
    ke->lr = teoLogReaderInit(ke);
    #endif

    // Async module
    #ifdef M_ENAMBE_ASYNC
    ke->ta = teoAsyncInit(ke);
    #endif

    // Metric module
    #ifdef M_ENAMBE_METRIC
    ke->tm = teoMetricInit(ke);
    #endif

    return 1;
}

/**
 * Destroy modules
 *
 * @param ke
 */
void modules_destroy(ksnetEvMgrClass *ke) {
    #ifdef M_ENAMBE_METRIC
    teoMetricDestroy(ke->tm);
    #endif
    #ifdef M_ENAMBE_ASYNC
    teoAsyncDestroy(ke->ta);
    #endif
    #ifdef M_ENAMBE_LOG_READER
    teoLogReaderDestroy(ke->lr);
    #endif
    #ifdef M_ENAMBE_LOGGING_CLIENT
    teoLoggingClientDestroy(ke->lc);
    #endif
    #ifdef M_ENAMBE_LOGGING_SERVER
    teoLoggingServerDestroy(ke->ls);
    #endif
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

    ke->teo_cfg.port = ke->kc->port;
    ksnetHotkeysDestroy(ke->kh);
    ksnCoreDestroy(ke->kc);

    #if M_ENAMBE_TCP_P
    ksnTCPProxyDestroy(ke->tp);
    #endif
    #if M_ENAMBE_TCP
    ksnTcpDestroy(ke->kt);
    #endif
}
