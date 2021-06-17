#ifndef TCP_CLIENT_CALLBACK_H
#define TCP_CLIENT_CALLBACK_H
#include <iostream>
#include <boost/asio.hpp>
#include <memory>

enum { TCP_DATA_MAX = 100*1024 };

class TcpClientBase {
public:
  virtual void connect() = 0;
  virtual void send_request(char* data, size_t data_size) = 0;
  virtual void receive_response() = 0;
};

class TcpClientCallback
{
public:
  virtual void on_connected(const boost::system::error_code& error, std::shared_ptr<TcpClientBase>) = 0;
  virtual void on_handshake(const boost::system::error_code& error, std::shared_ptr<TcpClientBase>) = 0;
  virtual void on_read(const boost::system::error_code& error, char* data, size_t data_size, std::shared_ptr<TcpClientBase>) = 0;
  virtual void on_write(const boost::system::error_code& error, size_t data_size, std::shared_ptr<TcpClientBase>) = 0;
};

#endif //TCP_CLIENT_CALLBACK_H