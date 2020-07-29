//
// Created by 杨成进 on 2020/7/24.
//

#include "Buffer.h"

Buffer::Buffer(size_t initBufferSize) : buffer_(initBufferSize), readPos_(0), writePos_(0) {
}

size_t Buffer::WritableBytes() const {
    return buffer_.size() - writePos_;
}

size_t Buffer::ReadableBytes() const {
    return writePos_ - readPos_;
}

size_t Buffer::PrependableBytes() const {
    return readPos_;
}

const char *Buffer::Peek() const {
    return BeginPtr_() + readPos_;
}

void Buffer::EnsureWritable(size_t len) {
    if (WritableBytes() < len)
        MakeSpace_(len);
    assert(WritableBytes() >= len);
}

void Buffer::HasWritten(size_t len) {
    writePos_ += len;
}

void Buffer::Retrieve(size_t len) {
    assert(len <= ReadableBytes());
    readPos_ += len;
}

void Buffer::RetrieveUntil(const char *end) {
    assert(Peek() <= end);
    Retrieve(end - Peek());
}

//回收所有缓冲区空间
void Buffer::RetrieveAll() {
    bzero(&buffer_[0], buffer_.size());
    readPos_ = 0;
    writePos_ = 0;
}

std::string Buffer::RetrieveAllToStr() {
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

const char *Buffer::BeginWriteConst() const {
    return BeginPtr_() + writePos_;
}

char *Buffer::BeginPtr_() {
    return &*buffer_.begin();
}

const char *Buffer::BeginPtr_() const {
    return &*buffer_.begin();
}

//腾出空间
void Buffer::MakeSpace_(size_t len) {
    if (WritableBytes() + PrependableBytes() < len)
        buffer_.resize(writePos_ + len + 1);
    else{
        size_t readable = ReadableBytes();
        std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
        readPos_ = 0;
        writePos_ = readPos_ + readable;
        assert(readable == ReadableBytes());
    }
}

char *Buffer::BeginWrite() {
    return BeginPtr_() + writePos_;
}

void Buffer::Append(const std::string &str) {
    Append(str.data(), str.length());
}

void Buffer::Append(const char *str, size_t len) {
    assert(str);
    EnsureWritable(len);
    std::copy(str, str + len, BeginPtr_());
}

void Buffer::Append(const void *data, size_t len) {
    assert(data);
    Append(static_cast<const char *>(data), len);
}

void Buffer::Append(const Buffer &buff) {
    Append(buff.Peek(), buff.ReadableBytes());
}

//从文件描述符读入缓冲区
ssize_t Buffer::ReadFd(int fd, int *saveErrno) {
    char buff[65535];
    struct iovec iov[2];
    const size_t writable = WritableBytes();
    iov[0].iov_base = BeginPtr_() + writePos_;
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    //ssize_t readv(int fd, const struct iovec *vector, int count);
    //将数据从文件描述符读到分散的内存块中；
    const size_t len = readv(fd, iov, 2);
    if (len < 0)
        *saveErrno = errno;
    else if (static_cast<int>(len) <= writable)
        writePos_ += len;
    else {
        writePos_ = buffer_.size();
        Append(buff, len - writePos_);
    }
    return len;
}

//将缓冲区写入描述符
ssize_t Buffer::WriteFd(int fd, int *saveErrno) {
    size_t readSize = ReadableBytes();
    ssize_t len = write(fd, Peek(), readSize);
    if (len < 0) {
        *saveErrno = errno;
        return len;
    }
    readPos_ += len;
    return len;
}

