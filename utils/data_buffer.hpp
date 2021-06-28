#ifndef DATA_BUFFER_H
#define DATA_BUFFER_H
#include "logger.hpp"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <string>
#include <vector>

#define EXTRA_LEN (100*1024)

class data_buffer
{
public:
    data_buffer(size_t data_size = EXTRA_LEN) {
        buffer_      = new char[data_size];
        buffer_size_ = data_size;
        start_       = 0;
        end_         = 0;
        data_len_    = 0;

        memset(buffer_, 0, data_size);
    }

    ~data_buffer() {
        if (buffer_)
        {
            delete[] buffer_;
        }
    }

public:
    int append_data(const char* input_data, size_t input_len) {
        if ((size_t)end_ + input_len > buffer_size_) {
            log_infof("it may be over, end:%d, input len:%lu, buffer size:%lu",
                end_, input_len, buffer_size_)
            if (data_len_ + input_len >= buffer_size_) {
                int new_len = data_len_ + (int)input_len + EXTRA_LEN;
                char* new_buffer_ = new char[new_len];

                memcpy(new_buffer_, buffer_ + start_, data_len_);
                memcpy(new_buffer_ + data_len_, input_data, input_len);
                delete[] buffer_;
                buffer_      = new_buffer_;
                buffer_size_ = new_len;
                data_len_    += input_len;

                start_     = 0;
                end_       = data_len_;
                return data_len_;
            }

            memcpy(buffer_, buffer_ + start_, data_len_);
            memcpy(buffer_ + data_len_, input_data, input_len);
            
            data_len_ += input_len;
            start_     = 0;
            end_       = data_len_;
            return data_len_;
        }

        memcpy(buffer_ + end_, input_data, input_len);
        data_len_ += (int)input_len;
        end_ += (int)input_len;

        log_infof("after append end:%d, data len:%d", end_, data_len_);
        return data_len_;
    }

    int consume_data(size_t consume_len) {
        if (consume_len > (size_t)data_len_) {
            return -1;
        }

        start_    += consume_len;
        data_len_ -= consume_len;

        return 0;
    }

    void reset() {
        start_    = 0;
        end_      = 0;
        data_len_ = 0;
    }

    char* data() {
        return buffer_ + start_;
    }

    size_t data_len() {
        return data_len_;
    }

    bool require(size_t len) {
        if ((int)len <= data_len_) {
            return true;
        }
        return false;
    }

public:
    char* buffer_       = nullptr;
    size_t buffer_size_ = 0;
    int data_len_       = 0;
    int start_          = 0;
    int end_            = 0;
};
#endif //DATA_BUFFER_H