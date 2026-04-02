package parser

import (
	"fmt"
	"strings"
)

type JSONParser struct{}

func NewJSONParser() *JSONParser {
	return &JSONParser{}
}

func (p *JSONParser) ExtractCategories(data map[string]interface{}, path string) ([]string, error) {
	if path == "" {
		return nil, fmt.Errorf("empty path")
	}

	parts := strings.Split(path, ".")
	current := data

	for i, part := range parts {
		val, exists := current[part]
		if !exists {
			return nil, fmt.Errorf("path part %q not found", part)
		}

		if i == len(parts)-1 {
			return p.convertToStringSlice(val)
		}

		next, ok := val.(map[string]interface{})
		if !ok {
			return nil, fmt.Errorf("path %q is not an object, got %T", part, val)
		}
		current = next
	}

	return nil, fmt.Errorf("path %s not found", path)
}

func (p *JSONParser) convertToStringSlice(val interface{}) ([]string, error) {
	switch v := val.(type) {
	case []interface{}:
		result := make([]string, len(v))
		for i, item := range v {
			result[i] = fmt.Sprint(item)
		}
		return result, nil

	case map[string]interface{}:
		result := make([]string, 0, len(v))
		for _, val := range v {
			result = append(result, fmt.Sprint(val))
		}
		return result, nil

	case string:
		return []string{v}, nil

	default:
		return nil, fmt.Errorf("unexpected type: %T", v)
	}
}
