#pragma once 

#include "socket.hpp"

namespace mio {

class AsyncReader : public Reader {
private:
    std::weak_ptr<Socket> socket_;
    std::shared_ptr<InputProtocol> protocol_;

    static constexpr size_t BUFFER_SIZE = 4096;

public:
    AsyncReader(std::weak_ptr<Socket> socket, 
            std::shared_ptr<InputProtocol> protocol) :
        socket_(socket),
        protocol_(protocol)
        {}

    virtual bool read() {
        char buffer[BUFFER_SIZE];

        std::shared_ptr<Socket> socket = socket_.lock();
        if (!socket) {
            return true;
        }
    
        while (true) {
            memset(buffer, 0, BUFFER_SIZE);

            auto recv_result = socket->recv(buffer, sizeof(buffer));
            if (recv_result > 0) {
                // read successful
                char *buffer_end = buffer + recv_result;
                Buffer data_chunk(std::make_shared<BufferVector>(buffer, buffer_end));

                protocol_->processDataChunk(data_chunk);
            } else if (recv_result < 0) {
                if (-recv_result == EWOULDBLOCK || -recv_result == EAGAIN) {
                    return false;
                } else {
                    throw std::runtime_error("recv failed");
                }
            } else {
                return true;
            }
        }
    }
};

class AsyncWriter : public Writer {
private:
    std::weak_ptr<Socket> socket_;
    Buffer buffer_;
    std::shared_ptr<OutputProtocol> protocol_;

public:
    typedef Buffer OutputBuffer;

    AsyncWriter(std::weak_ptr<Socket> socket, 
            std::shared_ptr<OutputProtocol> protocol) :
        socket_(socket),
        buffer_(createBuffer()),
        protocol_(protocol)
        {}

    virtual void write(Buffer buffer) {
        if (buffer_->empty()) {
            buffer_ = protocol_->getResponse(buffer);
        }

        std::shared_ptr<Socket> socket = socket_.lock();
        if (!socket) {
            return;
        }
    
        auto result = socket->write(buffer_->data(), buffer_->size());
        if (result < 0) {
            if (result == -EWOULDBLOCK || result == -EAGAIN) {
                // remembered this buffer, will try to put it next time
                return; 
            }
            throw std::runtime_error("write failed");
        }
        buffer_ = createBuffer();
    }
};
} // namespace mio
