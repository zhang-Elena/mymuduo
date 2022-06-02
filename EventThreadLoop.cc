#include "EventThreadLoop.h"
#include "EventLoopThread.h"

#include <memory>

EventThreadLoop::EventThreadLoop(EventLoop *baseLoop, const std::string &nameArg)
                :baseLoop_(baseLoop), name_(nameArg), started_(false), numThreads_(0),next_(0)
{

}
EventThreadLoop::~EventThreadLoop(){

}


void EventThreadLoop::start(const ThreadInitCallback &cb){
    started_ = true;

    for(int i = 0; i < numThreads_; ++i){
        char buf[numThreads_ + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
        EventLoopThread *t = new EventLoopThread(cb, buf);
        threads_.emplace_back(std::unique_ptr<EventLoopThread>(t));
        loops_.emplace_back(t->startloop()); //底层创建线程，绑定一个新的EventLoop，并返回该loop的地址
    }

    //整个服务端只有一个线程，运行着baseLoop
    if(numThreads_ == 0 && cb){
        cb(baseLoop_);
    }
}

//如果工作在多线程中，baseLoop默认以轮询方式分配channel给subLoop
EventLoop* EventThreadLoop::getNextLoop(){
    EventLoop *loop = baseLoop_;

    //有独立事件循环
    //通过轮询获取下一个处理事件的loop
    if(!loops_.empty()){
        loop = loops_[next_];
        ++next_;
        if(next_ >= loops_.size()){
            next_ = 0;
        }
    }
    return loop;
}

std::vector<EventLoop*> EventThreadLoop::getAllLoops(){
    if(loops_.empty()){
        return std::vector<EventLoop*>(1,baseLoop_);
    }
    else{
        return loops_;
    }
}