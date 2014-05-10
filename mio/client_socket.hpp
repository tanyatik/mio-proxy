#pragma once

#include "internet_address.hpp"

namespace mio {

class ClientSocket : public Socket {
private:
    void connectToHost(std::string hostname) {
        InternetAddress internet_address = InternetAddress::getAddressByHostname(hostname);

        auto c_address = internet_address.getAddress();
        int connect_result = ::connect(fd_, &c_address, sizeof(c_address));
        if (connect_result < 0) {
            if (errno != EINPROGRESS) {
                throw std::runtime_error("Failed to connect to address");
            }
        }
    }

public:
    ClientSocket(std::string hostname) :
        Socket(true) {
        connectToHost(hostname);
    }        
}; 

} // namespace mio
