#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H
#include "TcpSslClient.hpp"
#include "TcpClient.hpp"
#include "DataBuffer.hpp"
#include "HttpResponse.hpp"
#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>


class HttpResponseCallback
{
public:
    virtual void on_response(int err_code, const HttpResponse* resp) = 0;
};

class HttpClient : public TcpClientCallback
{
public:
    HttpClient(boost::asio::io_context& io_ctx, const std::string& host, uint16_t port):send_buffer_(TCP_DATA_MAX)
        , io_ctx_(io_ctx)
        , host_(host)
        , port_(port)
    {
        boost::asio::ip::tcp::resolver resolver(io_ctx);
        std::string port_str = std::to_string(port);
        endpoint_ = resolver.resolve(host_, port_str);
    };
    virtual ~HttpClient() {
    };

public:
    int async_get(const std::string& uri, const std::map<std::string, std::string>& headers, HttpResponseCallback* resp_callback, bool https_enable = true) {
        std::string host;

        send_buffer_.data_len_ += snprintf(send_buffer_.buffer_ + send_buffer_.data_len_,
                                    send_buffer_.buffer_size_ - send_buffer_.data_len_,
                                    "GET %s HTTP/1.1\r\n", uri.c_str());
        send_buffer_.data_len_ += snprintf(send_buffer_.buffer_ + send_buffer_.data_len_,
                                    send_buffer_.buffer_size_ - send_buffer_.data_len_,
                                    "Host: %s\r\n", host_.c_str());
        send_buffer_.data_len_ += snprintf(send_buffer_.buffer_ + send_buffer_.data_len_,
                                    send_buffer_.buffer_size_ - send_buffer_.data_len_,
                                    "Accept: */*\r\n");

        for (auto item : headers) {
            send_buffer_.data_len_ += snprintf(send_buffer_.buffer_ + send_buffer_.data_len_,
                                    send_buffer_.buffer_size_ - send_buffer_.data_len_,
                                    "%s: %s\r\n", item.first.c_str(), item.second.c_str());
        }
        send_buffer_.data_len_ += snprintf(send_buffer_.buffer_ + send_buffer_.data_len_,
                                    send_buffer_.buffer_size_ - send_buffer_.data_len_, "\r\n");

        send_buffer_.start_ = 0;
        send_buffer_.end_   = send_buffer_.data_len_;

        https_enable_ = https_enable;

        std::cout << "http request:" << send_buffer_.buffer_ << std::endl;
        std::cout << "http request len:" << send_buffer_.data_len_ << std::endl;

        if (https_enable_) {
            boost::asio::ssl::context ctx(boost::asio::ssl::context::tls);
            ctx.set_default_verify_paths();
            ctx.set_verify_mode(boost::asio::ssl::verify_none);

            tcp_client_ptr_ = std::make_shared<TcpSslClient>(io_ctx_, ctx, endpoint_, this);
        } else {
            tcp_client_ptr_ = std::make_shared<TcpClient>(io_ctx_, endpoint_, this);
        }


        resp_callback_ = resp_callback;
        tcp_client_ptr_->connect();

        return 0;
    };

    int async_post(const std::string& uri, std::map<std::string, std::string> headers, const std::string& post_data, HttpResponseCallback* resp_callback) {

        return 0;
    };

    uint16_t get_port() {
        return port_;
    };

public:
    virtual void on_connected(const boost::system::error_code& error, std::shared_ptr<TcpClientBase> tcpClientPtr) override {
        if (error) {
            std::cout << "http tcp connected error:" << error.message() << std::endl;
            return;
        }
        std::cout << "http tcp connected ok" << std::endl;
        if (!https_enable_) {
            tcpClientPtr->send_request(send_buffer_.buffer_, (size_t)send_buffer_.data_len_);
        }
    };
  
    virtual void on_handshake(const boost::system::error_code& error, std::shared_ptr<TcpClientBase> tcpClientPtr) override {
        if (error) {
            std::cout << "http handshake error: " << error.message() << std::endl;
            return;
        }
        std::cout << "http handshake ok." << std::endl;
        tcpClientPtr->send_request(send_buffer_.buffer_, (size_t)send_buffer_.data_len_);
    }
  
