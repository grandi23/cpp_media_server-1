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
    if (pkt_ptr->av_type_ == MEDIA_METADATA_TYPE) {
        metadata_hdr_ = pkt_ptr;
        log_infof("update rtmp metadata len:%lu", metadata_hdr_->buffer_.data_len())
    }

    packet_list.push_back(pkt_ptr);
    return packet_list.size();
}

void gop_cache::writer_gop(av_writer_base* writer_p) {
    if (metadata_hdr_.get() && metadata_hdr_->buffer_.data_len() > 0) {
        log_info_data((uint8_t*)metadata_hdr_->buffer_.data(), metadata_hdr_->buffer_.data_len(), "metadata hdr");
        writer_p->write_packet(metadata_hdr_);
    }

    if (video_hdr_.get() && video_hdr_->buffer_.data_len() > 0) {
        log_info_data((uint8_t*)video_hdr_->buffer_.data(), video_hdr_->buffer_.data_len(), "video hdr data");
        writer_p->write_packet(video_hdr_);
    }
    
    if (audio_hdr_.get() && audio_hdr_->buffer_.data_len() > 0) {
        log_info_data((uint8_t*)audio_hdr_->buffer_.data(), audio_hdr_->buffer_.data_len(), "audio hdr data");
        writer_p->write_packet(audio_hdr_);
    }

    for (auto iter : packet_list) {
        MEDIA_PACKET_PTR pkt_ptr = iter;
        writer_p->write_packet(pkt_ptr);
    }
}