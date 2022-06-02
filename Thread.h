#pragma once

#include "noncopyable.h"

#include <functional>
#include <thread>
#include <memory>
#include <string>
#include <atomic>


class Thread : noncopyable{
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc, const std::string &name = std::string());
    ~Thread();

    void start();
    void join();

    bool started() const {return started_;}
    //返回线程tid 返回的线程是linux用top命令打印出来的，不是pthread self打印出来的，不是真正的线程号
    pid_t tid() const {return tid_;}
    const std::string& name() {return name_;}

    static int numCreated() {return numCreated_;}

private:
    void setDefaultName();

    bool started_;
    bool joined_;
    std::shared_ptr<std::thread> thread_;
    pid_t tid_;
    ThreadFunc func_;
    std::string name_;
    static std::atomic<std::int32_t> numCreated_;


};