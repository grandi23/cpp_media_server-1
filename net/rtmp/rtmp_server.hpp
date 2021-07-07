#ifndef RTMP_SERVER_HPP
#define RTMP_SERVER_HPP
#include "rtmp_session.hpp"
#include "rtmp_pub.hpp"
#include "tcp_server.hpp"
#include "logger.hpp"
#include <unordered_map>
#include <ctime>
#include <chrono>

class rtmp_server : public tcp_server_callbackI, public rtmp_server_callbackI
{
public:
    rtmp_server(boost::asio::io_context& io_context, uint16_t port);
    virtual ~rtmp_server();

protected:
    virtual void on_close(std::string session_key) override;

protected:
    virtual void on_accept(int ret_code, boost::asio::ip::tcp::socket socket) override;

private:
    void make_endpoint_string(boost::asio::ip::tcp::endpoint endpoint, std::string& info);
    void start_check_alive_timer();
    void on_check_alive();

private:
    std::shared_ptr<tcp_server> server_;
    std::unordered_map< std::string, std::shared_ptr<rtmp_session> > session_ptr_map_;
    boost::asio::deadline_timer check_alive_timer_;
};

#endif //RTMP_SERVER_HPP