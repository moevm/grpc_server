package tests

import (
	"task/internal/models"
	"testing"
)

var testCategories = models.CategoryList{
	Categories: []models.Category{
		{
			ID:          1,
			Name:        "Phishing",
			RiskLevel:   5,
			Description: "Фишинговые сайты",
			Mappings: map[string][]string{
				"kaspersky":  {"Phishing", "Fraud"},
				"virustotal": {"phishing", "scam"},
				"skydns":     {"3"},
			},
		},
		{
			ID:          2,
			Name:        "Malware",
			RiskLevel:   5,
			Description: "Вредоносное ПО",
			Mappings: map[string][]string{
				"kaspersky":  {"Malware", "Virus"},
				"virustotal": {"malware"},
				"skydns":     {"3"},
			},
		},
	},
}

func TestCategoryLoadFromFile(t *testing.T) {
	filename := "categories_test.json"
	list := models.CategoryList{}

	err := list.LoadFromFile(filename)
	if err != nil {
		t.Errorf("Failed to parse file %s: %v", filename, err)
	}
}

func TestCategoryGet(t *testing.T) {
	tests := []struct {
		name     string
		id       int
		wantOk   bool
		wantName string
	}{
		{"existing category", 1, true, "Phishing"},
		{"existing category", 2, true, "Malware"},
		{"non-existent category", 3, false, ""},
		{"non-existent category", 999, false, ""},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			cat, ok := testCategories.GetCategory(tt.id)

			if ok != tt.wantOk {
				t.Errorf("GetCategory(%d) ok = %v, want %v", tt.id, ok, tt.wantOk)
			}

			if ok && cat.Name != tt.wantName {
				t.Errorf("GetCategory(%d) name = %s, want %s", tt.id, cat.Name, tt.wantName)
			}
		})
	}
}

func TestCategoryGet_CheckMappings(t *testing.T) {
	cat, ok := testCategories.GetCategory(1)
	if !ok {
		t.Fatal("Category 1 not found")
	}

	expectedMappings := map[string]int{
		"kaspersky":  2,
		"virustotal": 2,
		"skydns":     1,
	}

	for provider, expectedLen := range expectedMappings {
		if len(cat.Mappings[provider]) != expectedLen {
			t.Errorf("Provider %s: got %d values, want %d",
				provider, len(cat.Mappings[provider]), expectedLen)
		}
	}
}
