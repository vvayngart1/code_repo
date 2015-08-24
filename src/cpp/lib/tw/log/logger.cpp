#include <tw/log/logger.h>
#include <tw/log/defs.h>
#include <tw/common/pool.h>
#include <tw/common/filesystem.h>

#define LOG_LOGGER_INFO std::cout << "INFO: " << TW_LOG_AT
#define LOG_LOGGER_WARN std::cout << "WARN: " << TW_LOG_AT
#define LOG_LOGGER_ERRO std::cout << "ERRO: " << TW_LOG_AT

namespace tw {
namespace log {
    
// class StreamDecorator
//
StreamDecorator::StreamDecorator(const eLogLevel configuredLevel,
                                 const eLogLevel level,
                                 bool outputHeader) {
    _toLog = (configuredLevel <= level) ? true : false;
    if ( !_toLog )
        return;
    
    _stream = tw::log::Logger::instance().getStream();
    
    if ( !outputHeader )
        return;
    
    (*_stream) << tw::common::THighResTime::now();
    
    switch(level) {
        case eLogLevel::kInfo:
            (*_stream) << " INFO: ";
            break;
        case eLogLevel::kWarn:
            (*_stream) << " WARN: ";
            break;
        case eLogLevel::kErro:
            (*_stream) << " ERRO: ";
            break;
        default:
            (*_stream) << " UNKN: ";
            break;
    }
}

StreamDecorator::StreamDecorator(const StreamDecorator& rhs) {
    *this = rhs;
}

StreamDecorator& StreamDecorator::operator=(const StreamDecorator& rhs) {
    if ( this != &rhs ) {
        _toLog = rhs._toLog;
        _configuredLevel = rhs._configuredLevel;
        _level = rhs._level;
        _stream = rhs._stream;
    }
    
    return (*this);
}        

StreamDecorator::~StreamDecorator() {
    if ( !_toLog )
        return;
    
    tw::log::Logger::instance().log(_stream);
}
  
// class Logger
//
typedef tw::common::Pool<TStream, tw::common_thread::Lock> TPool;
static TPool _pool;

Logger::Logger() {
    clear();
}

Logger::~Logger() {
    stop();
}

void Logger::clear() {
    _done = false;
    _threadPipe.clear();
    _thread.reset();
    _pool.clear();
    _settings.clear();
    _level = eLogLevel::kInfo;
    _file.close();
}
    
bool Logger::start(const tw::common::Settings& settings) {
    bool status = true;
    try {
        if ( _thread != NULL ) {
            LOG_LOGGER_ERRO << "Logger already started" << "\n";
            return false;
        }
        
        if ( settings._log_toFile ) {
            // Create subdir if doesn't exist
            //
            if ( !tw::common::Filesystem::exists_dir(tw::common::Filesystem::parent_path(settings._log_fileName)) ) {
                if ( !tw::common::Filesystem::create_dir(tw::common::Filesystem::parent_path(settings._log_fileName)) ) {
                    LOG_LOGGER_ERRO << "Can't create dir for writing file: " << settings._log_fileName << "\n";
                    return false;
                }
            }
            
            _file.open(settings._log_fileName.c_str(), std::ios_base::app);
            if ( !_file.good() ) {
                LOG_LOGGER_ERRO << "Can't open for writing file: " << settings._log_fileName << "\n";
                return false;
            }
        }
        
        const std::string logLevel = std::to_lower(settings._log_level);
        if ( logLevel == "info" ) {
            _level = eLogLevel::kInfo;
        } else if ( logLevel == "warn" ) {
            _level = eLogLevel::kWarn;
        } else if ( logLevel == "erro" ) {
            _level = eLogLevel::kErro;
        } else {
            LOG_LOGGER_ERRO << "Only levels ('info', 'warn', 'erro') are supported: " << settings._log_level << "\n";
            return false;
        }
        
        _settings = settings;
        _thread = tw::common_thread::ThreadPtr(new tw::common_thread::Thread(boost::bind(&Logger::ThreadMain, this)));
    } catch(const std::exception& e) {
        LOG_LOGGER_ERRO << "Exception: "  << e.what() << "\n\n";        
        status = false;
    } catch(...) {
        LOG_LOGGER_ERRO << "Exception: UNKNOWN" << "\n\n";
        status = false;
    }
    
    if ( !status )
        stop();
    
    return status;
}

void Logger::stop() {
    try {
        _done = true;
        if ( _thread != NULL ) {
            _threadPipe.stop();
            _thread->join();
            _thread = tw::common_thread::ThreadPtr();
        }
        
        clear();
    } catch(const std::exception& e) {
        LOG_LOGGER_ERRO << "Exception: "  << e.what() << "\n\n";
    } catch(...) {
        LOG_LOGGER_ERRO << "Exception: UNKNOWN" << "\n\n";
    }
}

inline TStreamPtr Logger::getStream() {
    return TStreamPtr(_pool.obtain(), _pool.getDeleter());
}

inline void Logger::log(const TStreamPtr& stream) {
    try {
        if ( !stream || stream->empty() )
            return;
        
        if ( !_thread ) {
            std::cout << stream->c_str() << std::flush;
            return;
        }
        
        if ( _threadPipe.size() > _settings._log_maxQueueSize ) {
            LOG_LOGGER_ERRO << "LogQueue is FULL and being CLEARED! - queue.size() >" << _settings._log_maxQueueSize << std::endl;
            _threadPipe.clear();
        }
        
        _threadPipe.push(stream);
    } catch(const std::exception& e) {
        LOG_LOGGER_ERRO << "Exception: "  << e.what() << "\n\n";
    } catch(...) {
        LOG_LOGGER_ERRO << "Exception: UNKNOWN" << "\n\n";
    }
}

void Logger::logItem(const TStreamPtr& item) {
    try {
        if ( !item || item->empty() )
            return;
        
        if ( _settings._log_toFile )
            _file << item->c_str() << std::flush;
        
        if ( _settings._log_toConsole )
            std::cout << item->c_str() << std::flush;
        
        item->clear();
    } catch(const std::exception& e) {
        LOG_LOGGER_ERRO << "Exception: "  << e.what() << "\n\n";
    } catch(...) {
        LOG_LOGGER_ERRO << "Exception: UNKNOWN" << "\n\n";
    }
}

void Logger::ThreadMain() {
    LOG_LOGGER_INFO << "Started" << "\n";
    
    try {
        TStreamPtr item;
        while ( !_done ) {
            _threadPipe.read(item);
            logItem(item);
            
            while ( !_done && _threadPipe.try_read(item) ) {
                logItem(item);
            }
        }
        
        while ( _threadPipe.try_read(item) ) {
            logItem(item);
        }
    } catch(const std::exception& e) {
        LOG_LOGGER_ERRO << "Exception: "  << e.what() << "\n\n";
    } catch(...) {
        LOG_LOGGER_ERRO << "Exception: UNKNOWN" << "\n\n";
    }
    
    LOG_LOGGER_INFO << "Finished" << "\n";
}

} // namespace log
} // namespace tw
