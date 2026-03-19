package models

type Group struct {
	name          string
	categoriesIds []int
}

func (s *Group) AddIds(ids ...int) {
	s.categoriesIds = append(s.categoriesIds, ids...)
}

func (s *Group) GetIds() []int {
	return s.categoriesIds
}

func (s *Group) GetName() string {
	return s.name
}
