package models

import (
	"encoding/json"
	"fmt"
	"os"
)

type Category struct {
	ID          int                 `json:"id"`
	Name        string              `json:"name"`
	RiskLevel   int                 `json:"risk_level"`
	Description string              `json:"description"`
	Mappings    map[string][]string `json:"mappings"`
}

type CategoryList struct {
	Categories []Category `json:"categories"`
}

func (cl *CategoryList) GetCategory(id int) (Category, bool) {
	for _, cat := range cl.Categories {
		if cat.ID == id {
			return cat, true
		}
	}
	return Category{}, false
}

func (cl *CategoryList) LoadFromFile(filename string) error {
	data, err := os.ReadFile(filename)
	if err != nil {
		return fmt.Errorf("reading categories: %w", err)
	}
	return json.Unmarshal(data, cl)
}
