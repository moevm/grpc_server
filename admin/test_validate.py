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


if __name__ == "__main__":
    unittest.main()