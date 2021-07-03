#include "rtmp_control_handler.hpp"
#include "rtmp_server.hpp"
#include "data_buffer.hpp"

rtmp_control_handler::rtmp_control_handler(rtmp_session* session):session_(session)
{
}

rtmp_control_handler::~rtmp_control_handler()
{
}

int rtmp_control_handler::handle_rtmp_command_message(CHUNK_STREAM_PTR cs_ptr, std::vector<AMF_ITERM*>& amf_vec) {
    int ret = 0;
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
        return -1;
    }

    cmd_type = item->desc_str_;

    log_infof("**** receive rtmp command type:%s", cmd_type.c_str());
    if (cmd_type == CMD_Connect) {
        ret = handle_rtmp_connect_command(cs_ptr->msg_stream_id_, amf_vec);
    } else if (cmd_type == CMD_CreateStream) {
        ret = handle_rtmp_createstream_command(cs_ptr->msg_stream_id_, amf_vec);
    } else if (cmd_type == CMD_Publish) {
        ret = handle_rtmp_publish_command(cs_ptr->msg_stream_id_, amf_vec);
    } else if (cmd_type == CMD_Play) {

    }

    if (ret == RTMP_OK) {
        ret = RTMP_NEED_READ_MORE;
    }
    return ret;
}


int rtmp_control_handler::handle_rtmp_connect_command(uint32_t stream_id, std::vector<AMF_ITERM*>& amf_vec) {
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
                    session_->req_.app_ = item->desc_str_;
                }
                iter = item->amf_obj_.find("tcUrl");
                if (iter != item->amf_obj_.end()) {
                    AMF_ITERM* item = iter->second;
                    if (item->get_amf_type() != AMF_DATA_TYPE_STRING) {
                        log_errorf("tcUrl type is not string:%d", (int)item->get_amf_type());
                        return -1;
                    }
                    session_->req_.tcurl_ = item->desc_str_;
                }
                iter = item->amf_obj_.find("flashVer");
                if (iter != item->amf_obj_.end()) {
                    AMF_ITERM* item = iter->second;
                    if (item->get_amf_type() != AMF_DATA_TYPE_STRING) {
                        log_errorf("flash ver type is not string:%d", (int)item->get_amf_type());
                        return -1;
                    }
                    session_->req_.flash_ver_ = item->desc_str_;
                }
                session_->req_.dump();
                break;
            }
            default:
                break;
        }
    }
    return send_rtmp_connect_resp(session_->stream_id_);
}

int rtmp_control_handler::handle_rtmp_createstream_command(uint32_t stream_id, std::vector<AMF_ITERM*>& amf_vec) {
    if (amf_vec.size() < 3) {
        log_errorf("rtmp create stream amf vector count error:%lu", amf_vec.size());
        return -1;
    }
    double transactionId = 0;
    
    session_->req_.stream_id_ = stream_id;
    for (int index = 1; index < amf_vec.size(); index++) {
        AMF_ITERM* item = amf_vec[index];
        switch (item->get_amf_type())
        {
            case AMF_DATA_TYPE_NUMBER:
            {
                log_infof("rtmp create stream transaction id:%f", item->number_);
                transactionId = item->number_;
                session_->req_.transaction_id_ = (int64_t)transactionId;
                break;
            }
            case AMF_DATA_TYPE_OBJECT:
            {
                log_infof("rtmp create stream object:");
                item->dump_amf();
                session_->req_.dump();
                break;
            }
            default:
                break;
        }
    }
    return send_rtmp_create_stream_resp(transactionId);
}

int rtmp_control_handler::handle_rtmp_publish_command(uint32_t stream_id, std::vector<AMF_ITERM*>& amf_vec) {
    if (amf_vec.size() < 3) {
        log_errorf("rtmp publish amf vector count error:%lu", amf_vec.size());
        return -1;
    }
    double transactionId = 0;
    std::string stream_name;
    for (int index = 1; index < amf_vec.size(); index++) {
        AMF_ITERM* item = amf_vec[index];
        switch (item->get_amf_type())
        {
            case AMF_DATA_TYPE_NUMBER:
            {
                log_infof("rtmp publish transaction id:%f", item->number_);
                transactionId = item->number_;
                session_->req_.transaction_id_ = (int64_t)transactionId;
                break;
            }
            case AMF_DATA_TYPE_STRING:
            {
                log_infof("rtmp publish string:%s", item->desc_str_.c_str());
                if (stream_name.empty()) {
                    stream_name = item->desc_str_;
                }
                break;
            }
            case AMF_DATA_TYPE_OBJECT:
            {
                log_infof("rtmp publish object:");
                item->dump_amf();
                break;
            }
            default:
                break;
        }
    }
    return send_rtmp_publish_resp();
}

