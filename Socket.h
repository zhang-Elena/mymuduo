#pragma once

#include "noncopyable.h"

class InetAddress;
//封装socket
class Socket : noncopyable
{
public:
    explicit Socket(int sockfd) : sockfd_(sockfd){}
    ~Socket();

    int fd() {return sockfd_;}
    void bindAddress(const InetAddress &localaddr);
    void listen();
    int accept(InetAddress *peeraddr);

    void shutdownWrite();

    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlice(bool on);

private:
    const int sockfd_;
   
};