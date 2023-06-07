#ifndef MUDUO_NET_BUFFER_H
#define MUDUO_NET_BUFFER_H

#include "SocketsOps.h"
#include "datetime/copyable.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

namespace muduo {
class Buffer : public muduo::copyable {
  public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    Buffer()
        : buffer_(kCheapPrepend + kInitialSize), readerIndex_(kCheapPrepend),
          writerIndex_(kCheapPrepend) {}

    size_t readableBytes() const { return writerIndex_ - readerIndex_; }
    size_t writableBytes() const { return buffer_.size() - writerIndex_; }
    size_t prependableBytes() const { return readerIndex_; }

    const char *peek() const { return begin() + readerIndex_; }

    void retrieve(size_t len) {
        assert(len <= readableBytes());
        readerIndex_ += len;
    }
    void retrieveUntil(const char *end) {
        assert(peek() <= end);
        assert(end <= beginWrite());
        retrieve(end - peek());
    }
    void retrieveAll() {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    std::string retrieveAsString() {
        std::string str(peek(), readableBytes());
        retrieveAll();
        return str;
    }

    void append(const std::string &str) { append(str.data(), str.length()); }
    void append(const char * /*restrict*/ data, size_t len) {
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        hasWritten(len);
    }
    void append(const void * /*restrict*/ data, size_t len) {
        append(static_cast<const char *>(data), len);
    }
    void ensureWritableBytes(size_t len) {
        if (writableBytes() < len) {
            makeSpace(len);
        }
        assert(len <= writableBytes());
    }

    char *beginWrite() { return begin() + writerIndex_; }
    const char *beginWrite() const { return begin() + writerIndex_; }
    void hasWritten(size_t len) { writerIndex_ += len; }

    void prepend(const void * /*restrict*/ data, size_t len) {
        assert(len <= prependableBytes());
        readerIndex_ -= len;
        const char *d = static_cast<const char *>(data);
        std::copy(d, d + len, begin() + readerIndex_);
    }

    void shrink(size_t reverse) {
        std::vector<char> buf(kCheapPrepend + readableBytes() + reverse);
        std::copy(peek(), peek() + readableBytes(),
                  buf.begin() + kCheapPrepend);
        buf.swap(buffer_);
    }

    ssize_t readFd(int fd, int *savedError);

    int32_t peekInt32() {
        assert(readableBytes() >= sizeof(int32_t));
        int32_t be32 = 0;
        ::memcpy(&be32, peek(), sizeof be32);
        return sockets::networkToHost32(be32);
    }

    void appendInt32(int32_t x) {
        int32_t be32 = sockets::hostToNetwork32(x);
        append(&be32, sizeof(be32));
    }

    int32_t readInt32() {
        int32_t result = peekInt32();
        retrieve(sizeof(result));
        return result;
    }

  private:
    char *begin() { return &*buffer_.begin(); }
    const char *begin() const { return &*buffer_.begin(); }
    void makeSpace(size_t len) {
        if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
            buffer_.resize(writerIndex_ + len);
        } else {
            assert(kCheapPrepend < readerIndex_);
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_, begin() + writerIndex_,
                      begin() + kCheapPrepend);

            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
            assert(readableBytes() == readable);
        }
    }

  private:
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};
} // namespace muduo

#endif