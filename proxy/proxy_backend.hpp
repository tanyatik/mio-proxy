#pragma once

namespace mioproxy {

class ProxyBackendRequestHandler;

class ProxyBackendRequestHandler : public mio::RequestHandler {
private:
    std::weak_ptr<mio::Connection> client_connection_;

public:
    ProxyBackendRequestHandler(std::weak_ptr<mio::Connection> connection) :
        client_connection_(connection)
        {}

    virtual void handleRequest(mio::Buffer request) {
        std::shared_ptr<mio::Connection> conn = client_connection_.lock();
        if (conn) {
            conn->addOutput(request);
        }
    }
};

class ProxyBackendCloser : public mio::Closer {
private:
    std::weak_ptr<mio::Connection> client_connection_;

public:
    explicit ProxyBackendCloser(std::weak_ptr<mio::Connection> client_connection) :
        client_connection_(client_connection)
        {}

    virtual void onClose() {
        std::shared_ptr<mio::Connection> conn = client_connection_.lock();

        if (conn) {
            conn->setCloseAfterOutput();
        }
    }
};

class ProxyBackendConnection : public mio::ConnectionWithOutput,
    public std::enable_shared_from_this<ProxyBackendConnection> {
private:
    std::queue<mio::Buffer> output_queue_;

    ProxyBackendConnection(std::shared_ptr<mio::ConnectionManager> connection_manager,
            std::shared_ptr<mio::Socket> socket,
            std::shared_ptr<mio::Connection> client_connection) :

        ConnectionWithOutput(socket, 
            nullptr, 
            std::make_shared<mio::AsyncWriter>(socket,
                std::make_shared<OutputBinaryProtocol>()), 
            std::make_shared<ProxyBackendCloser>(client_connection)) {

        reader_ = initReader(socket, client_connection);
        
        std::shared_ptr<ProxyBackendConnection> this_ptr(this);
        connection_manager->addConnection(this_ptr);    
    }

    std::shared_ptr<mio::AsyncReader> initReader(std::shared_ptr<mio::Socket> socket, 
            std::shared_ptr<mio::Connection> client_connection) {
        auto request_handler = std::make_shared<ProxyBackendRequestHandler>
                (client_connection);

        return std::make_shared<mio::AsyncReader>(socket, 
                std::make_shared<InputBinaryProtocol>(request_handler)); 
    }

public:
    static std::shared_ptr<ProxyBackendConnection> create
        (std::weak_ptr<mio::ConnectionManager> connection_manager,
         std::string host,
         std::weak_ptr<mio::Connection> client_connection) {

        std::shared_ptr<mio::ConnectionManager> conn_m = connection_manager.lock();
        std::shared_ptr<mio::Connection> conn = client_connection.lock();

        if (conn_m && conn) {
            auto socket = std::make_shared<mio::ClientSocket>(host);
            return (new ProxyBackendConnection(conn_m, socket, conn))->shared_from_this();
        } else {
            return nullptr;
        }
    }
};

} // namespace mioproxy
