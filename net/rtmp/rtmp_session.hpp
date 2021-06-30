#ifndef RTMP_SESSION_HPP
#define RTMP_SESSION_HPP
#include "tcp_session.hpp"
#include "data_buffer.hpp"
#include "logger.hpp"
#include "rtmp_pub.hpp"
#include "rtmp_handshake.hpp"
#include "chunk_stream.hpp"
#include "rtmp_session_base.hpp"
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
public:
    rtmp_session(boost::asio::ip::tcp::socket socket, rtmp_server_callbackI* callback) : callback_(callback) {
        session_ptr_ = std::make_shared<tcp_session>(std::move(socket), this);

        try_read();
    }
    virtual ~rtmp_session() {

    }

public://implement rtmp_session_base
    virtual void try_read() override {
        log_infof("try to read....");
        session_ptr_->async_read();
    }

    virtual data_buffer* get_recv_buffer() override {
        return &recv_buffer_;
    }

    virtual void send(char* data, int len) override {
        log_infof("send data len:%d", len);
        session_ptr_->async_write(data, len);
        return;
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
        if ((ret_code != 0) || (sent_size == 0)) {
            log_errorf("write callback code:%d, sent size:%lu", ret_code, sent_size);
            close();
            return;
        }
        log_infof("**** on write callback sent_size:%lu", sent_size);
    }

    virtual void on_read(int ret_code, const char* data, size_t data_size) override {
        log_infof("on read callback return code:%d, data_size:%lu, recv buffer size:%lu",
            ret_code, data_size, recv_buffer_.data_len());
        if ((ret_code != 0) || (data == nullptr) || (data_size == 0)){
            close();
            return;
        }
        
        if (session_phase_ > connect_phase) {
            log_info_data((uint8_t*)data, data_size, "read callback");
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

        if (recv_buffer_.require(1)) {
            p = (uint8_t*)recv_buffer_.data();
            log_infof("chunk 1st byte:0x%02x", *p);
            fmt_  = ((*p) >> 6) & 0x3;
            csid_ = (*p) & 0x3f;
            recv_buffer_.consume_data(1);
        } else {
            try_read();
            return RTMP_NEED_READ_MORE;
        }

        log_infof("rtmp chunk fmt:%d, csid:%d", fmt_, csid_);

        if (csid_ == 0) {
            if (recv_buffer_.require(1)) {//need 1 byte
                p = (uint8_t*)recv_buffer_.data();
                recv_buffer_.consume_data(1);
                csid_ = 64 + *p;
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
            } else {
                try_read();
                return RTMP_NEED_READ_MORE;
            }
        } else {
            log_infof("normal csid:%d", csid_);
        }

        return RTMP_OK;
    }


    int handle_rtmp_connect_command(uint32_t stream_id, std::vector<AMF_ITERM*>& amf_vec) {
        if (amf_vec.size() < 3) {
            log_errorf("rtmp connect amf vector count error:%lu", amf_vec.size());
            return -1;
        }
        double transactionId = 0;
        for (int index = 1; index < amf_vec.size(); index++) {
            AMF_ITERM* item = amf_vec[index];
            switch (item->get_amf_type())
            {
                case AMF_DATA_TYPE_NUMBER:
                {
                    transactionId = item->number_;
                    break;
                }
                case AMF_DATA_TYPE_OBJECT:
                {
                    std::unordered_map<std::string, AMF_ITERM*>::iterator iter = item->amf_obj_.find("app");
                    if (iter != item->amf_obj_.end()) {
                        AMF_ITERM* item = iter->second;
                        if (item->get_amf_type() != AMF_DATA_TYPE_STRING) {
                            log_errorf("app type is not string:%d", (int)item->get_amf_type());
                            return -1;
                        }
                        req_.app_ = item->desc_str_;
                    }

                    iter = item->amf_obj_.find("tcUrl");
                    if (iter != item->amf_obj_.end()) {
                        AMF_ITERM* item = iter->second;
                        if (item->get_amf_type() != AMF_DATA_TYPE_STRING) {
                            log_errorf("tcUrl type is not string:%d", (int)item->get_amf_type());
                            return -1;
                        }
                        req_.tcurl_ = item->desc_str_;
                    }

                    iter = item->amf_obj_.find("flashVer");
                    if (iter != item->amf_obj_.end()) {
                        AMF_ITERM* item = iter->second;
                        if (item->get_amf_type() != AMF_DATA_TYPE_STRING) {
                            log_errorf("flash ver type is not string:%d", (int)item->get_amf_type());
                            return -1;
                        }
                        req_.flash_ver_ = item->desc_str_;
                    }

                    req_.dump();
                    break;
                }
                default:
                    break;
            }
        }

        return send_rtmp_connect_resp(stream_id);
    }

    int send_rtmp_connect_resp(uint32_t stream_id) {
        data_buffer amf_buffer;

        chunk_stream win_size_cs(this, 0, 3, CHUNK_DEF_SIZE);
        chunk_stream peer_bw_cs(this, 0, 3, CHUNK_DEF_SIZE);
        chunk_stream set_chunk_size_cs(this, 0, 3, CHUNK_DEF_SIZE);

        win_size_cs.gen_control_message(RTMP_CONTROL_WINDOW_ACK_SIZE, 4, 2500000);
        peer_bw_cs.gen_control_message(RTMP_CONTROL_SET_PEER_BANDWIDTH, 5, 2500000);
        set_chunk_size_cs.gen_control_message(RTMP_CONTROL_SET_CHUNK_SIZE, 4, CHUNK_DEF_SIZE);

        send(win_size_cs.chunk_all_.data(), win_size_cs.chunk_all_.data_len());
        send(peer_bw_cs.chunk_all_.data(), peer_bw_cs.chunk_all_.data_len());
        send(set_chunk_size_cs.chunk_all_.data(), set_chunk_size_cs.chunk_all_.data_len());

        //encode resp amf
        std::unordered_map<std::string, AMF_ITERM*> resp_amf_obj;

        AMF_ITERM* fms_ver_item = new AMF_ITERM();
        fms_ver_item->set_amf_type(AMF_DATA_TYPE_STRING);
        fms_ver_item->desc_str_ = "FMS/3,0,1,123";
        resp_amf_obj.insert(std::make_pair("fmsVer", fms_ver_item));

        AMF_ITERM* capability_item = new AMF_ITERM();
        capability_item->set_amf_type(AMF_DATA_TYPE_NUMBER);
        capability_item->number_ = 31.0;
        resp_amf_obj.insert(std::make_pair("capabilities", capability_item));

        std::unordered_map<std::string, AMF_ITERM*> event_amf_obj;
        AMF_ITERM* level_item = new AMF_ITERM();
        level_item->set_amf_type(AMF_DATA_TYPE_STRING);
        level_item->desc_str_ = "status";
        event_amf_obj.insert(std::make_pair("level", level_item));

        AMF_ITERM* code_item = new AMF_ITERM();
        code_item->set_amf_type(AMF_DATA_TYPE_STRING);
        code_item->desc_str_ = "NetConnection.Connect.Success";
        event_amf_obj.insert(std::make_pair("code", code_item));

        AMF_ITERM* desc_item = new AMF_ITERM();
        desc_item->set_amf_type(AMF_DATA_TYPE_STRING);
        desc_item->desc_str_ = "Connection succeeded.";
        event_amf_obj.insert(std::make_pair("description", desc_item));

        std::string result_str = "_result";
        AMF_Encoder::encode(result_str, amf_buffer);
        double transaction_id = 1.0;
        AMF_Encoder::encode(transaction_id, amf_buffer);
        AMF_Encoder::encode(resp_amf_obj, amf_buffer);
        AMF_Encoder::encode(event_amf_obj, amf_buffer);

        delete fms_ver_item;
        delete capability_item;

        delete level_item;
        delete code_item;
        delete desc_item;

        data_buffer connect_resp;
        int ret = gen_data_by_chunk_stream(this, 0, RTMP_COMMAND_MESSAGES_AMF0,
                                        stream_id, chunk_size_,
                                        amf_buffer, connect_resp);
        if (ret != RTMP_OK) {
            return ret;
        }

        log_info_data((uint8_t*)connect_resp.data(), connect_resp.data_len(), "connect response");
        send(connect_resp.data(), connect_resp.data_len());

        session_phase_ = create_stream_phase;
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
            cs_ptr = std::make_shared<chunk_stream>(this, fmt_, csid_, chunk_size_);
            cs_map_.insert(std::make_pair(csid_, cs_ptr));
        } else {
            cs_ptr =iter->second;
        }

        ret = cs_ptr->read_message_header(fmt_, csid_);
        if (ret < RTMP_OK) {
            close();
            return ret;
        } else if (ret == RTMP_NEED_READ_MORE) {
            return ret;
        } else {
            log_infof("read message header ok");
            cs_ptr->dump_header();

            ret = cs_ptr->read_message_payload();
            if (ret != RTMP_OK) {
                return ret;
            }
            //cs_ptr->dump_payload();
        }

        return ret;
    }

    int handle_rtmp_control_message(CHUNK_STREAM_PTR cs_ptr) {
        if (cs_ptr->type_id_ == RTMP_CONTROL_SET_CHUNK_SIZE) {
            if (cs_ptr->chunk_data_.data_len() < 4) {
                log_errorf("set chunk size control message size error:%d", cs_ptr->chunk_data_.data_len());
                return -1;
            }
            chunk_size_ = read_4bytes((uint8_t*)cs_ptr->chunk_data_.data());
            log_infof("update chunk size:%u", chunk_size_);
        } else if (cs_ptr->type_id_ == RTMP_CONTROL_WINDOW_ACK_SIZE) {
            if (cs_ptr->chunk_data_.data_len() < 4) {
                log_errorf("window ack size control message size error:%d", cs_ptr->chunk_data_.data_len());
                return -1;
            }
            remote_window_acksize_ = read_4bytes((uint8_t*)cs_ptr->chunk_data_.data());
            log_infof("update remote window ack size:%u", remote_window_acksize_);
        } else {
            log_infof("don't handle rtmp control message:%d", cs_ptr->type_id_);
        }
        return RTMP_OK;
    }

    int send_rtmp_ack(uint32_t size) {
        int ret = 0;
        ack_received_ += size;

        if (ack_received_ > remote_window_acksize_) {
            chunk_stream ack(this, 0, 3, CHUNK_DEF_SIZE);

            ret += ack.gen_control_message(RTMP_CONTROL_ACK, 4, chunk_size_);

            send(ack.chunk_all_.data(), ack.chunk_all_.data_len());
        }

        return RTMP_OK;
    }

    int receive_chunk_stream() {
        CHUNK_STREAM_PTR cs_ptr;
        int ret;
        std::vector<AMF_ITERM*> amf_vec;

        while(true) {
            //receive fmt+csid | basic header | message header | data
            ret = read_chunk_stream(cs_ptr);
            if (ret < RTMP_OK) {
                return ret;
            }

            if (!cs_ptr || !cs_ptr->is_ready()) {
                continue;
            }

            ret = send_rtmp_ack(cs_ptr->chunk_data_.data_len());
            if (ret < RTMP_OK) {
                return ret;
            }

            if ((cs_ptr->type_id_ >= RTMP_CONTROL_SET_CHUNK_SIZE) && (cs_ptr->type_id_ <= RTMP_CONTROL_SET_PEER_BANDWIDTH)) {
                ret = handle_rtmp_control_message(cs_ptr);
                if (ret < RTMP_OK) {
                    return ret;
                }
                if (recv_buffer_.data_len() > 0) {
                    continue;
                }
                break;
            } else if ((cs_ptr->type_id_ == RTMP_COMMAND_MESSAGES_AMF0) || (cs_ptr->type_id_ == RTMP_COMMAND_MESSAGES_AMF3)) {
                //handle rtmp command chunk in amf
                uint8_t* data = (uint8_t*)cs_ptr->chunk_data_.data();
                int len = (int)cs_ptr->chunk_data_.data_len();
    
                while (len > 0) {
                    AMF_ITERM* amf_item = new AMF_ITERM();
                    AMF_Decoder::decode(data, len, *amf_item);
    
                    amf_vec.push_back(amf_item);
                }

                if (amf_vec.size() < 1) {
                    log_errorf("amf vector count error:%lu", amf_vec.size());
                    return -1;
                }
        
                AMF_ITERM* item = amf_vec[0];
                std::string cmd_type;
        
                if (item->get_amf_type() != AMF_DATA_TYPE_STRING) {
                    log_errorf("first amf type error:%d", (int)item->get_amf_type());
                    for (auto iter : amf_vec) {
                        AMF_ITERM* temp = iter;
                        delete temp;
                    }
                    return -1;
                }
        
                cmd_type = item->desc_str_;
        
                log_infof("**** receive rtmp command type:%s", cmd_type.c_str());
                if (cmd_type == CMD_Connect) {
                    ret = handle_rtmp_connect_command(cs_ptr->msg_stream_id_, amf_vec);
                } else if (cmd_type == CMD_CreateStream) {
        
                } else if (cmd_type == CMD_Publish) {
        
                } else if (cmd_type == CMD_Play) {
        
                }
                if (recv_buffer_.data_len() > 0) {
                    //TODO: delete item
                    amf_vec.clear();
                    cs_ptr->chunk_data_.reset();
                    continue;
                }
                break;
            } else {
                //handle video/audio
            }
            break;
        }
        cs_ptr->chunk_data_.reset();
        return ret;
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

        send(s0s1s2, (int)sizeof(s0s1s2));

        try_read();

        return;
    }

    void handle_request() {
        int ret;

        if (session_phase_ == initial_phase) {
            ret = handle_c0c1();
            if (ret < 0) {
                close();
                return;
            } else if (ret == RTMP_NEED_READ_MORE) {
                return;
            }
            recv_buffer_.reset();//be ready to receive c2;
            session_phase_ = handshake_c0c1_phase;
            log_infof("rtmp session phase become c0c1.");

            send_s0s1s2();
            session_phase_ = handshake_c2_phase;
        } else if (session_phase_ == handshake_c2_phase) {
            log_infof("start handle c2...");
            ret = handle_c2();
            if (ret < 0) {
                close();
                return;
            } else if (ret == RTMP_NEED_READ_MORE) {
                return;
            }

            log_infof("rtmp session phase become rtmp connect, buffer len:%lu", recv_buffer_.data_len());
            session_phase_ = connect_phase;
            if (recv_buffer_.data_len() == 0) {
                try_read();
            } else {
                log_infof("start handle rtmp phase:%d", (int)session_phase_);
                ret = receive_chunk_stream();
                if (ret < 0) {
                    close();
                    return;
                }
                try_read();
            }
        } else if (session_phase_ >= connect_phase) {
            log_infof("start handle rtmp phase:%d", (int)session_phase_);

            ret = receive_chunk_stream();
            if (ret < 0) {
                close();
                return;
            }
            try_read();
        }
    }

private:
    std::shared_ptr<tcp_session> session_ptr_;
    rtmp_server_callbackI* callback_ = nullptr;

private:
    RTMP_SESSION_PHASE session_phase_ = initial_phase;
    data_buffer recv_buffer_;
    rtmp_handshake hs_;
    rtmp_request req_;

private:
    uint8_t fmt_  = 0;
    uint16_t csid_ = 0;
    uint32_t chunk_size_ = CHUNK_DEF_SIZE;
    uint32_t remote_window_acksize_ = 2500000;
    uint32_t ack_received_ = 0;
    std::unordered_map<uint8_t, CHUNK_STREAM_PTR> cs_map_;
};

#endif