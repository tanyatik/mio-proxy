#pragma once

#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <assert.h>

namespace mio {

class Socket {
protected:
    int fd_;
    bool have_resources_;

private:
    void makeNonblocking() {
        auto flags = fcntl (fd_, F_GETFL, 0);
        if (flags == -1) {
            throw std::runtime_error("Error in fcntl");
        }

        flags |= O_NONBLOCK;
        auto result = fcntl (fd_, F_SETFL, flags);
        if (result == -1) {
            throw std::runtime_error("Error in fcntl");
        }
    }

public:
    Socket(int fd, bool non_blocking = false) :
        fd_(fd),
        have_resources_(true) {
        if (non_blocking) {
            this->makeNonblocking();
        }
    }
   
    Socket(bool non_blocking = false) :
        fd_(0),
        have_resources_(true) {
        fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd_ < 0) {
            throw std::runtime_error("Failed to create socket");
        }
        if (non_blocking) {
            this->makeNonblocking();
        }
    }

    Socket(const Socket &) = delete;
    Socket &operator=(const Socket &) = delete;
    Socket(Socket &&other) {
        fd_ = other.fd_;
        other.have_resources_ = false;
        have_resources_ = true;
    }
    
    void close() {
        ::close(fd_);
        have_resources_ = false;
    }

    virtual ~Socket() {
        if (have_resources_) {
            ::close(fd_);
        }
    }
    
    int recv(char *buffer, int buffer_size) {
        assert(have_resources_);
        int result = ::recv(fd_, buffer, buffer_size, 0);
        if (result < 0) {
            result = -errno;
        }
        return result;
    }

    int write(char *buffer, int buffer_size) {
        assert(have_resources_);
        int result = ::write(fd_, buffer, buffer_size);
        if (result < 0) {
            result = -errno;
        }
        return result;
    } 

    // possible rakes :(
    int getDescriptor() const {
        return fd_;
    }

    bool operator == (int fd) {
        return fd_ == fd;
    }

    bool operator < (const Socket &other) {
        return fd_ < other.fd_;
    }
};

} // namespace mio
