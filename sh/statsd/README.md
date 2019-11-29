# Prometheus StatsD

This folder contains Prometheus StatsD configuration file and docker start shell 
script.

Teonet application send default metrics:

    teonet_g_tick{instance="xx.xx.xx.xx:xxxx",job="teonet_application",network="local",peer="teo-vpn-2",type="g"}


    teonet_c_total{instance="xx.xx.xx.xx:xxxx",job="teonet_application",network="local",peer="teo-vpn-2",type="c"}

Teonet application paramenters to send metrics:

    --statsd_ip xx.xx.xx.xx
    --statsd_port xxxx

Teonet C functions to send metrics:

    // Send counter metric
    void teoMetricCounter(teoMetricClass *tm, char *name, int value);

    // Send time(ms) metric
    void teoMetricMs(teoMetricClass *tm, char *name, int value);

    // Send gauge metric
    void teoMetricGauge(teoMetricClass *tm, char *name, int value);
