package models

import (
	"encoding/json"
	"fmt"
	"os"
)

type ProviderList struct {
	Providers map[string]Provider `json:"providers"`
}

type Provider struct {
	BaseURL   string              `json:"base_url"`
	Headers   map[string]string   `json:"headers"`
	Endpoints map[string]Endpoint `json:"endpoints"`
}

type Endpoint struct {
	Method     string            `json:"method,omitempty"`
	Path       string            `json:"path"`
	Query      map[string]string `json:"query,omitempty"`
	Body       map[string]string `json:"body,omitempty"`
	Categories string            `json:"categories"`
}

func (p *Provider) GetEndpoint(name string) (Endpoint, bool) {
	endpoint, ok := p.Endpoints[name]
	return endpoint, ok
}

func interpolateEnvVars(data []byte) []byte {
	re := regexp.MustCompile(`\${env:([^}]+)}`)
	return re.ReplaceAllFunc(data, func(match []byte) []byte {
		varName := string(re.FindSubmatch(match)[1])
		if envValue := os.Getenv(varName); envValue != "" {
			return []byte(envValue)
		}
		return []byte("")
	})
}

func (pl *ProviderList) LoadFromFile(filename string) error {
	data, err := os.ReadFile(filename)
	if err != nil {
		return fmt.Errorf("reading providers: %w", err)
	}

    interpolatedData := interpolateEnvVars(data)

	return json.Unmarshal(data, pl)
}

func (pl *ProviderList) GetProvider(name string) (Provider, bool) {
	provider, ok := pl.Providers[name]
	return provider, ok
}
