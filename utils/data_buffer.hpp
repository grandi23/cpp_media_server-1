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
    data_buffer(size_t data_size = EXTRA_LEN);
    ~data_buffer();

public:
    int append_data(const char* input_data, size_t input_len);
    int consume_data(size_t consume_len);
    void reset();

    char* data();
    size_t data_len();
    bool require(size_t len);

public:
    char* buffer_       = nullptr;
    size_t buffer_size_ = 0;
    int data_len_       = 0;
    int start_          = 0;
    int end_            = 0;
};
#endif //DATA_BUFFER_H