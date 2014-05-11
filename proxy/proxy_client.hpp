#pragma once

namespace mioproxy {

class ProxyBackendConnection;
class ProxyClientRequestHandler;

class ProxyClientConnection : public mio::ConnectionWithOutput,
    public std::enable_shared_from_this<ProxyClientConnection> {
private:
    ProxyClientConnection(std::shared_ptr<mio::ConnectionManager> connection_manager,
            std::shared_ptr<mio::Socket> socket) :
        ConnectionWithOutput(socket, 
                nullptr,
                std::make_shared<mio::AsyncWriter>(socket,
                    std::make_shared<OutputBinaryProtocol>()), 
                std::make_shared<mio::Closer>()) {
        std::shared_ptr<ProxyClientConnection> this_ptr(this);
        connection_manager->addConnection(this_ptr);
        this_ptr->initReader(connection_manager, socket);
    }

    void initReader(std::shared_ptr<mio::ConnectionManager> connection_manager,
            std::shared_ptr<mio::Socket> socket) {
        auto request_handler = std::make_shared<ProxyClientRequestHandler>
            (connection_manager, shared_from_this());

        reader_ = std::make_shared<mio::AsyncReader>(socket, 
                std::make_shared<InputHttpProtocol>(request_handler));
    }

public:
    static std::shared_ptr<ProxyClientConnection> create
        (std::weak_ptr<mio::ConnectionManager> connection_manager,
         std::shared_ptr<mio::Socket> socket) {
        std::shared_ptr<mio::ConnectionManager> conn_m = connection_manager.lock();

        if (conn_m) {
            return (new ProxyClientConnection(conn_m, socket))->shared_from_this();
        } else {
            return nullptr;
        }
    }
};

class ProxyClientRequestHandler : public mio::RequestHandler {
private:
    std::weak_ptr<mio::ConnectionManager> connection_manager_;
    std::weak_ptr<mio::Connection> client_connection_;
    
public:
    explicit ProxyClientRequestHandler(std::weak_ptr<mio::ConnectionManager> connection_manager,
        std::weak_ptr<mio::Connection> client_connection) :
        connection_manager_(connection_manager),
        client_connection_(client_connection)
        {}
   
    virtual void handleRequest(mio::Buffer request) {
        std::string request_str(request->data(), request->data() + request->size());
        boost::smatch match;
        boost::regex regex("Host: ([\\.a-zA-Z0-9-]*)");
        if (boost::regex_search(request_str, match, regex)) {
            auto hostname = match[1];
            //std::cerr << "Request for host " << match[1] << std::endl;
            try {
                std::shared_ptr<mio::ConnectionManager> conn_m(connection_manager_.lock()); 
                std::shared_ptr<mio::Connection> conn(client_connection_.lock());

                auto new_connection = ProxyBackendConnection::create(conn_m, 
                        hostname,
                        conn); 
                new_connection->addOutput(request);

            } catch (const std::runtime_error &exception) {
                std::cerr << "Failed to establish connection: " <<
                    exception.what() << std::endl;
            }
        }
    }
};

}
