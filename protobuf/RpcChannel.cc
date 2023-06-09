#include "RpcChannel.h"

#include "../logging/Logging.h"
#include "rpc.pb.h"

#include <boost/bind.hpp>
#include <google/protobuf/descriptor.h>

using namespace muduo;

RpcChannel::RpcChannel()
    : codec_(boost::bind(&RpcChannel::onRpcMessage, this, _1, _2, _3)),
      services_(NULL) {
    LOG_INFO << "RpcChannel::ctor - " << this;
}

RpcChannel::RpcChannel(const TcpConnectionPtr &conn)
    : codec_(boost::bind(&RpcChannel::onRpcMessage, this, _1, _2, _3)),
      conn_(conn), services_(NULL) {
    LOG_INFO << "RpcChannel::ctor - " << this;
}

RpcChannel::~RpcChannel() {
    // update
}

void RpcChannel::onMessage(const muduo::TcpConnectionPtr &conn,
                           muduo::Buffer *buf, Timestamp receiveTime) {
    codec_.onMessage(conn, buf, receiveTime);
}

void RpcChannel::CallMethod(const ::google::protobuf::MethodDescriptor *method,
                            ::google::protobuf::RpcController *controller,
                            const google::protobuf::Message *request,
                            ::google::protobuf::Message *response,
                            ::google::protobuf::Closure *done) {
    RpcMessage message;
    message.set_type(REQUEST);
    int64_t id = id_.incrementAndGet();
    message.set_id(id);
    message.set_service(method->service()->full_name());
    message.set_method(method->name());
    message.set_request(request->SerializeAsString());

    OutstandingCall out;
    out.response = response;
    out.done = done;
    {
        MutexLockGuard lock(mutex_);
        outstandings_[id] = out;
    }

    codec_.send(conn_, message);
}

void RpcChannel::onRpcMessage(const muduo::TcpConnectionPtr &conn,
                              const MessagePtr &messagePtr,
                              muduo::Timestamp receiveTime) {
    assert(conn == conn_);
    RpcMessage &message = *(down_pointer_cast<RpcMessage>(messagePtr));
    if (message.type() == RESPONSE) {
        int64_t id = message.id();
        assert(message.has_response() || message.has_error());
        OutstandingCall out = {NULL, NULL};
        {
            MutexLockGuard lock(mutex_);
            std::map<int64_t, OutstandingCall>::iterator it =
                outstandings_.find(id);
            if (it != outstandings_.end()) {
                out = it->second;
                outstandings_.erase(it);
            }
        }
        if (out.response) {
            std::unique_ptr<google::protobuf::Message> d(out.response);
            if (message.has_response()) {
                out.response->ParseFromString(message.response());
            }
            if (out.done) {
                out.done->Run();
            }
        }
    } else if (message.type() == REQUEST) {
        ErrorCode error = WRONG_PROTO;
        if (services_) {
            std::map<std::string, google::protobuf::Service *>::const_iterator
                it = services_->find(message.service());
            if (it != services_->end()) {
                google::protobuf::Service *service = it->second;
                assert(service != NULL);
                const google::protobuf::ServiceDescriptor *desc =
                    service->GetDescriptor();
                const google::protobuf::MethodDescriptor *method =
                    desc->FindMethodByName(message.method());
                if (method) {
                    std::unique_ptr<google::protobuf::Message> request(
                        service->GetRequestPrototype(method).New());
                    if (request->ParseFromString(message.request())) {
                        google::protobuf::Message *response =
                            service->GetResponsePrototype(method).New();
                        int64_t id = message.id();
                        service->CallMethod(
                            method, NULL, request.get(), response,
                            google::protobuf::NewCallback(
                                this, &RpcChannel::doneCallback, response, id));
                        error = NO_ERROR;
                    } else {
                        error = INVALID_REQUEST;
                    }
                } else {
                    error = NO_METHOD;
                }

            } else {
                error = NO_SERVICE;
            }
        } else {
            error = NO_SERVICE;
        }
        if (error != NO_ERROR) {
            RpcMessage response;
            response.set_type(RESPONSE);
            response.set_id(message.id());
            response.set_error(error);
            codec_.send(conn_, response);
        }
    } else if (message.type() == ERROR) {
    }
}

void RpcChannel::doneCallback(::google::protobuf::Message *response,
                              int64_t id) {
    std::unique_ptr<google::protobuf::Message> d(response);
    RpcMessage message;
    message.set_type(RESPONSE);
    message.set_id(id);
    message.set_response(response->SerializeAsString());
    codec_.send(conn_, message);
}