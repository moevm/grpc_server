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