int rtmp_control_handler::send_rtmp_connect_resp(uint32_t stream_id) {
    data_buffer amf_buffer;
    chunk_stream win_size_cs(session_, 0, 3, session_->chunk_size_);
    chunk_stream peer_bw_cs(session_, 0, 3, session_->chunk_size_);
    chunk_stream set_chunk_size_cs(session_, 0, 3, session_->chunk_size_);

    win_size_cs.gen_control_message(RTMP_CONTROL_WINDOW_ACK_SIZE, 4, 2500000);
    peer_bw_cs.gen_control_message(RTMP_CONTROL_SET_PEER_BANDWIDTH, 5, 2500000);
    set_chunk_size_cs.gen_control_message(RTMP_CONTROL_SET_CHUNK_SIZE, 4, session_->chunk_size_);

    session_->rtmp_send(win_size_cs.chunk_all_.data(), win_size_cs.chunk_all_.data_len());
    session_->rtmp_send(peer_bw_cs.chunk_all_.data(), peer_bw_cs.chunk_all_.data_len());
    session_->rtmp_send(set_chunk_size_cs.chunk_all_.data(), set_chunk_size_cs.chunk_all_.data_len());

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
    int ret = gen_data_by_chunk_stream(session_, 0, RTMP_COMMAND_MESSAGES_AMF0,
                                    stream_id, session_->chunk_size_,
                                    amf_buffer, connect_resp);
    if (ret != RTMP_OK) {
        return ret;
    }
    log_info_data((uint8_t*)connect_resp.data(), connect_resp.data_len(), "connect response");
    session_->rtmp_send(connect_resp.data(), connect_resp.data_len());
    session_->session_phase_ = create_stream_phase;
    return RTMP_OK;
}

int rtmp_control_handler::send_rtmp_create_stream_resp(double transaction_id) {
    data_buffer amf_buffer;
    std::string result_str = "_result";
    double stream_id = (double)session_->stream_id_;

    AMF_Encoder::encode(result_str, amf_buffer);
    AMF_Encoder::encode(transaction_id, amf_buffer);
    AMF_Encoder::encode_null(amf_buffer);
    AMF_Encoder::encode(stream_id, amf_buffer);

    data_buffer create_stream_resp;
    int ret = gen_data_by_chunk_stream(session_, 0, RTMP_COMMAND_MESSAGES_AMF0,
                                    session_->stream_id_, session_->chunk_size_,
                                    amf_buffer, create_stream_resp);
    if (ret != RTMP_OK) {
        return ret;
    }
    log_info_data((uint8_t*)create_stream_resp.data(), create_stream_resp.data_len(), "create stream response");
    session_->rtmp_send(create_stream_resp.data(), create_stream_resp.data_len());
    session_->session_phase_ = create_publish_play_phase;
    return RTMP_OK;
}

int rtmp_control_handler::send_rtmp_publish_resp() {
    data_buffer amf_buffer;
    double transaction_id = 0.0;

    std::string result_str = "onStatus";
    AMF_Encoder::encode(result_str, amf_buffer);
    AMF_Encoder::encode(transaction_id, amf_buffer);
    AMF_Encoder::encode_null(amf_buffer);

    std::unordered_map<std::string, AMF_ITERM*> resp_amf_obj;

    AMF_ITERM* level_item = new AMF_ITERM();
    level_item->set_amf_type(AMF_DATA_TYPE_STRING);
    level_item->desc_str_ = "status";
    resp_amf_obj.insert(std::make_pair("level", level_item));

    AMF_ITERM* code_item = new AMF_ITERM();
    code_item->set_amf_type(AMF_DATA_TYPE_STRING);
    code_item->desc_str_ = "NetStream.Publish.Start";
    resp_amf_obj.insert(std::make_pair("code", code_item));

    AMF_ITERM* desc_item = new AMF_ITERM();
    desc_item->set_amf_type(AMF_DATA_TYPE_STRING);
    desc_item->desc_str_ = "Start publising.";
    resp_amf_obj.insert(std::make_pair("description", desc_item));
    AMF_Encoder::encode(resp_amf_obj, amf_buffer);

    data_buffer publish_resp;
    int ret = gen_data_by_chunk_stream(session_, 0, RTMP_COMMAND_MESSAGES_AMF0,
                                    session_->stream_id_, session_->chunk_size_,
                                    amf_buffer, publish_resp);
    if (ret != RTMP_OK) {
        return ret;
    }
    log_info_data((uint8_t*)publish_resp.data(), publish_resp.data_len(), "publish response");
    session_->rtmp_send(publish_resp.data(), publish_resp.data_len());
    session_->session_phase_ = media_handle_phase;
    return RTMP_OK;
}


int rtmp_control_handler::send_rtmp_ack(uint32_t size) {
    int ret = 0;
    session_->ack_received_ += size;

    if (session_->ack_received_ > session_->remote_window_acksize_) {
        chunk_stream ack(session_, 0, 3, session_->chunk_size_);
        ret += ack.gen_control_message(RTMP_CONTROL_ACK, 4, session_->chunk_size_);
        session_->rtmp_send(ack.chunk_all_.data(), ack.chunk_all_.data_len());
    }
    return RTMP_OK;
}

int rtmp_control_handler::handle_rtmp_control_message(CHUNK_STREAM_PTR cs_ptr) {
    if (cs_ptr->type_id_ == RTMP_CONTROL_SET_CHUNK_SIZE) {
        if (cs_ptr->chunk_data_.data_len() < 4) {
            log_errorf("set chunk size control message size error:%d", cs_ptr->chunk_data_.data_len());
            return -1;
        }
        session_->chunk_size_ = read_4bytes((uint8_t*)cs_ptr->chunk_data_.data());
        log_infof("update chunk size:%u", session_->chunk_size_);
    } else if (cs_ptr->type_id_ == RTMP_CONTROL_WINDOW_ACK_SIZE) {
        if (cs_ptr->chunk_data_.data_len() < 4) {
            log_errorf("window ack size control message size error:%d", cs_ptr->chunk_data_.data_len());
            return -1;
        }
        session_->remote_window_acksize_ = read_4bytes((uint8_t*)cs_ptr->chunk_data_.data());
        log_infof("update remote window ack size:%u", session_->remote_window_acksize_);
    } else {
        log_infof("don't handle rtmp control message:%d", cs_ptr->type_id_);
    }
    return RTMP_OK;
}