#include "sudoku.pb.h"

#include "../EventLoop.h"
#include "../InetAddress.h"
#include "../TcpClient.h"
#include "../TcpConnection.h"
#include "../logging/Logging.h"
#include "RpcChannel.h"

#include <boost/bind.hpp>
#include <stdio.h>
#include <unistd.h>

using namespace muduo;

class RpcClient {
  public:
    RpcClient(EventLoop *loop, const InetAddress &serverAddr)
        : loop_(loop), client_(loop, serverAddr), channel_(new RpcChannel),
          stub_(channel_.get()) {
        client_.setConnectionCallback(
            boost::bind(&RpcClient::onConnection, this, _1));
        client_.setMessageCallback(
            boost::bind(&RpcChannel::onMessage, channel_.get(), _1, _2, _3));
        // client_.enableRetry();
    }

    void connect() { client_.connect(); }

  private:
    void onConnection(const TcpConnectionPtr &conn) {
        if (conn->connected()) {
            // channel_.reset(new RpcChannel(conn));
            channel_->setConnection(conn);
            sudoku::SudokuRequest request;
            request.set_checkerboard("001010");
            sudoku::SudokuResponse *response = new sudoku::SudokuResponse;

            stub_.Solve(NULL, &request, response,
                        NewCallback(this, &RpcClient::solved, response));
        } else {
            loop_->quit();
        }
    }

    void solved(sudoku::SudokuResponse *resp) {
        LOG_INFO << "solved:\n" << resp->DebugString();
        client_.disconnect();
    }

    EventLoop *loop_;
    TcpClient client_;
    RpcChannelPtr channel_;
    sudoku::SudokuService::Stub stub_;
};

int main(int argc, char *argv[]) {
    LOG_INFO << "pid = " << getpid();
    if (argc > 1) {
        EventLoop loop;
        InetAddress serverAddr(argv[1], 9981);

        RpcClient rpcClient(&loop, serverAddr);
        rpcClient.connect();
        loop.loop();
    } else {
        printf("Usage: %s host_ip\n", argv[0]);
    }
    google::protobuf::ShutdownProtobufLibrary();
}
