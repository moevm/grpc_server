package service

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"service/internal/client"
	"service/internal/models"
	"service/internal/parser"
	"time"
)

type Service struct {
	providers  *models.ProviderList
	categories *models.CategoryList
	httpClient *client.HTTPClient
	jsonParser *parser.JSONParser
}

func NewService(categoryFile, providerFile string) (*Service, error) {
	categories := &models.CategoryList{}
	if err := categories.LoadFromFile(categoryFile); err != nil {
		return nil, fmt.Errorf("loading categories: %w", err)
	}

	providers := &models.ProviderList{}
	if err := providers.LoadFromFile(providerFile); err != nil {
		return nil, fmt.Errorf("loading providers: %w", err)
	}

	return &Service{
		providers:  providers,
		categories: categories,
		httpClient: client.NewHTTPClient(20 * time.Second),
		jsonParser: parser.NewJSONParser(),
	}, nil
}

func (s *Service) Check(group *models.Group, checkValue string, endpointName string) (bool, error) {
	expectedMappings := make(map[string][]string)

	for _, id := range group.GetIds() {
		category, ok := s.categories.GetCategory(id)
		if !ok {
			return false, fmt.Errorf("category %d not found", id)
		}

		for provider, values := range category.Mappings {
			expectedMappings[provider] = append(expectedMappings[provider], values...)
		}
	}

	for providerName, expected := range expectedMappings {
		provider, ok := s.providers.GetProvider(providerName)
		if !ok {
			log.Printf("Warning: Provider %s not found", providerName)
			continue
		}

		resp, err := s.httpClient.Request(providerName, provider, checkValue, endpointName)
		if err != nil {
			log.Printf("Error requesting %s: %v", providerName, err)
			continue
		}
		err = resp.Body.Close()
		if err != nil {
			log.Printf("Error to close body: %v", err)
			continue
		}

		if resp.StatusCode != http.StatusOK {
			log.Printf("Provider %s returned %s", providerName, resp.Status)
			continue
		}

		var data map[string]interface{}
		if err := json.NewDecoder(resp.Body).Decode(&data); err != nil {
			log.Printf("Error parsing response from %s: %v", providerName, err)
			continue
		}

		categoryPath := provider.Endpoints[endpointName].Categories
		actual, err := s.jsonParser.ExtractCategories(data, categoryPath)
		if err != nil {
			log.Printf("Error extracting categories from %s: %v", providerName, err)
			continue
		}

		if s.hasIntersection(expected, actual) {
			return true, nil
		}

	}

	return false, nil
}

func (s *Service) hasIntersection(a, b []string) bool {
	lookup := make(map[string]bool, len(b))
	for _, v := range b {
		lookup[v] = true
	}
	for _, v := range a {
		if lookup[v] {
			return true
		}
	}
	return false
}
