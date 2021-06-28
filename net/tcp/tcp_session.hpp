#ifndef TCP_SERVER_BASE_H
#define TCP_SERVER_BASE_H
#include <memory>
#include <string>
#include <stdint.h>
#include <boost/asio.hpp>
#include <iostream>
#include <stdio.h>

#define TCP_DEF_RECV_BUFFER_SIZE (2*1024)

class tcp_session_callbackI
{
public:
    virtual void on_write(int ret_code, size_t sent_size) = 0;
    virtual void on_read(int ret_code, const char* data, size_t data_size) = 0;
};


class tcp_session : public std::enable_shared_from_this<tcp_session>
{
public:
    tcp_session(boost::asio::ip::tcp::socket socket, tcp_session_callbackI* callback):socket_(std::move(socket))
        ,callback_(callback)
    {
        buffer_ = new char[buffer_size_];
    }
    
    virtual ~tcp_session()
    {
        close();
        delete[] buffer_;
    }

public:
    void async_write(const char* data, size_t data_size) {
        if (!socket_.is_open()) {
            return;
        }
        auto self(shared_from_this());
        socket_.async_write_some(boost::asio::buffer(data, data_size),
            [self](boost::system::error_code ec, size_t written_size) {
                if (!ec) {
                    self->callback_->on_write(0, written_size);
                    return;
                }
                self->callback_->on_write(-1, written_size);
            });

        return;
    }

    void async_read() {
        if (!socket_.is_open()) {
            return;
        }
        auto self(shared_from_this());
        try
        {
            memset(buffer_, 0, buffer_size_);
            socket_.async_read_some(boost::asio::buffer(buffer_, buffer_size_),
                [self](boost::system::error_code ec, size_t read_length) {
                    if (!ec) {
                        self->callback_->on_read(0, self->buffer_, read_length);
                        return;
                    }
                    self->callback_->on_read(-1, nullptr, read_length);
                });
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }

        return;
    }

    void close() {
        if (socket_.is_open()) {
            socket_.close();
        }
    }

    boost::asio::ip::tcp::endpoint get_remote_endpoint() {
        return socket_.remote_endpoint();
    }

    boost::asio::ip::tcp::endpoint get_local_endpoint() {
        return socket_.local_endpoint();
    }

private:
    boost::asio::ip::tcp::socket socket_;
    tcp_session_callbackI* callback_ = nullptr;
    char* buffer_ = nullptr;
    size_t buffer_size_ = TCP_DEF_RECV_BUFFER_SIZE;
};

#endif //TCP_SERVER_BASE_H