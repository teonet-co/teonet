# Prometheus StatsD

This folder contains Prometheus StatsD configuration file and docker start shell
script.

## Description

Teonet send metrics to the Statsd service. Statsd resend this messages to
prometheus. And statsd should be configured appropriately.

### Teonet application send two default metrics

    teonet_g_tick{instance="xx.xx.xx.xx:xxxx",job="teonet_application",network="local",peer="teo-vpn-01",type="g"}

    teonet_c_total{instance="xx.xx.xx.xx:xxxx",job="teonet_application",network="local",peer="teo-vpn-01",type="c"}

### Teonet application paramenters to send metrics

    --statsd_ip xx.xx.xx.xx
    --statsd_port xxxx

### Teonet C functions to send metrics

    /**
     * Send counter teonet metric
     * 
     * @param tm Pointer to teoMetricClass
     * @param name Metrics name
     * @param value Metrics counter value
     * 
     */
    void teoMetricCounter(teoMetricClass *tm, char *name, int value);

    /**
     * Send time(ms) teonet metric
     * 
     * @param tm Pointer to teoMetricClass
     * @param name Metrics name
     * @param value Metrics ms value
     * 
     */
    void teoMetricMs(teoMetricClass *tm, char *name, int value);

    /**
     * Send gauge teonet metrics
     * 
     * @param tm Pointer to teoMetricClass
     * @param name Metrics name
     * @param value Metrics gauge value
     * 
     */
    void teoMetricGauge(teoMetricClass *tm, char *name, int value);

### Teonet C++ functions to send metrics

    /**
     * Send counter metric
     * 
     * @param name Metric name
     * @param value Metric counter value 
     */
    inline void metricCounter(const std::string &name, int value);

    /**
     * Send time(ms) metric
     * 
     * @param name Metric name
     * @param value Metric ms value 
     */
    inline void metricMs(const std::string &name, int value);

    /**
     * Send gauge metric
     * 
     * @param name Metric name
     * @param value Metric gauge value
     */
    inline void metricGauge(const std::string &name, int value);

### Examples

#### Send counter metric (C language)

    teoMetricCounter(tm, "total", 1);

#### Send conter metric (C language)

    teoMetricGauge(tm, "tick", 25);

#### Send counter metric (C++ language)

    metricCounter("total", 1);

#### Send conter metric (C++ language)

    metricGauge("tick", 25);
