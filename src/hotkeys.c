/**
 * File:   hotkeys.c
 * Author: Kirill Scherba
 *
 * Created on April 16, 2015, 10:43 PM
 *
 * Hot keys monitor module
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "hotkeys.h"
#include "ev_mgr.h"
#include "utils/rlutil.h"
#include "utils/utils.h"

/**
 * Yes answer action
 */
enum wait_y {
    Y_NONE,
    Y_QUIT
};

// Local functions
ping_timer_data *ping_timer_init(ksnCoreClass *kn, char *peer);
void ping_timer_stop(ping_timer_data **pt);
monitor_timer_data *monitor_timer_init(ksnCoreClass *kn);
void monitor_timer_stop(monitor_timer_data **mt);
peer_timer_data *peer_timer_init(ksnCoreClass *kn);
void peer_timer_stop(peer_timer_data **pet);
void _keys_non_blocking_start(ksnetHotkeysClass *kh);
/**
 * Stop keyboard non-blocking mode
 *
 * @param kh
 */
#define _keys_non_blocking_stop(kh) \
    tcsetattr(0, TCSANOW, &kh->initial_settings); \
    kh->non_blocking = 0


// Constants
#define COLOR_DW "\033[1;37m"
#define COLOR_END "\033[0m"
//
const char
    *SHOW = "show",
    *DONT_SHOW = "don't show",
    *START = "start",
    *STOP = "stop",
    *ON = "on",
    *OFF = "off",
    *PING = "ping",
    *TRACE = "trace",
    *MONITOR = "monitor",
    *TRIPTIME = "triptime";

/******************************************************************************/
/* Hot keys functions                                                         */
/*                                                                            */
/******************************************************************************/

/**
 * Callback procedure which called by event manager when STDIN FD has any data
 */
