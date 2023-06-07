#include "Acceptor.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "SocketsOps.h"

#include <boost/bind.hpp>
using namespace muduo;

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr)
    : loop_(loop), acceptSocket_(sockets::createNonblockingOrDie()),
      acceptChannel_(loop_, acceptSocket_.fd()), listening_(false) {
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.bindAddress(listenAddr);
    acceptChannel_.setReadCallback(boost::bind(&Acceptor::handleRead, this));
}

void Acceptor::listen() {
    loop_->assertInLoopThread();
    acceptSocket_.listen();
    listening_ = true;
    acceptChannel_.enableReading();
}

void Acceptor::handleRead() {
    InetAddress peeraddr(0);
    int connfd = acceptSocket_.accept(&peeraddr);
    if (connfd >= 0) {
        if (newConnectionCallback_) {
            newConnectionCallback_(connfd, peeraddr);
        } else {
            sockets::close(connfd);
        }
    }
}