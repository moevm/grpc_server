# How to Add Environment Variables to JSON Config

## 1. JSON Syntax

Use this pattern in your `providers.json`:

```json
{
  "providers": {
    "provider_name": {
      "headers": {
        "x-api-key": "${env:YOUR_VARIABLE_NAME}"
      }
    }
  }
}
```

Examples:

```json
{
  "providers": {
    "kaspersky": {
      "headers": {
        "x-api-key": "${env:KASPERSKY_API_KEY}"
      }
    },
    "virustotal": {
      "headers": {
        "x-apikey": "${env:VT_API_KEY}"
      }
    }
  }
}
```

## 2. .env File
Create `.env` file in your project root:

```env
KASPERSKY_API_KEY=KM3wxfz4TVCUx8mpeBiXhg==
VT_API_KEY=your_actual_api_key_here
CUSTOM_TOKEN=token_value_here
```