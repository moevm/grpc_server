import argparse
import grpc
from dotenv import load_dotenv
import os
import sys
import toml

import admin_service_pb2
import admin_service_pb2_grpc

load_dotenv(".env")

def validate_config(content: bytes) -> list[str]:
    errors = []

    try:
        config = toml.loads(content.decode('utf-8'))
    except Exception as e:
        errors.append(f"Invalid TOML: {e}")
        return errors

    if not config:
        errors.append("Config file is empty")
        return errors

    if 'global' not in config:
        errors.append("Missing required section: 'global'")
    else:
        if 'rules' not in config['global']:
            errors.append("Missing 'global.rules' section")
        else:
            rules = config['global']['rules']

            if 'block_categories' in rules and not isinstance(rules['block_categories'], list):
                errors.append("global.rules.block_categories must be a list")

            if 'block_domains' in rules and not isinstance(rules['block_domains'], list):
                errors.append("global.rules.block_domains must be a list")

            if 'allow_domains' in rules and not isinstance(rules['allow_domains'], list):
                errors.append("global.rules.allow_domains must be a list")

            if 'min_trust_level' in rules:
                if not isinstance(rules['min_trust_level'], int):
                    errors.append("global.rules.min_trust_level must be an integer")
                elif rules['min_trust_level'] < 0:
                    errors.append("global.rules.min_trust_level must be >= 0")

            if 'block_by_trust' in rules and not isinstance(rules['block_by_trust'], dict):
                errors.append("global.rules.block_by_trust must be a table")

    if 'filters' in config:
        if not isinstance(config['filters'], dict):
            errors.append("'filters' must be a table")
        else:
            filter_names = set()
            for filter_name in config['filters'].keys():
                if filter_name in filter_names:
                    errors.append(f"Duplicate filter name: '{filter_name}'")
                filter_names.add(filter_name)

                filter_config = config['filters'][filter_name]
                if not isinstance(filter_config, dict):
                    errors.append(f"Filter '{filter_name}' must be a table")
                    continue

                if 'block_categories' in filter_config and not isinstance(filter_config['block_categories'], list):
                    errors.append(f"Filter '{filter_name}'.block_categories must be a list")

                if 'block_domains' in filter_config and not isinstance(filter_config['block_domains'], list):
                    errors.append(f"Filter '{filter_name}'.block_domains must be a list")

                if 'allow_domains' in filter_config and not isinstance(filter_config['allow_domains'], list):
                    errors.append(f"Filter '{filter_name}'.allow_domains must be a list")

                if 'min_trust_level' in filter_config:
                    if not isinstance(filter_config['min_trust_level'], int):
                        errors.append(f"Filter '{filter_name}'.min_trust_level must be an integer")
                    elif filter_config['min_trust_level'] < 0:
                        errors.append(f"Filter '{filter_name}'.min_trust_level must be >= 0")

                if 'block_by_trust' in filter_config and not isinstance(filter_config['block_by_trust'], dict):
                    errors.append(f"Filter '{filter_name}'.block_by_trust must be a table")

    return errors
    
class AdminClient:
    def __init__(self):
        self.host: str = os.environ["SERVER_HOST"]
        self.port: str = os.environ["SERVER_PORT"]
        self.channel: grpc.channel = grpc.insecure_channel(f"{self.host}:{self.port}")
        self.stub: admin_service_pb2_grpc.AdminServiceStub = admin_service_pb2_grpc.AdminServiceStub(self.channel)

    def load_config(self, toml_file: str) -> admin_service_pb2.LoadConfigResponse:
        with open(toml_file, "rb") as f:
            content: bytes = f.read()

        errors = validate_config(content)

        if errors:
            error_msg = "\n".join(errors)
            raise ValueError(f"Config validation failed:\n{error_msg}")

        request: admin_service_pb2.LoadConfigRequest = (admin_service_pb2.LoadConfigRequest(config_data=content))
        response = self.stub.LoadConfig(request)

        if not response.success:
            error_msg = response.error_message if hasattr(response, 'error_message') else "Unknown error"
            raise Exception(f"Server error: {error_msg}")
        return response

    def get_config_toml(self, output_file: str):
        request = admin_service_pb2.GetConfigRequest()
        response = self.stub.GetConfigAdmin(request)
        
        toml_data = response.config_data
        
        if output_file:
            with open(output_file, "wb") as f:
                f.write(toml_data)
            print(f"TOML config saved to {output_file}")
        
        return toml_data

    def toggle_filtering(self, worker_id: int, enabled: bool):
        request = admin_service_pb2.ToggleFilteringRequest(
            worker_id=worker_id,
            enabled=enabled,
        )
        response = self.stub.ToggleFiltering(request)
        return response

def main():
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="action")

    load_parser = subparsers.add_parser("load")
    load_parser.add_argument("--file", default="config.toml")

    get_parser = subparsers.add_parser("get")
    get_parser.add_argument("--save", "-s", default="policy.toml")

    toggle_parser = subparsers.add_parser("toggle")
    toggle_parser.add_argument("--id", type=int, required=True)
    toggle_parser.add_argument("--on", action="store_true", dest="enabled")
    toggle_parser.add_argument("--off", action="store_false", dest="enabled")
    toggle_parser.set_defaults(enabled=True)

    args = parser.parse_args()

    client = AdminClient()
    try:
        if args.action == "load":
            client.load_config(args.file)
            print("Config loaded")
        elif args.action == "get":
            toml_data = client.get_config_toml(args.save)
            print("Config saved")
        elif args.action == "toggle":
            resp = client.toggle_filtering(args.id, args.enabled)
            state = "enabled" if args.enabled else "disabled"
            print(f"Filtering {state}: {resp.message}")
        else:
            parser.print_help()
    except Exception as e:
        print(f"Error: {e}")
        return 1
    return 0

if __name__ == "__main__":
    main()

