#include "media_stream_manager.hpp"
#include "media_packet.hpp"
#include "logger.hpp"

std::unordered_map<std::string, MEDIA_STREAM_PTR> media_stream_manager::writers_map_;

int media_stream_manager::add_player(av_writer_base* writer_p) {
    std::string key_str = writer_p->get_key();
    std::string writerid = writer_p->get_writerid();

    std::unordered_map<std::string, MEDIA_STREAM_PTR>::iterator iter = writers_map_.find(key_str);
    if (iter == media_stream_manager::writers_map_.end()) {
        MEDIA_STREAM_PTR new_stream_ptr = std::make_shared<media_stream>();

        new_stream_ptr->writer_map_.insert(std::make_pair(writerid, writer_p));
        media_stream_manager::writers_map_.insert(std::make_pair(key_str, new_stream_ptr));
        log_infof("add player request:%s in new writer list", key_str.c_str());
        return 1;
    }

    log_infof("add play request:%s", key_str.c_str());
    iter->second->writer_map_.insert(std::make_pair(writerid, writer_p));
    return iter->second->writer_map_.size();
}

void media_stream_manager::remove_player(av_writer_base* writer_p) {
    std::string key_str = writer_p->get_key();
    std::string writerid = writer_p->get_writerid();

    log_infof("remove player key:%s", key_str.c_str());
    std::unordered_map<std::string, MEDIA_STREAM_PTR>::iterator map_iter = media_stream_manager::writers_map_.find(key_str);
    if (map_iter == media_stream_manager::writers_map_.end()) {
        log_warnf("it's empty when remove player:%s", key_str.c_str());
        return;
    }

    auto writer_iter = map_iter->second->writer_map_.find(writerid);
    if (writer_iter != map_iter->second->writer_map_.end()) {
        map_iter->second->writer_map_.erase(writer_iter);
    }

    if (map_iter->second->writer_map_.empty()) {
        media_stream_manager::writers_map_.erase(map_iter);
        log_infof("remove player list empty, key:%s", key_str.c_str());
    }
    log_infof("remove play request:%s", key_str.c_str());
    return;
}

int media_stream_manager::writer_media_packet(MEDIA_PACKET_PTR pkt_ptr) {
    std::unordered_map<std::string, MEDIA_STREAM_PTR>::iterator iter = media_stream_manager::writers_map_.find(pkt_ptr->key_);
    if (iter == media_stream_manager::writers_map_.end()) {
        //create new stream and insert pkt in gop_cache
        MEDIA_STREAM_PTR new_stream_ptr = std::make_shared<media_stream>();
        media_stream_manager::writers_map_.insert(std::make_pair(pkt_ptr->key_, new_stream_ptr));
        new_stream_ptr->cache_.insert_packet(pkt_ptr);
        return 0;
    }

    iter->second->cache_.insert_packet(pkt_ptr);

    for (auto item : iter->second->writer_map_) {
        auto writer = item.second;
        if (!writer->is_inited()) {
            writer->set_init_flag(true);
            iter->second->cache_.writer_gop(writer);
        } else {
            writer->write_packet(pkt_ptr);
        }
    }

    return 0;
}