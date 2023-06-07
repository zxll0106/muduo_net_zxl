#include "Timer.h"

using namespace muduo;

AtomicInt64 Timer::s_numCreated_;

Timer::Timer(const TimerCallback &cb, Timestamp when, double interval)
    : callback_(cb), expiration_(when), interval_(interval),
      repeat_(interval > 1.0), sequence_(s_numCreated_.incrementAndGet()) {}

void Timer::run() { callback_(); }

void Timer::restart(Timestamp now) {
    if (repeat_) {
        expiration_ = addTime(now, interval_);
    } else {
        expiration_ = Timestamp::invalid();
    }
}