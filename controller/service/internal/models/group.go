package models

type Group struct {
	Name          string
	CategoriesIds []int
}

func (s *Group) AddIds(ids ...int) {
	s.CategoriesIds = append(s.CategoriesIds, ids...)
}

func (s *Group) GetIds() []int {
	return s.CategoriesIds
}

func (s *Group) GetName() string {
	return s.Name
}
