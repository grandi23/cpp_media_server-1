#ifndef ROOM_SERVICE_HPP
#define ROOM_SERVICE_HPP
#include "net/websocket/wsimple/protoo_pub.hpp"
#include "rtc_session_pub.hpp"
#include "user_info.hpp"
#include "json.hpp"
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

using json = nlohmann::json;

class rtc_subscriber;

typedef std::unordered_map<std::string, std::shared_ptr<rtc_subscriber>> SUBSCRIBER_MAP;

class room_service : public protoo_event_callback, public room_callback_interface
{
public:
    room_service(const std::string& roomId);
    virtual ~room_service();

public:
    virtual void on_open() override;
    virtual void on_failed() override;
    virtual void on_disconnected() override;
    virtual void on_close() override;
    virtual void on_request(const std::string& id, const std::string& method, const std::string& data,
                        protoo_request_interface* feedback_p) override;
    virtual void on_response(int err_code, const std::string& err_message, const std::string& id, 
                        const std::string& data) override;
    virtual void on_notification(const std::string& method, const std::string& data) override;

public:
    virtual void on_rtppacket_publisher2room(rtc_base_session* session, rtc_publisher* publisher, rtp_packet* pkt) override;
    virtual void on_request_keyframe(const std::string& pid, const std::string& sid, uint32_t media_ssrc) override;
    virtual void on_unpublish(const std::string& pid) override;
    virtual void on_unsubscribe(const std::string& pid, const std::string& sid) override;

private:
    void handle_join(const std::string& id, const std::string& method,
                const std::string& data, protoo_request_interface* feedback_p);
    void handle_publish(const std::string& id, const std::string& method,
                const std::string& data, protoo_request_interface* feedback_p);
    void response_publish(const std::string& id, protoo_request_interface* feedback_p,
                int code, const std::string& desc, const std::string& sdp, const std::string& pc_id);
    void handle_unpublish(const std::string& id, const std::string& method,
                const std::string& data, protoo_request_interface* feedback_p);
    void handle_subscribe(const std::string& id, const std::string& method,
                const std::string& data, protoo_request_interface* feedback_p);
    void handle_unsubscribe(const std::string& id, const std::string& method,
                const std::string& data, protoo_request_interface* feedback_p);

private:
    std::shared_ptr<user_info> get_user_info(const std::string& uid);
    std::string get_uid_by_json(json& json_obj);
    std::vector<publisher_info> get_publishers_info_by_json(const json& publishers_json);
    void insert_subscriber(const std::string& publisher_id, std::shared_ptr<rtc_subscriber> subscriber_ptr);
    void notify_userin_to_others(const std::string& uid);
    void notify_userout_to_others(const std::string& uid);
    void notify_publisher_to_others(const std::string& uid, const std::string& pc_id, const std::vector<publisher_info>& publisher_vec);
    void notify_unpublisher_to_others(const std::string& uid, const std::vector<publisher_info>& publisher_vec);
    void notify_others_publisher_to_me(const std::string& uid, std::shared_ptr<user_info> me);
    
private:
    std::string roomId_;
    std::unordered_map<std::string, std::shared_ptr<user_info>> users_;//key: uid, value: user_info
    std::unordered_map<std::string, SUBSCRIBER_MAP> pid2subscribers_;//key: publisher_id, value: rtc_subscribers
};

std::shared_ptr<protoo_event_callback> GetorCreate_room_service(const std::string& roomId);
void remove_room_service(const std::string& roomId);

#endif