void hotkeys_cb(void *ke, void *data) {

    #define kev ((ksnetEvMgrClass*)ke) // Event manager
    #define kh  kev->kh   // Hotkeys class
    #define kc  kev->kc   // Net core class

    int hotkey;

    // Check hot key
    if(!kh->non_blocking) hotkey = kh->last_hotkey;
    else hotkey = *(int*)data;

    if(hotkey != 'y') kh->wait_y = Y_NONE;
    switch(hotkey) {

        // Help
        case '?':
        case 'h':
            printf(
            "\n"
//            "KSVpn hot keys monitor, ver. 0.0.1\n"
            "Hot keys list:\n"
            "--------------------------------------------------------------------\n"
            " "COLOR_DW"h"COLOR_END" - show this help screen\n"
            " "COLOR_DW"p"COLOR_END" - show peers\n"
            "%s"
            #ifdef DEBUG_KSNET
            " "COLOR_DW"d"COLOR_END" - show/hide debug messages: %s\n"
            " "COLOR_DW"w"COLOR_END" - show/hide debug_vv messages: %s\n"
            #endif
            " "COLOR_DW"m"COLOR_END" - send message to peer\n"
            " "COLOR_DW"i"COLOR_END" - send ping to peer %s\n"
//            " "COLOR_DW"t"COLOR_END" - trace route to peer\n"
            " "COLOR_DW"M"COLOR_END" - monitor network %s\n"
//            " "COLOR_DW"C"COLOR_END" - direct connect to peer\n"
//            " "COLOR_DW"A"COLOR_END" - direct connect to all peers\n"
            " "COLOR_DW"q"COLOR_END" - quit from application\n"
            "--------------------------------------------------------------------\n"
//            , kev->kvpn == NULL ? "" : " "COLOR_DW"v"COLOR_END" - show VPN\n"
            , ""
            #ifdef DEBUG_KSNET
            , (kev->ksn_cfg.show_debug_f ? SHOW : DONT_SHOW)
            , (kev->ksn_cfg.show_debug_vv_f ? SHOW : DONT_SHOW)
            #endif
            , (kh->pt != NULL ? "(running now, press i to stop)" : "")
            , (kh->mt != NULL ? "(running now, press M to stop)" : "")
            );
            break;

        // Show peers
        case 'p':
            if(kh->pet == NULL) {
                int num_lines = ksnetArpShow(kc->ka);
                if(kh->last_hotkey == hotkey) kh->peer_m = !kh->peer_m;
                if(kh->peer_m) {
                    kh->pet = peer_timer_init(kc);
                    kh->pet->num_lines = num_lines;
                }
                printf("\n");
            }
            else if(kh->peer_m) {
                kh->peer_m = 0;
                peer_timer_stop(&kh->pet);
                //printf("Peer timer stopped\n");
            }
            printf("Press p to %s continuously refresh\n",
                   (kh->peer_m ? STOP : START));
            break;

//        // Show VPN
//        case 'v':
//            ksnVpnListShow(kev->kvpn);
//            break;

        // Show debug
        case 'd':
            kev->ksn_cfg.show_debug_f = !kev->ksn_cfg.show_debug_f;
            printf("Show debug messages switch %s\n",
                   (kev->ksn_cfg.show_debug_f ? ON :OFF));
            break;

        // Show debug_vv
        case 'w':
            kev->ksn_cfg.show_debug_vv_f = !kev->ksn_cfg.show_debug_vv_f;
            printf("Show debug_vv messages switch %s\n",
                   (kev->ksn_cfg.show_debug_vv_f ? ON :OFF));
            break;

        // Send message
        case 'm':
            // Got hot key
            if(kh->non_blocking) {
                // Request string 'to'
                kh->str_number = 0;
                printf("to: ");
                fflush(stdout);
                // Switch STDIN to receive string
                _keys_non_blocking_stop(kh);
            }
            // Got requested strings
            else switch(kh->str_number) {
                // Got 'to' string
                case 0:
                    trimlf((char*)data);
                    if(((char*)data)[0]) {
                        strncpy(kh->str[kh->str_number], (char*)data, KSN_BUFFER_SM_SIZE);
                        // Request string 'message'
                        kh->str_number = 1;
                        printf("message: ");
                        fflush(stdout);
                    }
                    else _keys_non_blocking_start(kh);
                    break;
                // Got 'message' string
                case 1:
                    trimlf((char*)data);
                    if(((char*)data)[0]) {
                        // Send message
                        printf("Send message => to: '%s', message: '%s'\n",
                               kh->str[0], (char*)data);
                        ksnCommandSendCmdEcho(kc->kco, kh->str[0], data,
                                              strlen(data) + 1);
                    }
                    // Switch STDIN to receive hot key
                    _keys_non_blocking_start(kh);
                    break;
            }
            break;

        // Send ping
        case 'i':
            // Got hot key
            if(kh->non_blocking) {

                // Request string 'to'
                if(kh->pt == NULL) {
                    kh->str_number = 0;
                    printf("Ping to: ");
                    fflush(stdout);
                    _keys_non_blocking_stop(kh); // Switch STDIN to string
                }
                // Stop ping timer
                else {
                    ping_timer_stop(&kh->pt);
                    printf("Ping stopped\n");
                }
            }
            // Got requested strings
            else switch(kh->str_number) {
                // Got 'to' string
                case 0:
                    trimlf((char*)data);
                    if(((char*)data)[0]) {
                        // Send message
                        printf("Send message => to: '%s', message: '%s'\n"
                               "\n"
                               "Press i to stop ping\n\n",
                               (char*)data, PING);
                        kh->pt = ping_timer_init(kc, (char *)data);
                    }
                    _keys_non_blocking_start(kh); // Switch STDIN to hot key
                    break;
            }
            break;

        // Trace route
        case 't':
            // Got hot key
            if(kh->non_blocking) {

                // Request string 'to'
                kh->str_number = 0;
                printf("Trace to: ");
                fflush(stdout);
                _keys_non_blocking_stop(kh); // Switch STDIN to string
            }
            // Got requested strings
            else switch(kh->str_number) {

                // Got 'to' string
                case 0:
                    trimlf((char*)data);
                    if(((char*)data)[0]) {
                        // Send message
                        printf("Send message => to: '%s', message: '%s'\n"
                               "\n"
                               ,(char*)data
                               ,TRACE
                        );
                        size_t data_len =  strlen(data) + 1,
                               trace_buf_len = TRACE_LEN + data_len;
                        char trace_buf[trace_buf_len];
                        memcpy(trace_buf, (void*)TRACE, TRACE_LEN);
                        memcpy(trace_buf + TRACE_LEN, data, data_len);
                        // TODO: peer_send_cmd_echo(kn, data, (void*)trace_buf, trace_buf_len, 0);
                    }
                    _keys_non_blocking_start(kh); // Switch STDIN to hot key
                    break;
            }
            break;

        // Start/Stop network monitor
        case 'M':
            // Got hot key
            if(kh->non_blocking) {

                // Request string 'to'
                if(kh->mt == NULL) {
                    kh->mt = monitor_timer_init(kc);
                    printf("Network monitor started (press M to stop)\n");
                }
                // Stop ping timer
                else {
                    monitor_timer_stop(&kh->mt);
                    printf("Network monitor stopped\n");
                }
            }
            break;

        // Quit
        case 'q':
            puts("Press y to quit application");
            kh->wait_y = Y_QUIT;
            break;

        // Y - yes
        case 'y':
            switch(kh->wait_y) {
                case Y_QUIT:
                    puts("Quit ...");
                    ksnetEvMgrStop(kev);
                    break;
                default:
                    break;
            }
            kh->wait_y = Y_NONE;
            break;
    }
    kh->last_hotkey = hotkey;

    #undef kc
    #undef kh
    #undef kev
}

