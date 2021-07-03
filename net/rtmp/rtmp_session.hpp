#ifndef RTMP_SESSION_HPP
#define RTMP_SESSION_HPP
#include "tcp_session.hpp"
#include "data_buffer.hpp"
#include "logger.hpp"
#include "rtmp_pub.hpp"
#include "rtmp_handshake.hpp"
#include "chunk_stream.hpp"
#include "rtmp_session_base.hpp"
#include "rtmp_control_handler.hpp"
#include "amf/afm0.hpp"
#include <memory>
#include <stdint.h>
#include <unordered_map>
#include <vector>

class rtmp_server_callbackI
{
public:
    virtual void on_close(boost::asio::ip::tcp::endpoint endpoint) = 0;
};

class rtmp_session : public tcp_session_callbackI, public rtmp_session_base
{
friend class rtmp_control_handler;
friend class rtmp_handshake;

public:
    rtmp_session(boost::asio::ip::tcp::socket socket, rtmp_server_callbackI* callback);
    virtual ~rtmp_session();

public://implement rtmp_session_base
    virtual void try_read(const char* filename, int line) override;
    virtual data_buffer* get_recv_buffer() override;
    virtual void rtmp_send(char* data, int len) override;

public:
    void close();

protected://implement tcp_session_callbackI
    virtual void on_write(int ret_code, size_t sent_size) override;
    virtual void on_read(int ret_code, const char* data, size_t data_size) override;

private:
    int read_fmt_csid();
    int read_chunk_stream(CHUNK_STREAM_PTR& cs_ptr);
    int receive_chunk_stream();
    int handle_request();
    int send_rtmp_ack(uint32_t size);

private:
    std::shared_ptr<tcp_session> session_ptr_;
    rtmp_server_callbackI* callback_ = nullptr;

private:
    RTMP_SESSION_PHASE session_phase_ = initial_phase;
    data_buffer recv_buffer_;
    rtmp_handshake hs_;
    rtmp_request req_;
    uint32_t stream_id_ = 1;

private:
    bool fmt_ready_ = false;
    uint8_t fmt_    = 0;
    uint16_t csid_  = 0;
    uint32_t chunk_size_ = CHUNK_DEF_SIZE;
    uint32_t remote_window_acksize_ = 2500000;
    uint32_t ack_received_ = 0;
    std::unordered_map<uint8_t, CHUNK_STREAM_PTR> cs_map_;

private:
    rtmp_control_handler ctrl_handler_;
};

#endif