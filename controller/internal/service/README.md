# Service for domain/IP check

## Overview

This service provides functionality to check domains and IP addresses against categories and providers.

## Usage

### Check Domain

```go
package main

import (
    "log"
    "task/internal/models"
    "task/internal/service"
)

func main() {
    domain := "1xbet.com"
    
    serv, err := service.NewService("config/categories.json", "config/providers.json")
    if err != nil {
        log.Fatalf("Failed to create service: %v", err)
    }
   
    result, err := serv.Check(domain, "domain")
    if err != nil {
        log.Fatalf("Check failed: %v", err)
    }
    
}
```

### Check IP


```go
package main

import (
    "log"
    "task/internal/models"
    "task/internal/service"
)

func main() {
    ip := "8.8.8.8"
    
    serv, err := service.NewService("config/categories.json", "config/providers.json")
    if err != nil {
        log.Fatalf("Failed to create service: %v", err)
    }
    
    result, err := serv.Check(ip, "ip")
    if err != nil {
        log.Fatalf("Check failed: %v", err)
    }
}
```

## Parameters

| Parameter         | Type | Description |
|-------------------|------|-------------|
| `categories.json` | `string` | Path to categories configuration file |
| `providers.json`  | `string` | Path to providers configuration file |
| `target`          | `string` | Domain or IP address to check |
| `checkType`       | `string` | Type of check: `"domain"` or `"ip"` |

## Returns

Returns a list of category ids that were found in the request.
