#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"
#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

//防止一个线程创建多个EventLoop
//__：thread_local
__thread EventLoop *t_loopInThisThread = nullptr;

//定义默认的poller IO复用接口的超时时间
const int kPollTimeMs = 10000;

//创建wakeupfd， 用来notify唤醒subReactor处理新来的channel
int createEventFd(){
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    //没办法通知
    if(evtfd < 0){
        LOG_FATAL("eventfd error:%d", errno);
    }
    return evtfd;
}

EventLoop::EventLoop(): looping_(false), quit_(false), callingPendingFunctors_(false),
                       threadId_(CurrentThread::tid()), poller_(Poller::newDefaultPoller(this)),
                       wakeupFd_(createEventFd()), wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG_DEBUG("EventLoop created %p in thread %d\n", this, threadId_);
    if(t_loopInThisThread){
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
    }
    else{
        t_loopInThisThread = this;
    }

    //设置wakeup的事件类型以及发生事件后的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    //每一个eventloop都将监听wakeupchannel的EPOLLIN读事件
    wakeupChannel_->enableReading();

}

EventLoop::~EventLoop(){
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop(){
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);

    while(!quit_){
        activeChannels_.clear();
        //监听两类fd 一种时client的fd，一种wakeupfd
        pollReturnTime_ = poller_->poll(kPollTimeMs,&activeChannels_);
        for(Channel* channel : activeChannels_){
            //poller监听哪些channel发生事件了，然后上报给EventLoop,通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        //执行当前EventLoop事件循环需要处理的回调操作
        /*
        *  IO线程 主用户只监听新用户连接 mainLoop accept fd <= channel 已连接新用户分发给subloop
        *  如果没有调用过muduo库的setthreadnumber 就只有一个loop=》mainloop 
        *  mainloop 不仅监听新用户连接还有负责已连接用花读写事件
        *  若调用setthreadnumber作为服务器，会起三个（四核）subloop 
        *  mainLoop 事先注册一个回调cb（需要subloop执行）  wakeup subloop后执行下面的方法，执行之前mainloop注册的cb操作
        */
        doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping. \n",this);
    looping_ = false;
}

/**退出事件循环
 * 1. loop在自己的线程中调用quit
 * 2. 在非loop的线程中，调用loop的quit eg：在一个subloop(woker)中，调用了mainLoop(IO)的quit
 * =》结束其他loop需要先把其他loop唤醒
 * 每一个loop里都有wakeupfd_是可以唤醒对应loop
 *                mainLoop
 * 
 *      muduo库：通过wakeupfd进行唤醒             ===============生产者-消费者的线程安全的队列(muduo库没)
 * 
 * subLoop1       subLoop2      subLoop3
*/
void EventLoop::quit(){
    quit_ = true;

    //isInLoopThread:mainloop
    if(!isInLoopThread()){
        wakeup();
    }
}

//在当前loop中执行cb
void EventLoop::runInLoop(Functor cb){
    //在当前loop线程中执行cb
    if(isInLoopThread()){
        cb();
    }
    //在非loop线程中执行cb,就需要唤醒loop所在线程，执行cb
    else{
        queueInLoop(cb);
    }
}


//把cb放入队列中，唤醒loop所在线程，执行cb
void EventLoop::queueInLoop(Functor cb){
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }
    
    //唤醒相应的，需要执行上面回调操作的loop的线程
    //callingPendingFunctors_：当前loop正在执行回调，但是loop又有新的回调，所以需要唤醒
    if(!isInLoopThread() || callingPendingFunctors_){
        //唤醒loop所在线程
        wakeup();
    }
}

//用来唤醒loop所在线程 向wakeupfd_写一个数据, wakeupChannel就发生读事件，当前loop线程就会被唤醒
void EventLoop::wakeup(){
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    //写数据写错
    if(n != sizeof one){
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n",n);
    }
}

 //EventLoop的方法-》Poller的方法
void EventLoop::updateChannel(Channel* channel){
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel){
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel){
    return poller_->hasChannel(channel);
}

void EventLoop::handleRead(){
    uint16_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if(n != sizeof one){
        LOG_ERROR("EventLoop::handleRead() reads %d bytes instead of 8", n);
    }
}

//执行回调
void EventLoop::doPendingFunctors(){
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for(const Functor &functor : functors){
        functor(); //执行当前loop需要执行的回调操作
    }
    callingPendingFunctors_ = false;
}

