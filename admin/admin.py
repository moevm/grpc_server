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

    if 'filters' in config:
        filter_names = set()
        for filter_name in config['filters'].keys():
            if filter_name in filter_names:
                errors.append(f"Duplicate filter name: '{filter_name}'")
            filter_names.add(filter_name)
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

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--file", default="config.toml")
    args = parser.parse_args()

    try:
        client = AdminClient()
        client.load_config(args.file)
        print("Config loaded")
    except Exception as e:
        print(f"Error loading config: {e}")
        return 1
    
    return 0

if __name__ == "__main__":
    main()

