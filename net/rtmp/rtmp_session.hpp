#ifndef RTMP_SESSION_HPP
#define RTMP_SESSION_HPP
#include "tcp_session.hpp"
#include "data_buffer.hpp"
#include "logger.hpp"
#include "rtmp_pub.hpp"
#include "rtmp_handshake.hpp"
#include "chunk_stream.hpp"
#include "rtmp_session_interface.hpp"
#include "amf/afm0.hpp"
#include <memory>
#include <stdint.h>
#include <unordered_map>


class rtmp_server_callbackI
{
public:
    virtual void on_close(boost::asio::ip::tcp::endpoint endpoint) = 0;
};

class rtmp_session : public tcp_session_callbackI, public rtmp_session_interface
{
public:
    rtmp_session(boost::asio::ip::tcp::socket socket, rtmp_server_callbackI* callback) : callback_(callback) {
        session_ptr_ = std::make_shared<tcp_session>(std::move(socket), this);

        try_read();
    }
    virtual ~rtmp_session() {

    }

public://implement rtmp_session_interface
    virtual void try_read() override {
        log_infof("try to read....");
        session_ptr_->async_read();
    }

    virtual data_buffer* get_recv_buffer() override {
        return &recv_buffer_;
    }

public:
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

        if (!recv_buffer_.require(c0_size + c1_size)) {
            try_read();
            return RTMP_NEED_READ_MORE;
        }

        return hs_.parse_c0c1(recv_buffer_.buffer_);
    }

    int handle_c2() {
        const size_t c2_size = 1536;
        if (!recv_buffer_.require(c2_size)) {
            try_read();
            return RTMP_NEED_READ_MORE;
        }
        //TODO_JOB: handle c2 data.
        recv_buffer_.consume_data(c2_size);

        return RTMP_OK;
    }

    int read_fmt_csid() {
        uint8_t* p = nullptr;

        if (!fmt_ready_) {
            csid_ready_ = false;
            if (recv_buffer_.require(1)) {
                p = (uint8_t*)recv_buffer_.data();
                log_infof("chunk 1st byte:0x%02x", *p);
                fmt_  = ((*p) >> 6) & 0x3;
                csid_ = (*p) & 0x3f;
                fmt_ready_ = true;
                recv_buffer_.consume_data(1);
            } else {
                try_read();
                return RTMP_NEED_READ_MORE;
            }
        }
        log_infof("rtmp chunk fmt:%d, csid:%d", fmt_, csid_);

        if (csid_ > 1) {//1bytes basic header
            csid_ready_ = true;
        } else if (csid_ == 0) {
            if (recv_buffer_.require(1)) {//need 1 byte
                p = (uint8_t*)recv_buffer_.data();
                recv_buffer_.consume_data(1);
                csid_ = 64 + *p;
                csid_ready_ = true;
            } else {
                try_read();
                return RTMP_NEED_READ_MORE;
            }
        } else if (csid_ == 1) {
            if (recv_buffer_.require(2)) {//need 2 bytes
                p = (uint8_t*)recv_buffer_.data();
                recv_buffer_.consume_data(2);
                csid_ = 64;
                csid_ += *p++;
                csid_ += *p;
                csid_ready_ = true;
            } else {
                try_read();
                return RTMP_NEED_READ_MORE;
            }
        } else {
            assert(0);
            return -1;
        }
        return RTMP_OK;
    }

    int read_chunk_stream(CHUNK_STREAM_PTR& cs_ptr) {
        int ret;

        ret = read_fmt_csid();
        if (ret != 0) {
            return ret;
        }

        auto iter = cs_map_.find(csid_);
        if (iter == cs_map_.end()) {
            cs_ptr = std::make_shared<chunk_stream>(this, fmt_, csid_);
            cs_map_.insert(std::make_pair(csid_, cs_ptr));
        } else {
            cs_ptr =iter->second;
        }

        ret = cs_ptr->read_message_header();
        if (ret < RTMP_OK) {
            close();
            return ret;
        } else if (ret == RTMP_NEED_READ_MORE) {
            return ret;
        } else {
            log_infof("read message header ok");
            cs_ptr->dump_header();

            ret = cs_ptr->read_message_payload();
            if (ret == RTMP_OK) {
                cs_ptr->dump_payload();
            }
        }

        return ret;
    }

    int handle_rtmp_connect() {
        CHUNK_STREAM_PTR cs_ptr;
        int ret;

        ret = read_chunk_stream(cs_ptr);
        if (ret < RTMP_OK) {
            return ret;
        }

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
            log_infof("start handle c2...");
            ret = handle_c2();
            if (ret < 0) {
                close();
                return;
            }

            fmt_ready_  = false;//be ready to receive basic_header in chuch stream
            csid_ready_ = false;//be ready to receive basic_header in chuch stream
            log_infof("rtmp session phase become rtmp connect.");
            session_phase_ = connect_phase;
            try_read();
        } else if (session_phase_ == connect_phase) {
            log_infof("start handle connect...");
            ret = handle_rtmp_connect();
            if (ret < 0) {
                close();
                return;
            }
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

private:
    bool fmt_ready_  = false;
    bool csid_ready_ = false;
    uint8_t fmt_  = 0;
    uint8_t csid_ = 0;
    std::unordered_map<uint8_t, CHUNK_STREAM_PTR> cs_map_;
};

#endif