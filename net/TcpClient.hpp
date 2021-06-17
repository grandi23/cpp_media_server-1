#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H
#include "TcpClientBase.hpp"
#include "DataBuffer.hpp"
#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include <stddef.h>
#include <stdint.h>

class TcpClient : public TcpClientBase, public std::enable_shared_from_this<TcpClient>
{
public:
  TcpClient(boost::asio::io_context& io_context,
      const boost::asio::ip::tcp::resolver::results_type& endpoints,
      TcpClientCallback* callback)
    : socket_(io_context),
      endpoint_(endpoints),
      callback_(callback)
  {
  }
  virtual ~TcpClient() {

  }

public:
  virtual void connect() override {
    auto self(shared_from_this());
    boost::asio::async_connect(socket_, endpoint_,
        [self](boost::system::error_code ec, boost::asio::ip::tcp::endpoint)
        {
          self->callback_->on_connected(ec, self);
        });
    return;
  }

  virtual void send_request(char* data, size_t data_size) override {
    auto self(shared_from_this());
    boost::asio::async_write(socket_,
        boost::asio::buffer(data, data_size),
        [self](boost::system::error_code ec, std::size_t writen_len)
        {
          self->callback_->on_write(ec, writen_len, self);
        });
    return;
  }

  virtual void receive_response() override {
    std::cout << "tcp client start receiving..." << std::endl;
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
        boost::asio::buffer(recv_buffer_.buffer_, recv_buffer_.buffer_size_),
        [this, self](boost::system::error_code ec, std::size_t read_len)
        {
          self->callback_->on_read(ec, recv_buffer_.buffer_, read_len, self);
        });
    return;
  }

  void close()
  {
    socket_.close();
  }

private:
  boost::asio::ip::tcp::socket socket_;
  boost::asio::ip::tcp::resolver::results_type endpoint_;
  TcpClientCallback* callback_;
  DataBuffer recv_buffer_;
};

#endif //TCP_CLIENT_H