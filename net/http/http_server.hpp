#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP
#include "tcp_server.hpp"
#include "tcp_session.hpp"
#include "data_buffer.hpp"
#include "http_common.hpp"
#include "stringex.hpp"
#include "logger.hpp"
#include "net_pub.hpp"

#include <stdint.h>
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>
#include <assert.h>

class http_session;
class http_callbackI
{
public:
    virtual void on_close(boost::asio::ip::tcp::endpoint endpoint) = 0;
    virtual HTTP_HANDLE_Ptr get_handle(const http_request* request) = 0;
};

class http_session : public tcp_session_callbackI, public http_session_interface
{
friend class http_response;

public:
    http_session(boost::asio::ip::tcp::socket socket, http_callbackI* callback) : callback_(callback)
        ,request_(this)
    {
        session_ptr_ = std::make_shared<tcp_session>(std::move(socket), this);
        remote_endpoint_ = session_ptr_->get_remote_endpoint();

        try_read();
    }

    virtual ~http_session() {
        log_infof("http session destruct......")
    }

public:
    void close() {
        log_infof("http session close....");
        auto ep = session_ptr_->get_remote_endpoint();
        session_ptr_->close();
        callback_->on_close(ep);
    }

public://http_session_interface
    virtual void try_read() override {
        session_ptr_->async_read();
    }

    virtual void write(const char* data, size_t len) override {
        std::string send_str(data, len);

        log_infof("response data:%s", send_str.c_str());

        send_data_.append_data(data, len);
        session_ptr_->async_write(data, len);
    }

protected://tcp_session_callbackI
    virtual void on_write(int ret_code, size_t sent_size) override {
        log_infof("writen return code:%d, sent len:%lu, buffer len:%d",
            ret_code, sent_size, send_data_.data_len_);

        assert(send_data_.data_len_ >= (int)sent_size);

        if (send_data_.data_len_ >= (int)sent_size) {
            send_data_.data_len_ -= sent_size;
            send_data_.start_    += sent_size;
        }
        
        if (send_data_.data_len_ > 0) {
            session_ptr_->async_write(send_data_.buffer_ + send_data_.start_, send_data_.data_len_);
            return;
        }
        close();
        return;
    }

    virtual void on_read(int ret_code, const char* data, size_t data_size) override {
        if (!header_is_ready_) {
            header_data_.append_data(data, data_size);
            int ret = analyze_header();
            if (ret != 0) {
                callback_->on_close(remote_endpoint_);
                return;
            }
            if (!header_is_ready_) {
                try_read();
                return;
            }
            char* start = header_data_.buffer_ + content_start_;
            int len     = header_data_.data_len_ - content_start_;
            content_data_.append_data(start, len);
        } else {
            content_data_.append_data(data, data_size);
        }

        if ((request_.content_length_ == 0) || (request_.method_ == "GET")) {
            HTTP_HANDLE_Ptr handle_ptr = callback_->get_handle(&request_);
            if (!handle_ptr) {
                close();
                return;
            }
            //call handle
            std::shared_ptr<http_response> response_ptr = std::make_shared<http_response>(this);
            handle_ptr(&request_, response_ptr);

            return;
        }

        if (content_data_.data_len_ >= request_.content_length_) {
            HTTP_HANDLE_Ptr handle_ptr = callback_->get_handle(&request_);
            if (!handle_ptr) {
                close();
                return;
            }
            //call handle
            std::shared_ptr<http_response> response_ptr = std::make_shared<http_response>(this);
            request_.content_body_ = content_data_.buffer_;
            handle_ptr(&request_, response_ptr);

            return;
        }

        try_read();

        return;
    }

private:
    int analyze_header() {
        std::string info(header_data_.buffer_, header_data_.data_len_);

        size_t pos = info.find("\r\n\r\n");
        if (pos == info.npos) {
            header_is_ready_ = false;
            return 0;
        }
        content_start_ = (int)pos + 4;
        header_is_ready_ = true;

        std::string header_str(header_data_.buffer_, pos);
        std::vector<std::string> header_vec;

        string_split(header_str, "\r\n", header_vec);
        if (header_vec.empty()) {
            return -1;
        }
        const std::string& first_line = header_vec[0];
        std::vector<std::string> version_vec;

        string_split(first_line, " ", version_vec);

        if (version_vec.size() != 3) {
            log_errorf("version line error:%s", first_line.c_str());
            return -1;
        }
        request_.method_ = version_vec[0];
        request_.uri_ = version_vec[1];

        const std::string& version_info = version_vec[2];
        pos = version_info.find("HTTP");
        if (pos == version_info.npos) {
            log_errorf("http version error:%s", version_info.c_str());
            return -1;
        }
        request_.version_ = version_info.substr(pos+5);

        for (size_t index = 1; index < header_vec.size(); index++) {
            const std::string& info_line = header_vec[index];
            pos = info_line.find(":");
            if (pos == info_line.npos) {
                log_errorf("header line error:%s", info_line.c_str());
                return -1;
            }
            std::string key = info_line.substr(0, pos);
            std::string value = info_line.substr(pos+2);
            if (key == "Content-Length") {
                request_.content_length_ = atoi(value.c_str());
            }
            request_.headers_.insert(std::make_pair(key, value));
        }

        return 0;
    }

private:
    http_callbackI* callback_;
    std::shared_ptr<tcp_session> session_ptr_;
    data_buffer header_data_;
    data_buffer content_data_;
    data_buffer send_data_;
    http_request request_;
    int content_start_ = -1;

private:
    bool header_is_ready_ = false;
    boost::asio::ip::tcp::endpoint remote_endpoint_;
};

