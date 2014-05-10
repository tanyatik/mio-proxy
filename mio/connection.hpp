#include "mio.hpp"

namespace mio {

class Connection {
protected:
    std::shared_ptr<Socket> socket_;
    std::shared_ptr<Reader> reader_;
    std::shared_ptr<Writer> writer_;
    std::shared_ptr<Closer> closer_;
    bool need_close_;
    int flags_;

public:
    Connection(std::shared_ptr<Socket> socket,
            std::shared_ptr<Reader> reader,
            std::shared_ptr<Writer> writer,
            std::shared_ptr<Closer> closer) :
        socket_(socket),
        reader_(reader),
        writer_(writer),
        closer_(closer),
        need_close_(false)
        {}

    virtual bool onInput() {
        bool closed = false;
        if (reader_) {
            closed = reader_->read();
            if (closed) {
                need_close_ = true;
            }
        }
        return closed;
    }

    virtual void onOutput() {}

    virtual void onClose() {
        if (closer_) {
            closer_->onClose();
        }
    }

    virtual void addOutput(Buffer output) = 0;
    virtual void setCloseAfterOutput() {}

    virtual bool needClose() {
        return need_close_;
    }

    int getDescriptor() {
        return socket_->getDescriptor();
    }

    virtual ~Connection() {
    }
};


class ConnectionWithOutput : public Connection {
private:
    std::queue<Buffer> output_queue_;
    bool close_after_output_;

public:
    ConnectionWithOutput(std::shared_ptr<Socket> socket,
            std::shared_ptr<Reader> in_handler,
            std::shared_ptr<Writer> out_handler,
            std::shared_ptr<Closer> close_handler) :
        Connection(socket, in_handler, out_handler, close_handler),
        close_after_output_(false)
        {}

    virtual void addOutput(Buffer output) {
        output_queue_.push(output);
    }

    virtual void onOutput() {
        while (!output_queue_.empty()) {
            writer_->write(output_queue_.front());
            output_queue_.pop();
        }
        if (close_after_output_ == true) {
            need_close_ = true;
        }
    }

    virtual void setCloseAfterOutput() {
        close_after_output_ = true;
    }
};

} // namespace mio
