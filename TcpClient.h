#ifndef MUDUO_NET_TCPCLIENT_H
#define MUDUO_NET_TCPCLIENT_H

#include "TcpConnection.h"
#include "thread/Mutex.h"
#include <boost/noncopyable.hpp>

namespace muduo {
class Connector;
typedef boost::shared_ptr<Connector> ConnectorPtr;
class TcpClient : boost::noncopyable {
  public:
    TcpClient(EventLoop *loop, const InetAddress &serverAddr);
    ~TcpClient();

    void connect();
    void disconnect();
    void stop();

    TcpConnectionPtr connection() const {
        MutexLockGuard lock(mutex_);
        return connection_;
    }

    bool retry() const { return retry_; }
    void enableRetry() { retry_ = true; }

    void setConnectionCallback(const ConnectionCallback &cb) {
        connectionCallback_ = cb;
    }
    void setMessageCallback(const MessageCallback &cb) {
        messageCallback_ = cb;
    }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) {
        writeCompleteCallback_ = cb;
    }

  private:
    void newConnection(int sockfd);
    void removeConnection(const TcpConnectionPtr &conn);

    EventLoop *loop_;
    ConnectorPtr connector_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    bool retry_;
    bool connect_;
    int nextConnId_;
    mutable MutexLock mutex_;
    TcpConnectionPtr connection_;
};
} // namespace muduo

#endif