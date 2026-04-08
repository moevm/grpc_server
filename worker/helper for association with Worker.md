REQUESTED_CLASSIFICATION структура для передачи от контроллера к воркеру:
struct requested_classification {
    char get_categories[MAX_CATEGORIES][CATEGORY_MAX_LEN] - политика
    int get_trust_level                              - уровень доверия к сайту
}


Структура для хранения категории с минимальным уровнем доверия для этой категории  
struct trust_categories_with_lvl {
    char locked_by_trust_category[CATEGORY_MAX_LEN];
    int trust_lvl;
}


у нас есть переменные, которые получаем при инициализации воркера и заносим в структуру (периодически обновляем)
struct BASE_POLICY {
    char locked_categories[MAX_CATEGORIES][CATEGORY_MAX_LEN];
    struct trust_categories_with_lvl categories_with_lvl[MAX_CATEGORIES_BY_TRUST_LVL];
    char block_domains[MAX_DOMAINS][MAX_LEN_DOMEIN];
    char allow_domains[MAX_DOMAINS][MAX_LEN_DOMEIN];
    int min_trust_level;
}
