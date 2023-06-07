#ifndef MUDUO_PROTOBUF_RPCCHANNEL_H
#define MUDUO_PROTOBUF_RPCCHANNEL_H

#include "../TcpConnection.h"
#include "../datetime/Timestamp.h"
#include "../thread/Atomic.h"
#include "../thread/Mutex.h"
#include "/home/zxl/Downloads/muduo_ZXL/Callbacks.h"
#include "codec.h"
#include "rpc.pb.h"
#include <google/protobuf/service.h>
#include <map>
#include <string>

namespace google {
namespace protobuf {
class Descriptor;
class ServiceDescriptor;
class MethodDescriptor;
class Message;

class Closure;

class RpcController;
class Service;
} // namespace protobuf
} // namespace google
typedef std::shared_ptr<muduo::RpcMessage> RpcMessagePtr;

class RpcChannel : public ::google::protobuf::RpcChannel {

  public:
    RpcChannel();
    explicit RpcChannel(const muduo::TcpConnectionPtr &conn);

    ~RpcChannel() override;

    void setConnection(const muduo::TcpConnectionPtr &conn) { conn_ = conn; }
    void setServices(
        const std::map<std::string, ::google::protobuf::Service *> *services) {
        services_ = services;
    }

    void CallMethod(const ::google::protobuf::MethodDescriptor *method,
                    ::google::protobuf::RpcController *controller,
                    const google::protobuf::Message *request,
                    ::google::protobuf::Message *response,
                    ::google::protobuf::Closure *done) override;

    void onMessage(const muduo::TcpConnectionPtr &conn, muduo::Buffer *buf,
                   muduo::Timestamp receiveTime);

  private:
    void onRpcMessage(const muduo::TcpConnectionPtr &conn,
                      const MessagePtr &messagePtr,
                      muduo::Timestamp receiveTime);

    void doneCallback(::google::protobuf::Message *response, int64_t id);

    ProtobufCodec codec_;
    muduo::TcpConnectionPtr conn_;

    muduo::AtomicInt64 id_;
    int64_t idx_;

    muduo::MutexLock mutex_;

    struct OutstandingCall {
        ::google::protobuf::Message *response;
        ::google::protobuf::Closure *done;
    };
    std::map<int64_t, OutstandingCall> outstandings_;

    const std::map<std::string, ::google::protobuf::Service *> *services_;
};
typedef std::shared_ptr<RpcChannel> RpcChannelPtr;
#endif