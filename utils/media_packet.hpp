#ifndef MEDIA_PACKET_HPP
#define MEDIA_PACKET_HPP
#include "data_buffer.hpp"
#include <stdint.h>
#include <string>
#include <memory>

typedef enum {
    MEDIA_UNKOWN_TYPE = 0,
    MEDIA_VIDEO_TYPE = 1,
    MEDIA_AUDIO_TYPE
} MEDIA_PKT_TYPE;

typedef enum {
    MEDIA_CODEC_UNKOWN = 0,
    MEDIA_CODEC_H264 = 1,
    MEDIA_CODEC_H265,
    MEDIA_CODEC_VP8,
    MEDIA_CODEC_VP9,
    MEDIA_CODEC_AAC = 100,
    MEDIA_CODEC_OPUS,
    MEDIA_CODEC_MP3
} MEDIA_CODEC_TYPE;

#define FLV_VIDEO_KEY_FLAG   1
#define FLV_VIDEO_INTER_FLAG 2

#define FLV_VIDEO_AVC_SEQHDR 0
#define FLV_VIDEO_AVC_NALU   1

#define FLV_VIDEO_H264_CODEC 0x07
#define FLV_VIDEO_H265_CODEC 0x0c

class MEDIA_PACKET
{
public:
    MEDIA_PACKET()
    {
    }
    ~MEDIA_PACKET()
    {
    }

//av common info: 
//    av type;
//    codec type;
//    timestamp;
//    is key frame;
//    is seq hdr;
//    media data in bytes;
public:
    MEDIA_PKT_TYPE av_type_ = MEDIA_UNKOWN_TYPE;
    MEDIA_CODEC_TYPE codec_type_ = MEDIA_CODEC_UNKOWN;
    int64_t timestamp_ = 0;
    bool is_key_frame_ = false;
    bool is_seq_hdr_   = false;
    data_buffer buffer_;

//rtmp info:
public:
    std::string key_;//vhost(option)_appname_streamname
    std::string vhost_;
    std::string app_;
    std::string streamname_;
};

typedef std::shared_ptr<MEDIA_PACKET> MEDIA_PACKET_PTR;

#endif//MEDIA_PACKET_HPP