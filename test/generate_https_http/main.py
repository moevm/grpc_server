import json

import random
import argparse
import logging
import asyncio
import httpx


class GenerateTraficHttpHttps:

    def __init__(self):
        self._config = {}

    async def _request(self, url: str, client: httpx.AsyncClient):
        try:
            response = await client.get(url, timeout=10.0)
            return response
        except Exception as ex:
            logging.error(f"Error request to {url}: {ex}")
            return None

    def load_config_file(self, file_path: str):
        with open(file_path, 'r') as config_file:
            config = json.load(config_file)
            self.set_config(config)

    def set_config(self, config):
        self._config = config

    def set_option(self, option, value):
        self._config[option] = value

    async def _generate_async(self):

        async with httpx.AsyncClient() as client:
            while True:
                url = random.choice(self._config["root_urls"])

                response = await self._request(url, client)

                if response:
                    logging.info(f"Request to {url} status {response.status_code}")

                delay = 1 / self._config["RPS"]

                await asyncio.sleep(delay)

    def generate(self):

        asyncio.run(self._generate_async())


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--config', metavar='-c', required=True, type=str, help='config file')
    parser.add_argument('--log', metavar='-l', required=True, type=str, help='logging level')
    args = parser.parse_args()

    level = getattr(logging, args.log.upper())
    logging.basicConfig(level=level)

    generator = GenerateTraficHttpHttps()
    generator.load_config_file(args.config)

    generator.generate()



