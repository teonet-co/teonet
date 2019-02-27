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

#define BUFINC 256

teoLogReaderClass *teoLogReaderInit(void *ke) {
    teoLogReaderClass *lr = malloc(sizeof(teoLogReaderClass));
    lr->buf_size = 0;
    lr->buffer = NULL;
    lr->line_buf_size = 0;
    lr->line_buffer = NULL;
    lr->ke = ke;
    #ifdef DEBUG_KSNET
    ksn_puts(kev, MODULE, DEBUG_VV, "have been initialized");
    #endif
    return lr;
}

void teoLogReaderDestroy(teoLogReaderClass *lr) {
    if(lr) {
        ksnetEvMgrClass *ke = lr->ke;
        if(lr->line_buffer) free(lr->line_buffer);
        if(lr->buffer) free(lr->buffer);
        free(lr);

        #ifdef DEBUG_KSNET
        ksn_puts(ke, MODULE, DEBUG_VV, "have been de-initialized");
        #endif
    }
}

static void *readBuffer(teoLogReaderClass *lr, int fd, size_t *data_length) {

    size_t ptr = 0, rd;
    for(;;) {
        // Allocate and reallocate buffer
        if(!lr->buf_size) {
            lr->buf_size = BUFINC * 2;
            lr->buffer = malloc(lr->buf_size);
        }
        else if(lr->buf_size - ptr < BUFINC) {
            lr->buf_size += BUFINC * 2;
            lr->buffer = realloc(lr->buffer, lr->buf_size);
        }
        // Read from file
        rd = read(fd, lr->buffer + ptr, lr->buf_size - ptr);
        if(rd > 0) ptr += rd;
        else break;
    }
    if(data_length) *data_length = ptr;
    return lr->buffer;
}

static char *readLineBuffer(teoLogReaderClass *lr, size_t *ptr, size_t data_length) {
    // Allocate line buffer
    if(!lr->line_buffer) {
        lr->line_buf_size = BUFINC;
        lr->line_buffer = malloc(lr->line_buf_size);
    }
    // Split input buffer by end of line  
    #define retstr ((char*)lr->line_buffer)
    retstr[0] = 0;
    for(int i = 0; *ptr < data_length; ) {
        char ch = ((char*)lr->buffer)[(*ptr)++];
        if(ch == '\r') continue;
        if(ch != '\n') {
            retstr[i++] = ch;
            retstr[i] = 0;
            // Reallocate line buffer
            if(i >= lr->line_buf_size-1) {
                lr->line_buf_size += BUFINC;
                lr->line_buffer = realloc(lr->line_buffer, lr->line_buf_size);
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
    size_t data_length, ptr = 0;
    readBuffer(lr, wd->fd, &data_length);
    if(data_length) do {
        const char *line = readLineBuffer(lr, &ptr, data_length);
        empty = !line[0];
        if(!((wd->flags & SKIP_EMPTY) && empty)) {
            size_t data_len = strlen(line) + 1;
            kev->event_cb(kev, EV_K_LOG_READER, (void*)line, data_len, wd);
            if(wd->cb) wd->cb((void*)line, data_len, wd);
        }
    } while(ptr < data_length);
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
        wd->flags = flags;
        wd->lr = lr;
        wd->fd = fd;
        wd->cb = cb;
        wd->w = w;
        ev_stat_init (w, receive_cb, file_name, 0.01);
        w->data = wd;
        ev_stat_start (kev->ev_loop, w);
        if(flags & READ_FROM_END) lseek(fd, 0, SEEK_END);
        else receive_cb(kev->ev_loop, w, 0);
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
        free((void*)wd->file_name);
        free((void*)wd->name);
        free(wd->w);
        free(wd);
    }
    return retval;
}