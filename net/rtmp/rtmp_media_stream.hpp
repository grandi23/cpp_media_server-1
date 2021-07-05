#ifndef RTMP_MEDIA_STREAM_HPP
#define RTMP_MEDIA_STREAM_HPP
#include "chunk_stream.hpp"
#include "media_packet.hpp"
#include "rtmp_pub.hpp"
#include <stdint.h>
#include <unordered_map>
#include <string>
#include <memory>
#include <list>

class rtmp_session;
class rtmp_request;

typedef std::list<rtmp_writer_base*> RTMP_WRITER_LIST;

class rtmp_media_stream
{
public:
    rtmp_media_stream(rtmp_session* session);
    ~rtmp_media_stream();

public:
    int input_chunk_packet(CHUNK_STREAM_PTR cs_ptr);

public:
    static int add_player(rtmp_request* req, rtmp_session* session);
    static void remove_player(rtmp_request* req, rtmp_session* session);

private:
    int writer_media_packet(MEDIA_PACKET_PTR pkt_ptr);

private:
    static std::unordered_map<std::string, RTMP_WRITER_LIST> writers_map_;//key("app/stream"), RTMP_WRITER_LIST

private:
    rtmp_session* session_ = nullptr;
};

#endif //RTMP_MEDIA_STREAM_HPP