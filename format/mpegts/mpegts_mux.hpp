#ifndef MPEGTS_MUX_HPP
#define MPEGTS_MUX_HPP
#include "mpegts_pub.hpp"
#include "data_buffer.hpp"
#include "format/av_format_interface.hpp"

#define PAT_DEF_INTERVAL 2000 //2000ms

class mpegts_mux
{
public:
    mpegts_mux(av_format_callback* cb);
    ~mpegts_mux();

public:
    int input_packet(MEDIA_PACKET_PTR pkt_ptr);

private:
    int write_pat(uint8_t* data);
    int write_pmt(uint8_t* data);
    int write_pes(uint8_t* data);

private:
    av_format_callback* cb_ = nullptr;

private:
    uint32_t pmt_count_     = 2;
    uint32_t pat_interval_  = PAT_DEF_INTERVAL;
    int64_t last_pat_dts_   = -1;
    uint8_t pat_data_[TS_PACKET_SIZE];
    uint8_t pmt_data_[TS_PACKET_SIZE];

private://about pat
    uint16_t transport_stream_id_ = 1;
    uint8_t  ver_ = 0;
    uint16_t program_number_ = 1;
    uint16_t pmt_pid_ = 0x1001;
};

#endif