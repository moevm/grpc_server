#include <grpcpp/grpcpp.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include "worker.grpc.pb.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class WorkerServiceImpl final : public WorkerService::Service {
public:
    Status ProcessTask(ServerContext* context, const TaskData* request, TaskResult* response) override
    {
        std::string data = request->data();

        // hash calculation will be here someday
        std::reverse(data.begin(), data.end());
        
        response->set_success(true);
        response->set_message("OK (" + request->algorithm() + " algorithm)");
        response->set_result(data);
        return Status::OK;
    }
};

int main(int argc, char** argv)
{
    std::string server_address("0.0.0.0:50051");
    WorkerServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    builder.SetMaxSendMessageSize(INT_MAX);
    builder.SetMaxMessageSize(INT_MAX);
    builder.SetMaxReceiveMessageSize(INT_MAX);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << " (container port)" << std::endl;

    server->Wait();
return 0;
}