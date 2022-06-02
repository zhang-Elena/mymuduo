#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <sys/epoll.h>
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

//EventLoop:ChannelList Poller
Channel::Channel(EventLoop *loop, int fd)
    :loop_(loop) , fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
{
}

Channel::~Channel(){}

//channel的tie方法什么时候调用过
void Channel::tie(const std::shared_ptr<void> &obj){
    tie_ = obj;
    tied_ = true;
}

//更新通道
//作用：当改变channel所表示fd的events事件后，update负责在poller里面更改fd相应的事件epoll_ctl
//EventLoop => ChannelLIst Poller
void Channel::update(){
    //通过channel所属的EventLoop，调用poller的相应方法，注册fd的events事件
    //add code ...
    //loop_->updateChannel(this)
}

//删除通道
//在channel所属的EventLoop中，把当前的channel删除掉
void Channel::remove(){
    //add code
    //loop_->removeChannel(this);
}


void Channel::handleEvent(Timestamp receiveTime){
    if(tied_){
        //弱指针提升成为强指针
        std::shared_ptr<void> guard = tie_.lock();
        if(guard){
            handleEventWithGuard(receiveTime);
        }
    }
    else{
        handleEventWithGuard(receiveTime);
    }
}

//根据poller通知的channel发生的具体事件，由channel负责调用具体的回调事件
void Channel::handleEventWithGuard(Timestamp receiveTime){
    LOG_INFO("channel handleEvent revents:%d\n", revents_);

    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)){
        if(closeCallBack_){
            closeCallBack_();
        }
    }

    if(revents_ & EPOLLERR){
        if(errorCallBack_){
            errorCallBack_();
        }
    }

    if(revents_ & (EPOLLIN | EPOLLPRI)){
        if(readCallBack_){
            readCallBack_(receiveTime);
        }
    }

    if(revents_ & EPOLLOUT){
        if(writeCallBack_){
            writeCallBack_();
        }
    }
}