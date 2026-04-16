package tests

import (
	"os"
	"testing"
	"time"
	"github.com/moevm/grpc_server/internal/service/client"
	"github.com/moevm/grpc_server/internal/service/models"

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
		name         string
		providerName string
		provider     models.Provider
		checkValue   string
		endpointName string
		expectError  bool
	}{
		{
			name:         "Not exist endpoint",
			providerName: "kaspersky",
			provider:     provider,
			checkValue:   "8.8.8.8",
			endpointName: "ip_not_exist",
			expectError:  true,
		},
		{
			name:         "Domain check",
			providerName: "kaspersky",
			provider:     provider,
			checkValue:   "1xbet.com",
			endpointName: "domain",
			expectError:  false,
		},
		{
			name:         "IP check",
			providerName: "kaspersky",
			provider:     provider,
			checkValue:   "8.8.8.8",
			endpointName: "ip",
			expectError:  false,
		},
		{
			name:         "Empty check value",
			providerName: "kaspersky",
			provider:     provider,
			checkValue:   "",
			endpointName: "ip",
			expectError:  false,
		},
		{
			name:         "Invalid IP format",
			providerName: "kaspersky",
			provider:     provider,
			checkValue:   "999.999.999.999",
			endpointName: "ip",
			expectError:  false,
		},
		{
			name:         "Invalid domain format",
			providerName: "kaspersky",
			provider:     provider,
			checkValue:   "invalid_domain",
			endpointName: "domain",
			expectError:  false,
		},
		{
			name:         "Missing API key",
			providerName: "kaspersky",
			provider: models.Provider{
				BaseURL:   provider.BaseURL,
				Headers:   map[string]string{},
				Endpoints: provider.Endpoints,
			},
			checkValue:   "8.8.8.8",
			endpointName: "ip",
			expectError:  false,
		},
		{
			name:         "Very long domain",
			providerName: "kaspersky",
			provider:     provider,
			checkValue:   string(make([]byte, 300)),
			endpointName: "domain",
			expectError:  false,
		},
		{
			name:         "Special characters input",
			providerName: "kaspersky",
			provider:     provider,
			checkValue:   "!@#$%^&*()",
			endpointName: "domain",
			expectError:  false,
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			resp, err := httpClient.Request(
				tt.providerName,
				tt.provider,
				tt.checkValue,
				tt.endpointName,
			)

			if tt.expectError {
				if err == nil {
					t.Error("Expected error but got success")
				}
				return
			}

			if err != nil {
				t.Errorf("Unexpected error: %v", err)
				return
			}

			if resp == nil {
				t.Error("Response is nil")
				return
			}

			defer resp.Body.Close()

			if resp.StatusCode == 0 {
				t.Error("Invalid status code")
			}
		})
	}
}