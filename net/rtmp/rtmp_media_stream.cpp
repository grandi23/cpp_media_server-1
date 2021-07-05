#include "rtmp_media_stream.hpp"
#include "rtmp_session.hpp"
#include "rtmp_pub.hpp"
#include "rtmp_request.hpp"
#include "media_packet.hpp"
#include "logger.hpp"
#include "rtmp_writer.hpp"

std::unordered_map<std::string, RTMP_WRITER_LIST> rtmp_media_stream::writers_map_;

rtmp_media_stream::rtmp_media_stream(rtmp_session* session):session_(session)
{
}

rtmp_media_stream::~rtmp_media_stream()
{
}

int rtmp_media_stream::add_player(rtmp_request* req, rtmp_session* session) {
    rtmp_writer* write_p = new rtmp_writer(session);

    std::unordered_map<std::string, RTMP_WRITER_LIST>::iterator iter = rtmp_media_stream::writers_map_.find(req->key_);
    if (iter == rtmp_media_stream::writers_map_.end()) {
        RTMP_WRITER_LIST writers;

        writers.push_back(write_p);
        rtmp_media_stream::writers_map_.insert(std::make_pair(req->key_, writers));
        log_infof("add player request:%s in new writer list", req->key_.c_str());
        return RTMP_OK;
    }

    log_infof("add play request:%s", req->key_.c_str());
    iter->second.push_back(write_p);
    return RTMP_OK;
}

void rtmp_media_stream::remove_player(rtmp_request* req, rtmp_session* session) {
    std::unordered_map<std::string, RTMP_WRITER_LIST>::iterator map_iter = rtmp_media_stream::writers_map_.find(req->key_);
    if (map_iter == rtmp_media_stream::writers_map_.end()) {
        log_warnf("it's empty when remove player:%s", req->key_.c_str());
        return;
    }

    for (std::list<rtmp_writer_base*>::iterator writer_iter = map_iter->second.begin();
        writer_iter != map_iter->second.end();
        writer_iter++)
    {
        rtmp_writer_base* writer_p = *writer_iter;
        void* src_session = writer_p->get_source_session();
        if (((uint64_t)src_session) == ((uint64_t)session)) {
            writer_p->close_writer();
            map_iter->second.erase(writer_iter);
            break;
        }
    }

    if (map_iter->second.empty()) {
        rtmp_media_stream::writers_map_.erase(map_iter);
        log_infof("remove player list empty, key:%s", req->key_.c_str());
    }
    log_infof("remove play request:%s", req->key_.c_str());
    return;
}

int rtmp_media_stream::writer_media_packet(MEDIA_PACKET_PTR pkt_ptr) {
    std::unordered_map<std::string, RTMP_WRITER_LIST>::iterator iter = rtmp_media_stream::writers_map_.find(pkt_ptr->key_);
    if (iter == rtmp_media_stream::writers_map_.end()) {
        return RTMP_OK;
    }
    
    for (rtmp_writer_base* write_item : iter->second) {
        write_item->write_packet(pkt_ptr);
    }

    return RTMP_OK;
}

int rtmp_media_stream::input_chunk_packet(CHUNK_STREAM_PTR cs_ptr) {
    MEDIA_PACKET_PTR pkt_ptr = std::make_shared<MEDIA_PACKET>();

    if (cs_ptr->chunk_data_.data_len() < 2) {
        log_errorf("rtmp chunk media size:%lu is too small", cs_ptr->chunk_data_.data_len());
        return -1;
    }
    uint8_t* p = (uint8_t*)cs_ptr->chunk_data_.data();

    if (cs_ptr->type_id_ == RTMP_MEDIA_PACKET_VIDEO) {
        uint8_t codec = p[0] & 0x0f;
        pkt_ptr->av_type_ = MEDIA_VIDEO_TYPE;
        if (codec == FLV_VIDEO_H264_CODEC) {
            pkt_ptr->codec_type_ = MEDIA_CODEC_H264;
        } else if (codec == FLV_VIDEO_H265_CODEC) {
            pkt_ptr->codec_type_ = MEDIA_CODEC_H265;
        } else {
            log_errorf("does not support video codec typeid:%d, 0x%02x", cs_ptr->type_id_, p[0]);
            return -1;
        }

        uint8_t frame_type = (p[0] & 0xf0) >> 4;
        uint8_t nalu_type = p[1];
        if (frame_type == FLV_VIDEO_KEY_FLAG) {
            if (nalu_type == FLV_VIDEO_AVC_SEQHDR) {
                pkt_ptr->is_seq_hdr_ = true;
            } else if (nalu_type == FLV_VIDEO_AVC_NALU) {
                pkt_ptr->is_key_frame_ = true;
            } else {
                log_errorf("input flv video error, 0x%02x 0x%02x", p[0], p[1]);
                return -1;
            }
        } else if (frame_type == FLV_VIDEO_INTER_FLAG) {
            pkt_ptr->is_key_frame_ = false;
        }
        log_infof("flv video codec:%d, is key:%d, is hdr:%d, 0x%02x 0x%02x",
            pkt_ptr->codec_type_, pkt_ptr->is_key_frame_, pkt_ptr->is_seq_hdr_,
            p[0], p[1]);
    } else if (cs_ptr->type_id_ == RTMP_MEDIA_PACKET_AUDIO) {
        pkt_ptr->av_type_ = MEDIA_AUDIO_TYPE;
        if ((p[0] & 0xf0) == 0xa0) {
            pkt_ptr->codec_type_ = MEDIA_CODEC_AAC;
            if(p[1] == 0x00) {
                pkt_ptr->is_seq_hdr_ = true;
            } else if (p[1] == 0x01) {
                pkt_ptr->is_key_frame_ = false;
                pkt_ptr->is_seq_hdr_   = false;
            }
        } else {
            log_errorf("does not support audio codec typeid:%d, 0x%02x", cs_ptr->type_id_, p[0]);
            return -1;
        }
        log_infof("flv audio codec:%d, is key:%d, is hdr:%d, 0x%02x 0x%02x",
            pkt_ptr->codec_type_, pkt_ptr->is_key_frame_, pkt_ptr->is_seq_hdr_,
            p[0], p[1]);
    } else {
        log_warnf("rtmp input unkown media type:%d", cs_ptr->type_id_);
        return RTMP_OK;
    }

    pkt_ptr->timestamp_  = cs_ptr->timestamp32_;
    pkt_ptr->buffer_.reset();
    pkt_ptr->buffer_.append_data(cs_ptr->chunk_data_.data(), cs_ptr->chunk_data_.data_len());

    pkt_ptr->app_        = session_->req_.app_;
    pkt_ptr->streamname_ = session_->req_.stream_name_;
    pkt_ptr->key_        = session_->req_.key_;

    writer_media_packet(pkt_ptr);
    return RTMP_OK;
}