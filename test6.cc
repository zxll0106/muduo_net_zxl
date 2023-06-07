#include "EventLoop.h"
#include "EventLoopThread.h"
#include "TimerId.h"
#include "logging/Logging.h"
#include <stdio.h>

void runInThread() {
    printf("runInThread(): pid = %d, tid = %d\n", getpid(),
           muduo::CurrentThread::tid());
}

int main() {
    muduo::Logger::setLogLevel(muduo::Logger::LogLevel::TRACE);
    printf("main(): pid = %d, tid = %d\n", getpid(),
           muduo::CurrentThread::tid());

    // muduo::EventLoop baseloop;
    muduo::EventLoopThread loopThread;
    muduo::EventLoop *loop = loopThread.startLoop();
    loop->runInLoop(runInThread);
    sleep(1);
    loop->runAfter(2, runInThread);
    // baseloop.cancel(timer);
    sleep(3);
    loop->quit();

    printf("exit main().\n");
}
