#ifndef BOOST_TCP_SSL_CLIENT_H
#define BOOST_TCP_SSL_CLIENT_H
#include "DataBuffer.hpp"
#include "TcpClientBase.hpp"
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

using boost::asio::ip::tcp;
using std::placeholders::_1;
using std::placeholders::_2;

class TcpSslClient : public TcpClientBase, public std::enable_shared_from_this<TcpSslClient>
{
public:
  TcpSslClient(boost::asio::io_context& io_context,
      boost::asio::ssl::context& context,
      const tcp::resolver::results_type& endpoints,
      TcpClientCallback* callback)
    : socket_(io_context, context), endpoint_(endpoints), callback_(callback)
  {
    socket_.set_verify_mode(boost::asio::ssl::verify_peer);
    socket_.set_verify_callback(
        std::bind(&TcpSslClient::verify_certificate, this, _1, _2));
  }
  virtual ~TcpSslClient()
  {
  }

private:
  bool verify_certificate(bool preverified,
      boost::asio::ssl::verify_context& ctx)
  {
    // The verify callback can be used to check whether the certificate that is
    // being presented is valid for the peer. For example, RFC 2818 describes
    // the steps involved in doing this for HTTPS. Consult the OpenSSL
    // documentation for more details. Note that the callback is called once
    // for each certificate in the certificate chain, starting from the root
    // certificate authority.

    // In this example we will simply print the certificate's subject name.
    char subject_name[256];
    X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
    std::cout << "Verifying " << subject_name << "\n";
    std::cout << "preverified:" << preverified << "\n";

    return true;
  }

  void handshake()
  {
    auto self(shared_from_this());
    socket_.async_handshake(boost::asio::ssl::stream_base::client,
        [self](const boost::system::error_code& error)
        {
          self->callback_->on_handshake(error, self);
        });
  }

public:
  virtual void connect() override
  {
    auto self(shared_from_this());
    boost::asio::async_connect(socket_.lowest_layer(), endpoint_,
        [self](const boost::system::error_code& error,
          const tcp::endpoint& endpoint)
        {
          self->callback_->on_connected(error, self);
          if (!error)
          {
            self->handshake();
          }
          else
          {
            std::cout << "Connect failed: " << error.message() << "\n";
          }
        });
  }

  virtual void send_request(char* data, size_t data_size) override
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_,
        boost::asio::buffer(data, data_size),
        [self](const boost::system::error_code& error, std::size_t length)
        {
          self->callback_->on_write(error, length, self);
        });
  }

  virtual void receive_response() override
  {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
        boost::asio::buffer(recv_buffer_.buffer_, recv_buffer_.buffer_size_),
        [this, self](const boost::system::error_code& error, std::size_t length)
        {
          std::cout << "read callback error:" << error << " return length:" << length << std::endl;
          self->callback_->on_read(error, recv_buffer_.buffer_, length, shared_from_this());
        });
  }

private:
  boost::asio::ssl::stream<tcp::socket> socket_;
  DataBuffer recv_buffer_;
  tcp::resolver::results_type endpoint_;
  TcpClientCallback* callback_ = nullptr;
};

#endif //BOOST_TCP_SSL_CLIENT_H