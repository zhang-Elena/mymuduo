#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory>

class EventLoop;
/*
理清楚 EventLoop、Channel、 Poller之间的关系 《= Reactor模型上对应Demultiplex
Channel 理解为通道，封装了sockfd和其感兴趣的event，如EPOLLIN、EPOLLOUT事件
还绑定了poller返回的具体事件
*/
class Channel : noncopyable
{
public:
    using EventCallBack = std::function<void()>;
    using ReadEventCallBack = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    //fd得到poller通知以后，处理事件。
    void handleEvent(Timestamp receiveTime);

    //设置回调函数对象
    void setReadCallback(ReadEventCallBack cb) {readCallback_ = std::move(cb);}
    void setWriteCallback(EventCallBack cb) { writeCallback_ = std::move(cb);}
    void setCloseCallback(EventCallBack cb) { closeCallback_ = std::move(cb);}
    void setErrorCallback(EventCallBack cb) { errorCallback_ = std::move(cb);}

    //防止channel被手动remove掉，channel还在执行回调操作
    void tie(const std::shared_ptr<void>&);


    int fd() const { return fd_;}
    int events() const { return events_;}
    //poller监听事件，设置channel
    int set_revents(int revt) { revents_ = revt; }

    //设置fd相应的事件状态
    void enableReading() { events_ |= kReadEvent; update();}
    void disableReading() { events_ &= ~kReadEvent; update();}
    void enableWriting() { events_ |= kWriteEvent; update();}
    void disableWriting() { events_ &= ~kWriteEvent; update();}
    void disableAll() { events_ = kNoneEvent; update();}

    bool isNoneEvent() const { return events_ == kNoneEvent;}
    bool isReading() const { return events_ & kReadEvent;}
    bool isWriting() const { return events_ & kWriteEvent;}

    int index() { return index_;}
    void set_index(int idx) { index_ = idx;}

    //one loop per thread
    EventLoop* ownerLoop() {return loop_;}
    void remove();

private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_; //事件循环
    const int fd_;  //fd，Poller监听的对象
    int events_;   //注册fd感兴趣的事情
    int revents_;  //poller返回的具体发生的事件
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_;

    //因为channel通道里面能够获知fd最终发生的具体事件revents，所以它负责调用具体事件的回调操作
    ReadEventCallBack readCallback_;
    EventCallBack writeCallback_;
    EventCallBack closeCallback_;
    EventCallBack errorCallback_;

};


