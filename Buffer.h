#pragma once 

#include <vector>
#include <stddef.h>
#include <string>
#include <algorithm>
/**
 * 网络库底层的缓冲器类型定义
 * prependable bytes     readable bytes          writeable bytes
 *                         (CONTENT)
 * 
 * 0       <=     readerIndex   <=     writerIndex    <=         size
 */

class Buffer
{
public:
    static const size_t kCheapPrepend = 8; 
    static const size_t kInitialSize = 1024;

    /**
     * 默认给buffer开辟的长度为8+1024
     * readerIndex_ 和 writerIndex_初始都指向同一位置
    */
    explicit Buffer(size_t initialSize = kInitialSize)
     : buffer_(kCheapPrepend + initialSize),
       readerIndex_(kCheapPrepend),
       writerIndex_(kCheapPrepend)
    {

    }

    size_t readableBytes() const{
        return writerIndex_ - readerIndex_;
    }

    size_t writableBytes() const{
        return buffer_.size() - writerIndex_;
    }

    size_t prependableBytes() const{
        return readerIndex_;
    }

    //返回缓冲区中可读数据的起始地址
    const char* peek() const{
        return begin() + readerIndex_;
    }

    //onMessage string <- buffer
    void retrieve(size_t len){
        if(len < readableBytes()){
            //应用只读取了可读缓冲区数据的一部分len，还剩下readerIndex_ += len -> writerIndex_
            readerIndex_ += len;
        }
        //len == readableBytes()
        else{
            retrieveAll();
        }
    }

    void retrieveAll(){
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    //把onMessage函数上报的Buffer数据，转成string类型的数据返回
    std::string retrieveAllAsString(){
        //应用可读取数据的长度
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len){
        std::string result(peek(), len);
        retrieve(len); //上面一句把缓冲区中可读数据已经读取，这里肯定对缓冲区进行复位
        return result;
    }

    //buffer_.size() - writeIndex_ 
    void ensureWriteableBytes(size_t len){
        if(writableBytes() < len){
            //扩容
            makeSpace(len);
        }
    }

    //把[data, data+len]内存上的数据，添加到writeable缓冲区中
    void append(const char *data, size_t len){
        ensureWriteableBytes(len);
        std::copy(data, data+len, beginWrite());
        writerIndex_ += len;
        
    }

    char* beginWrite(){
        return begin()+writerIndex_;
    }
    const char* beginWrite() const{
        return begin() + writerIndex_;
    }

    //从fd上读取数据
    ssize_t readFd(int fd, int* saveErrno);

    ssize_t writeFd(int fd, int* saveErrno);
private:
    char* begin(){
        //调用迭代器*号重载函数，访问底层元素本身，取地址又返回数据的地址
        return &*buffer_.begin();//vector底层数组首元素的地址，也就是数组的起始地址
    }
    const char* begin() const{
        return &*buffer_.begin();
    }

    void makeSpace(size_t len){
        /**
         * 注意readerIndex_不永远是8
         * kCheapPrepend | reader | writer  |
         * kCheapPrepend |            len                      |
        */
        if(writableBytes() + prependableBytes() < len + kCheapPrepend){
            ///不管前面空闲的空间，直接可以括后面的
            buffer_.resize(writerIndex_ + len);
        }
        else{
            size_t readable = readerIndex_;
            std::copy(begin() + readerIndex_, begin() + writerIndex_, begin()+kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};