/**
 * Start keyboard non-blocking mode
 *
 * @param kh
 */
void _keys_non_blocking_start(ksnetHotkeysClass *kh) {

    kh->new_settings = kh->initial_settings;
    kh->new_settings.c_lflag &= ~ICANON;
    kh->new_settings.c_lflag &= ~ECHO;
    //kh->new_settings.c_lflag &= ~ISIG;    // SIGNAL
    kh->new_settings.c_cc[VMIN] = 0;
    kh->new_settings.c_cc[VTIME] = 0;

    tcsetattr(0, TCSANOW, &kh->new_settings);

    kh->non_blocking = 1;
}

/**
 * Initialize hot keys module
 *
 * @return
 */
ksnetHotkeysClass *ksnetHotkeysInit(void *ke) {

    ksnetHotkeysClass *kh = malloc(sizeof(ksnetHotkeysClass));

    tcgetattr(0,&kh->initial_settings);
    _keys_non_blocking_start(kh);
    kh->wait_y = Y_NONE;
    kh->peer_m = 0;
    kh->pt = NULL;
    kh->mt = NULL;
    kh->pet = NULL;

    // Start show peer
    if(((ksnetEvMgrClass*)ke)->ksn_cfg.show_peers_f) {
        kh->pet = peer_timer_init( ((ksnetEvMgrClass*)ke)->kc );
        kh->peer_m = 1;
    }

    return kh;
}

/**
 * De-initialize hot key module
 *
 * @param kh
 */
void ksnetHotkeysDestroy(ksnetHotkeysClass *kh) {

    if(kh != NULL) {
        
        _keys_non_blocking_stop(kh);
        free(kh);
    }
}


/******************************************************************************/
/* Ping timer functions                                                       */
/*                                                                            */
/******************************************************************************/

/**
 * Ping timer callback
 *
 * @param loop
 * @param w
 * @param revents
 */
void ping_timer_cb(EV_P_ ev_timer *w, int revents) {

    // Create buffer with size of 64 byte minus size of double which will be
    // added in peer_send_cmd_echo function
    ping_timer_data *pt = w->data;
    size_t ping_buf_len = 64 - sizeof(double);
    char ping_buf[ping_buf_len];
    memcpy(ping_buf, (void*)PING, PING_LEN);

    // Send echo-ping command
    // TODO: peer_send_cmd_echo(pt->kn, pt->peer_name, (void*)ping_buf, ping_buf_len, 0);
    ksnCommandSendCmdEcho(pt->kn->kco, pt->peer_name, (void*)ping_buf, ping_buf_len);
}

/**
 * Initialize ping timer
 *
 * @param peer
 * @return
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
ping_timer_data *ping_timer_init(ksnCoreClass *kn, char *peer) {

    ping_timer_data *pt = malloc(sizeof(ping_timer_data));
    pt->peer_name_len = strlen(peer) + 1;
    pt->peer_name = malloc(pt->peer_name_len);
    strcpy(pt->peer_name, peer);
    pt->kn = kn;

    // Initialize and start main timer watcher, it is a repeated timer
    pt->loop = EV_DEFAULT;
    ev_timer_init (&pt->w, ping_timer_cb, 0.0, 1.0);
    pt->w.data = pt;
    ev_timer_start (pt->loop, &pt->w);

    return pt;
}
#pragma GCC diagnostic pop

/**
 * Stop ping timer
 *
 * @param pt
 */
void ping_timer_stop(ping_timer_data **pt) {

    ev_timer_stop((*pt)->loop, &(*pt)->w);

    free((*pt)->peer_name);
    free(*pt);

    *pt = NULL;
}


/******************************************************************************/
/* Monitor timer functions                                                    */
/*                                                                            */
/******************************************************************************/

/**
 * Send network monitor ping to one peer
 *
 * @param kn
 * @param peer_name
 */
