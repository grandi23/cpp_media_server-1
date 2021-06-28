#ifndef AFM0_HPP
#define AFM0_HPP
#include "byte_stream.hpp"
#include "logger.hpp"
#include <stdint.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <assert.h>

typedef enum {
    AMF_DATA_TYPE_UNKNOWN     = -1,
    AMF_DATA_TYPE_NUMBER      = 0x00,
    AMF_DATA_TYPE_BOOL        = 0x01,
    AMF_DATA_TYPE_STRING      = 0x02,
    AMF_DATA_TYPE_OBJECT      = 0x03,
    AMF_DATA_TYPE_NULL        = 0x05,
    AMF_DATA_TYPE_UNDEFINED   = 0x06,
    AMF_DATA_TYPE_REFERENCE   = 0x07,
    AMF_DATA_TYPE_MIXEDARRAY  = 0x08,
    AMF_DATA_TYPE_OBJECT_END  = 0x09,
    AMF_DATA_TYPE_ARRAY       = 0x0a,
    AMF_DATA_TYPE_DATE        = 0x0b,
    AMF_DATA_TYPE_LONG_STRING = 0x0c,
    AMF_DATA_TYPE_UNSUPPORTED = 0x0d,
} AMF_DATA_TYPE;

class AMF_ITERM
{
public:
    AMF_ITERM(){}
    ~AMF_ITERM(){}

public:
    AMF_DATA_TYPE get_amf_type() {
        return amf_type_;
    }

    void set_amf_type(AMF_DATA_TYPE type) {
        amf_type_ = type;
    }

    void dump_amf() {
        switch (amf_type_)
        {
            case AMF_DATA_TYPE_NUMBER:
            {
                log_infof("amf type: number, value:%f", number_);
                break;
            }
            case AMF_DATA_TYPE_BOOL:
            {
                log_infof("amf type: bool, value:%d", enable_);
                break;
            }
            case AMF_DATA_TYPE_STRING:
            {
                log_infof("amf type: string, value:%s", desc_str_.c_str());
                break;
            }
            case AMF_DATA_TYPE_OBJECT:
            {
                log_infof("amf type: object, count:%lu", amf_obj_.size());
                for (auto iter : amf_obj_) {
                    log_infof("object key:%s", iter.first);
                    iter.second->dump_amf();
                }
                break;
            }
            case AMF_DATA_TYPE_NULL:
            {
                log_infof("amf type: null");
                break;
            }
            case AMF_DATA_TYPE_UNDEFINED:
            {
                log_infof("amf type: undefined");
                break;
            }
            case AMF_DATA_TYPE_REFERENCE:
            {
                log_errorf("not support for reference");
                break;
            }
            case AMF_DATA_TYPE_MIXEDARRAY:
            {
                log_infof("amf type: ecma array, count:%lu", amf_obj_.size());
                for (auto iter : amf_obj_) {
                    log_infof("object key:%s", iter.first);
                    iter.second->dump_amf();
                }
                break;
            }
            case AMF_DATA_TYPE_ARRAY:
            {
                log_infof("amf type: strict array, count:%lu", amf_array_.size());
                for (auto iter : amf_array_) {
                    iter->dump_amf();
                }
                break;
            }
            case AMF_DATA_TYPE_DATE:
            {
                log_infof("amf type: date, number:%f", number_);
                break;
            }
            case AMF_DATA_TYPE_LONG_STRING:
            {
                log_infof("amf type: long string, string:%s", desc_str_.c_str());
                break;
            }
            default:
                break;
        }
    }

public:
    AMF_DATA_TYPE amf_type_ = AMF_DATA_TYPE_UNKNOWN;

public:
    double number_ = 0.0;
    bool enable_   = false;
    std::string desc_str_;
    std::unordered_map<std::string, AMF_ITERM*> amf_obj_;
    std::vector<AMF_ITERM*> amf_array_;
};

