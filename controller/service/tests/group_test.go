package tests

import (
	"reflect"
	"task/internal/models"
	"testing"
)

func TestGroupAddIds(t *testing.T) {
	tests := []struct {
		name     string
		initial  []int
		add      []int
		expected []int
	}{
		{
			name:     "add single id",
			initial:  []int{1, 2, 3},
			add:      []int{5},
			expected: []int{1, 2, 3, 5},
		},
		{
			name:     "add multiple ids",
			initial:  []int{1, 2},
			add:      []int{3, 4, 5},
			expected: []int{1, 2, 3, 4, 5},
		},
		{
			name:     "add to empty",
			initial:  []int{},
			add:      []int{10, 20},
			expected: []int{10, 20},
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			group := models.Group{
				Name:          "test",
				CategoriesIds: tt.initial,
			}

			group.AddIds(tt.add...)

			if !reflect.DeepEqual(group.CategoriesIds, tt.expected) {
				t.Errorf("got %v, want %v", group.CategoriesIds, tt.expected)
			}
		})
	}
}
