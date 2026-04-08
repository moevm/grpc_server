package tests

import (
	"encoding/json"
	"github.com/moevm/grpc_server/internal/service/parser"
	"os"
	"reflect"
	"testing"
)

func TestJsonParserExtractCategories(t *testing.T) {
	tests := []struct {
		name          string
		initialFile   string
		path          string
		shouldSucceed bool
		expected      []string
	}{
		{
			name:          "correct path",
			initialFile:   "extract_categories_test1.json",
			path:          "response.path.categories",
			shouldSucceed: true,
			expected:      []string{"Gambling", "Gaming"},
		},
		{
			name:          "wrong path",
			initialFile:   "extract_categories_test1.json",
			path:          "response.something.request",
			shouldSucceed: false,
			expected:      nil,
		},
		{
			name:          "empty path",
			initialFile:   "extract_categories_test1.json",
			path:          "",
			shouldSucceed: false,
			expected:      nil,
		},
		{
			name:          "path to string",
			initialFile:   "extract_categories_test2.json",
			path:          "data.category",
			shouldSucceed: true,
			expected:      []string{"Malware"},
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {

			data, err := os.ReadFile(tt.initialFile)
			if err != nil {
				t.Fatalf("Failed to read file %s: %v", tt.initialFile, err)
			}

			var testData map[string]interface{}
			err = json.Unmarshal(data, &testData)
			if err != nil {
				t.Fatalf("Failed to parse JSON: %v", err)
			}

			jsonParser := parser.JSONParser{}
			categories, err := jsonParser.ExtractCategories(testData, tt.path)

			if tt.shouldSucceed && err != nil {
				t.Errorf("Expected success but got error: %v", err)
			}
			if !tt.shouldSucceed && err == nil {
				t.Error("Expected error but got success")
			}

			if tt.shouldSucceed && !reflect.DeepEqual(categories, tt.expected) {
				t.Errorf("got %v, want %v", categories, tt.expected)
			}
		})
	}
}
