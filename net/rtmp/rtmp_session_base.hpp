#ifndef RTMP_SESSION_INTERFACE_HPP
#define RTMP_SESSION_INTERFACE_HPP
#include "data_buffer.hpp"
#include "logger.hpp"

class rtmp_session_base
{
public:
    virtual void try_read(const char* filename, int line) = 0;
    virtual void send(char* data, int len) = 0;
    virtual data_buffer* get_recv_buffer() = 0;
};

class rtmp_request
{
public:
    rtmp_request() {};
    ~rtmp_request() {};

public:
    void dump() {
        log_infof("app:%s, tcurl:%s, flash version:%s.",
            app_.c_str(), tcurl_.c_str(), flash_ver_.c_str());
    }

public:
    std::string app_;
    std::string tcurl_;
    std::string flash_ver_;
    int64_t transaction_id_ = 0;
    uint32_t stream_id_ = 0;
};

#endif