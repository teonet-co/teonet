/** 
 * \file   tcp_proxy.h
 * \author Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet TCP Proxy module
 *
 * Created on September 8, 2015, 2:00 AM
 */

#ifndef TCP_PROXY_H
#define	TCP_PROXY_H

/**
 * TCP Proxy class data
 */
typedef struct ksnTCPProxyClass {
    
    void *ke;   ///< Pointer to ksnetEvMgrClass
    int fd;     ///< TCP Server fd or 0 if not started
    
} ksnTCPProxyClass;

#ifdef	__cplusplus
extern "C" {
#endif

ksnTCPProxyClass *ksnTCPProxyInit(void *ke);
void ksnTCPProxyDestroy(ksnTCPProxyClass *tp);

#ifdef	__cplusplus
}
#endif

#endif	/* TCP_PROXY_H */
