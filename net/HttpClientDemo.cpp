#include "HttpClient.hpp"
#include <iostream>

class ResponseDemo : public HttpResponseCallback
{
public:
    virtual void on_response(int err_code, const HttpResponse* resp) override {
        if (err_code != 0) {
            std::cout << "http response error:" << err_code << std::endl;
            return;
        }

        std::cout << resp->proto_ << "/" << resp->version_ << " " << resp->status_code_ << " " << resp->status_ << std::endl;
        for (const auto& item : resp->headers_) {
            std::cout << item.first << ": " << item.second << std::endl;
        }
        std::cout << "content length:" << resp->content_length_ << std::endl;
        std::cout << "body length:" << resp->body_len_ << std::endl;
        std::string body_str(resp->body_, resp->body_len_);
        std::cout << body_str << std::endl;
    }
};

int main(int argn, char** argv) {
    if (argn < 3) {
        std::cout << "please input ip and port" << std::endl;
        return -1;
    }

    std::cout << "input " << argv[1] << ", " << argv[2] << std::endl;
    try
    {
        boost::asio::io_context io_ctx;

        boost::asio::io_service::work work(io_ctx);  
        auto client = new HttpClient(io_ctx, argv[1], (uint16_t)atoi(argv[2]));

        std::map<std::string, std::string> headers;
        ResponseDemo demo_callback;
        client->async_get("/demo.txt", headers, &demo_callback, false);
        io_ctx.run();
        std::cout << "########## asio is out" << std::endl;
        delete client;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    return 1;
}