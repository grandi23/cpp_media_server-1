#ifndef CHUNK_STREAM_HPP
#define CHUNK_STREAM_HPP
#include "data_buffer.hpp"
#include "rtmp_session_base.hpp"
#include "byte_stream.hpp"
#include <stdint.h>
#include <memory>

static char message_header_sizes[] = {11, 7, 3, 0};

typedef enum {
    CHUNK_STREAM_PHASE_HEADER,
    CHUNK_STREAM_PHASE_PAYLOAD
} CHUNK_STREAM_PHASE;

class chunk_stream
{
public:
    chunk_stream(rtmp_session_base* session, uint8_t fmt, uint16_t csid, uint32_t chunk_size)
    {
        session_ = session;
        fmt_     = fmt;
        csid_    = csid;
        chunk_size_ = chunk_size;
    }
    ~chunk_stream()
    {
    }

public:
    bool is_ready() {
        return chunk_ready_;
    }

    int gen_control_message(RTMP_CONTROL_TYPE ctrl_type, uint32_t size, uint32_t value) {
        fmt_           = 0;
        type_id_       = (uint8_t)ctrl_type;
        msg_stream_id_ = 0;
        msg_len_       = size;

        uint8_t* data = new uint8_t[size];

        switch (size) {
            case 1:
            {
                data[0] = (uint8_t)value;
                break;
            }
            case 2:
            {
                write_2bytes(data, value);
                break;
            }
            case 3:
            {
                write_3bytes(data, value);
                break;
            }
            case 4:
            {
                write_4bytes(data, value);
                break;
            }
            case 5:
            {
                write_4bytes(data, value);
                data[4] = 2;
                break;
            }
            default:
            {
                log_errorf("unkown control size:%u", size);
                delete[] data;
                return -1;
            }
        }

        gen_data(data, size);
        delete[] data;

        return RTMP_OK;
    }

    int read_message_header(uint8_t input_fmt, uint16_t input_csid) {
        if (phase_ != CHUNK_STREAM_PHASE_HEADER) {
            return RTMP_OK;
        }
        fmt_  = input_fmt;
        csid_ = input_csid;
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
                session_->try_read(__FILE__, __LINE__);
                return RTMP_NEED_READ_MORE;
            }
            p = (uint8_t*)buffer_p->data();
            uint32_t ts_delta = read_3bytes(p);
            p += 3;

            if (ts_delta >= 0xffffff) {
                if (!buffer_p->require(msg_hd_size + 1)) {
                    session_->try_read(__FILE__, __LINE__);
                    return RTMP_NEED_READ_MORE;
                }
            }
            if (fmt_ == 0) {
                msg_len_ = read_3bytes(p);
                remain_  = msg_len_;
                p += 3;
                type_id_ = *p;
                p++;
                msg_stream_id_ = read_4bytes(p);
                p += 4;
                if (ts_delta >= 0xffffff) {
                    timestamp32_ = read_4bytes(p);
                } else {
                    timestamp32_ = ts_delta;
                }
            } else if (fmt_ == 1) {
                msg_len_ = read_3bytes(p);
                remain_  = msg_len_;
                p += 3;
                type_id_ = *p;
                p++;
                if (ts_delta >= 0xffffff) {//it's timestamp delta
                    timestamp32_ += read_4bytes(p);
                } else {
                    timestamp32_ += ts_delta;
                }
            } else if (fmt_ == 2) {
                if (ts_delta >= 0xffffff) {//it's timestamp delta
                    timestamp32_ += read_4bytes(p);
                } else {
                    timestamp32_ += ts_delta;
                }
            }

