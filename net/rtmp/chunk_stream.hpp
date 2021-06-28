#ifndef CHUNK_STREAM_HPP
#define CHUNK_STREAM_HPP
#include "data_buffer.hpp"
#include "rtmp_session_interface.hpp"
#include "byte_stream.hpp"
#include <stdint.h>
#include <memory>

static char message_header_sizes[] = {11, 7, 3, 0};

class chunk_stream
{
public:
    chunk_stream(rtmp_session_interface* session, uint8_t fmt, uint8_t csid)
    {
        session_ = session;
        fmt_     = fmt;
        csid_    = csid;
        log_infof("chunk_stream construct fmt:%d, csid:%d", fmt_, csid_);
    }
    ~chunk_stream()
    {
    }

public:
    bool is_message_header_ready() {
        return msg_hd_ready_;
    }

    int read_message_header() {
        uint8_t* p;
        bool is_first_chunk = (chunk_data_.data_len() == 0);

        log_infof("read message header fmt:%d, csid:%d", fmt_, csid_);
        if (fmt_ > 3) {
            log_errorf("fmt is invalid:%d", fmt_);
            return -1;
        }
        if (is_first_chunk && (fmt_ != 0)) {
            if (fmt_ == 1) {
                log_warnf("chunk stream start with fmt=1.");
            } else {
                log_errorf("chunk stream start with fmt=%d", fmt_);
                return -1;
            }
        }

        if (!is_first_chunk && (fmt_ == 0)) {
            log_errorf("chunk stream fmt is 0, but it's not the first chunk.");
            return -1;
        }

        int msg_hd_size = message_header_sizes[fmt_];
        data_buffer* buffer_p = session_->get_recv_buffer();

        //when it has message header;
        if (msg_hd_size > 0) {
            if (!buffer_p->require(msg_hd_size)) {
                session_->try_read();
                return RTMP_NEED_READ_MORE;
            }
            p = (uint8_t*)buffer_p->data();
            uint32_t ts_delta = read_3bytes(p);
            p += 3;

            if (ts_delta >= 0xffffff) {
                if (!buffer_p->require(msg_hd_size + 1)) {
                    session_->try_read();
                    return RTMP_NEED_READ_MORE;
                }
            }
            if (fmt_ == 0) {
                msg_len_ = read_3bytes(p);
                p += 3;
                type_id_ = *p;
                p++;
                msg_stream_id_ = read_4bytes(p);
                p += 4;
                if (ts_delta >= 0xffffff) {
                    timestamp64_ = read_4bytes(p);
                } else {
                    timestamp64_ = ts_delta;
                }
            } else if (fmt_ == 1) {
                msg_len_ = read_3bytes(p);
                p += 3;
                type_id_ = *p;
                p++;
                if (ts_delta >= 0xffffff) {//it's timestamp delta
                    timestamp64_ += read_4bytes(p);
                } else {
                    timestamp64_ += ts_delta;
                }
            } else if (fmt_ == 2) {
                if (ts_delta >= 0xffffff) {//it's timestamp delta
                    timestamp64_ += read_4bytes(p);
                } else {
                    timestamp64_ += ts_delta;
                }
            }

            if (ts_delta >= 0xffffff) {
                buffer_p->consume_data(msg_hd_size + 1);
            } else {
                buffer_p->consume_data(msg_hd_size);
            }
        }

        msg_hd_ready_ = true;
        return RTMP_OK;
    }

    int read_message_payload() {
        data_buffer* buffer_p = session_->get_recv_buffer();
        
        if (!buffer_p->require(msg_len_)) {
            session_->try_read();
            return RTMP_NEED_READ_MORE;
        }

        chunk_data_.append_data(buffer_p->data(), msg_len_);
        log_infof("chunck message len:%u, buffer data len:%lu", msg_len_, buffer_p->data_len());
        buffer_p->consume_data(buffer_p->data_len());

        return RTMP_OK;
    }

    void dump_header() {
        log_infof("basic header: fmt=%d, csid:%d; message header: timestamp=%lu, msglen=%u, typeid:%d, msg streamid:%u",
            fmt_, csid_, timestamp64_, msg_len_, type_id_, msg_stream_id_);
    }

    void dump_payload() {
        char desc[128];

        snprintf(desc, sizeof(desc), "chunk stream payload:%lu", chunk_data_.data_len());
        log_info_data((uint8_t*)chunk_data_.data(), chunk_data_.data_len(), desc);
    }

public:
    uint32_t timestamp64_   = 0;
    uint32_t msg_len_       = 0;
    uint8_t  type_id_       = 0;
    uint32_t msg_stream_id_ = 0;

    data_buffer chunk_data_;

private:
    rtmp_session_interface* session_ = nullptr;
    uint8_t fmt_  = 0;
    uint8_t csid_ = 0;
    bool msg_hd_ready_ = false;
};

using CHUNK_STREAM_PTR = std::shared_ptr<chunk_stream>;

#endif