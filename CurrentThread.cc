#include "CurrentThread.h"



namespace CurrentThread{
    __thread int t_catchedTid = 0;

    void cacheTid(){
        if(t_catchedTid == 0){
            //通过linux系统调用，获取当前线程的tid值
            t_catchedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}