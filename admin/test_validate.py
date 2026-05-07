import unittest
import toml
from admin import validate_config


class TestValidateConfig(unittest.TestCase):

    def test_valid_config(self):
        config = {
            "global": {
                "rules": {
                    "block_categories": ["MALWARE"],
                    "block_domains": ["youtube.com"],
                    "allow_domains": ["github.com"],
                    "min_trust_level": 5,
                    "block_by_trust": {"SOCIAL": 6}
                }
            }
        }
        content = toml.dumps(config).encode('utf-8')
        errors = validate_config(content)
        self.assertEqual(errors, [])

    def test_valid_config_with_lists_of_2_elements(self):
        config = {
            "global": {
                "rules": {
                    "block_categories": ["MALWARE", "SOCIAL"],
                    "block_domains": ["youtube.com", "tiktok.com"],
                    "allow_domains": ["github.com", "stackoverflow.com"],
                    "min_trust_level": 5,
                    "block_by_trust": {"ENTERTAINMENT": 6, "NEWS": 4}
                }
            },
            "filters": {
                "filter_1": {
                    "rules": {
                        "block_categories": ["SOCIAL", "ENTERTAINMENT"],
                        "block_domains": ["instagram.com"],
                        "allow_domains": ["vk.com"],
                        "min_trust_level": 0,
                        "block_by_trust": {"SOCIAL": 8, "ENTERTAINMENT": 7}
                    }
                },
                "filter_2": {
                    "rules": {
                        "block_categories": ["MALWARE"],
                        "allow_domains": ["github.com", "gitlab.com"],
                        "min_trust_level": 3
                    }
                }
            }
        }
        content = toml.dumps(config).encode('utf-8')
        errors = validate_config(content)
        self.assertEqual(errors, [])

    def test_valid_config_with_lists_of_3_elements(self):
        config = {
            "global": {
                "rules": {
                    "block_categories": ["MALWARE", "SOCIAL", "SPYWARE"],
                    "block_domains": ["youtube.com", "tiktok.com", "facebook.com"],
                    "allow_domains": ["github.com", "stackoverflow.com", "gitlab.com"],
                    "min_trust_level": 5,
                    "block_by_trust": {"ENTERTAINMENT": 6, "NEWS": 4, "SOCIAL": 5}
                }
            },
            "filters": {
                "custom_filter": {
                    "rules": {
                        "block_categories": ["SOCIAL", "ENTERTAINMENT", "ADULT"],
                        "block_domains": ["instagram.com", "twitter.com", "reddit.com"],
                        "allow_domains": ["vk.com", "ok.ru", "mail.ru"],
                        "min_trust_level": 2,
                        "block_by_trust": {"SOCIAL": 8, "ENTERTAINMENT": 7, "ADULT": 9}
                    }
                }
            }
        }
        content = toml.dumps(config).encode('utf-8')
        errors = validate_config(content)
        self.assertEqual(errors, [])

    def test_empty_config(self):
        errors = validate_config(b"")
        self.assertIn("Config file is empty", errors)

    def test_invalid_toml(self):
        errors = validate_config(b"not valid toml {")
        self.assertTrue(any("Invalid TOML" in e for e in errors))

    def test_missing_global(self):
        config = {"filters": {}}
        content = toml.dumps(config).encode('utf-8')
        errors = validate_config(content)
        self.assertIn("Missing required section: 'global'", errors)

    def test_block_categories_not_list(self):
        config = {
            "global": {
                "rules": {
                    "block_categories": "not a list"
                }
            }
        }
        content = toml.dumps(config).encode('utf-8')
        errors = validate_config(content)
        self.assertIn("global.rules.block_categories must be a list", errors)

    def test_min_trust_level_negative(self):
        config = {
            "global": {
                "rules": {
                    "min_trust_level": -5
                }
            }
        }
        content = toml.dumps(config).encode('utf-8')
        errors = validate_config(content)
        self.assertIn("global.rules.min_trust_level must be >= 0", errors)

    def test_block_domains_with_3_elements_in_filter(self):
        config = {
            "global": {
                "rules": {
                    "block_categories": ["MALWARE"],
                    "min_trust_level": 1
                }
            },
            "filters": {
                "test_filter": {
                    "rules": {
                        "block_domains": ["evil.com", "bad.org", "malware.net"],
                        "min_trust_level": 3
                    }
                }
            }
        }
        content = toml.dumps(config).encode('utf-8')
        errors = validate_config(content)
        self.assertEqual(errors, [])

    def test_allow_domains_with_various_list_lengths(self):
        config = {
            "global": {
                "rules": {
                    "allow_domains": ["trusted.com"],
                    "min_trust_level": 5
                }
            },
            "filters": {
                "filter_a": {
                    "rules": {
                        "allow_domains": ["site1.com", "site2.com"],
                        "min_trust_level": 2
                    }
                },
                "filter_b": {
                    "rules": {
                        "allow_domains": ["a.com", "b.com", "c.com"],
                        "min_trust_level": 1
                    }
                }
            }
        }
        content = toml.dumps(config).encode('utf-8')
        errors = validate_config(content)
        self.assertEqual(errors, [])


if __name__ == "__main__":
    unittest.main()