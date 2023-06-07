#ifndef MUDUO_PROTOBUF_CODEC_H
#define MUDUO_PROTOBUF_CODEC_H

#include "../Buffer.h"
#include "../Callbacks.h"
#include "../TcpConnection.h"
#include "../datetime/Timestamp.h"

#include <boost/noncopyable.hpp>
#include <google/protobuf/message.h>

typedef std::shared_ptr<google::protobuf::Message> MessagePtr;

class ProtobufCodec : boost::noncopyable {
  public:
    enum ErrorCode {
        kNoError = 0,
        kInvalidLength,
        kCheckSumError,
        kInvalidNameLen,
        kUnknownMessageType,
        kParseError,
    };

    typedef std::function<void(const muduo::TcpConnectionPtr &,
                               const MessagePtr &, muduo::Timestamp)>
        ProtobufMessageCallback;
    typedef std::function<void(const muduo::TcpConnectionPtr &, muduo::Buffer *,
                               muduo::Timestamp, ErrorCode)>
        ErrorCallback;

    explicit ProtobufCodec(const ProtobufMessageCallback &messageCb)
        : messageCallback_(messageCb), errorCallback_(defaultErrorCallback) {}

    ProtobufCodec(const ProtobufMessageCallback &messageCb,
                  const ErrorCallback &errorCb)
        : messageCallback_(messageCb), errorCallback_(errorCb) {}

    void onMessage(const muduo::TcpConnectionPtr &conn, muduo::Buffer *buf,
                   muduo::Timestamp receiveTime);
    void send(const muduo::TcpConnectionPtr &conn,
              const google::protobuf::Message &message) {
        muduo::Buffer buf;
        fillEmptyBuffer(&buf, message);
        std::string s(buf.peek(), buf.readableBytes());
        conn->send(s);
    }

    static const std::string &errorCodeToString(ErrorCode errorCode);
    static void fillEmptyBuffer(muduo::Buffer *buf,
                                const google::protobuf::Message &message);
    static google::protobuf::Message *
    createMessage(const std::string &type_name);
    static MessagePtr parse(const char *buf, int len, ErrorCode *errorCode);

  private:
    static void defaultErrorCallback(const muduo::TcpConnectionPtr &conn,
                                     muduo::Buffer *buffer,
                                     muduo::Timestamp time,
                                     ErrorCode errorCode);
    ProtobufMessageCallback messageCallback_;
    ErrorCallback errorCallback_;

    const static int kHeaderLen = sizeof(int32_t);
    const static int kMinMessageLen =
        2 * kHeaderLen + 2; // nameLen + typeName + checkSum
    const static int kMaxMessageLen =
        64 * 1024 * 1024; // same as codec_stream.h kDefaultTotalBytesLimit
};

#endif