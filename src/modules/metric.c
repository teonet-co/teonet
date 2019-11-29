#include "metric.h"
#include "ev_mgr.h"

teoMetricClass *teoMetricInit(void *kep) {

    ksnetEvMgrClass *ke = (ksnetEvMgrClass *)kep;
    if (ke->ksn_cfg.statsd_ip[0] == 0 || ke->ksn_cfg.statsd_port == 0)
        return NULL;

    printf("Send metrics to statsd exporter at address: %s:%ld\n",
           ke->ksn_cfg.statsd_ip, ke->ksn_cfg.statsd_port);

    teoMetricClass *tm = malloc(sizeof(teoMetricClass));
    tm->ke = ke;

    memset(&tm->to, 0, sizeof(tm->to));
    tm->to.sin_family = AF_INET;
    tm->to.sin_addr.s_addr = inet_addr(ke->ksn_cfg.statsd_ip);
    tm->to.sin_port = htons(ke->ksn_cfg.statsd_port);

    return tm;
}

void teoMetricKill(teoMetricClass *tm) {
    if (tm) free(tm);
}

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

// Send counter metric
void teoMetricCounter(teoMetricClass *tm, const char *name, int value) {
    teoMetric(tm, name, "c", value);
}

// Send time(ms) metric
void teoMetricMs(teoMetricClass *tm, const char *name, int value) {
    teoMetric(tm, name, "ms", value);
}

// Send gauge metric
void teoMetricGauge(teoMetricClass *tm, const char *name, int value) {
    teoMetric(tm, name, "g", value);
}

// Send default teonet metrics
void metric_teonet_count(teoMetricClass *tm) {
    if (!tm) return;
    static uint64_t gauge = 0;
    teoMetricCounter(tm, "total", 1);
    teoMetricGauge(tm, "tick", gauge++);
}
