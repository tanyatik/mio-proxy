#pragma once 

#include <memory>
#include <utility>
#include <list>
#include <queue>

#include "socket.hpp"

namespace mio {

typedef std::vector<char> BufferVector;
typedef std::shared_ptr<BufferVector> Buffer;

template<typename... Args> 
Buffer createBuffer(Args... args) {
    return std::make_shared<BufferVector>(args...);
}

class Socket;

class Reader {
public:
    virtual bool read() = 0;
    virtual ~Reader() {}
};

class Writer {
public:
    virtual void write(Buffer buffer) {}
    virtual ~Writer() {}
};

class Closer {
public:
    virtual void onClose() {};
    virtual ~Closer() {}
};

class Connection;

class ConnectionManager {
public:
    virtual std::shared_ptr<Connection> addConnection(std::shared_ptr<Connection>) = 0;
};

class InputProtocol {
public:
    virtual ~InputProtocol() {}
    virtual void processDataChunk(Buffer buffer) = 0;
};

class OutputProtocol {
public:
    virtual Buffer getResponse(Buffer) = 0;
    virtual ~OutputProtocol() {}
};

class RequestHandler {
public:
    virtual void handleRequest(Buffer request) = 0;
    virtual ~RequestHandler() {}
};

} // namespace mio
