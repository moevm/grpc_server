# Metrics in the Project

## How Metrics Work
The project uses Prometheus for metrics collection and Grafana for visualization. Metrics are collected using Pushgateway.

## Metrics Collection Architecture:
1. Workers (C++) collect local counters:
    - `packets_received`- packets received
    - `packets_passed `- packets allowed through
    - `packets_dropped` - packets blocked
2. MetricsCollector periodically (every 5 seconds) pushes metrics to Pushgateway via HTTP POST
3. Prometheus scrapes Pushgateway (every 5 seconds) and stores metrics in a time-series database
4. Grafana connects to Prometheus and displays metrics on dashboards

## Metrics Format:
```text
packets_received{worker_id="worker-1"} 12345
packets_passed{worker_id="worker-1"} 10000
packets_dropped{worker_id="worker-1"} 2345
```

# How to View Metrics

## Via Grafana


**On our test stand**:  
Open in browser: http://localhost:3000

**On the board**:  
Open in browser: http://<BOARD_IP>:3000  
Where <BOARD_IP> is the IP address of the board where Grafana is deployed.  
Default login: `admin / admin`  
Available dashboards:
- Overall statistics across all workers
- Detailed analysis per worker
- Real-time graphs


