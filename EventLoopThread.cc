#include "EventLoopThread.h"
#include "EventLoop.h"
#include "logging/Logging.h"
#include "thread/Thread.h"

#include <boost/bind.hpp>

using namespace muduo;

EventLoopThread::EventLoopThread()
    : loop_(NULL), exiting_(false),
      thread_(boost::bind(&EventLoopThread::threadFunc, this)), mutex_(),
      cond_(mutex_) {}
EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    loop_->quit();
    thread_.join();
}
EventLoop *EventLoopThread::startLoop() {
    assert(!thread_.started());
    thread_.start();
    {
        MutexLockGuard lock(mutex_);
        while (loop_ == NULL) {
            cond_.wait();
        }
    }
    return loop_;
}
void EventLoopThread::threadFunc() {
    EventLoop loop;
    {
        MutexLockGuard lock(mutex_);
        loop_ = &loop;
        cond_.notify();
    }
    loop_->loop();
}
