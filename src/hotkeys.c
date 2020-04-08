/**
 * File:   hotkeys.c
 * Author: Kirill Scherba <kirill@scherba.ru>
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
#include "net_multi.h"
#include "utils/rlutil.h"
#include "utils/utils.h"
#include "tr-udp_stat.h"
#include "utils/teo_memory.h"

#undef MODULE
#define MODULE _ANSI_CYAN "event_manager" _ANSI_NONE

/**
 * Yes answer action
 */
enum wait_y {
    Y_NONE,
    Y_QUIT
};

// Local functions
void stdin_cb (EV_P_ ev_io *w, int revents); // STDIN callback
void idle_stdin_cb (EV_P_ ev_idle *w, int revents); // STDIN idle callback
ping_timer_data *ping_timer_init(ksnCoreClass *kn, char *peer);
void ping_timer_stop(ping_timer_data **pt);
monitor_timer_data *monitor_timer_init(ksnCoreClass *kn);
void monitor_timer_stop(monitor_timer_data **mt);
peer_timer_data *peer_timer_init(ksnCoreClass *kn);
void peer_timer_stop(peer_timer_data **pet);
tr_udp_timer_data *tr_udp_timer_init(ksnCoreClass *kn);
void tr_udp_timer_stop(tr_udp_timer_data **pet);

// Constants
#define COLOR_DW "\033[01;32m"
//"\033[1;37m"
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
#define kev ((ksnetEvMgrClass*)ke) // Event manager
#define khv  kev->kh   // Hotkeys class
#define kc  kev->kc   // Net core class

void teoHotkeySetFilter(void *ke, void *filter) {
    if (khv->filter_arr != NULL) {
        ksnet_stringArrFree(&khv->filter_arr);
    }
    khv->filter_arr = ksnet_stringArrSplit((char *)filter, "|", 0, 0);
}

unsigned char teoFilterFlagCheck(void *ke) {
    if (khv != NULL) {
        if (khv->filter_f) {
            return 1;
        } else {
            return 0;
        }
    }
    return 1;
}

unsigned char teoLogCheck(void *ke, void *log) {

    if ((log != NULL) && (khv != NULL) && (khv->filter_arr != NULL)) {
        unsigned i = 0;
        for (i = 0; khv->filter_arr[i] != NULL; ++i) {
            if (strstr((char *)log, khv->filter_arr[i]) != NULL) {
                return 1;
            }
        }
    } else return 1;
    return 0;
}

/**
 * Callback procedure which called by event manager when STDIN FD has any data
 */
