/**
 * File:   metric.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Metrics module
 *
 * Created on November 28, 2019, 1:38 PM
 * 
 * See Teonet Metrics System description in sh/statsd/README.md 
 * 
 */

#include "metric.h"
#include "ev_mgr.h"

#define MODULE "metrics"

/**
 * Initialize Metrics module
 * 
 * @param kep Pointer to ksnetEvMgrClass
 */ 
teoMetricClass *teoMetricInit(void *kep) {

    ksnetEvMgrClass *ke = (ksnetEvMgrClass *)kep;
    if (ke->ksn_cfg.statsd_ip[0] == 0 || ke->ksn_cfg.statsd_port == 0)
        return NULL;

    ksn_printf(ke, MODULE, MESSAGE, 
        "started, and ready to send metrics to statsd exporter at address: %s:%ld\n",
        ke->ksn_cfg.statsd_ip, ke->ksn_cfg.statsd_port);

    teoMetricClass *tm = malloc(sizeof(teoMetricClass));
    tm->ke = ke;

    memset(&tm->to, 0, sizeof(tm->to));
    tm->to.sin_family = AF_INET;
    tm->to.sin_addr.s_addr = inet_addr(ke->ksn_cfg.statsd_ip);
    tm->to.sin_port = htons(ke->ksn_cfg.statsd_port);

    return tm;
}

/**
 * Destroy Metrics module
 * 
 * @param tm Pointer to teoMetricClass
 * 
 */ 
void teoMetricDestroy(teoMetricClass *tm) {
    if (tm) free(tm);
}

/**
 * Send teonet metrics (modules local function)
 * 
 * @param tm Pointer to teoMetricClass
 * @param name Metrics name
 * @param name Metrics type
 * @param value Metrics value
 * 
 */
static void teoMetric(teoMetricClass *tm, const char *name, const char *type,
                      int value) {
    if (!tm) return;
    ksnetEvMgrClass *ke = (ksnetEvMgrClass *)tm->ke;

    char buffer[256];
    const char *fmt = "teonet.%s.%s.%s.%s:%d|%s";
    int len = snprintf(buffer, 255, fmt, type, ke->ksn_cfg.network,
                       ke->kc->name, name, value, type);

    sendto(ke->kc->fd, buffer, len, 0, (struct sockaddr *)&tm->to,
           sizeof(tm->to));
}

/**
 * Send teonet metrics (modules local function)
 * 
 * @param tm Pointer to teoMetricClass
 * @param name Metrics name
 * @param name Metrics type
 * @param value Metrics value
 * 
 */
static void teoMetricf(teoMetricClass *tm, const char *name, const char *type,
                      double value) {
    if (!tm) return;
    ksnetEvMgrClass *ke = (ksnetEvMgrClass *)tm->ke;

    char buffer[256];
    const char *fmt = "teonet.%s.%s.%s.%s:%f|%s";
    int len = snprintf(buffer, 255, fmt, type, ke->ksn_cfg.network,
                       ke->kc->name, name, value, type);

    sendto(ke->kc->fd, buffer, len, 0, (struct sockaddr *)&tm->to,
           sizeof(tm->to));
}

/**
 * Send counter teonet metric
 * 
 * @param tm Pointer to teoMetricClass
 * @param name Metrics name
 * @param value Metrics counter value
 * 
 */
void teoMetricCounter(teoMetricClass *tm, const char *name, int value) {
    teoMetric(tm, name, "c", value);
}

/**
 * Send counter teonet metric
 * 
 * @param tm Pointer to teoMetricClass
 * @param name Metrics name
 * @param value Metrics counter value
 * 
 */
void teoMetricCounterf(teoMetricClass *tm, const char *name, double value) {
    teoMetricf(tm, name, "c", value);
}

/**
 * Send time(ms) teonet metric
 * 
 * @param tm Pointer to teoMetricClass
 * @param name Metrics name
 * @param value Metrics ms value
 * 
 */
void teoMetricMs(teoMetricClass *tm, const char *name, double value) {
    teoMetricf(tm, name, "ms", value);
}

/**
 * Send gauge teonet metrics
 * 
 * @param tm Pointer to teoMetricClass
 * @param name Metrics name
 * @param value Metrics gauge value
 * 
 */
void teoMetricGauge(teoMetricClass *tm, const char *name, int value) {
    teoMetric(tm, name, "g", value);
}

/**
 * Send gauge teonet metrics
 * 
 * @param tm Pointer to teoMetricClass
 * @param name Metrics name
 * @param value Metrics gauge value
 * 
 */
void teoMetricGaugef(teoMetricClass *tm, const char *name, double value) {
    teoMetricf(tm, name, "g", value);
}

/**
 * Send default teonet metrics
 * 
 * @param tm Pointer to teoMetricClass
 * 
 */
void metric_teonet_count(teoMetricClass *tm) {
    if (!tm) return;

    // Standart metrics
    static uint64_t gauge = 0;
    teoMetricCounter(tm, "total", 1);
    teoMetricGauge(tm, "tick", gauge++);

    // ARP table metrics
    ksnetEvMgrClass *ke = (ksnetEvMgrClass *)tm->ke;
    if(ke->ksn_cfg.statsd_peers_f) {
        ksnetArpMetrics(ke->kc->ka);
    }

    // L0 server metrics
    ksnLNullSStat *kls = ksnLNullStat(ke->kl);
    if(kls) {        
        // Clients counter
        teoMetricGauge(tm, "l0_clients", kls->clients);
        // Packets counters
        teoMetricGauge(tm, "l0_packets_from_client", kls->packets_from_client);
        teoMetricGauge(tm, "l0_packets_to_client", kls->packets_to_client);
        teoMetricGauge(tm, "l0_packets_to_peer", kls->packets_to_peer);
        teoMetricGauge(tm, "l0_packets_from_peer", kls->packets_from_peer);
    }
}
