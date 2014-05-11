#pragma once

#include <memory>

#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>

#include "socket.hpp"
#include "internet_address.hpp"

namespace mio {

class ServerSocket : public Socket {
private:
    bool non_blocking_;

    int setReuseAddress() {
        int yes = 1;
        if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 ) {
            throw std::runtime_error("Failed to set socket options");
        }
        return fd_;
    }

    void bindToAddress(const InternetAddress &internet_address) {
        struct sockaddr address = internet_address.getAddress();
        int bound = ::bind(fd_, (struct sockaddr *) &address, sizeof(address));
        if (bound == -1) {
            throw std::runtime_error("Failed to bind socket");
        }
    }

    void listenTo() {
        int listen_result = ::listen(fd_, 4096);
        if (listen_result == -1) {
            throw std::runtime_error("Failed to listen");
        } 
    }

public:
    ServerSocket(std::string ip, int port, bool non_blocking = true) :
        Socket(true),
        non_blocking_(non_blocking) {
        setReuseAddress();
        bindToAddress(InternetAddress::getAddressByIP(ip, port));
        listenTo();
    }

    std::shared_ptr<Socket> acceptNewConnection() {
        struct sockaddr internet_address;
        memset(&internet_address, 0, sizeof(internet_address));
        socklen_t length = 0;
        
        int new_fd = ::accept(fd_, &internet_address, &length);
        if (new_fd == -1) { 
            if (non_blocking_ && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                char *ip_buffer = inet_ntoa(((sockaddr_in *) &internet_address)->sin_addr);
                ///std::cerr << "Accept from host " << ip_buffer << std::endl;

                return std::shared_ptr<Socket>(nullptr);
            } else {
                throw std::runtime_error("Failed to accept socket");
            }
        }

        return std::make_shared<Socket>(new_fd, non_blocking_);
    }
};

} // namespace mio
