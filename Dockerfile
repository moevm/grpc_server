FROM alpine:3.16.3

RUN apk update && apk add g++ cmake make openssl-dev libcurl curl-dev

WORKDIR /tmp
COPY scripts/get-prometheus-cpp.sh scripts/
RUN sh scripts/get-prometheus-cpp.sh

WORKDIR /app
COPY . .
RUN make

ENTRYPOINT [ "build/grpc_server" ]
