#include "gop_cache.hpp"
#include "logger.hpp"

gop_cache::gop_cache() {
    
}

gop_cache::~gop_cache() {

}

int gop_cache::insert_packet(MEDIA_PACKET_PTR pkt_ptr) {
    if (pkt_ptr->av_type_ == MEDIA_VIDEO_TYPE) {
        if (pkt_ptr->is_seq_hdr_) {
            video_hdr_ = pkt_ptr;
            log_infof("update video hdr len:%lu", video_hdr_->buffer_.data_len());
            return packet_list.size();
        }
        if (pkt_ptr->is_key_frame_) {
            log_infof("update gop cache...");
            packet_list.clear();
        }
    }
    if (pkt_ptr->av_type_ == MEDIA_AUDIO_TYPE) {
        if (pkt_ptr->is_seq_hdr_) {
            audio_hdr_ = pkt_ptr;
            log_infof("update audio hdr len:%lu", audio_hdr_->buffer_.data_len());
            return packet_list.size();
        }
    }

    packet_list.push_back(pkt_ptr);
    return packet_list.size();
}

void gop_cache::writer_gop(av_writer_base* writer_p) {
    if (video_hdr_.get() && video_hdr_->buffer_.data_len() > 0) {
        log_infof("write video hdr len:%lu", video_hdr_->buffer_.data_len());
        writer_p->write_packet(video_hdr_);
    }
    
    if (audio_hdr_.get() && audio_hdr_->buffer_.data_len() > 0) {
        log_infof("write audio hdr len:%lu", audio_hdr_->buffer_.data_len());
        writer_p->write_packet(audio_hdr_);
    }

    for (auto iter : packet_list) {
        MEDIA_PACKET_PTR pkt_ptr = iter;
        log_infof("writer gop packet av type:%d, key:%d, len:%lu",
            pkt_ptr->av_type_, pkt_ptr->is_key_frame_, pkt_ptr->buffer_.data_len());
        writer_p->write_packet(pkt_ptr);
    }
}