#ifndef RTMP_WRITER_HPP
#define RTMP_WRITER_HPP
#include "rtmp_pub.hpp"
#include <memory>

class rtmp_session;
class rtmp_writer : public rtmp_writer_base
{
public:
    rtmp_writer(rtmp_session* session);
    virtual ~rtmp_writer();

public:
    virtual int write_packet(MEDIA_PACKET_PTR) override;
    virtual void* get_source_session() override;
    virtual void close_writer() override;

private:
    rtmp_session* session_;
};

typedef std::shared_ptr<rtmp_writer> RTMP_WRITER_PTR;

#endif