class http_server : public tcp_server_callbackI, public http_callbackI
{
public:
    http_server(boost::asio::io_context& io_context, uint16_t port) {
        server_ = std::make_shared<tcp_server>(io_context, port, this);
        server_->accept();
    }
    virtual ~http_server() {

    }

public:
    void add_get_handle(const std::string uri, HTTP_HANDLE_Ptr handle_func) {
        get_handle_map_.insert(std::make_pair(uri, handle_func));
    }

    void add_post_handle(const std::string uri, HTTP_HANDLE_Ptr handle_func) {
        post_handle_map_.insert(std::make_pair(uri, handle_func));
    }

protected://tcp_server_callbackI
    virtual void on_accept(int ret_code, boost::asio::ip::tcp::socket socket) override {
        if (ret_code == 0) {
            std::string key;
            make_endpoint_string(socket.remote_endpoint(), key);
            std::shared_ptr<http_session> session_ptr = std::make_shared<http_session>(std::move(socket), this);
            session_ptr_map_.insert(std::make_pair(key, session_ptr));
        }

        server_->accept();

        return;
    }

protected://http_callbackI
    virtual void on_close(boost::asio::ip::tcp::endpoint endpoint) override {
        std::string key;
        make_endpoint_string(endpoint, key);

        auto iter = session_ptr_map_.find(key);
        if (iter != session_ptr_map_.end()) {
            session_ptr_map_.erase(iter);
        }
    }

    virtual HTTP_HANDLE_Ptr get_handle(const http_request* request) override {
        HTTP_HANDLE_Ptr handle_func = nullptr;

        if (request->method_ == "GET") {
            std::unordered_map< std::string, HTTP_HANDLE_Ptr >::iterator iter = get_handle_map_.find(request->uri_);
            if (iter != get_handle_map_.end()) {
                handle_func = iter->second;
                return handle_func;
            }
        } else if (request->method_ == "POST") {
            std::unordered_map< std::string, HTTP_HANDLE_Ptr >::iterator iter = post_handle_map_.find(request->uri_);
            if (iter != post_handle_map_.end()) {
                handle_func = iter->second;
                return handle_func;
            }
        }
        
        return handle_func;
    }

private:
    std::shared_ptr<tcp_server> server_;
    std::unordered_map< std::string, std::shared_ptr<http_session> > session_ptr_map_;
    std::unordered_map< std::string, HTTP_HANDLE_Ptr > get_handle_map_;
    std::unordered_map< std::string, HTTP_HANDLE_Ptr > post_handle_map_;
};
#endif //HTTP_SERVER_HPP