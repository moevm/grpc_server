# Generator http/https trafic

Script to run multiple HTTP/HTTPS traffic generation processes.

# Requirements
* Python 3.9+

# Install

```bash
chmod +x generator.sh
```

# Usage

```bash
./generator.sh [COMMAND] [OPTIONS]
```

## Commands

| Command | Description |
| :--- | :--- |
| start | Start processes |
| stop | Stop all process |

## Options

| Option           | Description                      |
|:-----------------|:---------------------------------|
| --count=N        | Number of processes (default: 2) | 
| --help           | show help                        |
| --max_concurrent | max concurrent requests          |

# Examples

```bash
# Start 2 processes (default)
./generator.sh start

# Start 5 processes and max concurrent 3
./generator.sh start --count=5 --max_concurrent=3

# Stop all processes
./generator.sh stop

# Show help
./generator.sh --help
```

# Example config.json

```json
{
    "RPS": 4,
    "root_urls": [
        "http://4chan.org",
        "https://www.reddit.com",
        "https://www.yahoo.com",
        "http://www.cnn.com",
        "http://www.ebay.com",
        "https://wikipedia.org",
        "https://youtube.com",
        "https://github.com",
        "https://medium.com",
        "https://thepiratebay.org"
    ]
}
```

# Logs
All process logs are stored in the logs/ directory:
```text
logs/
├── logs_0.log  # Process #0 log
├── logs_1.log  # Process #1 log
└── logs_2.log  # Process #2 log
```

Example logs_{i}.log:
```text
2026-02-25 18:27:44 - httpx - INFO - HTTP Request: GET http://www.cnn.com "HTTP/1.1 302 Found"
2026-02-25 18:27:45 - httpx - INFO - HTTP Request: GET https://www.reddit.com "HTTP/1.1 403 Blocked"
2026-02-25 18:27:45 - httpx - INFO - HTTP Request: GET https://thepiratebay.org "HTTP/1.1 302 Found"
2026-02-25 18:27:45 - httpx - INFO - HTTP Request: GET https://wikipedia.org "HTTP/1.1 403 Forbidden"
2026-02-25 18:27:45 - httpx - INFO - HTTP Request: GET http://4chan.org "HTTP/1.1 403 Forbidden"
2026-02-25 18:27:45 - httpx - INFO - HTTP Request: GET https://medium.com "HTTP/1.1 403 Forbidden"
2026-02-25 18:27:46 - httpx - INFO - HTTP Request: GET http://www.cnn.com "HTTP/1.1 302 Found"
...
# In end
Url http://www.cnn.com status codes: 302: 8
Url https://thepiratebay.org status codes: 302: 4
Url https://www.reddit.com status codes: 403: 7
Url https://wikipedia.org status codes: 403: 9
Url http://4chan.org status codes: 403: 5
```
