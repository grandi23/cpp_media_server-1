#ifndef DATA_BUFFER_H
#define DATA_BUFFER_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <string>
#include <vector>

#define EXTRA_LEN (10*1024)

inline int string_split(const std::string& input_str, const std::string& split_str, std::vector<std::string>& output_vec) {
    if (input_str.length() == 0) {
        return 0;
    }
    
    std::string tempString(input_str);
    do {

        size_t pos = tempString.find(split_str);
        if (pos == tempString.npos) {
            output_vec.push_back(tempString);
            break;
        }
        std::string seg_str = tempString.substr(0, pos);
        tempString = tempString.substr(pos+split_str.size());
        output_vec.push_back(seg_str);
    } while(tempString.size() > 0);

    return output_vec.size();
}

class DataBuffer
{
public:
    DataBuffer(size_t data_size = EXTRA_LEN) {
        buffer_      = new char[data_size];
        buffer_size_ = data_size;
        start_       = 0;
        end_         = 0;
        data_len_    = 0;

        memset(buffer_, 0, data_size);
    }

    ~DataBuffer() {
        if (buffer_)
        {
            delete[] buffer_;
        }
    }

public:
    int append_data(char* input_data, size_t input_len) {
        if (data_len_ + (int)input_len > buffer_size_) {
            int new_len = ((data_len_ + (int)input_len + EXTRA_LEN)/4 + 1)*4;
            char* new_buffer_ = new char[new_len];

            memcpy(new_buffer_, buffer_, data_len_);
            memcpy(new_buffer_ + data_len_, input_data, input_len);
            free(buffer_);
            buffer_ = new_buffer_;

            buffer_size_ = new_len;
            data_len_ += (int)input_len;
            end_ += (int)input_len;
            return data_len_;
        }

        memcpy(buffer_ + data_len_, input_data, input_len);
        data_len_ += (int)input_len;
        end_ += (int)input_len;

        return data_len_;
    }

public:
    char* buffer_       = nullptr;
    size_t buffer_size_ = 0;
    int data_len_       = 0;
    int start_          = 0;
    int end_            = 0;
};
#endif //DATA_BUFFER_H