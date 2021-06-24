#ifndef RTMP_PUB_HPP
#define RTMP_PUB_HPP

#define RTMP_OK             0
#define RTMP_NEED_READ_MORE 1

enum RTMP_SESSION_PHASE
{
    initial_phase = -1,
    handshake_c0c1_phase,
    handshake_c2_phase,
    connect_phase
};

#endif //RTMP_PUB_HPP