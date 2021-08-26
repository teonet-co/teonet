/**
* \file text-filter.h
* \author max
* Created on Fri Feb 19 00:37:51 2021

* Empty License
* 
*/

#ifndef TEXT_FILTER_H
#define TEXT_FILTER_H


/*
 * This function applies logic expression "match" to string "log"
 * and return 1 on success and 0 on failed.
 *
 * For example:
 * log = "[2021-04-07 11:49:18:442] DEBUG_VV net_core: ksnCoreSendto:(net_core.c:250): send 17 bytes data";
 * match = "(net_core|net_crypt)&(!tr_udp)";
 * result of log_string_match = 1
 *
 * log = "MESSAGE tcp_server: ksnTcpServerStop:(modules/net_tcp.c:246): server fd 8 was stopped";
 * match = "(net_core|net_crypt)&(!tr_udp)";
 * result of log_string_match = 0
 */


int log_string_match(char *log, char *match);

#endif
