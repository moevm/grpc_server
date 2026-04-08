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

    if err := godotenv.Load(); err != nil {
		log.Println("Warning: .env file not found, using system environment variables")
	}

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

func (s *Service) Check(checkValue string, endpointName string) ([]int, error) {
	type CategoryInfo struct {
		ID       int
		Mappings []string
	}

	providerCategories := make(map[string][]CategoryInfo)

	for _, category := range s.categories.Categories {

		for provider, mappings := range category.Mappings {
			providerCategories[provider] = append(providerCategories[provider], CategoryInfo{
				ID:       category.ID,
				Mappings: mappings,
			})
		}
	}

	foundIDs := make(map[int]bool)

	for providerName, categories := range providerCategories {
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

	    resp.Body.Close()

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

		for _, catInfo := range categories {
			if s.hasIntersection(catInfo.Mappings, actual) {
				foundIDs[catInfo.ID] = true
				log.Printf("Category %d found via %s (matching %v with %v)",
					catInfo.ID, providerName, catInfo.Mappings, actual)
			}
		}
	}

	result := make([]int, 0, len(foundIDs))
	for id := range foundIDs {
		result = append(result, id)
	}

	return result, nil
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

func (s *Service) GetCategory (id int) (string, int) {
	for _, category := range s.categories.Categories {
		if category.ID == id {
            return category.Name, category.RiskLevel
        }
	}
	return "", 0
}