    virtual void on_read(const boost::system::error_code& error, char* data,
                size_t data_size, std::shared_ptr<TcpClientBase> tcpClientPtr) override {
        if (https_enable_) {
            if (!error || error != boost::asio::ssl::error::stream_truncated || (error.value() != 0)) {
                std::cout << "read callback return:" << error.message() << std::endl;
                if (resp_callback_) {
                    if (!header_ready_) {
                        resp_callback_->on_response(-1, nullptr);
                        return;
                    }
                    response_.body_len_ = recv_buffer_.end_ - recv_buffer_.start_;
                    resp_callback_->on_response(0, &response_);
                }
                return;
            }
        } else {
            if (!error) {
                response_.body_len_ = recv_buffer_.end_ - recv_buffer_.start_;
                resp_callback_->on_response(0, &response_);
                return;
            }
        }

        std::cout << "read data size:" << data_size << ", error:"<< error << std::endl;

        recv_buffer_.append_data(data, data_size);

        if (!header_ready_) {
            //try to find "\r\n\r\n"
            int header_end = get_http_header(recv_buffer_.buffer_, recv_buffer_.data_len_);
            if (header_end < 0) {
                std::cout << "try to get http header" << std::endl;
                tcpClientPtr->receive_response();
                return;
            }
            recv_buffer_.start_ = header_end + 4;
            response_.body_ = recv_buffer_.buffer_ + recv_buffer_.start_;
            int ret = analyze_http_header(recv_buffer_.buffer_, (size_t)header_end);
            if (ret != 0) {
                return;
            }
            header_ready_ = true;
            response_.body_len_ = recv_buffer_.end_ - recv_buffer_.start_;

            std::cout << "++++++body len:" << response_.body_len_ << ", content length:" << response_.content_length_ << std::endl;
            if ((error == boost::asio::ssl::error::stream_truncated) || (response_.body_len_ == response_.content_length_)) {
                if (resp_callback_) {
                    resp_callback_->on_response(0, &response_);
                }
                return;
            }
            tcpClientPtr->receive_response();
            return;
        }

        tcpClientPtr->receive_response();
    }

    virtual void on_write(const boost::system::error_code& error, size_t data_size, std::shared_ptr<TcpClientBase> tcpClientPtr) override {
        send_buffer_.data_len_  -= (int)data_size;
        send_buffer_.start_     += (int)data_size;

        std::cout << "on_write data size:" << data_size << std::endl;
        if (send_buffer_.data_len_ <= 0) {
            //write job is done and start reading...
            tcpClientPtr->receive_response();
            return;
        }

        tcpClientPtr->send_request(send_buffer_.buffer_ + send_buffer_.start_, (size_t)send_buffer_.data_len_);
    }

private:
    int get_http_header(char* data, size_t data_len) {
        if (data_len <= 4) {
            return -1;
        }

        std::cout << "data len:" << data_len << std::endl;
        int pos = 0;
        do {
            if ((data[pos] == '\r') && (data[pos+1] == '\n')
                && (data[pos+2] == '\r') && (data[pos+3] == '\n')) {
                return pos;
            }
            pos++;
        } while(pos < ((int)data_len - 4));

        return -1;
    }

    int analyze_http_header(char* data, size_t data_len) {
        std::string http_header_str(data, data_len);
        std::vector<std::string> headers_vec;

        string_split(http_header_str, "\r\n", headers_vec);

        if (headers_vec.empty()) {
            std::cout << "http header error:" << http_header_str << std::endl;
            return -1;
        }

        const std::string& first_line = headers_vec[0];//HTTP/1.1 200 OK
        std::vector<std::string> version_info_vec;
        string_split(first_line, " ", version_info_vec);
        if (version_info_vec.size() != 3) {
            std::cout << "http header version error:" << first_line << std::endl;
            return -1;
        }

        //get http code and version
        const std::string& proto_and_ver   = version_info_vec[0];
        const std::string& status_code_str = version_info_vec[1];
        const std::string& ret_desc        = version_info_vec[2];

        std::cout << proto_and_ver << " " << status_code_str << " " << ret_desc << std::endl;

        int pos = proto_and_ver.find("/");
        if ((pos == proto_and_ver.npos) || (pos == (proto_and_ver.size()-1))) {
            std::cout << "http header proto and version error:" << proto_and_ver << std::endl;
            return -1;
        }
        response_.proto_       = proto_and_ver.substr(0, pos);
        response_.version_     = proto_and_ver.substr(pos + 1);
        response_.status_code_ = atoi(status_code_str.c_str());
        response_.status_      = ret_desc;

        //get http headers which has content-length
        int index = 0;
        for (const auto& item : headers_vec) {
            if (index == 0) {
                index++;
                continue;
            }
            pos = item.find(":");
            if (pos == item.npos) {
                std::cout << "http header item error:" << item << std::endl;
                return -1;
            }
            std::string key   = item.substr(0, pos);
            std::string value = item.substr(pos + 2);

            std::cout << key << ": " << value << std::endl;

            if (key == "Content-Length") {
                response_.content_length_ = atoi(value.c_str());
                std::cout << "get Content-Length:" << response_.content_length_ << std::endl;
            }
            response_.headers_.insert(std::make_pair(key, value));
            index++;
        }
        return 0;
    }

private:
    boost::asio::ip::tcp::resolver::results_type endpoint_;
    std::shared_ptr<TcpClientBase> tcp_client_ptr_;
    HttpResponseCallback* resp_callback_ = nullptr;;
    DataBuffer send_buffer_;
    DataBuffer recv_buffer_;
    bool header_ready_  = false;
    HttpResponse response_;
    boost::asio::io_context& io_ctx_;
    std::string host_;
    uint16_t port_ = 0;
    bool https_enable_ = false;
};


#endif //HTTP_CLIENT_H
