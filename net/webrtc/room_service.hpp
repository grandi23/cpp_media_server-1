#ifndef ROOM_SERVICE_HPP
#define ROOM_SERVICE_HPP
#include "net/websocket/wsimple/protoo_pub.hpp"
#include <string>
#include <memory>
#include <unordered_map>

class user_info
{
public:
    user_info(const std::string& uid, const std::string& roomId, protoo_request_interface* fb):uid_(uid)
        , roomId_(roomId)
        , feedback_(fb)
    {
    }
    ~user_info()
    {
    }

    
public:
    std::string uid_;
    std::string roomId_;
    protoo_request_interface* feedback_ = nullptr;
};

class room_service : public protoo_event_callback
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

private:
    void handle_join(const std::string& id, const std::string& method, const std::string& data,
                protoo_request_interface* feedback_p);

private:
    std::string roomId_;
    std::unordered_map<std::string, std::shared_ptr<user_info>> users_;//key: uid, value: user_info
};

std::shared_ptr<protoo_event_callback> GetorCreate_room_service(const std::string& roomId);
void remove_room_service(const std::string& roomId);

#endif