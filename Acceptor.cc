#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

static int createNonblocking(){
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if(sockfd < 0){
        LOG_FATAL("%s:%s:%d listen socket create err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
}

Acceptor::Acceptor(EventLoop* loop, const InetAddress &listenAddr, bool reuseport)
        :loop_(loop),acceptSocket_(createNonblocking()),acceptChannel_(loop, acceptSocket_.fd()),
        listening_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr); //bind
    //TcpServer::start() Acceptor::listen()
    //若有新用户的连接，要执行一个回调 (connfd=》 channel=》subloop)
    //baseLoop=> acceptChannel_(listenfd)
    acceptChannel_.setReadCallBack(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor(){
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}


void Acceptor::listen(){
    listening_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

//listenfd有事件发生，就是有新用户连接
void Acceptor::handleRead(){
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if(connfd >= 0){
        if(newConnectionCallback_){
            //轮询找到subloop，唤醒，分发当前的新客户端的channel
            newConnectionCallback_(connfd, peerAddr);
        }
        else{
            ::close(connfd);
        }
    }
    else{
        LOG_ERROR("%s:%s:%d accept err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        if(errno == EMFILE){
            //单台服务器不支持现有流量，代表要进行集群
            LOG_ERROR("%s:%s:%d sockfd reached limit!\n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}