int hotkeys_cb(void *ke, void *data, ev_idle *w) {

    int hotkey;

    // Check hot key
    if(!khv->non_blocking) hotkey = khv->last_hotkey;
    else hotkey = *(int*)data;

    if(hotkey != 'y') khv->wait_y = Y_NONE;
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
            " "COLOR_DW"c"COLOR_END" - show/hide debug_vvv messages: %s\n"        
            #endif
            " "COLOR_DW"m"COLOR_END" - send message to peer\n"
            " "COLOR_DW"i"COLOR_END" - send ping to peer %s\n"
//            " "COLOR_DW"t"COLOR_END" - trace route to peer\n"
            " "COLOR_DW"M"COLOR_END" - monitor network %s\n"
//            " "COLOR_DW"C"COLOR_END" - direct connect to peer\n"
//            " "COLOR_DW"A"COLOR_END" - direct connect to all peers\n"
            "%s"
            " "COLOR_DW"u"COLOR_END" - TR-UDP statistics\n"
            " "COLOR_DW"Q"COLOR_END" - TR-UDP queues\n"
            " "COLOR_DW"s"COLOR_END" - show subscribers\n"
            " "COLOR_DW"a"COLOR_END" - show application menu\n"
            " "COLOR_DW"f"COLOR_END" - set filter\n"
            " "COLOR_DW"r"COLOR_END" - restart application\n"
            " "COLOR_DW"q"COLOR_END" - quit from application\n"
            "--------------------------------------------------------------------\n"
            #if M_ENAMBE_VPN
            , kev->kvpn == NULL ? "" : " "COLOR_DW"v"COLOR_END" - show VPN\n"
            #else
            , ""
            #endif
            #ifdef DEBUG_KSNET
            , (kev->ksn_cfg.show_debug_f ? SHOW : DONT_SHOW)
            , (kev->ksn_cfg.show_debug_vv_f ? SHOW : DONT_SHOW)
            , (kev->ksn_cfg.show_debug_vvv_f ? SHOW : DONT_SHOW)
            #endif
            , (khv->pt != NULL ? "(running now, press i to stop)" : "")
            , (khv->mt != NULL ? "(running now, press M to stop)" : "")
            , kev->num_nets > 1 ? " "COLOR_DW"n"COLOR_END" - switch to other network\n" : ""
            );
            break;

        // Show UDP statistics
        case 'u': {
            if(khv->put == NULL) {
                int num_lines = ksnTRUDPstatShow(kc->ku);
                if(khv->last_hotkey == hotkey) khv->tr_udp_m = !khv->tr_udp_m;
                if(khv->tr_udp_m) {
                    khv->put = tr_udp_timer_init(kc);
                    khv->put->num_lines = num_lines;
                }
                printf("\n");
            }
            else if(khv->tr_udp_m) {
                khv->tr_udp_m = 0;
                tr_udp_timer_stop(&khv->put);
                printf("TR-UDP timer stopped\n");
            }
            printf("Press u to %s continuously refresh\n",
                   (khv->tr_udp_m ? STOP : START));
        } break;
        
        // Show UDP send/receive queue list
        case 'Q': {
            
            //ksnTRUDPqueuesShow(kc->ku);     
            
            if(khv->put == NULL) {
                int num_lines = ksnTRUDPqueuesShow(kc->ku);
                if(khv->last_hotkey == hotkey) khv->tr_udp_queues_m = !khv->tr_udp_queues_m;
                if(khv->tr_udp_queues_m) {
                    khv->put = tr_udp_timer_init(kc);
                    khv->put->num_lines = num_lines;
                }
                printf("\n");
            }
            else if(khv->tr_udp_queues_m) {
                khv->tr_udp_queues_m = 0;
                tr_udp_timer_stop(&khv->put);
                printf("TR-UDP timer stopped\n");
            }
            printf("Press Q to %s continuously refresh\n",
                   (khv->tr_udp_queues_m ? STOP : START));
            
        } break;
        
        // Show subscribers
        case 's':
            teoSScrSubscriptionList(kc->kco->ksscr);
            break;
            
        // Send User event to Application
        case 'a':
        case 'A':
            if(kev->event_cb != NULL)
                kev->event_cb(ke, EV_K_USER , NULL, 0, NULL);
            break;
            
        // Show peers
        case 'p':
            if(khv->pet == NULL) {
                int num_lines = ksnetArpShow(kc->ka);
                if(khv->last_hotkey == hotkey) khv->peer_m = !khv->peer_m;
                if(khv->peer_m) {
                    khv->pet = peer_timer_init(kc);
                    khv->pet->num_lines = num_lines;
                }
                printf("\n");
            }
            else if(khv->peer_m) {
                khv->peer_m = 0;
                peer_timer_stop(&khv->pet);
                //printf("Peer timer stopped\n");
            }
            printf("Press p to %s continuously refresh\n",
                   (khv->peer_m ? STOP : START));
            break;

        // Show VPN
        #if M_ENAMBE_VPN
        case 'v':
            ksnVpnListShow(kev->kvpn);
            break;
        #endif

        // Show debug
        case 'd':
            if(kev->ksn_cfg.show_debug_vv_f || kev->ksn_cfg.show_debug_vvv_f)
                kev->ksn_cfg.show_debug_vv_f = kev->ksn_cfg.show_debug_vvv_f = 0;
            else {
                kev->ksn_cfg.show_debug_f = !kev->ksn_cfg.show_debug_f;
                printf("Show debug messages switch %s\n",
                     (kev->ksn_cfg.show_debug_f ? ON :OFF));
            }  
            break;

        // Show debug_vv
        case 'w':
            if(!kev->ksn_cfg.show_debug_vvv_f) {
              kev->ksn_cfg.show_debug_vv_f = !kev->ksn_cfg.show_debug_vv_f;
            }  
            kev->ksn_cfg.show_debug_vvv_f = 0;
            printf("Show debug_vv messages switch %s\n",
                   (kev->ksn_cfg.show_debug_vv_f ? ON :OFF));
            break;

        // Show debug_vvv
        case 'c':
            kev->ksn_cfg.show_debug_vvv_f = !kev->ksn_cfg.show_debug_vvv_f;
            kev->ksn_cfg.show_debug_vv_f = kev->ksn_cfg.show_debug_vvv_f;
            printf("Show debug_vvv messages switch %s\n",
                   (kev->ksn_cfg.show_debug_vvv_f ? ON :OFF));
            break;

        // Send message
        case 'm':
            // Got hot key
            if(khv->non_blocking) {
                // Request string 'to'
                khv->str_number = 0;
                printf("to: ");
                fflush(stdout);
                // Switch STDIN to receive string
                _keys_non_blocking_stop(khv);
            }
            // Got requested strings
            else switch(khv->str_number) {
                // Got 'to' string
                case 0:
                    trimlf((char*)data);
                    if(((char*)data)[0]) {
                        strncpy(khv->str[khv->str_number], (char*)data, KSN_BUFFER_SM_SIZE);
                        // Request string 'message'
                        khv->str_number = 1;
                        printf("message: ");
                        fflush(stdout);
                    }
                    else _keys_non_blocking_start(khv);
                    break;
                // Got 'message' string
                case 1:
                    trimlf((char*)data);
                    if(((char*)data)[0]) {
                        // Send message
                        printf("Send message => to: '%s', message: '%s'\n",
                               khv->str[0], (char*)data);
                        ksnCommandSendCmdEcho(kc->kco, khv->str[0], data,
                                              strlen(data) + 1);
                    }
                    // Switch STDIN to receive hot key
                    _keys_non_blocking_start(khv);
                    break;
            }
            break;

        // Switch network
        case 'n':
            if(kev->num_nets > 1) {
                
                if(khv->non_blocking) {
                    
                    // Show list of networks
                    if(kev->km != NULL) {
                        char *net_list = ksnMultiShowListStr(kev->km);
                        printf("%s", net_list);
                        free(net_list);
                    }
                    
                    // Request string with new network number
                    khv->str_number = 0;
                    printf("Enter new network number "
                           "(from 1 to %d, current net is %d): ",  
                           (int)kev->num_nets, (int)kev->n_num + 1);
                    fflush(stdout);
                    
                    // Switch STDIN to receive string
                    _keys_non_blocking_stop(khv);
                }
                
                // Got requested strings
                else switch(khv->str_number) {
                    
                    // Got network number in string
                    case 0:
                        
                        // Switch STDIN to receive hot key
                        _keys_non_blocking_start(khv);

                        // Parse data
                        trimlf((char*)data);
                        if(((char*)data)[0]) {
                            
                            strncpy(khv->str[khv->str_number], 
                                    (char*)data, KSN_BUFFER_SM_SIZE);
                            
                            // Switch to entered network number 
                            int n_num = atoi(khv->str[khv->str_number]);
                            if(n_num >= 1 && n_num <= kev->num_nets) {
                                
                                n_num--;
                                if(n_num != kev->n_num) {
                                    
                                    ksnetEvMgrClass *p_ke = ke;
                                    
                                    // Destroy hotkeys module
                                    ksnetHotkeysDestroy(khv);
                                    p_ke->kh = NULL;
                                    
                                    // Switch to new network (thread theme)
                                    if(p_ke->km == NULL) {
                                        for(;;) {
                                            if(n_num < p_ke->n_num) p_ke = p_ke->n_prev;
                                            else if(n_num > p_ke->n_num) p_ke = p_ke->n_next;
                                            else {
                                                // Initialize hotkeys module
                                                #define switch_to_net(p_ke) \
                                                p_ke->kh = ksnetHotkeysInit(p_ke); \
                                                printf("Switched to network #%d\n", n_num + 1); \
                                                return 1

                                                switch_to_net(p_ke);
                                            }
                                        }
                                    }
                                    // Switch to new network (multi net module theme)
                                    else {
                                        p_ke = (ksnetEvMgrClass *)ksnMultiGet(p_ke->km, n_num);
                                        switch_to_net(p_ke);
                                    }
                                }
                                else printf("Already in network #%d\n", n_num + 1);
                            }
                            else printf("Wrong network number #%d\n", n_num);
                        }
                        break;
                    }
                }
                break;
            
        // Send ping
        case 'i':
            // Got hot key
            if(khv->non_blocking) {

                // Request string 'to'
                if(khv->pt == NULL) {
                    khv->str_number = 0;
                    printf("Ping to: ");
                    fflush(stdout);
                    _keys_non_blocking_stop(khv); // Switch STDIN to string
                }
                // Stop ping timer
                else {
                    ping_timer_stop(&khv->pt);
                    printf("Ping stopped\n");
                }
            }
            // Got requested strings
            else switch(khv->str_number) {
                
                // Got 'to' string
                case 0:
                    trimlf((char*)data);
                    if(((char*)data)[0]) {
                        // Send message
                        printf("Send message => to: '%s', message: '%s'\n"
                               "\n"
                               "Press i to stop ping\n\n",
                               (char*)data, PING);
                        khv->pt = ping_timer_init(kc, (char *)data);
                    }
                    _keys_non_blocking_start(khv); // Switch STDIN to hot key
                    break;
            }
            break;

        // Trace route
        case 't':
            // Got hot key
            if(khv->non_blocking) {

                // Request string 'to'
                khv->str_number = 0;
                printf("Trace to: ");
                fflush(stdout);
                _keys_non_blocking_stop(khv); // Switch STDIN to string
            }
            // Got requested strings
            else switch(khv->str_number) {

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
                        //! \todo: peer_send_cmd_echo(kn, data, (void*)trace_buf, trace_buf_len, 0);
                    }
                    _keys_non_blocking_start(khv); // Switch STDIN to hot key
                    break;
            }
            break;

        // Start/Stop network monitor
        case 'M':
            // Got hot key
            if(khv->non_blocking) {

                // Request string 'to'
                if(khv->mt == NULL) {
                    khv->mt = monitor_timer_init(kc);
                    printf("Network monitor started (press M to stop)\n");
                }
                // Stop ping timer
                else {
                    monitor_timer_stop(&khv->mt);
                    printf("Network monitor stopped\n");
                }
            }
            break;

        // Filter
        case 'f':
        {
            khv->filter_f = !khv->filter_f;
                // Got hot key
            if(khv->non_blocking) {
                khv->str_number = 0;
                if(khv->filter_arr) {
                    printf("Current filter: %s\n",ksnet_stringArrCombine(khv->filter_arr, "|"));
                }
                printf("Enter word filter: ");
                fflush(stdout);
                _keys_non_blocking_stop(khv); // Switch STDIN to string
            }
            // Got requested strings
            else switch(khv->str_number) {
                
                // Got 'filter' string
                case 0:
                {
                    trimlf((char*)data);
                    if(((char*)data)[0]) {
                        printf("FILTER '%s'\n", (char*)data);
                        teoHotkeySetFilter(ke, data);
                    } else {
                        printf("FILTER was reset\n");
                        teoHotkeySetFilter(ke, " ");
                    }
                    _keys_non_blocking_start(khv); // Switch STDIN to hot key
                }
                break;
            }
        } break;
        // Quit
        case 'q':
            puts("Press y to quit application");
            khv->wait_y = Y_QUIT;
            break;
            
        // Restart
        case 'r':
            // Restart application by sending SIGUSR2
            printf("Restart application...\n");        
            kill(getpid(),SIGUSR2);
            break;
            
        // Emulate segmentation fault error
        case 'e':
        {
            int *segv;
            segv = 0; 
            *segv = 1;
        } break;

        // Y - yes
        case 'y':
            switch(khv->wait_y) {
                case Y_QUIT:
                    puts("Quit ...");
                    ksnetEvMgrStop(kev);
                    break;
                default:
                    break;
            }
            khv->wait_y = Y_NONE;
            break;
            
        default:
            if(kev->event_cb != NULL) {                
                kev->event_cb(kev, EV_K_HOTKEY, 
                                (void*)&hotkey, // Pointer to integer hotkey
                                sizeof(hotkey), // Length of integer (hotkey)
                                data); 
            }
            break;
    }
    khv->last_hotkey = hotkey;
    return 0;
    
    #undef kc
    #undef khv
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

