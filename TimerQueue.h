#ifndef MUDUO_NET_TIMERQUEUE_H
#define MUDUO_NET_TIMERQUEUE_H

#include "Callbacks.h"
#include "Channel.h"
#include "datetime/Timestamp.h"

#include <boost/noncopyable.hpp>
#include <set>
#include <utility>
#include <vector>

namespace muduo {
class EventLoop;
class Timer;
class TimerId;

class TimerQueue : boost::noncopyable {
  public:
    TimerQueue(EventLoop *loop);
    ~TimerQueue();

    TimerId addTimer(const TimerCallback &cb, Timestamp when, double interval);
    void cancel(TimerId timerId);

  private:
    typedef std::pair<Timestamp, Timer *> Entry;
    typedef std::set<Entry> TimerList;
    typedef std::pair<Timer *, int64_t> ActiveTimer;
    typedef std::set<ActiveTimer> ActiveTimerSet;

    void cancelInLoop(TimerId timerId);
    void addTimerInLoop(Timer *timer);
    void handleRead();

    std::vector<Entry> getExpired(Timestamp now);

    void reset(std::vector<Entry> &expired, Timestamp now);

    bool insert(Timer *timer);

    EventLoop *loop_;
    const int timerfd_;
    Channel timerfdChannel_;
    TimerList timers_;

    bool callingExpiredTimers_;
    ActiveTimerSet activeTimers_;
    ActiveTimerSet cancelingTimers_;
};
} // namespace muduo

#endif