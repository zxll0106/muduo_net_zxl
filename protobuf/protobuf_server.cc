#include "codec.h"
#include "dispatcher.h"
#include "query.pb.h"

#include "../EventLoop.h"
#include "../TcpServer.h"
#include "../logging/Logging.h"
#include "../thread/Mutex.h"

#include <boost/bind.hpp>
#include <stdio.h>
#include <unistd.h>

using namespace muduo;

typedef std::shared_ptr<muduo::Query> QueryPtr;
typedef std::shared_ptr<muduo::Answer> AnswerPtr;

class QueryServer {
  public:
    QueryServer(EventLoop *loop, const InetAddress &listenAddr)
        : server_(loop, listenAddr),
          dispatcher_(
              boost::bind(&QueryServer::onUnknownMessage, this, _1, _2, _3)),
          codec_(boost::bind(&ProtobufDispatcher::onProtobufMessage,
                             &dispatcher_, _1, _2, _3)) {
        dispatcher_.registerMessageCallback<muduo::Query>(
            boost::bind(&QueryServer::onQuery, this, _1, _2, _3));
        dispatcher_.registerMessageCallback<muduo::Answer>(
            boost::bind(&QueryServer::onAnswer, this, _1, _2, _3));
        server_.setConnectionCallback(
            boost::bind(&QueryServer::onConnection, this, _1));
        server_.setMessageCallback(
            boost::bind(&ProtobufCodec::onMessage, &codec_, _1, _2, _3));
    }

    void start() { server_.start(); }

  private:
    void onConnection(const TcpConnectionPtr &conn) {
        LOG_INFO << conn->peerAddr().toHostPort() << " -> "
                 << conn->localAddr().toHostPort() << " is "
                 << (conn->connected() ? "UP" : "DOWN");
    }

    void onUnknownMessage(const TcpConnectionPtr &conn,
                          const MessagePtr &message, Timestamp) {
        LOG_INFO << "onUnknownMessage: " << message->GetTypeName();
        conn->shutdown();
    }

    void onQuery(const muduo::TcpConnectionPtr &conn, const QueryPtr &message,
                 muduo::Timestamp) {
        LOG_INFO << "onQuery:\n"
                 << message->GetTypeName() << message->DebugString();
        Answer answer;
        answer.set_id(1);
        answer.set_questioner("Chen Shuo");
        answer.set_answerer("blog.csdn.net/Solstice");
        answer.add_solution("Jump!");
        answer.add_solution("Win!");
        codec_.send(conn, answer);

        conn->shutdown();
    }

    void onAnswer(const muduo::TcpConnectionPtr &conn, const AnswerPtr &message,
                  muduo::Timestamp) {
        LOG_INFO << "onAnswer: " << message->GetTypeName();
        conn->shutdown();
    }

    TcpServer server_;
    ProtobufDispatcher dispatcher_;
    ProtobufCodec codec_;
};

int main(int argc, char *argv[]) {
    LOG_INFO << "pid = " << getpid();
    if (argc > 1) {
        EventLoop loop;
        uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
        InetAddress serverAddr(port);
        QueryServer server(&loop, serverAddr);
        server.start();
        loop.loop();
    } else {
        printf("Usage: %s port\n", argv[0]);
    }
}
