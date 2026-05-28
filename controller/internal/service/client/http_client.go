package client

import (
	"bytes"
	"encoding/json"
	"fmt"
	"github.com/moevm/grpc_server/internal/service/models"
	"net/http"
	"strings"
	"time"
)

type HTTPClient struct {
	client *http.Client
}

func NewHTTPClient(timeout time.Duration) *HTTPClient {
	return &HTTPClient{
		client: &http.Client{Timeout: timeout},
	}
}

func (c *HTTPClient) Request(providerName string, provider models.Provider,
	checkValue string, endpointName string) (*http.Response, error) {

	endpoint, exists := provider.GetEndpoint(endpointName)
	if !exists {
		return nil, fmt.Errorf("provider %s does not support %s endpoint",
			providerName, endpointName)
	}

	urlSafe := checkValue
	querySafe := checkValue

	url := provider.BaseURL + endpoint.Path
	placeholder := fmt.Sprintf("{%s}", endpointName)
	url = strings.ReplaceAll(url, placeholder, urlSafe)

	var req *http.Request
	var err error

	if len(endpoint.Body) > 0 {
		bodyData := make(map[string]string)
		for key, value := range endpoint.Body {
			bodyData[key] = strings.ReplaceAll(value, placeholder, querySafe)
		}
		jsonBody, _ := json.Marshal(bodyData)
		req, err = http.NewRequest(endpoint.Method, url, bytes.NewBuffer(jsonBody))
		req.Header.Set("Content-Type", "application/json")
	} else {
		req, err = http.NewRequest(endpoint.Method, url, nil)
	}

	if err != nil {
		return nil, fmt.Errorf("failed to create request: %w", err)
	}

	for key, value := range provider.Headers {
		value = strings.ReplaceAll(value, placeholder, querySafe)
		req.Header.Add(key, value)
	}

	if len(endpoint.Query) > 0 {
		var queryParts []string
		for key, value := range endpoint.Query {
			value = strings.ReplaceAll(value, placeholder, querySafe)
			queryParts = append(queryParts, key+"="+value)
		}
		req.URL.RawQuery = strings.Join(queryParts, "&")
	}

	return c.client.Do(req)
}
