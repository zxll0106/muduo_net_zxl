#include "Channel.h"
#include "EventLoop.h"
#include "logging/Logging.h"

#include <poll.h>

using namespace muduo;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1),
      eventHanding_(false) {}

Channel::~Channel() { assert(!eventHanding_); }

void Channel::update() { loop_->updateChannel(this); }

void Channel::handleEvent(Timestamp receiveTime) {
    eventHanding_ = true;
    if (revents_ & POLLNVAL) {
        LOG_WARN << "Channel::handle_event() POLLNVAL";
    }
    if (revents_ & (POLLERR | POLLNVAL)) {
        if (errorCallback_) {
            errorCallback_();
        }
    }
    if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
        LOG_WARN << "Channel::handle_event() POLLHUP";
        if (closeCallback_) {
            closeCallback_();
        }
    }
    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
        if (readCallback_) {
            readCallback_(receiveTime);
        }
    }
    if (revents_ & POLLOUT) {
        if (writeCallback_) {
            writeCallback_();
        }
    }
    eventHanding_ = false;
}