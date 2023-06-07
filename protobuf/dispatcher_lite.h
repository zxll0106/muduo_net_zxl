#ifndef MUDUO_PROTOBUF_DISPATCHER_LITE_H
#define MUDUO_PROTOBUF_DISPATCHER_LITE_H

#include "../Callbacks.h"

#include <boost/noncopyable.hpp>
#include <google/protobuf/message.h>
#include <map>

typedef std::shared_ptr<google::protobuf::Message> MessagePtr;

class ProtobufDispatcherLite : boost::noncopyable {
  public:
    typedef std::function<void(const muduo::TcpConnectionPtr &,
                               const MessagePtr &, muduo::Timestamp)>
        ProtobufMessageCallback;

    explicit ProtobufDispatcherLite(const ProtobufMessageCallback &cb)
        : defaultCallback_(cb) {}

    void registerMessageCallback(const google::protobuf::Descriptor *desc,
                                 const ProtobufMessageCallback &cb) {
        callbacks_[desc] = cb;
    }

    void onProtobufMessage(const muduo::TcpConnectionPtr &conn,
                           const MessagePtr &message,
                           muduo::Timestamp receiveTime) {
        CallbackMap::const_iterator it =
            callbacks_.find(message->GetDescriptor());
        if (it != callbacks_.end()) {
            it->second(conn, message, receiveTime);
        } else {
            defaultCallback_(conn, message, receiveTime);
        }
    }

  private:
    typedef std::map<const google::protobuf::Descriptor *,
                     ProtobufMessageCallback>
        CallbackMap;
    CallbackMap callbacks_;
    ProtobufMessageCallback defaultCallback_;
};

#endif