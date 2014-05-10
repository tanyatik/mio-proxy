#pragma once

#include <assert.h>
#include <netinet/in.h>
#include <vector>
#include <functional>

#include "mio/mio.hpp"

namespace mioproxy {

class InputHttpProtocol : public mio::InputProtocol {
private:
    std::shared_ptr<mio::RequestHandler> request_handler_;
    mio::Buffer buffer_;

public:
    InputHttpProtocol(std::shared_ptr<mio::RequestHandler> request_handler) :
        request_handler_(request_handler),
        buffer_(std::make_shared<mio::BufferVector>())
        {}

    virtual void processDataChunk(mio::Buffer buffer) {
        auto buffer_begin = buffer->begin();
        auto buffer_seek = buffer->begin();

        while (buffer_seek != buffer->end()) {  
            if (std::distance(buffer_begin, buffer_seek) >= 4 &&
                    *buffer_seek == '\n' && 
                    *(buffer_seek - 1) == '\r' &&
                    *(buffer_seek - 2) == '\n' &&
                    *(buffer_seek - 3) == '\r') {
                buffer_->insert(buffer_->end(), buffer_begin, buffer_seek);
                buffer_->push_back('\n');

                request_handler_->handleRequest(buffer_);

                buffer_ = std::make_shared<mio::BufferVector>();

                buffer_begin = buffer_seek + 1;
                if (buffer_begin != buffer->end() && *buffer_begin == '\0') {
                    ++buffer_begin;
                }
            }
            ++buffer_seek;
        }

        if (buffer_begin != buffer->end()) {
            buffer_->insert(buffer_->end(), buffer_begin, buffer->end());
        }
    }
};

class InputBinaryProtocol : public mio::InputProtocol {
private:
    std::shared_ptr<mio::RequestHandler> request_handler_;

public:
    InputBinaryProtocol(std::shared_ptr<mio::RequestHandler> request_handler) :
        request_handler_(request_handler)
        {}

    virtual void processDataChunk(mio::Buffer buffer) {
        request_handler_->handleRequest(buffer);
    }
};

class OutputBinaryProtocol : public mio::OutputProtocol {
public:
    OutputBinaryProtocol() :
        OutputProtocol()
        {}

    virtual mio::Buffer getResponse(mio::Buffer buffer) {
        return buffer;
    } 
};

} // namespace mio
