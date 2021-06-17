#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H
#include <string>
#include <unordered_map>

class HttpResponse
{
public:
    HttpResponse() {
        status_code_    = -1;
        body_           = nullptr;
        content_length_ = 0;
    }
    ~HttpResponse() {
    }

public:
    std::string status_;  // e.g. "OK"
    int status_code_;     // e.g. 200
    std::string proto_;   // e.g. "HTTP"
    std::string version_; // e.g. "1.1"
    std::unordered_map<std::string, std::string> headers_; //headers
    int content_length_;

public:
    char* body_    = nullptr;
    int body_len_ = 0;
};

#endif //HTTP_RESPONSE_H