int monitor_timer_one_cb(ksnetArpClass *ka, char *peer_name, ksnet_arp_data *arp_data, void *data) {


    // Reset monitor time
    //ksnet_arp_data *arp_data = ksnetArpGet(ka, peer_name);
    printf("%s%s: %.3f ms %s \n",
            arp_data->monitor_time == 0.0 ? getANSIColor(LIGHTRED) : getANSIColor(LIGHTGREEN),
            peer_name, arp_data->monitor_time * 1000.0,
            getANSIColor(NONE));
    arp_data->monitor_time = 0;
    ksnetArpAdd(ka, peer_name, arp_data);

    //peer_send_cmd_echo(kn, peer_name, (void*)MONITOR, MONITOR_LEN, 1);
    ksnCommandSendCmdEcho(((ksnetEvMgrClass*)(ka->ke))->kc->kco, peer_name,
                          (void*)MONITOR, MONITOR_LEN);

    return 0;
}

#define MONITOR_TIMER_INTERVAL 5.0

/**
 * Monitor timer callback
 *
 * @param loop
 * @param w
 * @param revents
 */
void monitor_timer_cb(EV_P_ ev_timer *w, int revents) {

    monitor_timer_data *mt = w->data;

    printf("Network monitor => send request to: ");
    ksnetArpGetAll(mt->kn->ka, monitor_timer_one_cb, NULL);
    //printf("\n");
}

/**
 * Initialize monitor timer
 *
 * @param peer
 * @return
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
monitor_timer_data *monitor_timer_init(ksnCoreClass *kn) {

    monitor_timer_data *mt = malloc(sizeof(monitor_timer_data));
    mt->kn = kn;

    // Initialize and start main timer watcher, it is a repeated timer
    mt->loop = EV_DEFAULT;
    ev_timer_init (&mt->w, monitor_timer_cb, MONITOR_TIMER_INTERVAL, MONITOR_TIMER_INTERVAL);
    mt->w.data = mt;
    ev_timer_start (mt->loop, &mt->w);

    return mt;
}
#pragma GCC diagnostic pop

/**
 * Stop monitor timer
 *
 * @param mt
 */
void monitor_timer_stop(monitor_timer_data **mt) {

    ev_timer_stop((*mt)->loop, &(*mt)->w);

    free(*mt);

    *mt = NULL;
}


/******************************************************************************/
/* Show peers timer functions                                                 */
/*                                                                            */
/******************************************************************************/

#define PEER_TIMER_INTERVAL 0.150

/**
 * Peer show idle callback
 *
 * @param loop
 * @param tw
 * @param revents
 */
void peer_idle_cb(EV_P_ ev_idle *iw, int revents) {

    // Pointer to watcher data
    peer_timer_data *pet = iw->data;

    // Stop idle watcher
    ev_idle_stop(pet->loop, iw);

    // Clear previous shown lines
    if(pet->num_lines) printf("\033[%dA\r\033[J\n", pet->num_lines + 3);

    // Show ARP
    pet->num_lines = ksnetArpShow(pet->kn->ka);

    if(pet->num_lines)
    printf("\nPress p to %s continuously refresh\n", STOP);

    // Start timer watcher
    ev_timer_start(pet->loop, &(pet->tw));
}

/**
 * Peer timer callback
 *
 * @param loop
 * @param tw
 * @param revents
 */
void peer_timer_cb(EV_P_ ev_timer *tw, int revents) {

    // Pointer to watcher data
    peer_timer_data *pet = tw->data;

    // Stop peer timer and start peer idle to process timer request in idle time
    ev_timer_stop(pet->loop, tw);
    ev_idle_start(pet->loop, &(pet->iw));
}

/**
 * Initialize peer timer
 *
 * @param kn
 * @return
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
peer_timer_data *peer_timer_init(ksnCoreClass *kn) {

    peer_timer_data *pet = malloc(sizeof(peer_timer_data));
    pet->kn = kn;

    // Initialize and start main timer watcher, it is a repeated timer
    pet->loop = EV_DEFAULT;
    ev_idle_init (&pet->iw, peer_idle_cb);
    pet->iw.data = pet;
    ev_timer_init (&pet->tw, peer_timer_cb, PEER_TIMER_INTERVAL, PEER_TIMER_INTERVAL);
    pet->tw.data = pet;
    ev_timer_start (pet->loop, &pet->tw);

    return pet;
}
#pragma GCC diagnostic pop

/**
 * Stop peer timer
 *
 * @param pet
 */
void peer_timer_stop(peer_timer_data **pet) {

    ev_timer_stop((*pet)->loop, &(*pet)->tw);
    free(*pet);

    *pet = NULL;
}
