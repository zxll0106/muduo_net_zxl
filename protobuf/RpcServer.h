#ifndef MUDUO_PROTOBUF_RPCSERVER_H
#define MUDUO_PROTOBUF_RPCSERVER_H

#include "../Callbacks.h"
#include "../EventLoop.h"
#include "../InetAddress.h"
#include "../TcpServer.h"
#include <map>

namespace google {
namespace protobuf {
class Service;
} // namespace protobuf

} // namespace google

class RpcServer {
  public:
    RpcServer(muduo::EventLoop *loop, const muduo::InetAddress &listenAddr);
    void setThreadNum(int numThreads) { server_.setThreadNum(numThreads); }
    void registerService(google::protobuf::Service *);
    void start();

  private:
    void onConnection(const muduo::TcpConnectionPtr &conn);
    muduo::TcpServer server_;
    std::map<std::string, google::protobuf::Service *> services_;
};

#endif