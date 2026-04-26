package tests

import (
	"reflect"
	"testing"

	"github.com/moevm/grpc_server/internal/service/service"
)

func TestService(t *testing.T) {
	serv, err := service.NewService("../config/categories.json", "../config/providers.json")

	if err != nil {
		t.Error(err)
	}

	tests := []struct {
		name         string
		endpointName string
		checkValue   string
		expected     []int
	}{
		{
			name:         "Check gambling",
			endpointName: "domain",
			checkValue:   "1xbet.com",
			expected:     []int{2},
		},
		{
			name:         "Check social media",
			endpointName: "domain",
			checkValue:   "vk.com",
			expected:     []int{7},
		}, {
			name:         "Check non-existent domain",
			endpointName: "domain",
			checkValue:   "this-domain-does-not-exist.example",
			expected:     []int{},
		}, {
			name:         "Check empty domain",
			endpointName: "domain",
			checkValue:   "",
			expected:     []int{},
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			res, err := serv.Check(tt.checkValue, tt.endpointName)
			if err != nil {
				t.Error(err)
			}

			if !reflect.DeepEqual(tt.expected, res) {
				t.Errorf("got %v expected %v", res, tt.expected)
			}
		})
	}
}
