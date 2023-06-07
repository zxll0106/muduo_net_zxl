#include "Buffer.h"
#include "logging/Logging.h"

#include <sys/uio.h>

using namespace muduo;

ssize_t Buffer::readFd(int fd, int *savedError) {
    char extrabuf[65536];
    const size_t writable = writableBytes();
    struct iovec vec[2];
    vec[0].iov_base = beginWrite();
    vec[0].iov_len = writableBytes();
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    const ssize_t n = readv(fd, vec, 2);
    if (n < 0) {
        *savedError = errno;
    } else if (implicit_cast<size_t>(n) <= writableBytes()) {
        hasWritten(n);
    } else {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }
    return n;
}