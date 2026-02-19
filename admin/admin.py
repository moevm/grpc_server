import argparse
import grpc
from dotenv import load_dotenv
import os

import admin_service_pb2
import admin_service_pb2_grpc

load_dotenv(os.path.join(os.path.dirname(__file__), "..", "controller", ".env"))

class AdminClient:
    def __init__(self):
        self.host: str = os.environ["SERVER_HOST"]
        self.port: str = os.environ["SERVER_PORT"]
        self.channel: grpc.channel = grpc.insecure_channel(f"{self.host}:{self.port}")
        self.stub: admin_service_pb2_grpc.AdminServiceStub = admin_service_pb2_grpc.AdminServiceStub(self.channel)

    def load_config(self, toml_file: str) -> admin_service_pb2.LoadConfigResponse:
        with open(toml_file, "rb") as f:
            content: bytes = f.read()

        request: admin_service_pb2.LoadConfigRequest = (admin_service_pb2.LoadConfigRequest(config_data=content))
        return self.stub.LoadConfig(request)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--file", default="config.toml")
    args = parser.parse_args()

    try:
        client = AdminClient()
        response = client.load_config(args.file)
        print("Config loaded")
    except Exception as e:
        print(f"Error loading config: {e}")
        return 1
    
    return 0

if __name__ == "__main__":
    main()

