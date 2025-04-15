// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: worker.proto
#ifndef GRPC_worker_2eproto__INCLUDED
#define GRPC_worker_2eproto__INCLUDED

#include "worker.pb.h"

#include <functional>
#include <grpc/impl/codegen/port_platform.h>
#include <grpcpp/impl/codegen/async_generic_service.h>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/client_callback.h>
#include <grpcpp/impl/codegen/client_context.h>
#include <grpcpp/impl/codegen/completion_queue.h>
#include <grpcpp/impl/codegen/message_allocator.h>
#include <grpcpp/impl/codegen/method_handler.h>
#include <grpcpp/impl/codegen/proto_utils.h>
#include <grpcpp/impl/codegen/rpc_method.h>
#include <grpcpp/impl/codegen/server_callback.h>
#include <grpcpp/impl/codegen/server_callback_handlers.h>
#include <grpcpp/impl/codegen/server_context.h>
#include <grpcpp/impl/codegen/service_type.h>
#include <grpcpp/impl/codegen/status.h>
#include <grpcpp/impl/codegen/stub_options.h>
#include <grpcpp/impl/codegen/sync_stream.h>

class WorkerService final {
 public:
  static constexpr char const* service_full_name() {
    return "WorkerService";
  }
  class StubInterface {
   public:
    virtual ~StubInterface() {}
    virtual ::grpc::Status ProcessTask(::grpc::ClientContext* context, const ::TaskData& request, ::TaskResult* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::TaskResult>> AsyncProcessTask(::grpc::ClientContext* context, const ::TaskData& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::TaskResult>>(AsyncProcessTaskRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::TaskResult>> PrepareAsyncProcessTask(::grpc::ClientContext* context, const ::TaskData& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::TaskResult>>(PrepareAsyncProcessTaskRaw(context, request, cq));
    }
    class experimental_async_interface {
     public:
      virtual ~experimental_async_interface() {}
      virtual void ProcessTask(::grpc::ClientContext* context, const ::TaskData* request, ::TaskResult* response, std::function<void(::grpc::Status)>) = 0;
      virtual void ProcessTask(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::TaskResult* response, std::function<void(::grpc::Status)>) = 0;
      #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      virtual void ProcessTask(::grpc::ClientContext* context, const ::TaskData* request, ::TaskResult* response, ::grpc::ClientUnaryReactor* reactor) = 0;
      #else
      virtual void ProcessTask(::grpc::ClientContext* context, const ::TaskData* request, ::TaskResult* response, ::grpc::experimental::ClientUnaryReactor* reactor) = 0;
      #endif
      #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      virtual void ProcessTask(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::TaskResult* response, ::grpc::ClientUnaryReactor* reactor) = 0;
      #else
      virtual void ProcessTask(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::TaskResult* response, ::grpc::experimental::ClientUnaryReactor* reactor) = 0;
      #endif
    };
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
    typedef class experimental_async_interface async_interface;
    #endif
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
    async_interface* async() { return experimental_async(); }
    #endif
    virtual class experimental_async_interface* experimental_async() { return nullptr; }
  private:
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::TaskResult>* AsyncProcessTaskRaw(::grpc::ClientContext* context, const ::TaskData& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::TaskResult>* PrepareAsyncProcessTaskRaw(::grpc::ClientContext* context, const ::TaskData& request, ::grpc::CompletionQueue* cq) = 0;
  };
  class Stub final : public StubInterface {
   public:
    Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel);
    ::grpc::Status ProcessTask(::grpc::ClientContext* context, const ::TaskData& request, ::TaskResult* response) override;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::TaskResult>> AsyncProcessTask(::grpc::ClientContext* context, const ::TaskData& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::TaskResult>>(AsyncProcessTaskRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::TaskResult>> PrepareAsyncProcessTask(::grpc::ClientContext* context, const ::TaskData& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::TaskResult>>(PrepareAsyncProcessTaskRaw(context, request, cq));
    }
    class experimental_async final :
      public StubInterface::experimental_async_interface {
     public:
      void ProcessTask(::grpc::ClientContext* context, const ::TaskData* request, ::TaskResult* response, std::function<void(::grpc::Status)>) override;
      void ProcessTask(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::TaskResult* response, std::function<void(::grpc::Status)>) override;
      #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      void ProcessTask(::grpc::ClientContext* context, const ::TaskData* request, ::TaskResult* response, ::grpc::ClientUnaryReactor* reactor) override;
      #else
      void ProcessTask(::grpc::ClientContext* context, const ::TaskData* request, ::TaskResult* response, ::grpc::experimental::ClientUnaryReactor* reactor) override;
      #endif
      #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      void ProcessTask(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::TaskResult* response, ::grpc::ClientUnaryReactor* reactor) override;
      #else
      void ProcessTask(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::TaskResult* response, ::grpc::experimental::ClientUnaryReactor* reactor) override;
      #endif
     private:
      friend class Stub;
      explicit experimental_async(Stub* stub): stub_(stub) { }
      Stub* stub() { return stub_; }
      Stub* stub_;
    };
    class experimental_async_interface* experimental_async() override { return &async_stub_; }

   private:
    std::shared_ptr< ::grpc::ChannelInterface> channel_;
    class experimental_async async_stub_{this};
    ::grpc::ClientAsyncResponseReader< ::TaskResult>* AsyncProcessTaskRaw(::grpc::ClientContext* context, const ::TaskData& request, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientAsyncResponseReader< ::TaskResult>* PrepareAsyncProcessTaskRaw(::grpc::ClientContext* context, const ::TaskData& request, ::grpc::CompletionQueue* cq) override;
    const ::grpc::internal::RpcMethod rpcmethod_ProcessTask_;
  };
  static std::unique_ptr<Stub> NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options = ::grpc::StubOptions());

  class Service : public ::grpc::Service {
   public:
    Service();
    virtual ~Service();
    virtual ::grpc::Status ProcessTask(::grpc::ServerContext* context, const ::TaskData* request, ::TaskResult* response);
  };
  template <class BaseClass>
  class WithAsyncMethod_ProcessTask : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithAsyncMethod_ProcessTask() {
      ::grpc::Service::MarkMethodAsync(0);
    }
    ~WithAsyncMethod_ProcessTask() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status ProcessTask(::grpc::ServerContext* /*context*/, const ::TaskData* /*request*/, ::TaskResult* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestProcessTask(::grpc::ServerContext* context, ::TaskData* request, ::grpc::ServerAsyncResponseWriter< ::TaskResult>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(0, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  typedef WithAsyncMethod_ProcessTask<Service > AsyncService;
  template <class BaseClass>
  class ExperimentalWithCallbackMethod_ProcessTask : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    ExperimentalWithCallbackMethod_ProcessTask() {
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      ::grpc::Service::
    #else
      ::grpc::Service::experimental().
    #endif
        MarkMethodCallback(0,
          new ::grpc_impl::internal::CallbackUnaryHandler< ::TaskData, ::TaskResult>(
            [this](
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
                   ::grpc::CallbackServerContext*
    #else
                   ::grpc::experimental::CallbackServerContext*
    #endif
                     context, const ::TaskData* request, ::TaskResult* response) { return this->ProcessTask(context, request, response); }));}
    void SetMessageAllocatorFor_ProcessTask(
        ::grpc::experimental::MessageAllocator< ::TaskData, ::TaskResult>* allocator) {
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      ::grpc::internal::MethodHandler* const handler = ::grpc::Service::GetHandler(0);
    #else
      ::grpc::internal::MethodHandler* const handler = ::grpc::Service::experimental().GetHandler(0);
    #endif
      static_cast<::grpc_impl::internal::CallbackUnaryHandler< ::TaskData, ::TaskResult>*>(handler)
              ->SetMessageAllocator(allocator);
    }
    ~ExperimentalWithCallbackMethod_ProcessTask() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status ProcessTask(::grpc::ServerContext* /*context*/, const ::TaskData* /*request*/, ::TaskResult* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
    virtual ::grpc::ServerUnaryReactor* ProcessTask(
      ::grpc::CallbackServerContext* /*context*/, const ::TaskData* /*request*/, ::TaskResult* /*response*/)
    #else
    virtual ::grpc::experimental::ServerUnaryReactor* ProcessTask(
      ::grpc::experimental::CallbackServerContext* /*context*/, const ::TaskData* /*request*/, ::TaskResult* /*response*/)
    #endif
      { return nullptr; }
  };
  #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
  typedef ExperimentalWithCallbackMethod_ProcessTask<Service > CallbackService;
  #endif

  typedef ExperimentalWithCallbackMethod_ProcessTask<Service > ExperimentalCallbackService;
  template <class BaseClass>
  class WithGenericMethod_ProcessTask : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithGenericMethod_ProcessTask() {
      ::grpc::Service::MarkMethodGeneric(0);
    }
    ~WithGenericMethod_ProcessTask() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status ProcessTask(::grpc::ServerContext* /*context*/, const ::TaskData* /*request*/, ::TaskResult* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithRawMethod_ProcessTask : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawMethod_ProcessTask() {
      ::grpc::Service::MarkMethodRaw(0);
    }
    ~WithRawMethod_ProcessTask() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status ProcessTask(::grpc::ServerContext* /*context*/, const ::TaskData* /*request*/, ::TaskResult* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestProcessTask(::grpc::ServerContext* context, ::grpc::ByteBuffer* request, ::grpc::ServerAsyncResponseWriter< ::grpc::ByteBuffer>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(0, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class ExperimentalWithRawCallbackMethod_ProcessTask : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    ExperimentalWithRawCallbackMethod_ProcessTask() {
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      ::grpc::Service::
    #else
      ::grpc::Service::experimental().
    #endif
        MarkMethodRawCallback(0,
          new ::grpc_impl::internal::CallbackUnaryHandler< ::grpc::ByteBuffer, ::grpc::ByteBuffer>(
            [this](
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
                   ::grpc::CallbackServerContext*
    #else
                   ::grpc::experimental::CallbackServerContext*
    #endif
                     context, const ::grpc::ByteBuffer* request, ::grpc::ByteBuffer* response) { return this->ProcessTask(context, request, response); }));
    }
    ~ExperimentalWithRawCallbackMethod_ProcessTask() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status ProcessTask(::grpc::ServerContext* /*context*/, const ::TaskData* /*request*/, ::TaskResult* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
    virtual ::grpc::ServerUnaryReactor* ProcessTask(
      ::grpc::CallbackServerContext* /*context*/, const ::grpc::ByteBuffer* /*request*/, ::grpc::ByteBuffer* /*response*/)
    #else
    virtual ::grpc::experimental::ServerUnaryReactor* ProcessTask(
      ::grpc::experimental::CallbackServerContext* /*context*/, const ::grpc::ByteBuffer* /*request*/, ::grpc::ByteBuffer* /*response*/)
    #endif
      { return nullptr; }
  };
  template <class BaseClass>
  class WithStreamedUnaryMethod_ProcessTask : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithStreamedUnaryMethod_ProcessTask() {
      ::grpc::Service::MarkMethodStreamed(0,
        new ::grpc::internal::StreamedUnaryHandler<
          ::TaskData, ::TaskResult>(
            [this](::grpc_impl::ServerContext* context,
                   ::grpc_impl::ServerUnaryStreamer<
                     ::TaskData, ::TaskResult>* streamer) {
                       return this->StreamedProcessTask(context,
                         streamer);
                  }));
    }
    ~WithStreamedUnaryMethod_ProcessTask() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable regular version of this method
    ::grpc::Status ProcessTask(::grpc::ServerContext* /*context*/, const ::TaskData* /*request*/, ::TaskResult* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    // replace default version of method with streamed unary
    virtual ::grpc::Status StreamedProcessTask(::grpc::ServerContext* context, ::grpc::ServerUnaryStreamer< ::TaskData,::TaskResult>* server_unary_streamer) = 0;
  };
  typedef WithStreamedUnaryMethod_ProcessTask<Service > StreamedUnaryService;
  typedef Service SplitStreamedService;
  typedef WithStreamedUnaryMethod_ProcessTask<Service > StreamedService;
};


#endif  // GRPC_worker_2eproto__INCLUDED
