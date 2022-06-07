#pragma once

#include <unistd.h>
#include <sys/syscall.h>

//获取当前线程
namespace CurrentThread{
    extern __thread int t_catchedTid;

    void cacheTid();

    inline int tid(){
        if(__builtin_expect(t_catchedTid == 0, 0)){
            cacheTid();
        }
        return t_catchedTid;
    }
    
}