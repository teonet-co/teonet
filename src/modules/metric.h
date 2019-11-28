#ifndef METRIC_H
#define METRIC_H

#include <netinet/in.h>

typedef struct teoMetricClass {
    void *ke;
    struct sockaddr_in to;
} teoMetricClass;

teoMetricClass *teoMetricInit(void *ke);
void teoMetricKill(teoMetricClass *tm);

void metric_teonet_count(teoMetricClass *tm);

#endif /* METRIC_H */