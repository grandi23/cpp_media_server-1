#include "rtmp_writer.hpp"
#include "logger.hpp"

rtmp_writer::rtmp_writer(rtmp_session* session):session_(session)
{

}

rtmp_writer::~rtmp_writer()
{

}

int rtmp_writer::write_packet(MEDIA_PACKET_PTR pkt_ptr) {
    log_infof("pkt key:%s, av type:%d, codec type:%d, ts:%ld, data size:%lu",
        pkt_ptr->key_.c_str(), (int)pkt_ptr->av_type_, (int)pkt_ptr->codec_type_,
        pkt_ptr->timestamp_, pkt_ptr->buffer_.data_len());

    return RTMP_OK;
}

void* rtmp_writer::get_source_session() {
    return (void*)session_;
}

void rtmp_writer::close_writer() {

    delete this;
}
