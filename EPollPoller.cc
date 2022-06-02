#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <errno.h>
#include <unistd.h>
#include <string.h>
 //一个channel从来没有添加到poller
const int kNew = -1;  //channel的成员index_=-1;
//channel已经添加到epollpoller
const int kAdded = 1; 
//channel已经从poller删除
const int kDeleted = 2; 

EPollPoller::EPollPoller(EventLoop *loop) 
            :Poller(loop), epollfd_(::epoll_create1(EPOLL_CLOEXEC)), events_(kInitEventListSize)  
{
    if(epollfd_ < 0){
        LOG_FATAL("epoll_create error:%d \n", errno);
    }

}
EPollPoller::~EPollPoller(){
    ::close(epollfd_);
}

//channel update remove =》EventLoop updateChannel  removeChannel => Poller updateChannel removeChannel
/*
*          EventLoop=》 poller.poll
    ChannelList      Poller
                     ChannelMap <fd, channel*> epollfd
*/
void EPollPoller::updateChannel(Channel *channel){
    //index对应上面三种状态
    const int index = channel->index();
    LOG_INFO("func=%s fd=%d events=%d index=%d",__FUNCTION__, channel->fd(), channel->events(),index);

    if(index == kNew || index == kDeleted){
        if( index == kNew){
            int fd = channel->fd();
            channels_[fd] = channel;
        }

        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    //channel已经在poller上注册过了
    else{
        int fd = channel->fd();
        if(channel->isNoneEvent()){
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else{
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

//从poller中删除channel
void EPollPoller::removeChannel(Channel *channel){
    int fd = channel->fd();
    channels_.erase(fd);

    LOG_INFO("func=%s fd=%d", __FUNCTION__, fd);
    
    int index = channel->index();
    if(index == kAdded){
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

//一个线程就是一个eventloop
//通过epoll_wait把发生事件的参数告知给eventloop
Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels){
    //实际上应该用log_debug输出日志更为合理
    LOG_INFO("func=%s => fd total count:%lu\n",__FUNCTION__, channels_.size());

    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());

    //
    if(numEvents > 0){
        LOG_INFO("%d evets happened \n", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        //存储不够，扩容
        if(numEvents == events_.size()){
            events_.resize(events_.size() * 2);
        }
    }
    //这一轮监听过程没有事件,只是因为超时结束
    else if(numEvents == 0){
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    }
    else{
        if(saveErrno != EINTR){
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() err!");
        }
    }
    return now;
}

//填写活跃的连接
void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const{
    for(int i = 0; i < numEvents; ++i) {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel); //EventLoop就拿到了他的poller给他返回的所有发生事件的channel列表了
    }
}
//更新Channel通道 epoll_ctl add/mod/del
void EPollPoller::update(int operation, Channel* channel){
    epoll_event event;
    bzero(&event, sizeof event);

    int fd = channel->fd();
    event.events = channel->events();
    event.data.fd = fd;
    event.data.ptr = channel;

    if(::epoll_ctl(epollfd_, operation, fd, &event) < 0){
        if(operation == EPOLL_CTL_DEL){
            LOG_ERROR("epoll_ctl delete error:%d\n", errno);
        }
        else{
            LOG_FATAL("epoll_ctl delete error:%d\n", errno);
        }
    }

}
