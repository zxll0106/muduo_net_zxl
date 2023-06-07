#ifndef MUDUO_NET_EVENTLOOP_H
#define MUDUO_NET_EVENTLOOP_H

#include "Callbacks.h"
#include "TimerId.h"
#include "TimerQueue.h"
#include "datetime/Timestamp.h"
#include "thread/Condition.h"

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <vector>

namespace muduo {
class Poller;
class Channel;

class EventLoop : boost::noncopyable {
  public:
    typedef boost::function<void()> Functor;

    EventLoop();
    ~EventLoop();
    void loop();
    void assertInLoopThread();
    bool isInLoopThread();
    void quit();
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);

    void runInLoop(const Functor &cb);

    void queueInLoop(const Functor &cb);

    void wakeup();

    TimerId runAt(const Timestamp &time, const TimerCallback &cb) {
        return timerQueue_->addTimer(cb, time, 0.0);
    }

    TimerId runAfter(double delay, const TimerCallback &cb) {
        Timestamp time(addTime(Timestamp::now(), delay));
        return runAt(time, cb);
    }

    TimerId runEvery(double interval, const TimerCallback &cb) {
        Timestamp time(addTime(Timestamp::now(), interval));
        return timerQueue_->addTimer(cb, time, interval);
    }

    Timestamp pollReturnTime() const { return pollReturnTime_; }

    void cancel(TimerId timerId);

  private:
    void abortNotInLoopThread();
    void handleRead();
    void doPendingFunctors();

    typedef std::vector<Channel *> ChannelList;

    bool looping_;
    bool quit_;
    bool callingPendingFunctors_;
    const pid_t threadId_;
    Timestamp pollReturnTime_;
    boost::scoped_ptr<Poller> poller_;
    boost::scoped_ptr<TimerQueue> timerQueue_;
    int wakeupFd_;
    boost::scoped_ptr<Channel> wakeupChannel_;
    std::vector<Functor> pendingFunctors_;
    ChannelList activeChannels;
    MutexLock mutex_;
};
} // namespace muduo

#endif