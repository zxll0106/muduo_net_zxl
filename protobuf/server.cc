#include "sudoku.pb.h"

#include "../EventLoop.h"
#include "../logging/Logging.h"
#include "RpcServer.h"

#include <unistd.h>

using namespace muduo;

namespace sudoku {

class SudokuServiceImpl : public SudokuService {
  public:
    virtual void Solve(::google::protobuf::RpcController *controller,
                       const ::sudoku::SudokuRequest *request,
                       ::sudoku::SudokuResponse *response,
                       ::google::protobuf::Closure *done) {
        LOG_INFO << "SudokuServiceImpl::Solve";
        response->set_solved(true);
        response->set_checkerboard("1234567");
        done->Run();
    }
};

} // namespace sudoku

int main() {
    LOG_INFO << "pid = " << getpid();
    EventLoop loop;
    InetAddress listenAddr(9981);
    sudoku::SudokuServiceImpl impl;
    RpcServer server(&loop, listenAddr);
    server.registerService(&impl);
    server.start();
    loop.loop();
    google::protobuf::ShutdownProtobufLibrary();
}
