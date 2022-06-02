#pragma once

#include "noncopyable.h"

#include <functional>
#include <string>
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread;
class EventThreadLoop :noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventThreadLoop(EventLoop *baseLoop, const std::string &nameArg);
    ~EventThreadLoop();

    void setThreadNum(int numThreads) {numThreads_ = numThreads;}

    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    //如果工作在多线程中，baseLoop默认以轮询方式分配channel给subLoop
    EventLoop* getNextLoop();

    std::vector<EventLoop*> getAllLoops();

    bool started() const {return started_;}
    const std::string& name() const {return name_;}
private:
    
    
    EventLoop *baseLoop_; //EventLoop loop;
    std::string name_;
    bool started_;
    int numThreads_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_; //包含所有创建事件的线程
    std::vector<EventLoop*> loops_;  //包含所有线程里的eventloop指针
};