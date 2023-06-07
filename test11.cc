#include "EventLoop.h"
#include "InetAddress.h"
#include "TcpConnection.h"
#include "TcpServer.h"
#include "logging/Logging.h"
#include <stdio.h>

std::string message;

void onConnection(const muduo::TcpConnectionPtr &conn) {
    if (conn->connected()) {
        printf("onConnection(): tid=%d new connection [%s] from %s\n",
               muduo::CurrentThread::tid(), conn->name().c_str(),
               conn->peerAddr().toHostPort().c_str());
        conn->send(message);
    } else {
        printf("onConnection(): tid=%d connection [%s] is down\n",
               muduo::CurrentThread::tid(), conn->name().c_str());
    }
}

void onWriteComplete(const muduo::TcpConnectionPtr &conn) {
    conn->send(message);
}

void onMessage(const muduo::TcpConnectionPtr &conn, muduo::Buffer *buf,
               muduo::Timestamp receiveTime) {
    printf(
        "onMessage(): tid=%d received %zd bytes from connection [%s] at %s\n",
        muduo::CurrentThread::tid(), buf->readableBytes(), conn->name().c_str(),
        receiveTime.toFormattedString().c_str());

    buf->retrieveAll();
}

int main(int argc, char *argv[]) {
    muduo::Logger::setLogLevel(muduo::Logger::LogLevel::TRACE);
    printf("main(): pid = %d\n", getpid());

    std::string line;
    for (int i = 33; i < 127; ++i) {
        line.push_back(char(i));
    }
    line += line;

    for (size_t i = 0; i < 127 - 33; ++i) {
        message += line.substr(i, 72) + '\n';
    }

    muduo::InetAddress listenAddr(9981);
    muduo::EventLoop loop;

    muduo::TcpServer server(&loop, listenAddr);
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    server.setWriteCompleteCallback(onWriteComplete);
    if (argc > 1) {
        server.setThreadNum(atoi(argv[1]));
    }
    server.start();

    loop.loop();
}
