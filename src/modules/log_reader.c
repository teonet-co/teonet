/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   log_reader.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on February 16, 2019, 1:34 AM
 */

#include <fcntl.h>
#include <stdlib.h>

#include "ev_mgr.h"
#include "log_reader.h"

#define MODULE "log_reader"
#define kev ((ksnetEvMgrClass*)(lr->ke))

teoLogReaderClass *teoLogReaderInit(void *ke) {
    teoLogReaderClass *lr = malloc(sizeof(teoLogReaderClass));
    lr->ke = ke;
    #ifdef DEBUG_KSNET
    ksn_puts(kev, MODULE, DEBUG_VV, "have been initialized");
    #endif
    return lr;
}

void teoLogReaderDestroy(teoLogReaderClass *lr) {
    if(lr) {
        ksnetEvMgrClass *ke = lr->ke;
        free(lr);

        #ifdef DEBUG_KSNET
        ksn_puts(ke, MODULE, DEBUG_VV, "have been de-initialized");
        #endif
    }
}

static char *readLine(teoLogReaderClass *lr, int fd) {
    const size_t BUFINC  = 256; 
    size_t bufsize = BUFINC;
    char *retstr = malloc(bufsize);
    retstr[0] = 0;
    char ch;
    int i = 0;
    while(read(fd, &ch, 1) == 1) {
        if(ch == '\r') continue;
        if(ch != '\n') {
            retstr[i++] = ch;
            retstr[i] = 0;
            if(i >= bufsize-1) {
                bufsize += BUFINC;
                retstr = realloc(retstr, bufsize);
            }
        }
        else break;
    }
    return retstr;
}

static void receive_cb(EV_P_ ev_stat *w, int revents) {
    teoLogReaderWatcher *wd = w->data;
    teoLogReaderClass *lr = wd->lr;
    int empty;
    do {
        const char *line = readLine(lr, wd->fd);
        empty = !line[0];
        if(!empty) {
            size_t data_len = strlen(line) + 1;
            kev->event_cb(kev, EV_K_LOG_READER, (void*)line, data_len, wd);
            if(wd->cb) wd->cb((void*)line, data_len, wd);            
        }
        free((void*)line);
    } while(!empty);
}

teoLogReaderWatcher *teoLogReaderOpenCbPP(teoLogReaderClass *lr, 
        const char *name, const char *file_name, teoLogReaderFlag flags, 
        teoLogReaderCallback cb, void *user_data) {

    teoLogReaderWatcher *wd = NULL;
    int fd = open(file_name, O_RDONLY);
    if (fd == -1) ksn_printf(kev, MODULE, ERROR_M,
            "Data file %s not present\n", file_name);
    else {
        wd = malloc(sizeof(teoLogReaderWatcher));
        ev_stat *w = malloc(sizeof(ev_stat));
        wd->file_name = strdup(file_name);
        wd->user_data = user_data;
        wd->name = strdup(name);
        wd->lr = lr;
        wd->fd = fd;
        wd->cb = cb;
        wd->w = w;
        ev_stat_init (w, receive_cb, file_name, 0.01);
        w->data = wd;
        ev_stat_start (kev->ev_loop, w);
        if(flags == READ_FROM_BEGIN) receive_cb(kev->ev_loop, w, 0);
        else if(flags == READ_FROM_END) lseek(fd, 0, SEEK_END);
    }
    return wd;
}

inline teoLogReaderWatcher *teoLogReaderOpenCb(teoLogReaderClass *lr, 
        const char *name, const char *file_name, teoLogReaderFlag flags, 
        teoLogReaderCallback cb) {
    return teoLogReaderOpenCbPP(lr, name, file_name, flags, cb, NULL);
}

inline teoLogReaderWatcher *teoLogReaderOpen(teoLogReaderClass *lr, 
        const char *name, const char *file_name, teoLogReaderFlag flags) {
    return teoLogReaderOpenCbPP(lr, name, file_name, flags, NULL, NULL);
}

int teoLogReaderClose(teoLogReaderWatcher *wd) {
    int retval = -1;
    if(wd) {
        // Stop this watcher and free memory
        ev_stat_stop(((ksnetEvMgrClass*)wd->lr->ke)->ev_loop, wd->w);        
        retval = close(wd->fd);
        if(wd->user_data) free(wd->user_data);
        free((void*)wd->file_name);
        free((void*)wd->name);
        free(wd->w);
        free(wd);
    }
    return retval; 
}