/**
 * Initialize hot keys module
 *
 * @param ke
 * @return 
 */
ksnetHotkeysClass *ksnetHotkeysInit(void *ke) {

    ksnetHotkeysClass *kh = malloc(sizeof(ksnetHotkeysClass));
    memset(kh, 0, sizeof(ksnetHotkeysClass));
    
    tcgetattr(0, &kh->initial_settings);
    _keys_non_blocking_start(kh);
    kh->wait_y = Y_NONE;
    kh->peer_m = 0;
    kh->tr_udp_m = 0;
    kh->tr_udp_queues_m = 0;
    kh->pt = NULL;
    kh->mt = NULL;
    kh->pet = NULL;
    kh->put = NULL;
    kh->filter_arr = NULL;
    kh->filter_f = 1;
    kh->ke = ke;

    // Initialize and start STDIN keyboard input watcher
    ev_init (&kh->stdin_w, stdin_cb);
    ev_io_set (&kh->stdin_w, STDIN_FILENO, EV_READ);
    kh->stdin_w.data = ke;
    ev_io_start (((ksnetEvMgrClass*)ke)->ev_loop, &kh->stdin_w);

    // Initialize STDIN idle watchers
    ev_idle_init (&kh->idle_stdin_w, idle_stdin_cb);

    // Start show peer
    if(((ksnetEvMgrClass*)ke)->ksn_cfg.show_peers_f) {
        kh->pet = peer_timer_init( ((ksnetEvMgrClass*)ke)->kc );
        kh->peer_m = 1;
    }
    // Start show TR-UDP statistic
    else
    if(((ksnetEvMgrClass*)ke)->ksn_cfg.show_tr_udp_f) {
        kh->put = tr_udp_timer_init( ((ksnetEvMgrClass*)ke)->kc );
        kh->tr_udp_m = 1;
    }

    return kh;
}

