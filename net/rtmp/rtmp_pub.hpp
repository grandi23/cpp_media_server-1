#ifndef RTMP_PUB_HPP
#define RTMP_PUB_HPP
#include "media_packet.hpp"

#define RTMP_OK             0
#define RTMP_NEED_READ_MORE 1

#define CHUNK_DEF_SIZE      128

typedef enum {
    initial_phase = -1,
    handshake_c2_phase,
    connect_phase,
    create_stream_phase,
    create_publish_play_phase,
    media_handle_phase
} RTMP_SESSION_PHASE;

typedef enum {
	RTMP_CONTROL_SET_CHUNK_SIZE = 1,//idSetChunkSize = 1,
	RTMP_CONTROL_ABORT_MESSAGE,//idAbortMessage,
	RTMP_CONTROL_ACK,//idAck,
	RTMP_CONTROL_USER_CTRL_MESSAGES,//idUserControlMessages,
	RTMP_CONTROL_WINDOW_ACK_SIZE,//idWindowAckSize,
	RTMP_CONTROL_SET_PEER_BANDWIDTH,//idSetPeerBandwidth,
    RTMP_MEDIA_PACKET_AUDIO = 8,
    RTMP_MEDIA_PACKET_VIDEO = 9,
    RTMP_COMMAND_MESSAGES_META_DATA3 = 15,
    RTMP_COMMAND_MESSAGES_AMF3 = 17,
    RTMP_COMMAND_MESSAGES_META_DATA0 = 18,
    RTMP_COMMAND_MESSAGES_AMF0 = 20
} RTMP_CONTROL_TYPE;

typedef enum {
	Event_streamBegin = 0,
	Event_streamEOF,
	Event_streamDry,
	Event_setBufferLen,
	Event_streamIsRecorded,
	Event_pingRequest,
	Event_pingResponse,
 } RTMP_CTRL_EVENT_TYPE;

#define	CMD_Connect       "connect"
#define	CMD_Fcpublish     "FCPublish"
#define	CMD_ReleaseStream "releaseStream"
#define	CMD_CreateStream  "createStream"
#define	CMD_Publish       "publish"
#define	CMD_FCUnpublish   "FCUnpublish"
#define	CMD_DeleteStream  "deleteStream"
#define	CMD_Play          "play"

#endif //RTMP_PUB_HPP