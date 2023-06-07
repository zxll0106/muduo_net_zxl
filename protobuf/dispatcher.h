#ifndef MUDUO_PROTOBUF_DISPATCHER_H
#define MUDUO_PROTOBUF_DISPATCHER_H

#include "../Callbacks.h"
#include <boost/noncopyable.hpp>
#include <google/protobuf/message.h>
#include <map>

typedef std::shared_ptr<google::protobuf::Message> MessagePtr;

class Callback : boost::noncopyable {
  public:
    virtual ~Callback() = default;
    virtual void onMessage(const muduo::TcpConnectionPtr &conn,
                           const MessagePtr &message,
                           muduo::Timestamp) const = 0;
};

template <typename T> class CallbackT : public Callback {
    static_assert(std::is_base_of<google::protobuf::Message, T>::value,
                  "T must be derived from gpb::Message.");

  public:
    typedef std::function<void(const muduo::TcpConnectionPtr &,
                               const std::shared_ptr<T> &message,
                               muduo::Timestamp)>
        ProtobufMessageTCallback;
    CallbackT(const ProtobufMessageTCallback &callback) : callback_(callback) {}
    void onMessage(const muduo::TcpConnectionPtr &conn,
                   const MessagePtr &message,
                   muduo::Timestamp receiveTime) const override {
        std::shared_ptr<T> concrete = muduo::down_pointer_cast<T>(message);
        assert(concrete != NULL);
        callback_(conn, concrete, receiveTime);
    }

  private:
    ProtobufMessageTCallback callback_;
};

class ProtobufDispatcher {
  public:
    typedef std::function<void(const muduo::TcpConnectionPtr &,
                               const MessagePtr &, muduo::Timestamp)>
        ProtobufMessageCallback;

    explicit ProtobufDispatcher(const ProtobufMessageCallback &callback)
        : defaultCallback_(callback) {}

    template <typename T>
    void registerMessageCallback(
        const typename CallbackT<T>::ProtobufMessageTCallback &callback) {
        std::shared_ptr<CallbackT<T>> pd(new CallbackT<T>(callback));
        callbacks_[T::descriptor()] = pd;
    }

    void onProtobufMessage(const muduo::TcpConnectionPtr &conn,
                           const MessagePtr &message,
                           muduo::Timestamp receiveTime) {
        CallbackMap::const_iterator it =
            callbacks_.find(message->GetDescriptor());
        if (it != callbacks_.end()) {
            it->second->onMessage(conn, message, receiveTime);
        } else {
            defaultCallback_(conn, message, receiveTime);
        }
    }

  private:
    typedef std::map<const google::protobuf::Descriptor *,
                     std::shared_ptr<Callback>>
        CallbackMap;
    CallbackMap callbacks_;
    ProtobufMessageCallback defaultCallback_;
};

#endif