#ifndef RTMP_SERVER_HPP
#define RTMP_SERVER_HPP
#include "rtmp_session.hpp"
#include "rtmp_pub.hpp"
#include "tcp_server.hpp"
#include <unordered_map>

class rtmp_server : public tcp_server_callbackI, public rtmp_server_callbackI
{
public:
    rtmp_server(boost::asio::io_context& io_context, uint16_t port) {
        server_ = std::make_shared<tcp_server>(io_context, port, this);
        server_->accept();
    }
    virtual ~rtmp_server() {

    }

protected:
    virtual void on_close(boost::asio::ip::tcp::endpoint endpoint) override {
        std::string key;

        make_endpoint_string(endpoint, key);
        const auto iter = session_ptr_map_.find(key);
        if (iter != session_ptr_map_.end()) {
            session_ptr_map_.erase(iter);
        }
    }

protected:
    virtual void on_accept(int ret_code, boost::asio::ip::tcp::socket socket) override {
        if (ret_code == 0) {
            std::string key;
            make_endpoint_string(socket.remote_endpoint(), key);
            std::shared_ptr<rtmp_session> session_ptr = std::make_shared<rtmp_session>(std::move(socket), this);
            session_ptr_map_.insert(std::make_pair(key, session_ptr));
        }
        server_->accept();
    }

private:
    void make_endpoint_string(boost::asio::ip::tcp::endpoint endpoint, std::string& info) {
        info = endpoint.address().to_string();
        info += ":";
        info += std::to_string(endpoint.port());
    }

private:
    std::shared_ptr<tcp_server> server_;
    std::unordered_map< std::string, std::shared_ptr<rtmp_session> > session_ptr_map_;
};

#endif //RTMP_SERVER_HPP