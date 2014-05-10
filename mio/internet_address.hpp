#pragma once

namespace mio {

struct InternetAddress {
private:
    struct sockaddr address_;

    static constexpr int WEB_PORT = 80;

    InternetAddress(struct sockaddr address) :
        address_(address)
        {}

public:
    static InternetAddress getAddressByIP(std::string ip, int port) {
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        address.sin_addr.s_addr = inet_addr(ip.c_str());
        memset(address.sin_zero, '\0', sizeof address.sin_zero);
        return InternetAddress(*((sockaddr *) &address));
    } 
    
    static InternetAddress getAddressByHostname(std::string hostname, int port = WEB_PORT) {
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_port = htons(WEB_PORT);

        struct hostent *h = gethostbyname(hostname.c_str());
        if (!h) {
            throw std::runtime_error("Failed to get host by name");
        }

        int result = inet_aton(inet_ntoa(*((struct in_addr *)h->h_addr)), &address.sin_addr);
        if (result <= 0) {
            throw std::runtime_error("Failed to get ip by host\n");
        }

        return InternetAddress(*((sockaddr *) &address));
    }

    struct sockaddr getAddress() const {
        return address_;
    }

    ~InternetAddress() {}
};

} // namespace mio
