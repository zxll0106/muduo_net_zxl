#ifndef MUDUO_NET_POLLER_H
#define MUDUO_NET_POLLER_H

#include "EventLoop.h"
#include "datetime/Timestamp.h"

#include <boost/noncopyable.hpp>
#include <map>
#include <vector>

struct pollfd;

namespace muduo {
class Channel;

class Poller : boost::noncopyable {
  public:
    typedef std::vector<Channel *> ChannelList;

    Poller(EventLoop *loop);
    ~Poller();

    Timestamp poll(int timeoutMs, ChannelList *activeChannels);

    void updateChannel(Channel *channel);
    void removeChannel(Channel *Channel);
    void assertInLoopThread() { ownerLoop_->assertInLoopThread(); }

  private:
    void fillActiveChannels(int numEvents, ChannelList *activeChannels);

    typedef std::vector<struct pollfd> PollFdList;
    typedef std::map<int, Channel *> ChannelMap;

    EventLoop *ownerLoop_;
    PollFdList pollfds_;
    ChannelMap channels_;
};

} // namespace muduo

#endif