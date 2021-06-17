#ifndef TCP_SERVER_HPP
#define TCP_SERVER_HPP
#include "TcpServerBase.hpp"
#include <string>
#include <stdint.h>
#include <boost/asio.hpp>
#define TCP_DEF_RECV_BUFFER_SIZE (2*1024)

class TcpSession : public std::enable_shared_from_this<TcpSession>
{
public:
    TcpSession(boost::asio::ip::tcp::socket socket, TcpSessionCallback* callback):socket_(std::move(socket))
        ,callback_(callback)
    {
        buffer_ = new char[buffer_size_];
    }
    
    virtual ~TcpSession()
    {
        delete[] buffer_;
    }

public:
    void write(char* data, size_t data_size) {
        auto self(shared_from_this());
        boost::asio::async_write(socket_, boost::asio::buffer(data, data_size),
            [self](boost::system::error_code ec, size_t written_size) {
                if (!ec) {
                    self->callback_->on_write(0, written_size);
                    return;
                }
                self->callback_->on_write(-1, written_size);
            });
        return;
    }

    void read() {
        auto self(shared_from_this());
        boost::asio::async_read(socket_, boost::asio::buffer(buffer_, buffer_size_),
            [self](boost::system::error_code ec, size_t read_length) {
                if (!ec) {
                    self->callback_->on_read(0, buffer_, read_length);
                    return;
                }
                self->callback_->on_read(-1, nullptr, read_length);
            });
        return;
    }

private:
    boost::asio::ip::tcp::socket socket_;
    TcpSessionCallback* callback_ = nullptr;
    char* buffer_ = nullptr;
    size_t buffer_size_ = TCP_DEF_RECV_BUFFER_SIZE;
};

class TcpServerCallback
{
public:
    virtual void on_accept(std::shared_ptr<TcpSession> session_ptr) = 0;
};

class TcpServer
{
public:
    TcpServer(boost::asio::io_context& io_context, uint16_t local_port, TcpServerCallback* callback):local_port_(local_port)
        , acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), local_port))
        , callback_(callback)
    {

    }
    virtual ~TcpServer()
    {
    }

public:
    void accept() {
        acceptor_.async_accept(
            [this](boost::system::error ec, boost::asio::ip::tcp::socket socket)
            {
                if (!ec) {
                    this->callback_->on_accept(session_ptr);
                }
                this->accept();
            }
        );
    }

private:
    uint16_t local_port_;
    boost::asio::ip::tcp::acceptor acceptor_;
    TcpServerCallback* callback_;
};

#endif //TCP_SERVER_HPP