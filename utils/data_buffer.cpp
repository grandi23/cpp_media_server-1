#include "data_buffer.hpp"

data_buffer::data_buffer(size_t data_size) {
    buffer_      = new char[data_size];
    buffer_size_ = data_size;
    start_       = 0;
    end_         = 0;
    data_len_    = 0;
    memset(buffer_, 0, data_size);
}

data_buffer::~data_buffer() {
    if (buffer_)
    {
        delete[] buffer_;
    }
}

int data_buffer::append_data(const char* input_data, size_t input_len) {
    if ((input_data == nullptr) || (input_len == 0)) {
        return 0;
    }

    if ((size_t)end_ + input_len > buffer_size_) {
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

    return data_len_;
}

int data_buffer::consume_data(size_t consume_len) {
    if (consume_len > (size_t)data_len_) {
        log_errorf("consume_len:%lu, data_len_:%d", consume_len, data_len_);
        return -1;
    }

    start_    += consume_len;
    data_len_ -= consume_len;

    return 0;
}

void data_buffer::reset() {
    start_    = 0;
    end_      = 0;
    data_len_ = 0;
}

char* data_buffer::data() {
    return buffer_ + start_;
}

size_t data_buffer::data_len() {
    return data_len_;
}

bool data_buffer::require(size_t len) {
    if ((int)len <= data_len_) {
        return true;
    }
    return false;
}