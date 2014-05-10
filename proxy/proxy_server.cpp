#include <iostream>
#include <vector>
#include <map>

#include <boost/regex.hpp>

#include "mio/io_server.hpp"
#include "mio/mio.hpp"
#include "mio/async_io.hpp"
#include "mio/client_socket.hpp"

#include "http_protocol.hpp"
#include "proxy_backend.hpp"
#include "proxy_client.hpp"

namespace mioproxy {

class ProxyServerAcceptor : public mio::Reader {
private:
    std::shared_ptr<mio::ServerSocket> socket_;
    std::weak_ptr<mio::ConnectionManager> connection_manager_;

public:
    ProxyServerAcceptor(std::shared_ptr<mio::ServerSocket> socket,
            std::weak_ptr<mio::ConnectionManager> connection_manager) :

        socket_(socket),
        connection_manager_(connection_manager)
        {}

    bool read() {
        while (true) {
            auto new_socket = socket_->acceptNewConnection();
            std::shared_ptr<mio::ConnectionManager> con_m(connection_manager_.lock()); 
            if (new_socket != nullptr) { 
                ProxyClientConnection::create(con_m, new_socket);
            } else {
                break;
            }
        }
        return false;
    }
};

class ProxyServerConnection : public mio::Connection,
    public std::enable_shared_from_this<ProxyServerConnection> {
public:
    ProxyServerConnection(std::shared_ptr<mio::ConnectionManager> connection_manager,
            std::shared_ptr<mio::ServerSocket> server_socket, 
            std::shared_ptr<mio::Reader> reader) :

        Connection(server_socket, reader, nullptr, nullptr) {
        std::shared_ptr<ProxyServerConnection> this_ptr(this); 
        connection_manager->addConnection(this_ptr);
    }

    static std::shared_ptr<ProxyServerConnection> create
        (std::weak_ptr<mio::ConnectionManager> connection_manager,
         std::shared_ptr<mio::ServerSocket> server_socket) {

        auto reader = std::make_shared<ProxyServerAcceptor>
            (server_socket, connection_manager); 
        std::shared_ptr<mio::ConnectionManager> conn_m = connection_manager.lock();
        if (conn_m) {
            return (new ProxyServerConnection(conn_m, server_socket, reader))->shared_from_this();
        } else {
            return nullptr;
        }
    }

    virtual void addOutput(mio::Buffer output) {}
};

class LockConnectionManager : public mio::ConnectionManager {
private:
    std::shared_ptr<mio::IOServer<mio::EpollDescriptorManager>> io_server_;

    //std::mutex add_mutex_;

public:
    LockConnectionManager(std::shared_ptr<mio::IOServer<mio::EpollDescriptorManager>> server) :
        io_server_(server)
        {}

    virtual std::shared_ptr<mio::Connection> addConnection
        (std::shared_ptr<mio::Connection> connection) {
        //std::unique_lock<std::mutex> lock(add_mutex_);
        return io_server_->addConnection(connection);
    }
};

class ProxyServer {
private:
    std::shared_ptr<mio::IOServer<mio::EpollDescriptorManager>> io_server_;
    std::shared_ptr<mio::ConnectionManager> connection_manager_;

public:
    explicit ProxyServer(mio::ServerConfig config = mio::ServerConfig()) :
        io_server_(std::make_shared<mio::IOServer<mio::EpollDescriptorManager>>()),
        connection_manager_(std::make_shared<LockConnectionManager>(io_server_)) {
            ProxyServerConnection::create(connection_manager_,
                    std::make_shared<mio::ServerSocket>(config.address, config.port));
    }

    void run() {
        io_server_->eventLoop();
    } 
};

} // namespace mioproxy

int main() {
    try {
        mioproxy::ProxyServer proxy_server;
        proxy_server.run();
    } catch(const std::exception &ex) {
        std::cerr << ex.what() << std::endl;
        std::cerr << "Errno: " << errno << std::endl;
    }
    return 0;
}

