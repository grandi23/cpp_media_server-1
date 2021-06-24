#ifndef RTMP_SESSION_HPP
#define RTMP_SESSION_HPP
#include "tcp_session.hpp"
#include "data_buffer.hpp"
#include "logger.hpp"
#include "rtmp_pub.hpp"
#include "rtmp_handshake.hpp"
#include <memory>
#include <stdint.h>


class rtmp_server_callbackI
{
public:
    virtual void on_close(boost::asio::ip::tcp::endpoint endpoint) = 0;
};

class rtmp_session : public tcp_session_callbackI
{
public:
    rtmp_session(boost::asio::ip::tcp::socket socket, rtmp_server_callbackI* callback) : callback_(callback) {
        session_ptr_ = std::make_shared<tcp_session>(std::move(socket), this);

        try_read();
    }
    virtual ~rtmp_session() {

    }

public:
    void try_read() {
        log_infof("try to read....");
        session_ptr_->async_read();
    }

    void close() {
        log_infof("rtmp session close....");
        auto ep = session_ptr_->get_remote_endpoint();
        session_ptr_->close();
        callback_->on_close(ep);
    }

protected://implement tcp_session_callbackI
    virtual void on_write(int ret_code, size_t sent_size) override {
        if (session_phase_ == handshake_c0c1_phase) {
            int ret = send_buffer_.consume_data(sent_size);
            if (ret != 0) {
                log_errorf("handshake phase write callback error, sent size:%lu", sent_size);
                close();
                return;
            }
            if (send_buffer_.data_len() == 0) {
                session_phase_ = handshake_c2_phase;
                try_read();
                log_infof("rtmp s0s1s2 is sent, this size:%lu", sent_size);
                return;
            }

            session_ptr_->async_write(send_buffer_.data(), send_buffer_.data_len());
        }
    }

    virtual void on_read(int ret_code, const char* data, size_t data_size) override {
        log_infof("on read callback return code:%d, data_size:%lu", ret_code, data_size);
        if (ret_code != 0) {
            close();
            return;
        }
        
        recv_buffer_.append_data(data, data_size);

        handle_request();
    }

private:
    int handle_c0c1() {
        const size_t c0_size = 1;
        const size_t c1_size = 1536;

        if (recv_buffer_.data_len() < (c0_size + c1_size)) {
            try_read();
            return RTMP_NEED_READ_MORE;
        }

        return hs_.parse_c0c1(recv_buffer_.buffer_);
    }

    int handle_c2() {
        const size_t c2_size = 1536;
        if (recv_buffer_.data_len() < c2_size) {
            try_read();
            return RTMP_NEED_READ_MORE;
        }
        //TODO_JOB: handle c2 data.

        return RTMP_OK;
    }

    void send_s0s1s2() {
        char s0s1s2[3073];
        char* s1_data;
        int s1_len;
        char* s2_data;
        int s2_len;

        uint8_t* p = (uint8_t*)s0s1s2;

        /* ++++++ s0 ++++++*/
        rtmp_random_generate(s0s1s2, sizeof(s0s1s2));
        p[0] = RTMP_HANDSHAKE_VERSION;
        p++;

        /* ++++++ s1 ++++++*/
        s1_data = hs_.make_s1_data(s1_len);
        if (!s1_data) {
            log_errorf("make s1 data error...");
            close();
            return;
        }
        assert(s1_len == 1536);

        memcpy(p, s1_data, s1_len);
        p += s1_len;

        /* ++++++ s2 ++++++*/
        s2_data = hs_.make_s2_data(s2_len);
        if (!s2_data) {
            log_errorf("make s2 data error...");
            close();
            return;
        }
        assert(s2_len == 1536);
        memcpy(p, s2_data, s2_len);

        send_buffer_.reset();
        send_buffer_.append_data(s0s1s2, sizeof(s0s1s2));

        session_ptr_->async_write(send_buffer_.data(), send_buffer_.data_len());

        return;
    }

    void handle_request() {
        int ret;

        if (session_phase_ == initial_phase) {
            ret = handle_c0c1();
            if (ret < 0) {
                close();
                return;
            }
            recv_buffer_.reset();//be ready to receive c2;
            session_phase_ = handshake_c0c1_phase;
            log_infof("rtmp session phase become c0c1.");

            send_s0s1s2();
        } else if (session_phase_ == handshake_c2_phase) {
            log_infof("start hand c2...");
            ret = handle_c2();
            if (ret < 0) {
                close();
                return;
            }
            recv_buffer_.reset();//be ready to receive rtmp connect;
            log_infof("rtmp session phase become rtmp connect.");
            session_phase_ = connect_phase;
            try_read();
        }
    }

private:
    std::shared_ptr<tcp_session> session_ptr_;
    rtmp_server_callbackI* callback_ = nullptr;

private:
    RTMP_SESSION_PHASE session_phase_ = initial_phase;
    data_buffer recv_buffer_;
    data_buffer send_buffer_;
    rtmp_handshake hs_;
};

#endif