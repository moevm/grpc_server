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
    
    serv, err := service.NewService("categories.json", "providers.json")
    if err != nil {
        log.Fatalf("Failed to create service: %v", err)
    }
    
    group := models.Group{
        Name:          "name",
        CategoriesIds: []int{1, 2, 3},
    }
    
    result, err := serv.Check(&group, domain, "domain")
    if err != nil {
        log.Fatalf("Check failed: %v", err)
    }
    
    if result {
        log.Printf("Domain %s is blocked", domain)
    } else {
        log.Printf("Domain %s is not blocked", domain)
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
    
    serv, err := service.NewService("categories.json", "providers.json")
    if err != nil {
        log.Fatalf("Failed to create service: %v", err)
    }
    
    group := models.Group{
        Name:          "name",
        CategoriesIds: []int{1, 2, 3},
    }
    
    result, err := serv.Check(&group, ip, "ip")
    if err != nil {
        log.Fatalf("Check failed: %v", err)
    }
    
    if result {
        log.Printf("IP %s is blocked", ip)
    } else {
        log.Printf("IP %s is not blocked", ip)
    }
}
```

## Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `categories.json` | `string` | Path to categories configuration file |
| `providers.json` | `string` | Path to providers configuration file |
| `group` | `*models.Group` | Group with name and category IDs |
| `target` | `string` | Domain or IP address to check |
| `checkType` | `string` | Type of check: `"domain"` or `"ip"` |

## Returns

| Value | Type | Description |
|-------|------|-------------|
| `result` | `bool` | `true` if target is blocked, `false` otherwise |
| `err` | `error` | Error if check failed, `nil` otherwise |