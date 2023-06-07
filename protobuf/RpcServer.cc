#include "RpcServer.h"
#include "../TcpConnection.h"
#include "../logging/Logging.h"
#include "RpcChannel.h"
#include <boost/bind.hpp>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>

using namespace muduo;

RpcServer::RpcServer(EventLoop *loop, const InetAddress &listenAddr)
    : server_(loop, listenAddr) {
    server_.setConnectionCallback(
        boost::bind(&RpcServer::onConnection, this, _1));
}

void RpcServer::registerService(google::protobuf::Service *service) {
    const google::protobuf::ServiceDescriptor *desc = service->GetDescriptor();
    services_[desc->full_name()] = service;
}

void RpcServer::start() { server_.start(); }

void RpcServer::onConnection(const muduo::TcpConnectionPtr &conn) {
    LOG_INFO << "RpcServer - " << conn->peerAddr().toHostPort() << " -> "
             << conn->localAddr().toHostPort() << " is "
             << (conn->connected() ? "UP" : "DOWN");
    if (conn->connected()) {
        RpcChannelPtr channel(new RpcChannel(conn));
        channel->setServices(&services_);
        conn->setMessageCallback(
            boost::bind(&RpcChannel::onMessage, channel.get(), _1, _2, _3));
        conn->setContext(channel);
    } else {
        conn->setContext(RpcChannelPtr());
    }
}
