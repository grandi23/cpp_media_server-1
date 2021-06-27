#ifndef RTMP_SESSION_INTERFACE_HPP
#define RTMP_SESSION_INTERFACE_HPP
#include "data_buffer.hpp"

class rtmp_session_interface
{
public:
    virtual void try_read() = 0;
    virtual data_buffer* get_recv_buffer() = 0;
};

#endif