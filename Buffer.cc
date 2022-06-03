#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>
/**
 * 从fd上读取数据，Poller工作在LT模式（数据没读取完会不断上报，好处为数据不会丢失）
 * Buffer缓冲区是有大小的  但是从fd上读取数据的时候，却不知道tcp数据最终的大小
*/
ssize_t Buffer::readFd(int fd, int* saveErrno){
    char extrabuf[65536] = {0}; //栈上的内存空间
    struct iovec vec[2];
    //两块缓冲区
    const size_t writable = writableBytes(); //这是Buffer底层缓冲区剩余的可写空间大小
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;
    //缓冲区不够，就自动添加到栈上
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;
    //不够64k的空间
    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec,iovcnt);
    if(n < 0){
        *saveErrno = errno;
    }
    //buffer的可写缓冲区够存
    else if(n <= writable){
        writerIndex_ += n;
    }
    //extrabuf里面也写入了数据
    else{
        writerIndex_ = buffer_.size();
        //writerIndex_开始写n - writable大小的数据
        append(extrabuf, n - writable);
    }
    return n;
}
