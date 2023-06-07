#ifndef MUDUO_NET_TIMER_H
#define MUDUO_NET_TIMER_H

#include "Callbacks.h"
#include "datetime/Timestamp.h"
#include "thread/Atomic.h"

#include <boost/noncopyable.hpp>

namespace muduo {
class Timer : boost::noncopyable {
  public:
    Timer(const TimerCallback &cb, Timestamp when, double interval);

    void run();
    Timestamp expiration() const { return expiration_; }
    bool repeat() const { return repeat_; }
    void restart(Timestamp now);
    int64_t sequence() const { return sequence_; }

  private:
    const TimerCallback callback_;
    Timestamp expiration_;
    const double interval_;
    const bool repeat_;
    const int64_t sequence_;
    static AtomicInt64 s_numCreated_;
};
} // namespace muduo

#endif