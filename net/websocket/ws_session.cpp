#include "ws_server.hpp"
#include "logger.hpp"

websocket_session::websocket_session(boost::asio::ip::tcp::socket socket,
    websocket_server_callbackI* cb, std::string id):ws_(std::move(socket))
    , server_cb_(cb)
    , stream_id_(id) 
{
}

websocket_session::~websocket_session() {
}

void websocket_session::run() {
    // Accept the websocket handshake
    ws_.async_accept(
        boost::beast::bind_front_handler(
            &websocket_session::on_accept,
            shared_from_this()));
}

void websocket_session::on_accept(boost::beast::error_code ec) {
    if (ec) {
        log_errorf("websocket accept error:%s", ec.message().c_str());
        return;
    }
    log_infof("websocket accepted...");
    do_read();
}

void websocket_session::do_read() {
    // Read a message into our buffer
    ws_.async_read(buffer_,
        boost::beast::bind_front_handler(
            &websocket_session::on_read,
            shared_from_this()));
}

void websocket_session::on_read(boost::beast::error_code ec, size_t bytes_transferred) {
    if (ec) {
        boost::beast::websocket::close_reason cr;
        log_errorf("websocket read error:%s", ec.message().c_str());
        ws_.close(cr, ec);
        server_cb_->on_close(stream_id_);
        return;
    }
    //read the buffer
    log_infof("read buffer len:%lu", bytes_transferred);
    log_info_data(static_cast<uint8_t*>(buffer_.data().data()), bytes_transferred, "input data");
    buffer_.consume(bytes_transferred);
    do_read();
}