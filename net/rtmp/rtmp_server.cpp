#include "rtmp_server.hpp"
#include "net_pub.hpp"

rtmp_server::rtmp_server(boost::asio::io_context& io_context, uint16_t port):check_alive_timer_(io_context) {
    server_ = std::make_shared<tcp_server>(io_context, port, this);
    server_->accept();
    start_check_alive_timer();
}

rtmp_server::~rtmp_server() {

}

void rtmp_server::on_close(std::string session_key) {
    log_infof("tcp close key:%s", session_key.c_str());
    const auto iter = session_ptr_map_.find(session_key);
    if (iter != session_ptr_map_.end()) {
        session_ptr_map_.erase(iter);
    }
}

void rtmp_server::on_accept(int ret_code, boost::asio::ip::tcp::socket socket) {
    if (ret_code == 0) {
        std::string key;
        make_endpoint_string(socket.remote_endpoint(), key);
        log_infof("tcp accept key:%s", key.c_str());
        std::shared_ptr<rtmp_session> session_ptr = std::make_shared<rtmp_session>(std::move(socket), this, key);
        session_ptr_map_.insert(std::make_pair(key, session_ptr));
    }
    server_->accept();
}

void rtmp_server::on_check_alive() {
    for (auto item : session_ptr_map_) {
        std::shared_ptr<rtmp_session> session_ptr = item.second;
        if (!session_ptr->is_alive()) {
            session_ptr->close();
        }
    }
}

void rtmp_server::start_check_alive_timer() {
    check_alive_timer_.expires_from_now(boost::posix_time::seconds(5));
    check_alive_timer_.async_wait([this](const boost::system::error_code& ec) {
        if(!ec) {
            this->on_check_alive();
        } else {
            log_errorf("check alive error:%s, value:%d",
                ec.message().c_str(), ec.value())
        }
        this->start_check_alive_timer();
    });
}