class AMF_Decoder
{
public:
    static int decode(uint8_t*& data, int& left_len, AMF_ITERM& amf_item) {
        uint8_t type;
        AMF_DATA_TYPE amf_type;

        type = data[0];
        if ((type > AMF_DATA_TYPE_UNSUPPORTED) || (type < AMF_DATA_TYPE_NUMBER)) {
            return -1;
        }
        data++;
        left_len--;

        amf_type = (AMF_DATA_TYPE)type;
        switch (amf_type) {
            case AMF_DATA_TYPE_NUMBER:
            {
                uint64_t value = read_8bytes(data);

                amf_item.set_amf_type(amf_type);
                amf_item.number_ = av_int2double(value);

                data += 8;
                left_len -= 8;
                break;
            }
            case AMF_DATA_TYPE_BOOL:
            {
                uint8_t value = *data;

                amf_item.set_amf_type(amf_type);
                amf_item.enable_ = (value != 0) ? true : false;

                data++;
                left_len--;
                break;
            }
            case AMF_DATA_TYPE_STRING:
            {
                uint16_t str_len = read_2bytes(data);
                data += 2;
                left_len -= 2;

                std::string desc((char*)data, str_len);

                amf_item.set_amf_type(amf_type);
                amf_item.desc_str_ = desc;

                data += str_len;
                left_len -= str_len;
                break;
            }
            case AMF_DATA_TYPE_OBJECT:
            {
                amf_item.set_amf_type(amf_type);
                int ret = decode_amf_object(data, left_len, amf_item.amf_obj_);
                if (ret != 0) {
                    return ret;
                }
                break;
            }
            case AMF_DATA_TYPE_NULL:
            {
                amf_item.set_amf_type(amf_type);
                break;
            }
            case AMF_DATA_TYPE_UNDEFINED:
            {
                amf_item.set_amf_type(amf_type);
                break;
            }
            case AMF_DATA_TYPE_REFERENCE:
            {
                assert(0);
                return -1;
            }
            case AMF_DATA_TYPE_MIXEDARRAY:
            {
                uint32_t array_len = read_4bytes(data);
                data += 4;
                left_len -= 4;
                (void)array_len;

                amf_item.set_amf_type(AMF_DATA_TYPE_OBJECT);
                int ret = decode_amf_object(data, left_len, amf_item.amf_obj_);
                if (ret != 0) {
                    return ret;
                }
                break;
            }
            case AMF_DATA_TYPE_ARRAY:
            {
                amf_item.set_amf_type(AMF_DATA_TYPE_ARRAY);
                decode_amf_array(data, left_len, amf_item.amf_array_);
                break;
            }
            case AMF_DATA_TYPE_DATE:
            {
                amf_item.set_amf_type(AMF_DATA_TYPE_DATE);
                uint64_t value = read_8bytes(data);

                amf_item.number_ = av_int2double(value);

                data += 8;
                left_len -= 8;

                data += 2;
                left_len -= 2;
                break;
            }
            case AMF_DATA_TYPE_LONG_STRING:
            {
                uint32_t str_len = read_4bytes(data);
                data += 4;
                left_len -= 4;

                std::string desc((char*)data, str_len);

                amf_item.set_amf_type(AMF_DATA_TYPE_LONG_STRING);
                amf_item.desc_str_ = desc;

                data += str_len;
                left_len -= str_len;
                break;
            }
            case AMF_DATA_TYPE_UNSUPPORTED:
            {
                amf_item.set_amf_type(AMF_DATA_TYPE_LONG_STRING);
                break;
            }
            default:
            {
                amf_item.set_amf_type(AMF_DATA_TYPE_UNKNOWN);
                return -1;
            }
        }

        return 0;
    }

    static int decode_amf_object(uint8_t*& data, int& len, std::unordered_map<std::string, AMF_ITERM*>& amf_obj) {
        //amf object: <key: string type> : <value: amf object type>
        
        while (len > 0) {
            uint16_t key_len = read_2bytes(data);
            data += 2;
            len -= 2;

            if (key_len == 0) {
                assert((AMF_DATA_TYPE)data[0] == AMF_DATA_TYPE_OBJECT_END);
                break;
            }
            std::string key((char*)data, key_len);
            log_infof("amf key len:%d, key:%s", key_len, key.c_str());
            data += key_len;
            len -= key_len;

            if ((data[0] > AMF_DATA_TYPE_UNSUPPORTED) || (data[0] < AMF_DATA_TYPE_NUMBER)) {
                return -1;
            }
            AMF_ITERM* amf_item = new AMF_ITERM();
            int ret = decode(data, len, *amf_item);
            if (ret != 0) {
                return ret;
            }
            amf_item->dump_amf();
            amf_obj.insert(std::make_pair(key, amf_item));

        }
        return 0;
    }

    static int decode_amf_array(uint8_t*& data, int& len, std::vector<AMF_ITERM*>& amf_array) {
        uint32_t array_len = read_4bytes(data);
        data += 4;
        len -= 4;

        for (uint32_t index = 0; index < array_len; index++) {
            AMF_ITERM* item = new AMF_ITERM();
            int ret = decode(data, len, *item);
            if (ret != 0) {
                return ret;
            }
            amf_array.push_back(item);
        }
        return 0;
    }
};

#endif