#pragma GCC diagnostic pop

/**
 * De-initialize hot key module
 *
 * @param kh
 */
void ksnetHotkeysDestroy(ksnetHotkeysClass *kh) {

    if(kh != NULL) {
        
        ksnetEvMgrClass *ke = kh->ke;
        
        ev_io_stop (ke->ev_loop, &kh->stdin_w);
        _keys_non_blocking_stop(kh);
        ksnet_stringArrFree(&kh->filter_arr);
        free(kh);
        ke->kh = NULL;
    }
}

/**
 * STDIN (has data) callback
 *
 * param loop
 * @param w
 * @param revents
 */
void stdin_cb (EV_P_ ev_io *w, int revents) {

    #ifdef DEBUG_KSNET
    ksn_puts(((ksnetEvMgrClass *)w->data), MODULE, DEBUG_VV,
            "STDIN (has data) callback");
    #endif

    void *data;

    // Get string
    if(((ksnetEvMgrClass *)w->data)->kh->non_blocking == 0) {

        char *buffer = malloc(KSN_BUFFER_SM_SIZE);
        if(fgets(buffer, KSN_BUFFER_SM_SIZE , stdin) == NULL) {
            buffer[0] = 0;
        }
        data = buffer;
    }
    // Get character
    else {
        int ch = getchar();
        putchar(ch);
        putchar('\n');
        data = teo_malloc(sizeof(int));
        *(int*)data = ch;
    }

    // Create STDIN idle watcher data
    stdin_idle_data *id = teo_malloc(sizeof(stdin_idle_data));
    id->ke = ((ksnetEvMgrClass *)w->data);
    id->data = data;
    id->stdin_w = w;
    ((ksnetEvMgrClass *)w->data)->kh->idle_stdin_w.data = id;

    // Stop this watcher
    ev_io_stop(EV_A_ w);

    // Start STDIN idle watcher
    ev_idle_start(EV_A_ & ((ksnetEvMgrClass *)w->data)->kh->idle_stdin_w);
}

