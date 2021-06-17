#ifndef TCP_SERVER_BASE_H
#define TCP_SERVER_BASE_H
#include <memory>

class TcpSessionCallback
{
public:
    virtual void on_write(int ret_code, size_t sent_size) = 0;
    virtual void on_read(int ret_code, char* data, size_t data_size) = 0;
};

#endif //TCP_SERVER_BASE_H