# Setting up a devcontainer

Build the "builder" layer, it contains all the dependencies:

```sh
docker build -t grpc_server-builder --target builder -f Dockerfile.hash .
```

Now anytime you open VS code, it will ask you if you want to reopen this project in a container.