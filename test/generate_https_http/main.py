import json
import time
import random
import argparse
import logging
import asyncio
import httpx
import signal
import sys


class GenerateTrafficHttpHttps:

    def __init__(self):
        self._config = {}
        self._stats = {}
        self._is_running = True

        signal.signal(signal.SIGINT, self._signal_handler)

    def _signal_handler(self, sig, frame):
        self._is_running = False

    def _inc_stats_field(self, url: str, field: int) -> None:
        if not self._stats.get(url):
            self._stats[url] = {field: 1}
        elif not self._stats[url].get(field):
            self._stats[url][field] = 1
        else:
            self._stats[url][field] += 1

    async def _request(self, url: str, client: httpx.AsyncClient, timeout: float = 10.0):
        try:
            response = await client.get(url, timeout=timeout)
            self._inc_stats_field(url, response.status_code)
            return response
        except httpx.ConnectError:
            logging.error(f"Connect error for {url}")
            self._inc_stats_field(url, "Connect error")
        except httpx.ConnectTimeout:
            logging.error(f"Connect timeout for {url}")
            self._inc_stats_field(f"Connect timeout")
        except Exception as ex:
            logging.error(ex)

        return None

    def load_config_file(self, file_path: str) -> bool:

        try:
            with open(file_path, 'r') as config_file:
                config = json.load(config_file)
                self._config = config

        except Exception as ex:
            logging.error(ex)
            return False

        return True

    async def _generate_async(self, max_concurrent_requests: int):

        semaphore = asyncio.Semaphore(max_concurrent_requests)

        delay = 1.0 / self._config["RPS"]

        async with httpx.AsyncClient() as client:
            while self._is_running:
                url = random.choice(self._config["root_urls"])

                async def make_request(target_url: str):
                    async with semaphore:
                        return await self._request(target_url, client)

                asyncio.create_task(make_request(url))

                await asyncio.sleep(delay)

        self._print_stats()

    def _print_stats(self):
        for url in self._stats:
            print(f"Url {url} status codes:",
                  ' '.join(f"{field}: {count}" for field, count in self._stats[url].items()))

    def generate(self, max_concurrent_requests: int):

        asyncio.run(self._generate_async(max_concurrent_requests))

    def check_correct_config(self):
        if not self._config.get("root_urls"):
            logging.error("There is no root_urls field in the configuration file.")
            return False
        if not self._config.get("RPS"):
            logging.error("There is no RPS field in the configuration file.")
            return False

        return True


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--config', metavar='-c', required=True, type=str, help='config file')
    parser.add_argument('--log', metavar='-l', required=True, type=str, help='logging level')
    parser.add_argument('--max_concurent', metavar='m', required=True, type=int, help='max concurent requests')
    args = parser.parse_args()

    try:
        level = getattr(logging, args.log.upper())
    except Exception as ex:
        logging.error(ex)
        return

    logging.basicConfig(
        level=level,
        format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
        datefmt='%Y-%m-%d %H:%M:%S'
    )

    generator = GenerateTrafficHttpHttps()

    if not generator.load_config_file(args.config):
        return

    if not generator.check_correct_config():
        return

    generator.generate(args.max_concurent)


if __name__ == '__main__':
    main()



