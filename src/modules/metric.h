#ifndef METRIC_H
#define METRIC_H

#include <netinet/in.h>

typedef struct teoMetricClass {
    void *ke;
    struct sockaddr_in to;
} teoMetricClass;

teoMetricClass *teoMetricInit(void *ke);
void teoMetricDestroy(teoMetricClass *tm);

// Send counter metric
void teoMetricCounter(teoMetricClass *tm, const char *name, int value);

// Send time(ms) metric
void teoMetricMs(teoMetricClass *tm, const char *name, int value);

// Send gauge metric
void teoMetricGauge(teoMetricClass *tm, const char *name, int value);

#endif /* METRIC_H */
