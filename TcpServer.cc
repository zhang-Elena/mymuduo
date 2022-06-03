#include "TcpServer.h"
#include "Logger.h"

#include <functional>

EventLoop* checkLoopNotNull(EventLoop* loop){
    if(loop == nullptr){
        LOG_FATAL("%s:%s:%d mainLoop is null! \n",__FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr,
                     const std::string &nameArg, Option option)
                    : loop_(checkLoopNotNull(loop)),
                      inPort_(listenAddr.toIpPort()),
                      name_(nameArg), 
                      acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
                      threadPool_(new EventLoopThreadPool(loop,name_)),
                      connectionCallback_(), 
                      messageCallback_(),
                      nextConnId_(1)
{
    //当有新用户连接时，会执行TcpServer::newConnection回调
    //两个占位符表示客户端来的fd(connfd)和地址(peerAddr) <- Acceptor::handleRead 
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection,this,
                                        std::placeholders::_1, std::placeholders::_2));
}
TcpServer::~ TcpServer(){

}

//设置底层subLoop个数
void TcpServer::setThreadNum(int numThreads){
    threadPool_->setThreadNum(numThreads);
}

//开启服务器监听  loop.loop()
void TcpServer::start(){
    if(started_++ == 0) //防止一个TcpServer对象被start多次
    {
        //启动底层loop线程池
        threadPool_->start(threadInitCallback_);
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr){

}
void TcpServer::removeConnection(const TcpConnectionPtr& conn){

}
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn){

}