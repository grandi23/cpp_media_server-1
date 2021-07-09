#include "av_outputer.hpp"
#include "media_stream_manager.hpp"
#include <sstream>

int av_outputer::output_packet(MEDIA_PACKET_PTR pkt_ptr) {
    /*
    std::stringstream ss;

    ss << "av type:" << pkt_ptr->av_type_ << ", codec type:" << pkt_ptr->codec_type_
       << ", is key:" << pkt_ptr->is_key_frame_ << ", is seqhdr:" << pkt_ptr->is_seq_hdr_
       << ", timestamp:" << pkt_ptr->timestamp_ << ", stream key:" << pkt_ptr->key_
       << ", data len:" << pkt_ptr->buffer_ptr_->data_len();
    
    log_info_data((uint8_t*)pkt_ptr->buffer_ptr_->data(), pkt_ptr->buffer_ptr_->data_len(), ss.str().c_str());
    */
    return media_stream_manager::writer_media_packet(pkt_ptr);
}
