/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   log_reader.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on February 16, 2019, 1:34 AM
 */

#ifndef LOG_READER_H
#define LOG_READER_H

typedef uint32_t teoLogReaderFlag;
typedef enum _teoLogReaderFlag {
  READ_FROM_BEGIN,        // 000 Read log file from the beginning
  READ_FROM_END = 0b001,  // 001 Read new strings only (after current end)
  SKIP_EMPTY    = 0b010   // 010 Skip empty strings
} _teoLogReaderFlag;

typedef struct teoLogReaderClass {

    void *ke; // Pointer to ksnEvMgrClass
    void *buffer; // Pointer to read buffer
    size_t buf_size; // Size of current allocated read buffer size
    void *line_buffer; // Pointer to read line buffer
    size_t line_buf_size; // Size of current allocated read line buffer size

} teoLogReaderClass;

typedef struct teoLogReaderWatcher teoLogReaderWatcher;

typedef void (*teoLogReaderCallback) (void* data, size_t data_length, teoLogReaderWatcher *wd);

typedef struct teoLogReaderWatcher {
    teoLogReaderCallback cb;
    teoLogReaderFlag flags;
    teoLogReaderClass *lr;
    const char *file_name;
    const char *name; 
    void *user_data;
    ev_stat *w;
    int fd;
} teoLogReaderWatcher;

#ifdef	__cplusplus
extern "C" {
#endif

teoLogReaderClass *teoLogReaderInit(void *ke);
void teoLogReaderDestroy(teoLogReaderClass *lr);
teoLogReaderWatcher *teoLogReaderOpen(teoLogReaderClass *lr, const char *name, const char *file_name, teoLogReaderFlag flags);
teoLogReaderWatcher *teoLogReaderOpenCb(teoLogReaderClass *lr, const char *name, const char *file_name, teoLogReaderFlag flags, teoLogReaderCallback cb);
teoLogReaderWatcher *teoLogReaderOpenCbPP(teoLogReaderClass *lr, const char *name, const char *file_name, teoLogReaderFlag flags, teoLogReaderCallback cb, void *user_data);
int teoLogReaderClose(teoLogReaderWatcher *wd);

#ifdef	__cplusplus
}
#endif

#endif /* LOG_READER_H */
