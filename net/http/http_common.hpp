#ifndef HTTP_COMMOM_HPP
#define HTTP_COMMOM_HPP
#include <string>
#include <unordered_map>
#include <stdint.h>
#include <sstream>

class http_session;

class http_session_interface
{
public:
    virtual void try_read() = 0;
    virtual void write(const char* data, size_t len) = 0;
};

class http_request
{
friend class http_session;
public:
    http_request(http_session_interface* session) {
        session_ = session;
    }
    ~http_request() {
    }

public:
    void try_read(tcp_session_callbackI* callback) {
        callback_ = callback;
        session_->try_read();
        return;
    }

public:
    std::string method_;
    std::string uri_;
    std::string version_;
    char* content_body_ = nullptr;
    int content_length_ = 0;
    std::unordered_map<std::string, std::string> headers_;

private:
    http_session_interface* session_;
    tcp_session_callbackI* callback_;
};

class http_response
{
public:
    http_response(http_session_interface* session) {
        session_ = session;
    }
    ~http_response() {
    }

public:
    void write(const char* data, size_t len) {
        std::stringstream ss;

        if (!written_header_) {
            ss << proto_ << "/" << version_ << " " << status_code_ << " " << status_ << "\r\n";
            ss << "Content-Length:" << len  << "\r\n";
            for (const auto& header : headers_) {
                ss << header.first << ": " << header.second << "\r\n";
            }
            ss << "\r\n";
            send_buffer_.append_data(ss.str().c_str(), ss.str().length());
            written_header_ = true;
        }
        send_buffer_.append_data(data, len);

        session_->write(send_buffer_.buffer_, send_buffer_.data_len_);
    }

private:
    bool written_header_ = false;
    std::string status_  = "OK";  // e.g. "OK"
    int status_code_     = 200;     // e.g. 200
    std::string proto_   = "HTTP";   // e.g. "HTTP"
    std::string version_ = "1.1"; // e.g. "1.1"
    std::unordered_map<std::string, std::string> headers_; //headers

public:
    http_session_interface* session_;
    data_buffer send_buffer_;
};

using HTTP_HANDLE_Ptr = void (*)(const http_request* request, std::shared_ptr<http_response> response_ptr);

#endif