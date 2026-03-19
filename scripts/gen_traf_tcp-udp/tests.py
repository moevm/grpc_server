import unittest
import asyncio
import json
import time
import tempfile
from pathlib import Path
from unittest.mock import patch
from generate_rand_traf import main
import urllib3


class TestIntegration(unittest.TestCase):
    def setUp(self):
        urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

        self.temp_dir = tempfile.TemporaryDirectory()
        self.sites_file = Path(self.temp_dir.name) / "sites.txt"

        self.logs_dir = Path(f"logs/test")
        self.logs_dir.mkdir(exist_ok=True)

    def tearDown(self):
        self.temp_dir.cleanup()

    def create_sites_file(self, sites):
        with open(self.sites_file, "w") as f:
            for site in sites:
                f.write(f"{site}\n")

    def get_last_log_file(self):
        log_files = list(self.logs_dir.glob("LOG_*.json"))
        if not log_files:
            return None
        return max(log_files, key=lambda p: p.stat().st_mtime)

    def run_main(self, args):
        with patch(
            "sys.argv",
            ["script.py"] + args + ["-f", str(self.sites_file)] + ["-n", "logs/test"] + ["-ncl"] + ["-l" "CRITICAL"],
        ):
            asyncio.run(main())
        return self.get_last_log_file()

    def test_good_sites(self):
        good_sites = [
            "cloudflare.com",
            "4chan.org",
            "www.reddit.com",
            "wikipedia.org",
            "github.com",
        ]

        self.create_sites_file(good_sites)
        log_file = self.run_main(["-q", "20", "-t", "90", "-r", "50"])
        self.assertIsNotNone(log_file)

        with open(log_file, "r") as f:
            data = json.load(f)

        self.assertEqual(len(data["results"]), 20)
        self.assertEqual(data["statistics"]["total"], 20)
        self.assertTrue(
            data["statistics"]["success"] > 12
        )  # this condition is enough for us to confirm the success of the test. Packets can be ignored with a large number of simultaneous scanners.

        for result in data["results"]:
            if result["status"] == "success":
                self.assertIsNotNone(result["ip"])

    def test_bad_sites(self):
        bad_sites = [
            "this-site-does-not-exist-12345.xyz",
            "nonexistent.domain.test",
            "project.local",
            "my.project.local",
            "test-project.local",
            "nonexistent.invalid.test",
            "nonexistent.domain.invalid",
            "invalid.invalid.invalid",
        ]
        self.create_sites_file(bad_sites)

        log_file = self.run_main(
            [
                "-q",
                "20",
            ]
        )
        self.assertIsNotNone(log_file)

        with open(log_file, "r") as f:
            data = json.load(f)

        self.assertEqual(len(data["results"]), 20)

        self.assertEqual(data["statistics"]["total"], 20)
        self.assertEqual(data["statistics"]["success"], 0)
        self.assertEqual(
            data["statistics"]["no_ports_count"] + data["statistics"]["timeout"] + data["statistics"]["error"], 20
        )

        for result in data["results"]:
            self.assertIsNone(result["ip"])
            self.assertIn(result["status"], ["timeout", "error", "no_open_ports"])

    def test_mixed_sites(self):
        mixed_sites = [
            "google.com",
            "github.com",
            "www.yahoo.com",
            "www.cnn.com",
            "www.ebay.com",
            "this-site-does-not-exist-12345.xyz",
            "nonexistent.domain.test",
            "project.local",
            "my.project.local",
        ]

        self.create_sites_file(mixed_sites)

        log_file = self.run_main(["-q", "20", "-t", "80", "-r", "41"])
        self.assertIsNotNone(log_file)

        with open(log_file, "r") as f:
            data = json.load(f)

        self.assertEqual(data["statistics"]["total"], 20)

        self.assertGreater(data["statistics"]["success"], 0)
        self.assertGreater(
            data["statistics"]["timeout"] + data["statistics"]["error"] + data["statistics"]["no_ports_count"], 0
        )

        self.assertEqual(
            data["statistics"]["success"]
            + data["statistics"]["timeout"]
            + data["statistics"]["error"]
            + data["statistics"]["no_ports_count"],
            data["statistics"]["total"],
        )

        for result in data["results"]:
            if result["status"] == "success":
                self.assertIsNotNone(result["ip"])
            else:
                self.assertIsNone(result["ip"])


if __name__ == "__main__":
    unittest.main(verbosity=1)
