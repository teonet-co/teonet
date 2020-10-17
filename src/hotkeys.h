/**
 * File:   hotkeys.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on April 16, 2015, 10:43 PM
 *
 * Hot keys monitor module
 *
 */

#ifndef HOTKEYS_H
#define	HOTKEYS_H

#include "config/config.h"

#include <ev.h>
#ifndef HAVE_MINGW
#include <sys/termios.h>
#endif

#include "config/conf.h"
#include "net_core.h"

#include "utils/string_arr.h"
/**
 * Ping timer data
 */
typedef struct ping_timer_data {

    char *peer_name;
    size_t peer_name_len;
    struct ev_loop *loop;
    ksnCoreClass *kn;
    ev_timer w;

} ping_timer_data;

/**
 * Monitor timer data
 */
typedef struct monitor_timer_data {

    struct ev_loop *loop;
    ksnCoreClass *kn;
    ev_timer w;

} monitor_timer_data;

/**
 * TR UDP timer data
 */
typedef struct tr_udp_timer_data {

    struct ev_loop *loop;
    ksnCoreClass *kc;
    int num_lines;
    ev_timer tw;
    ev_idle iw;
    size_t num;

} tr_udp_timer_data;


/**
 * Peer timer data
 */
typedef struct peer_timer_data {

    struct ev_loop *loop;
    ksnCoreClass *kn;
    int num_lines;
    ev_timer tw;
    ev_idle iw;

} peer_timer_data;


/**
 * Hot keys class data
 */
typedef struct ksnetHotkeysClass  {

    void *ke;
    int non_blocking; ///< Non blocking mode: 1 - non-blocking
    #ifdef HAVE_MINGW
    int initial_settings, new_settings; ///< Keybord settings
    #else
    struct termios initial_settings, new_settings; ///< Keybord settings
    #endif
    int wait_y; ///< Wait y (yes) mode
    int peer_m; ///< Show peer mode: 0 - single; 1 - continously
    int tr_udp_m; ///< Show tr-udp mode: 0 - single; 1 - continously
    int tr_udp_queues_m; ///< Show tr-udp queues mode: 0 - single; 1 - continously
    int last_hotkey; ///< Last hotkey
    int str_number; ///< Nuber of current string
    char str[4][KSN_BUFFER_SM_SIZE]; ///< Strings

    unsigned filter_f;

    ksnet_stringArr filter_arr; ///< filter for logs
        
    ping_timer_data *pt; ///< Hotkey Pinger timer data
    monitor_timer_data *mt; ///< Hotkey Monitor timer data
    peer_timer_data *pet; ///< Hotkey Peer timer data
    tr_udp_timer_data *put; ///< Hotkey Peer timer data
    
    ev_io stdin_w; ///< STDIN watcher
    ev_idle idle_stdin_w; ///< Idle STDIN watcher

} ksnetHotkeysClass;

extern const char *PING, *TRACE, *MONITOR, *TRIPTIME;

#define PING_LEN 5
#define TRACE_LEN 6
#define MONITOR_LEN 8
#define TRIPTIME_LEN 9

#ifdef	__cplusplus
extern "C" {
#endif

ksnetHotkeysClass *ksnetHotkeysInit(void *ke);
void ksnetHotkeysDestroy(ksnetHotkeysClass *kh);

unsigned char teoFilterFlagCheck(void *ke);
unsigned char teoLogCheck(void *ke, void *log);

void _keys_non_blocking_start(ksnetHotkeysClass *kh);
/**
 * Stop keyboard non-blocking mode
 *
 * @param kh
 */
#define _keys_non_blocking_stop(kh) \
    tcsetattr(0, TCSANOW, &kh->initial_settings); \
    kh->non_blocking = 0


#ifdef	__cplusplus
}
#endif

#endif	/* HOTKEYS_H */
