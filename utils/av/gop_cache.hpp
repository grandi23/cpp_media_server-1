#ifndef GOP_CACHE_HPP
#define GOP_CACHE_HPP
#include "media_packet.hpp"
#include <list>

class gop_cache
{
public:
    gop_cache();
    ~gop_cache();

    int insert_packet(MEDIA_PACKET_PTR pkt_ptr);
    void writer_gop(av_writer_base* writer_p);

private:
    std::list<MEDIA_PACKET_PTR> packet_list;
    MEDIA_PACKET_PTR video_hdr_;
    MEDIA_PACKET_PTR audio_hdr_;
};
#endif