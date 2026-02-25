import unittest
import json
import time
import asyncio
import httpx
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).parent.parent))
from main import GenerateTrafficHttpHttps


class TestPythonTrafficGenerator(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.config_path = Path(__file__).parent / "test_config.json"
        with open(cls.config_path) as f:
            cls.test_config = json.load(f)

    def setUp(self):
        self.generator = GenerateTrafficHttpHttps()

    def _make_request(self, url, timeout=5.0):
        async def run_test():
            async with httpx.AsyncClient() as client:
                response = await self.generator._request(url, client, timeout)
                return response

        return asyncio.run(run_test())

    def _assert_request_status(self, url: str, status_code: int):
        response = self._make_request(url)
        self.assertIsNotNone(response)
        self.assertEqual(response.status_code, status_code)
        self.assertEqual(self.generator._stats[url][status_code], 1)

    def test_1_load_config(self):
        result = self.generator.load_config_file(str(self.config_path))

        self.assertTrue(result)

    def test_2_load_config(self):
        result = self.generator.load_config_file("nonexistent.json")
        self.assertFalse(result)

    def test_3_request_200(self):
        self._assert_request_status("https://httpbin.org/status/200", 200)

    def test_4_request_301(self):
        self._assert_request_status("https://httpbin.org/status/301", 301)

    def test_5_request_404(self):
        self._assert_request_status("https://httpbin.org/status/404", 404)

    def test_6_request_500(self):
        self._assert_request_status("https://httpbin.org/status/500", 500)








