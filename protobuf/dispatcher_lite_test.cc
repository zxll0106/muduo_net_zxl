#include "dispatcher_lite.h"

#include "query.pb.h"

#include "../Callbacks.h"
#include "../TcpConnection.h"

#include <iostream>

using std::cout;
using std::endl;

void onUnknownMessageType(const muduo::TcpConnectionPtr &conn,
                          const MessagePtr &message, muduo::Timestamp t) {
    cout << "onUnknownMessageType: " << message->GetTypeName() << endl;
}

void onQuery(const muduo::TcpConnectionPtr &conn, const MessagePtr &message,
             muduo::Timestamp t) {
    cout << "onQuery: " << message->GetTypeName() << endl;
    std::shared_ptr<muduo::Query> query =
        muduo::down_pointer_cast<muduo::Query>(message);
    assert(query != NULL);
}

void onAnswer(const muduo::TcpConnectionPtr &conn, const MessagePtr &message,
              muduo::Timestamp t) {
    cout << "onAnswer: " << message->GetTypeName() << endl;
    std::shared_ptr<muduo::Answer> answer =
        muduo::down_pointer_cast<muduo::Answer>(message);
    assert(answer != NULL);
}

int main() {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    ProtobufDispatcherLite dispatcher(onUnknownMessageType);
    dispatcher.registerMessageCallback(muduo::Query::descriptor(), onQuery);
    dispatcher.registerMessageCallback(muduo::Answer::descriptor(), onAnswer);

    muduo::TcpConnectionPtr conn;
    muduo::Timestamp t;

    std::shared_ptr<muduo::Query> query(new muduo::Query);
    std::shared_ptr<muduo::Answer> answer(new muduo::Answer);
    std::shared_ptr<muduo::Empty> empty(new muduo::Empty);
    dispatcher.onProtobufMessage(conn, query, t);
    dispatcher.onProtobufMessage(conn, answer, t);
    dispatcher.onProtobufMessage(conn, empty, t);

    google::protobuf::ShutdownProtobufLibrary();
}
