#ifndef MUDUO_NET_INETADDRESS_H
#define MUDUO_NET_INETADDRESS_H

#include "datetime/copyable.h"

#include <netinet/in.h>
#include <string>

namespace muduo {
class InetAddress : public muduo::copyable {
  public:
    explicit InetAddress(uint16_t port);
    InetAddress(const std::string &ip, uint16_t port);
    InetAddress(const sockaddr_in &addr) : addr_(addr) {}

    std::string toHostPort() const;

    const sockaddr_in &getSockAddrInet() const { return addr_; }

    void setSockAddrInet(const sockaddr_in addr) { addr_ = addr; }

  private:
    struct sockaddr_in addr_;
};
} // namespace muduo

#endif