# Prometheus StatsD

This folder contains Prometheus StatsD configuration file and docker start shell
script.

## Description

Teonet send metrics to the Statsd service. Statsd resend this messages to
prometheus. And statsd should be configured appropriately.

### Teonet application send next default metrics

Common metrics parameters:

    network -- tonet network name
    peer -- senders peer name
    remote -- remote peer name
    type -- g (gauge) or c (counter)

Gauge "tick" with value contained counter incremented every 5 sec:

    teonet_g_tick{network="local",peer="teo-vpn-01",type="g"} 9

Counter "total" with value 1 increment total coutner every 5 sec:

    teonet_c_total{network="local",peer="teo-vpn-01",type="c"} 20

Gauge "connected" with value 1 if remote peer connected or 0 if remote peer disconnected:

    teonet_g_connected{network="local",peer="teo-vpn-01",remote="teo-vpn-02",type="g"} 0

Gauge "peer_relay_time" with value of triptime to remote peer in ms * 1000:

    teonet_g_peer_relay_time{network="local",peer="teo-vpn-01",remote="teo-vpn-02",type="g"} 465

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
    void teoMetricCounterf(teoMetricClass *tm, char *name, double value);

    /**
     * Send time(ms) teonet metric
     * 
     * @param tm Pointer to teoMetricClass
     * @param name Metrics name
     * @param value Metrics ms value
     * 
     */
    void teoMetricMs(teoMetricClass *tm, char *name, double value);

    /**
     * Send gauge teonet metrics
     * 
     * @param tm Pointer to teoMetricClass
     * @param name Metrics name
     * @param value Metrics gauge value
     * 
     */
    void teoMetricGauge(teoMetricClass *tm, char *name, int value);
    void teoMetricGaugef(teoMetricClass *tm, char *name, double value);

### Teonet C++ functions to send metrics

    /**
     * Send counter metric
     * 
     * @param name Metric name
     * @param value Metric counter value 
     */
    inline void metricCounter(const std::string &name, int value);
    inline void metricCounter(const std::string &name, double value);

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
    inline void metricGauge(const std::string &name, double value);

### Examples

#### Send counter metric (C language)

    teoMetricCounter(tm, "total", 1);

#### Send conter metric (C language)

    teoMetricGauge(tm, "tick", 25);

#### Send counter metric (C++ language)

    metricCounter("total", 1);

#### Send conter metric (C++ language)

    metricGauge("tick", 25.73);
