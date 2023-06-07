#include "codec.h"
#include "../logging/Logging.h"
#include "google-inl.h"

#include <google/protobuf/descriptor.h>
#include <inttypes.h>
#include <zlib.h>

using namespace muduo;

void ProtobufCodec::fillEmptyBuffer(muduo::Buffer *buf,
                                    const google::protobuf::Message &message) {
    assert(buf->readableBytes() == 0);
    const std::string &typeName = message.GetTypeName();
    int32_t nameLen = static_cast<int32_t>(typeName.size() + 1);
    buf->appendInt32(nameLen);
    buf->append(typeName.c_str(), nameLen);

    GOOGLE_DCHECK(message.IsInitialized())
        << InitializationErrorMessage("serialize", message);
#if GOOGLE_PROTOBUF_VERSION > 3009002
    int byte_size =
        google::protobuf::internal::ToIntSize(message.ByteSizeLong());
#else
    int byte_size = message.ByteSize();
#endif
    buf->ensureWritableBytes(byte_size);

    uint8_t *start = reinterpret_cast<uint8_t *>(buf->beginWrite());
    uint8_t *end = message.SerializeWithCachedSizesToArray(start);
    if (end - start != byte_size) {
#if GOOGLE_PROTOBUF_VERSION > 3009002
        ByteSizeConsistencyError(
            byte_size,
            google::protobuf::internal::ToIntSize(message.ByteSizeLong()),
            static_cast<int>(end - start));
#else
        ByteSizeConsistencyError(byte_size, message.ByteSize(),
                                 static_cast<int>(end - start));
#endif
    }

    buf->hasWritten(byte_size);
    int32_t checkSum = static_cast<int32_t>(
        ::adler32(1, reinterpret_cast<const Bytef *>(buf->peek()),
                  static_cast<int>(buf->readableBytes())));
    buf->appendInt32(checkSum);
    assert(buf->readableBytes() ==
           sizeof(nameLen) + nameLen + byte_size + sizeof(checkSum));
    int32_t len =
        sockets::hostToNetwork32(static_cast<int32_t>(buf->readableBytes()));
    buf->prepend(&len, sizeof(len));
}

namespace {
const std::string kNoErrorStr = "NoError";
const std::string kInvalidLengthStr = "InvalidLength";
const std::string kCheckSumErrorStr = "CheckSumError";
const std::string kInvalidNameLenStr = "InvalidNameLen";
const std::string kUnknownMessageTypeStr = "UnknownMessageType";
const std::string kParseErrorStr = "ParseError";
const std::string kUnknownErrorStr = "UnknownError";
} // namespace

const std::string &ProtobufCodec::errorCodeToString(ErrorCode errorCode) {
    switch (errorCode) {
    case kNoError:
        return kNoErrorStr;
    case kInvalidLength:
        return kInvalidLengthStr;
    case kCheckSumError:
        return kCheckSumErrorStr;
    case kInvalidNameLen:
        return kInvalidNameLenStr;
    case kUnknownMessageType:
        return kUnknownMessageTypeStr;
    case kParseError:
        return kParseErrorStr;
    default:
        return kUnknownErrorStr;
    }
}

void ProtobufCodec::defaultErrorCallback(const muduo::TcpConnectionPtr &conn,
                                         muduo::Buffer *buffer,
                                         muduo::Timestamp time,
                                         ErrorCode errorCode) {
    LOG_ERROR << "ProtobufCodec::defaultErrorCallback - "
              << errorCodeToString(errorCode);
    if (conn && conn->connected()) {
        conn->shutdown();
    }
}

void ProtobufCodec::onMessage(const muduo::TcpConnectionPtr &conn,
                              muduo::Buffer *buf,
                              muduo::Timestamp receiveTime) {
    while (buf->readableBytes() >= kMinMessageLen + kHeaderLen) {
        // printf("aaaa\n");
        const int32_t len = buf->peekInt32();
        // printf("bbbb\n");
        if (len > kMaxMessageLen || len < kMinMessageLen) {
            errorCallback_(conn, buf, receiveTime, kInvalidLength);
            break;
        } else if (buf->readableBytes() >=
                   implicit_cast<size_t>(kHeaderLen + len)) {
            ErrorCode errorcode = kNoError;
            MessagePtr message =
                parse(buf->peek() + kHeaderLen, len, &errorcode);
            if (errorcode == kNoError && message) {
                messageCallback_(conn, message, receiveTime);
                buf->retrieve(kHeaderLen + len);
            } else {
                errorCallback_(conn, buf, receiveTime, errorcode);
                break;
            }
        } else {
            break;
        }
    }
}

int32_t asInt32(const char *buf) {
    int32_t be32 = 0;
    ::memcpy(&be32, buf, sizeof(be32));
    return sockets::networkToHost32(be32);
}

google::protobuf::Message *
ProtobufCodec::createMessage(const std::string &typeName) {
    google::protobuf::Message *message = NULL;
    const google::protobuf::Descriptor *descriptor =
        google::protobuf::DescriptorPool::generated_pool()
            ->FindMessageTypeByName(typeName);
    if (descriptor) {
        const google::protobuf::Message *prototype =
            google::protobuf::MessageFactory::generated_factory()->GetPrototype(
                descriptor);
        if (prototype) {
            message = prototype->New();
        }
    }
    return message;
}

MessagePtr ProtobufCodec::parse(const char *buf, int len,
                                ErrorCode *errorCode) {
    MessagePtr message;
    int32_t expectedCheckSum = asInt32(buf + len - kHeaderLen);
    int32_t checkSum =
        static_cast<int32_t>(::adler32(1, reinterpret_cast<const Bytef *>(buf),
                                       static_cast<int>(len - kHeaderLen)));
    if (expectedCheckSum == checkSum) {
        int32_t nameLen = asInt32(buf);
        if (nameLen >= 2 && nameLen <= len - 2 * kHeaderLen) {
            std::string typeName(buf + kHeaderLen,
                                 buf + kHeaderLen + nameLen - 1);
            message.reset(createMessage(typeName));
            if (message) {
                const char *data = buf + kHeaderLen + nameLen;
                int32_t dataLen = len - 2 * kHeaderLen - nameLen;
                if (message->ParseFromArray(data, dataLen)) {
                    *errorCode = kNoError;
                    // printf("kNoError\n");
                } else {
                    *errorCode = kParseError;
                    // printf("kParseError\n");
                }
            } else {
                *errorCode = kUnknownMessageType;
                // printf("kUnknownMessageType\n");
            }
        } else {
            *errorCode = kInvalidNameLen;
            // printf("kInvalidNameLen\n");
        }
    } else {
        *errorCode = kCheckSumError;
        // printf("kCheckSumError\n");
    }
    return message;
}