/**
 * STDIN Idle callback: execute STDIN callback in idle time
 *
 * param loop
 * @param w
 * @param revents
 */
void idle_stdin_cb(EV_P_ ev_idle *w, int revents) {

    stdin_idle_data *idata = ((stdin_idle_data *)w->data);
    
    #ifdef DEBUG_KSNET
    ksn_printf(idata->ke, MODULE, DEBUG_VV,
                "STDIN idle (process data) callback (%c)\n", 
                *((int*)idata->data));
    #endif

    // Stop this watcher
    ev_idle_stop(EV_A_ w);

    if (!idata->data) {
        return;
    }

    // Call the hot keys module callback
    if(!hotkeys_cb(idata->ke, idata->data, w)) {
    
        // Start STDIN watcher
        ev_io_start(EV_A_ idata->stdin_w);
    }

    // Free watchers data
    free(idata->data);
    free(w->data);
}

/******************************************************************************/
/* Ping timer functions                                                       */
/*                                                                            */
/******************************************************************************/

/**
 * Ping timer callback
 *
 * param loop
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
    //! \todo: peer_send_cmd_echo(pt->kn, pt->peer_name, (void*)ping_buf, ping_buf_len, 0);
    ksnCommandSendCmdEcho(pt->kn->kco, pt->peer_name, (void*)ping_buf, ping_buf_len);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
/**
 * Initialize ping timer
 *
 * @param kn Pointer to ksnCoreClass
 * @param peer Peer name
 * @return Pointer to ping_timer_data. Should be free after use 
 */
