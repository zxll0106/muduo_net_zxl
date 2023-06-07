#ifndef MUDUO_NET_TCPSERVER_H
#define MUDUO_NET_TCPSERVER_H

#include "Callbacks.h"

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <map>

namespace muduo {
class InetAddress;
class EventLoop;
class Acceptor;
class EventLoopThreadPool;

class TcpServer : boost::noncopyable {
  public:
    TcpServer(EventLoop *loop, const InetAddress &listenAddr);
    ~TcpServer();

    void setThreadNum(int numThreads);

    void start();

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
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;

    EventLoop *loop_;
    const std::string name_;
    boost::scoped_ptr<Acceptor> acceptor_;
    boost::scoped_ptr<EventLoopThreadPool> threadPool_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    bool started_;
    int nextConnId_;
    ConnectionMap connections_;
};
} // namespace muduo

#endif