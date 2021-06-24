#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H
#include <memory>
#include <string>
#include <stdint.h>
#include <boost/asio.hpp>

class tcp_client_callback
{
public:
    virtual void on_connect(int ret_code) = 0;
    virtual void on_write(int ret_code, size_t sent_size) = 0;
    virtual void on_read(int ret_code, const char* data, size_t data_size) = 0;
};

class tcp_client
{
public:
    tcp_client(boost::asio::io_context& io_ctx,
        const std::string& host, uint16_t dst_port,
        tcp_client_callback* callback)
      : io_ctx_(io_ctx),
        socket_(io_ctx),
        callback_(callback)
    {
        boost::asio::ip::tcp::resolver resolver(io_ctx);
        std::string port_str = std::to_string(dst_port);
        endpoint_ = resolver.resolve(host, port_str);

        buffer_ = new char[buffer_size_];
    }

    virtual ~tcp_client() {
        delete[] buffer_;
    }

    void connect() {
        boost::asio::async_connect(socket_, endpoint_,
            [this](boost::system::error_code ec, boost::asio::ip::tcp::endpoint) {
                if (!ec) {
                    //boost::asio::ip::tcp::socket socket, tcp_session_callbackI* callback
                    this->callback_->on_connect(0);
                    return;
                }
                this->callback_->on_connect(-1);
                return;
            });
    }

    void send(const char* data, size_t len) {
        socket_.async_write_some(boost::asio::buffer(data, len),
            [this](boost::system::error_code ec, size_t sent_length) {
                if (!ec) {
                    boost::asio::post(io_ctx_,
                        [this, sent_length]{
                            this->callback_->on_write(0, sent_length);
                        });
                    return;
                }
                this->callback_->on_write(-1, sent_length);
            });
    }

    void async_read() {
        socket_.async_read_some(boost::asio::buffer(buffer_, buffer_size_),
            [this](boost::system::error_code ec, size_t read_length) {
                if (!ec) {
                    this->callback_->on_read(0, this->buffer_, read_length);
                    return;
                }
                this->callback_->on_read(-1, nullptr, 0);
            });
    }
private:
    boost::asio::io_context& io_ctx_;
    boost::asio::ip::tcp::socket socket_;
    tcp_client_callback* callback_ = nullptr;
    boost::asio::ip::tcp::resolver::results_type endpoint_;
    char* buffer_ = nullptr;
    size_t buffer_size_ = 2048;
};

#endif //TCP_CLIENT_H