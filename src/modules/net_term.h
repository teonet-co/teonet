/**
 * File:   net_term.h
 * Author: Kirill Scherba
 *
 * Created on May 8, 2015, 10:52 PM
 *
 *
 * KSNet net terminal module
 *
 */

#ifndef NET_TERM_H
#define	NET_TERM_H

struct my_context {
  int value;
  char* message;
};

/**
 * KSNet terminal module class data definition
 */
typedef struct ksnTermClass {

    void *ke; // Pointer to ksnTermClass

    struct my_context myctx;
    struct cli_def *cli;

} ksnTermClass;


#ifdef	__cplusplus
extern "C" {
#endif

ksnTermClass *ksnTermInit(void *ke);
void ksnTermDestroy(ksnTermClass *kte);

#ifdef	__cplusplus
}
#endif

#endif	/* NET_TERM_H */