            if (ts_delta >= 0xffffff) {
                buffer_p->consume_data(msg_hd_size + 1);
            } else {
                buffer_p->consume_data(msg_hd_size);
            }
        }

        phase_ = CHUNK_STREAM_PHASE_PAYLOAD;
        return RTMP_OK;
    }

    int read_message_payload() {
        if (phase_ != CHUNK_STREAM_PHASE_PAYLOAD) {
            return RTMP_OK;
        }

        data_buffer* buffer_p = session_->get_recv_buffer();
        require_len_ = (remain_ > chunk_size_) ? chunk_size_ : remain_;

        if (!buffer_p->require(require_len_)) {
            return RTMP_NEED_READ_MORE;
        }

        chunk_data_.append_data(buffer_p->data(), require_len_);
        buffer_p->consume_data(require_len_);
        remain_ -= require_len_;

        log_infof("chunck message len:%u, require_len_:%lu, remain_:%lu",
            msg_len_, require_len_, remain_);
        if (remain_ <= 0) {
            chunk_ready_ = true;
        }

        phase_ = CHUNK_STREAM_PHASE_HEADER;
        return RTMP_OK;
    }

    void dump_header() {
        log_infof("basic header: fmt=%d, csid:%d; message header: timestamp=%lu, msglen=%u, typeid:%d, msg streamid:%u",
            fmt_, csid_, timestamp32_, msg_len_, type_id_, msg_stream_id_);
    }

    void dump_payload() {
        char desc[128];

        snprintf(desc, sizeof(desc), "chunk stream payload:%lu", chunk_data_.data_len());
        log_info_data((uint8_t*)chunk_data_.data(), chunk_data_.data_len(), desc);
    }

    int gen_data(uint8_t* data, int len) {
        if (csid_ < 64) {
            uint8_t fmt_csid_data[1];
            fmt_csid_data[0] = (fmt_ << 6) | (csid_ & 0x3f);
            chunk_all_.append_data((char*)fmt_csid_data, sizeof(fmt_csid_data));
        } else if ((csid_ - 64) < 256) {
            uint8_t fmt_csid_data[2];
            fmt_csid_data[0] = (fmt_ << 6) & 0xc0;
            fmt_csid_data[1] = csid_;
            chunk_all_.append_data((char*)fmt_csid_data, sizeof(fmt_csid_data));
        } else if ((csid_ - 64) < 65536) {
            uint8_t fmt_csid_data[3];
            fmt_csid_data[0] = ((fmt_ << 6) & 0xc0) | 0x01;
            write_2bytes(fmt_csid_data + 1, csid_);
            chunk_all_.append_data((char*)fmt_csid_data, sizeof(fmt_csid_data));
        } else {
            log_errorf("csid error:%d", csid_);
            return -1;
        }

        if (fmt_ == 0) {
            uint8_t header_data[11];
            uint8_t* p = header_data;

            write_3bytes(p, timestamp32_);
            p += 3;

            write_3bytes(p, msg_len_);
            p += 3;

            *p = type_id_;
            p++;

            write_4bytes(p, msg_stream_id_);

            chunk_all_.append_data((char*)header_data, sizeof(header_data));
        } else if (fmt_ == 1) {
            uint8_t header_data[7];
            uint8_t* p = header_data;

            write_3bytes(p, timestamp32_);
            p += 3;

            write_3bytes(p, msg_len_);
            p += 3;

            *p = type_id_;

            chunk_all_.append_data((char*)header_data, sizeof(header_data));
        } else if (fmt_ == 2) {
            uint8_t header_data[3];
            uint8_t* p = header_data;
            write_3bytes(p, timestamp32_);
            chunk_all_.append_data((char*)header_data, sizeof(header_data));
        } else if (fmt_ == 3) {
            log_debugf("fmt only append data");
        }

        chunk_all_.append_data((char*)data, (size_t)len);

        return RTMP_OK;
    }

public:
    uint32_t timestamp32_   = 0;
    uint32_t msg_len_       = 0;
    uint8_t  type_id_       = 0;
    uint32_t msg_stream_id_ = 0;
    int64_t  remain_        = 0;
    int64_t  require_len_   = 0;
    uint32_t chunk_size_    = CHUNK_DEF_SIZE;
    data_buffer chunk_all_;
    data_buffer chunk_data_;

private:
    rtmp_session_base* session_ = nullptr;
    uint8_t fmt_  = 0;
    uint16_t csid_ = 0;
    CHUNK_STREAM_PHASE phase_ = CHUNK_STREAM_PHASE_HEADER;
    bool chunk_ready_  = false;
};

using CHUNK_STREAM_PTR = std::shared_ptr<chunk_stream>;

inline int gen_data_by_chunk_stream(rtmp_session_base* session, uint32_t timestamp, uint8_t type_id,
                                uint32_t msg_stream_id, uint32_t chunk_size,
                                data_buffer& input_buffer, data_buffer& ret_buffer)
{
    int cs_count = input_buffer.data_len()/chunk_size;
    int fmt = 0;
    int csid = 3;
    uint8_t* p;
    int len;

    if ((input_buffer.data_len()%chunk_size) > 0) {
        cs_count++;
    }

    //log_infof("cs count:%d, data len:%lu, %lu", cs_count, input_buffer.data_len(), (input_buffer.data_len()%chunk_size));
    for (int index = 0; index < cs_count; index++) {
        if (index == 0) {
            fmt = 0;
        } else {
            fmt = 3;
        }
        p = (uint8_t*)input_buffer.data() + index * chunk_size;
        if (index == (cs_count-1)) {
            if ((input_buffer.data_len()%chunk_size) > 0) {
                len = input_buffer.data_len()%chunk_size;
            } else {
                len = chunk_size;
            }
        } else {
            len = chunk_size;
        }
        chunk_stream* c = new chunk_stream(session, fmt, csid, chunk_size);

        c->timestamp32_ = 0;
        c->msg_len_     = input_buffer.data_len();
        c->type_id_     = type_id;
        c->msg_stream_id_ = msg_stream_id;
        c->gen_data(p, len);
        //log_infof("msg len:%d", len);
        
        ret_buffer.append_data(c->chunk_all_.data(), c->chunk_all_.data_len());

        delete c;
    }
    return RTMP_OK;
}

#endif