package tests

import (
	"task/internal/models"
	"testing"
)

func TestProviderLoadFromFile(t *testing.T) {
	filename := "providers_test.json"
	list := models.ProviderList{}

	err := list.LoadFromFile(filename)
	if err != nil {
		t.Errorf("Failed to parse file %s: %v", filename, err)
	}
}
