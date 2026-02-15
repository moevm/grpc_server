import json

import requests
import time
import random
import argparse
import logging


class GenerateTraficHttpHttps:

    def __init__(self):
        self._config = {}
        self._links = []

    def _request(self, url: str):
        return requests.get(url, timeout=(3, 5))

    def load_config_file(self, file_path: str):
        with open(file_path, 'r') as config_file:
            config = json.load(config_file)
            self.set_config(config)

    def set_config(self, config):
        self._config = config

    def set_option(self, option, value):
        self._config[option] = value

    def generate(self):

        while True:
            url = random.choice(self._config["root_urls"])
            try:
                response = self._request(url)
                logging.info("Request to {} status: {}".format(url, response.status_code))
            except requests.exceptions.RequestException:
                logging.warning("Error connecting to root url: {}".format(url))

            time.sleep(random.randrange(self._config["min_sleep"], self._config["max_sleep"]))

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--config', metavar='-c', required=True, type=str, help='config file')
    parser.add_argument('--log', metavar='-l', required=True, type=str, help = 'logging level')
    args = parser.parse_args()

    level = getattr(logging, args.log.upper())
    logging.basicConfig(level=level)

    generator = GenerateTraficHttpHttps()
    generator.load_config_file(args.config)

    generator.generate()



