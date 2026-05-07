package tests

import (
	"os"
	"service/internal/client"
	"service/internal/models"
	"testing"
	"time"

	"github.com/joho/godotenv"
)

var httpClient = client.NewHTTPClient(20 * time.Second)

func init() {
	if err := godotenv.Load(); err != nil {
		_ = err
	}
}

func getKasperskyProvider() models.Provider {
	apiKey := os.Getenv("KASPERSKY_API_KEY")

	return models.Provider{
		BaseURL: "https://opentip.kaspersky.com/api/v1",
		Headers: map[string]string{
			"x-api-key": apiKey,
		},
		Endpoints: map[string]models.Endpoint{
			"domain": {
				Method: "GET",
				Path:   "/search/domain",
				Query: map[string]string{
					"request": "{domain}",
				},
				Categories: "DomainGeneralInfo.Categories",
			},
			"ip": {
				Method: "GET",
				Path:   "/search/ip",
				Query: map[string]string{
					"request": "{ip}",
				},
				Categories: "IpGeneralInfo.Categories",
			},
		},
	}
}

func TestHttpClientRequest(t *testing.T) {
	provider := getKasperskyProvider()

	tests := []struct {
		name          string
		providerName  string
		provider      models.Provider
		checkValue    string
		endpointName  string
		shouldSucceed bool
	}{
		{
			name:          "Not exist endpoint",
			providerName:  "kaspersky",
			provider:      provider,
			checkValue:    "8.8.8.8",
			endpointName:  "ip_not_exist",
			shouldSucceed: false,
		},
		{
			name:          "Domain check",
			providerName:  "kaspersky",
			provider:      provider,
			checkValue:    "1xbet.com",
			endpointName:  "domain",
			shouldSucceed: true,
		},
		{
			name:          "IP check",
			providerName:  "kaspersky",
			provider:      provider,
			checkValue:    "8.8.8.8",
			endpointName:  "ip",
			shouldSucceed: true,
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			resp, err := httpClient.Request(tt.providerName, tt.provider, tt.checkValue, tt.endpointName)

			if tt.shouldSucceed {
				if err != nil {
					t.Errorf("Expected success but got error: %v", err)
					return
				}
				if resp == nil {
					t.Error("Response is nil")
					return
				}
				err = resp.Body.Close()
				if err != nil {
					t.Errorf("Error to close body: %v", err)
					return
				}

				if resp.StatusCode != 200 {
					t.Errorf("Expected status 200, got %d", resp.StatusCode)
				}
			} else {
				if err == nil {
					t.Error("Expected error but got success")
				}
			}
		})
	}
}