ping_timer_data *ping_timer_init(ksnCoreClass *kn, char *peer) {

    ping_timer_data *pt = teo_malloc(sizeof(ping_timer_data));
    pt->peer_name_len = strlen(peer) + 1;
    pt->peer_name = teo_malloc(pt->peer_name_len);
    strcpy(pt->peer_name, peer);
    pt->kn = kn;

    // Initialize and start main timer watcher, it is a repeated timer
    pt->loop = ((ksnetEvMgrClass*)kn->ke)->ev_loop; /*EV_DEFAULT*/;
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
 * @param ka
 * @param peer_name
 * @param arp_data
 * @param data
 */
int monitor_timer_one_cb(ksnetArpClass *ka, char *peer_name, 
        ksnet_arp_data_ext *arp, void *data) {


    // Reset monitor time
    //ksnet_arp_data *arp_data = ksnetArpGet(ka, peer_name);
    printf("%s%s: %.3f ms %s \n",
            arp->data.monitor_time == 0.0 ? 
                getANSIColor(LIGHTRED) : getANSIColor(LIGHTGREEN),
            peer_name, arp->data.monitor_time * 1000.0,
            getANSIColor(NONE));
    arp->data.monitor_time = 0;
    ksnetArpAdd(ka, peer_name, arp);

    ksnCommandSendCmdEcho(((ksnetEvMgrClass*)(ka->ke))->kc->kco, peer_name,
                          (void*)MONITOR, MONITOR_LEN);

    return 0;
}

#define MONITOR_TIMER_INTERVAL 5.0

/**
 * Monitor timer callback
 *
 * param loop
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
 * @param kn
 * @return
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
monitor_timer_data *monitor_timer_init(ksnCoreClass *kn) {

    monitor_timer_data *mt = teo_malloc(sizeof(monitor_timer_data));
    mt->kn = kn;

    // Initialize and start main timer watcher, it is a repeated timer
    mt->loop = ((ksnetEvMgrClass*)kn->ke)->ev_loop; /*EV_DEFAULT*/;
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
 * param loop
 * @param iw
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
 * param loop
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

    peer_timer_data *pet = teo_malloc(sizeof(peer_timer_data));
    pet->kn = kn;

    // Initialize and start main timer watcher, it is a repeated timer
    pet->loop = ((ksnetEvMgrClass*)kn->ke)->ev_loop; /*EV_DEFAULT*/;
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

/******************************************************************************/
/* Show TR-UDP timer functions                                                 */
/*                                                                            */
/******************************************************************************/

#define TR_UDP_TIMER_INTERVAL 0.150

/**
 * TR-UDP show idle callback
 *
 * param loop
 * @param iw
 * @param revents
 */
void tr_udp_idle_cb(EV_P_ ev_idle *iw, int revents) {

    // Pointer to watcher data
    tr_udp_timer_data *pet = iw->data;

    // Stop idle watcher
    ev_idle_stop(pet->loop, iw);

    // Clear previous shown lines
    //if(pet->num_lines) printf("\033[%dA\r\033[J\n", pet->num_lines + 3);

    // Show TR-UDP
    if(((ksnetEvMgrClass*)pet->kc->ke)->kh->tr_udp_m) {
        pet->num_lines = ksnTRUDPstatShow(pet->kc->ku);
        // Show moving line
        printf("%c", "|/-\\"[pet->num]);
        if( (++(pet->num)) > 3) pet->num = 0;        

        if(pet->num_lines)
        printf("\nPress u to %s continuously refresh\n", STOP);
    }
    else {
        pet->num_lines = ksnTRUDPqueuesShow(pet->kc->ku);
        
        printf("\nPress Q to %s continuously refresh\n", STOP);
    }

    // Start timer watcher
    ev_timer_start(pet->loop, &(pet->tw));
}

/**
 * TR-UDP timer callback
 *
 * param loop
 * @param tw
 * @param revents
 */
void tr_udp_timer_cb(EV_P_ ev_timer *tw, int revents) {

    // Pointer to watcher data
    tr_udp_timer_data *pet = tw->data;

    // Stop peer timer and start peer idle to process timer request in idle time
    ev_timer_stop(pet->loop, tw);
    ev_idle_start(pet->loop, &(pet->iw));
}

/**
 * Initialize tr_udp timer
 *
 * @param kn
 * @return
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
tr_udp_timer_data *tr_udp_timer_init(ksnCoreClass *kn) {

    tr_udp_timer_data *pet = teo_malloc(sizeof(tr_udp_timer_data));
    pet->kc = kn;
    pet->num = 0;

    // Initialize and start main timer watcher, it is a repeated timer
    pet->loop = ((ksnetEvMgrClass*)kn->ke)->ev_loop; /*EV_DEFAULT*/;
    ev_idle_init (&pet->iw, tr_udp_idle_cb);
    pet->iw.data = pet;
    ev_timer_init (&pet->tw, tr_udp_timer_cb, TR_UDP_TIMER_INTERVAL, TR_UDP_TIMER_INTERVAL);
    pet->tw.data = pet;
    ev_timer_start (pet->loop, &pet->tw);

    return pet;
}
#pragma GCC diagnostic pop

/**
 * Stop tr_udp timer
 *
 * @param pet
 */
void tr_udp_timer_stop(tr_udp_timer_data **pet) {

    ev_timer_stop((*pet)->loop, &(*pet)->tw);
    free(*pet);

    *pet = NULL;
}
