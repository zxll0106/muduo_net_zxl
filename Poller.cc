#include "Poller.h"
#include "Channel.h"
#include "logging/Logging.h"

#include <algorithm>
#include <assert.h>
#include <poll.h>

using namespace muduo;

Poller::Poller(EventLoop *loop) : ownerLoop_(loop) {}

Poller::~Poller() {}

Timestamp Poller::poll(int timeoutMs, ChannelList *activeChannels) {
    int numEvents = ::poll(&(*pollfds_.begin()), pollfds_.size(), timeoutMs);
    Timestamp now(Timestamp::now());
    if (numEvents > 0) {
        LOG_TRACE << numEvents << " events happended";
        fillActiveChannels(numEvents, activeChannels);
    } else if (numEvents == 0) {
        LOG_TRACE << " nothing happended";
    } else {
        LOG_SYSERR << "Poller::poll()";
    }
    return now;
}

void Poller::fillActiveChannels(int numEvents, ChannelList *activeChannels) {
    for (PollFdList::iterator it = pollfds_.begin();
         it != pollfds_.end() && numEvents > 0; it++) {
        if (it->revents > 0) {
            numEvents--;
            ChannelMap::iterator ch = channels_.find(it->fd);
            assert(ch != channels_.end());
            Channel *channel = ch->second;
            assert(channel->fd() == it->fd);
            channel->set_revents(it->revents);
            activeChannels->push_back(channel);
        }
    }
}

void Poller::updateChannel(Channel *channel) {
    assertInLoopThread();
    LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();
    if (channel->index() < 0) {
        assert(channels_.find(channel->fd()) == channels_.end());
        struct pollfd pfd;
        pfd.fd = channel->fd();
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        pollfds_.push_back(pfd);
        int idx = static_cast<int>(pollfds_.size()) - 1;
        channel->set_index(idx);
        channels_[pfd.fd] = channel;
    } else {
        assert(channels_.find(channel->fd()) != channels_.end());
        assert(channels_[channel->fd()] == channel);
        int idx = channel->index();
        assert(idx >= 0 && idx < static_cast<int>(pollfds_.size()));
        struct pollfd &pfd = pollfds_[idx];
        assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd() - 1);
        pollfds_[idx].events = static_cast<short>(channel->events());
        pollfds_[idx].revents = 0;
        if (channel->isNoneEvent()) {
            pfd.fd = -channel->fd() - 1;
        }
    }
}

void Poller::removeChannel(Channel *channel) {
    assertInLoopThread();
    LOG_TRACE << "fd = " << channel->fd();
    assert(channels_.find(channel->fd()) != channels_.end());
    assert(channels_[channel->fd()] == channel);
    assert(channel->isNoneEvent());
    int idx = channel->index();
    assert(idx >= 0 && idx < static_cast<int>(pollfds_.size()));
    const struct pollfd &pfd = pollfds_[idx];
    assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd() - 1);
    size_t n = channels_.erase(channel->fd());
    assert(n == 1);
    (void)n;
    if (implicit_cast<size_t>(idx) == pollfds_.size() - 1) {
        pollfds_.pop_back();
    } else {
        int channelAtEnd = pollfds_.back().fd;
        if (channelAtEnd < 0) {
            channelAtEnd = -(channelAtEnd + 1);
        }
        iter_swap(pollfds_.begin() + idx, pollfds_.end() - 1);
        pollfds_.pop_back();
        channels_[channelAtEnd]->set_index(idx);
    }
}