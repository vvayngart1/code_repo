#pragma once

#include <tw/config/settings_config_file.h>
#include <tw/functional/utils.hpp>
#include <tw/instr/instrument_manager.h>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

#include <map>

namespace tw {
namespace channel_or {
    
class Settings : public tw::config::SettingsConfigFile {
    typedef tw::config::SettingsConfigFile TParent;
    
public:
    Settings() : TParent("ChannelOrSettings") {
        clear();
        
        TParent::add_options()
            (("storage.file_path"), _storage_file_path, "file storage's path for persistence in case db gets offline")
            (("storage.max_queue_size_db"), _storage_max_queue_size_db, "storage's max memory queue size for db", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(1000))
            (("storage.max_queue_size_file"), _storage_max_queue_size_file, "storage's max memory queue size for file", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(1000))
            (("storage.batch_cache_size"), _storage_batch_cache_size, "storage's batch cache size", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(100))
            (("storage.max_batch_cache_size"), _storage_max_batch_cache_size, "storage's max batch cache size", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(400))
            (("storage.persist_commands"), _storage_persist_commands, "specifies if to save commands log to db", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
        
        ;
    }
    
    void clear() {        
        _storage_file_path.clear();
        _storage_max_queue_size_db = 0;
        _storage_max_queue_size_file = 0;
        _storage_batch_cache_size = 0;
        _storage_max_batch_cache_size = 0;
        _storage_persist_commands = false;
    }
    
public:
    std::string _storage_file_path;
    uint32_t _storage_max_queue_size_db;
    uint32_t _storage_max_queue_size_file;
    uint32_t _storage_batch_cache_size;
    uint32_t _storage_max_batch_cache_size;
    bool _storage_persist_commands;
};
    
} // namespace channel_or
} // namespace tw
