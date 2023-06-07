#include "TcpServer.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "InetAddress.h"
#include "SocketsOps.h"
#include "TcpConnection.h"
#include "logging/Logging.h"

#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>

using namespace muduo;

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr)
    : loop_(loop), name_(listenAddr.toHostPort()),
      acceptor_(new Acceptor(loop, listenAddr)),
      threadPool_(new EventLoopThreadPool(loop)), started_(false),
      nextConnId_(1) {
    acceptor_->setNewConnectionCallback(
        boost::bind(&TcpServer::newConnection, this, _1, _2));
}
TcpServer::~TcpServer() {}

void TcpServer::setThreadNum(int numThreads) {
    assert(numThreads >= 0);
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start() {
    if (started_ == false) {
        started_ = true;
        threadPool_->start();
    }
    if (acceptor_->listening() == false) {
        loop_->runInLoop(
            boost::bind(&Acceptor::listen, get_pointer(acceptor_)));
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr) {
    loop_->assertInLoopThread();
    char buf[32];
    snprintf(buf, sizeof(buf), "#%d", nextConnId_);
    nextConnId_++;
    std::string connName = name_ + buf;
    LOG_INFO << "TcpServer::newConnection [" << name_ << "] - new connection ["
             << connName << "] from " << peerAddr.toHostPort();

    InetAddress localAddr(sockets::getLocalAddr(sockfd));
    EventLoop *ioLoop = threadPool_->getNextLoop();
    TcpConnectionPtr conn(
        new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
    connections_[connName] = conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setCloseCallback(boost::bind(&TcpServer::removeConnection, this, _1));
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    ioLoop->runInLoop(boost::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn) {
    printf("TcpServer::removeConnection() pid = %d, tid = %d\n", getpid(),
           muduo::CurrentThread::tid());

    loop_->runInLoop(
        boost::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn) {
    printf("TcpServer::removeConnectionInLoop() pid = %d, tid = %d\n", getpid(),
           muduo::CurrentThread::tid());
    loop_->assertInLoopThread();
    LOG_INFO << "TcpServer::removeConnection [" << name_ << "] - connection "
             << conn->name();
    size_t n = connections_.erase(conn->name());
    assert(n == 1);
    (void)n;
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(boost::bind(&TcpConnection::connectDestroyed, conn));
}