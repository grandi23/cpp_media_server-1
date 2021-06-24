#ifndef BYTE_STREAM_HPP
#define BYTE_STREAM_HPP
#include <stdint.h>
#include <stddef.h>

inline uint32_t read_4bytes(const uint8_t* data) {
    uint32_t value = 0;
    uint8_t* output = (uint8_t*)&value;

    output[3] = *data++;
    output[2] = *data++;
    output[1] = *data++;
    output[0] = *data++;

    return value;
}

inline uint32_t read_3bytes(const uint8_t* data) {
    uint32_t value = 0;
    uint8_t* output = (uint8_t*)&value;

    output[2] = *data++;
    output[1] = *data++;
    output[0] = *data++;

    return value;
}

inline uint16_t read_2bytes(const uint8_t* data) {
    uint16_t value = 0;
    uint8_t* output = (uint8_t*)&value;

    output[1] = *data++;
    output[0] = *data++;

    return value;
}

inline void write_4bytes(uint8_t* data, uint32_t value) {
    uint8_t* p = data;
    uint8_t* pp = (uint8_t*)&value;

    *p++ = pp[3];
    *p++ = pp[2];
    *p++ = pp[1];
    *p++ = pp[0];
}

inline void write_3bytes(uint8_t* data, uint32_t value) {
    uint8_t* p = data;
    uint8_t* pp = (uint8_t*)&value;

    *p++ = pp[2];
    *p++ = pp[1];
    *p++ = pp[0];
}

inline void write_2bytes(uint8_t* data, uint16_t value) {
    uint8_t* p = data;
    uint8_t* pp = (uint8_t*)&value;

    *p++ = pp[1];
    *p++ = pp[0];
}

inline bool bytes_is_equal(const char* p1, const char* p2, size_t len) {
    for (size_t index = 0; index < len; index++) {
        if (p1[index] != p2[index]) {
            return false;
        }
    }
    return true;
}

#endif //BYTE_STREAM_HPP