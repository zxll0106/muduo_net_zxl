#include "codec.h"
#include "dispatcher.h"
#include "query.pb.h"

#include "../EventLoop.h"
#include "../TcpClient.h"
#include "../logging/Logging.h"
#include "../thread/Mutex.h"

#include <boost/bind.hpp>
#include <stdio.h>
#include <unistd.h>

using namespace muduo;

typedef std::shared_ptr<muduo::Empty> EmptyPtr;
typedef std::shared_ptr<muduo::Answer> AnswerPtr;

google::protobuf::Message *messageToSend;

class QueryClient {
  public:
    QueryClient(EventLoop *loop, const InetAddress &serverAddr)
        : loop_(loop), client_(loop, serverAddr),
          dispatcher_(
              boost::bind(&QueryClient::onUnknownMessage, this, _1, _2, _3)),
          codec_(boost::bind(&ProtobufDispatcher::onProtobufMessage,
                             &dispatcher_, _1, _2, _3)) {
        dispatcher_.registerMessageCallback<muduo::Answer>(
            boost::bind(&QueryClient::onAnswer, this, _1, _2, _3));
        dispatcher_.registerMessageCallback<muduo::Empty>(
            boost::bind(&QueryClient::onEmpty, this, _1, _2, _3));
        client_.setConnectionCallback(
            boost::bind(&QueryClient::onConnection, this, _1));
        client_.setMessageCallback(
            boost::bind(&ProtobufCodec::onMessage, &codec_, _1, _2, _3));
    }

    void connect() { client_.connect(); }

  private:
    void onConnection(const TcpConnectionPtr &conn) {
        LOG_INFO << conn->localAddr().toHostPort() << " -> "
                 << conn->peerAddr().toHostPort() << " is "
                 << (conn->connected() ? "UP" : "DOWN");

        if (conn->connected()) {
            codec_.send(conn, *messageToSend);
        } else {
            loop_->quit();
        }
    }

    void onUnknownMessage(const TcpConnectionPtr &, const MessagePtr &message,
                          Timestamp) {
        LOG_INFO << "onUnknownMessage: " << message->GetTypeName();
    }

    void onAnswer(const muduo::TcpConnectionPtr &, const AnswerPtr &message,
                  muduo::Timestamp) {
        LOG_INFO << "onAnswer:\n"
                 << message->GetTypeName() << message->DebugString();
    }

    void onEmpty(const muduo::TcpConnectionPtr &, const EmptyPtr &message,
                 muduo::Timestamp) {
        LOG_INFO << "onEmpty: " << message->GetTypeName();
    }

    EventLoop *loop_;
    TcpClient client_;
    ProtobufDispatcher dispatcher_;
    ProtobufCodec codec_;
};

int main(int argc, char *argv[]) {
    LOG_INFO << "pid = " << getpid();
    if (argc > 2) {
        EventLoop loop;
        uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
        InetAddress serverAddr(argv[1], port);

        muduo::Query query;
        query.set_id(1);
        query.set_questioner("Chen Shuo");
        query.add_question("Running?");
        muduo::Empty empty;
        messageToSend = &query;

        if (argc > 3 && argv[3][0] == 'e') {
            messageToSend = &empty;
        }

        QueryClient client(&loop, serverAddr);
        client.connect();
        loop.loop();
    } else {
        printf("Usage: %s host_ip port [q|e]\n", argv[0]);
    }
}
