import json

import time
import random
import argparse
import logging
import asyncio
import httpx


class GenerateTrafficHttpHttps:

    def __init__(self):
        self._config = {}

    async def _request(self, url: str, client: httpx.AsyncClient, timeout: float = 10.0):
        try:
            response = await client.get(url, timeout=timeout)
            return response
        except Exception:
            logging.error(f"Error request to {url}")
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
            while True:
                url = random.choice(self._config["root_urls"])

                async def make_request(target_url: str):
                    async with semaphore:
                        return await self._request(target_url, client)

                asyncio.create_task(make_request(url))

                await asyncio.sleep(delay)

    def generate(self, max_concurrent_requests: int):

        asyncio.run(self._generate_async(max_concurrent_requests))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--config', metavar='-c', required=True, type=str, help='config file')
    parser.add_argument('--log', metavar='-l', required=True, type=str, help='logging level')
    parser.add_argument('--max_concurrent', metavar='m', required=True, type=int, help='max concurent requests')
    args = parser.parse_args()

    try:
        level = getattr(logging, args.log.upper())
    except Exception as ex:
        print(ex)
        return

    logging.basicConfig(level=level)

    generator = GenerateTrafficHttpHttps()

    if not generator.load_config_file(args.config):
        return

    generator.generate(args.max_concurent)


if __name__ == '__main__':
    main()



