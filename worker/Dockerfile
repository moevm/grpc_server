FROM ubuntu:22.04

WORKDIR /app
RUN apt update && apt install -y libgrpc++-dev protobuf-compiler protobuf-compiler-grpc g++

COPY . .
#RUN protoc --cpp_out=. --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` worker.proto
RUN g++ -o worker main.cc worker.pb.cc worker.grpc.pb.cc -lgrpc++ -lgrpc -lprotobuf -lpthread

ENTRYPOINT [ "./worker" ]