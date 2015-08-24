#pragma once

#include <tw/common/defs.h>
#include <tw/common/singleton.h>
#include <tw/common/settings.h>

#include <tw/common_str_util/fast_stream.h>
#include <tw/common_thread/locks.h>
#include <tw/common_thread/thread.h>
#include <tw/common_thread/thread_pipe.h>

#include <tw/generated/enums_logger.h>

#include <boost/shared_ptr.hpp>

#include <fstream>

namespace tw {
namespace log {
    
// NOTE: Right now log info is limited to 6K per entry
//
typedef tw::common_str_util::FastStream<1024*6> TStream;
typedef boost::shared_ptr<TStream> TStreamPtr;
    
class StreamDecorator {
public:
    StreamDecorator(const eLogLevel configuredLevel, const eLogLevel level, bool outputHeader = true);
    StreamDecorator(const StreamDecorator& rhs);
    
    StreamDecorator& operator=(const StreamDecorator& rhs);
    
    ~StreamDecorator();
    
public:    
    StreamDecorator& operator<<(char v) {
        if ( _toLog )
            (*_stream) << v;
        
        return (*this);
    }

    StreamDecorator& operator<<(int16_t v) {
        if ( _toLog )
            (*_stream) << v;
        
        return (*this);
    }

    StreamDecorator& operator<<(uint16_t v) {
        if ( _toLog )
            (*_stream) << v;
        
        return (*this);
    }

    StreamDecorator& operator<<(int32_t v) {
        if ( _toLog )
            (*_stream) << v;
        
        return (*this);
    }

    StreamDecorator& operator<<(uint32_t v) {
        if ( _toLog )
            (*_stream) << v;
        
        return (*this);
    }

    StreamDecorator& operator<<(int64_t v) {
        if ( _toLog )
            (*_stream) << v;
        
        return (*this);
    }

    StreamDecorator& operator<<(uint64_t v) {
        if ( _toLog )
            (*_stream) << v;
        
        return (*this);
    }

    StreamDecorator& operator<<(float v) {
        if ( _toLog )
            (*_stream) << v;
        
        return (*this);
    }

    StreamDecorator& operator<<(double v) {
        if ( _toLog )
            (*_stream) << v;
        
        return (*this);
    }
    
    StreamDecorator& operator<<(long double v) {
        if ( _toLog )
            (*_stream) << static_cast<double>(v);
        
        return (*this);
    }

    StreamDecorator& operator<<(void* v) {
        if ( _toLog )
            (*_stream) << v;
        
        return (*this);
    }		

    StreamDecorator& operator<<(const char* v) {
        if ( _toLog )
            (*_stream) << v;
        
        return (*this);
    }            

    StreamDecorator& operator<<(const std::string& v) {
        if ( _toLog )
            (*_stream) << v;
        
        return (*this);
    }
    
    StreamDecorator& operator<<(const tw::common::THighResTime& v) {
        if ( _toLog )
            (*_stream) << v;
        
        return (*this);
    }
    
    StreamDecorator& operator<<(const tw::common::TUuid& v) {
        if ( _toLog )
            (*_stream) << v;
        
        return (*this);
    }    
    
    StreamDecorator& operator<<(const std::_Setprecision& v) {
        if ( _toLog )
            (*_stream) << v;            
        
        return (*this);
    }
    
    bool _toLog;
    eLogLevel _configuredLevel;
    eLogLevel _level;
    TStreamPtr _stream;
};
    
    
class Logger : public tw::common::Singleton<Logger> {
    friend class StreamDecorator;
    
public:
    Logger();
    ~Logger();
    
    void clear();
    
public:
   StreamDecorator log() {
       StreamDecorator s(_level, eLogLevel::kInfo, false);
       return s;
   }
   
   StreamDecorator log(eLogLevel level) {
       StreamDecorator s(_level, level);
       return s;
   }
    
public:
    bool start(const tw::common::Settings& settings);
    void stop();
   
private:
    TStreamPtr getStream();
    void log(const TStreamPtr& stream);
    
    void logItem(const TStreamPtr& item);
    void ThreadMain();
    
private:
    typedef tw::common_thread::ThreadPipe<TStreamPtr> TThreadPipe;
    
    bool _done;
    TThreadPipe _threadPipe;
    tw::common_thread::ThreadPtr _thread;
    
    tw::common::Settings _settings;
    eLogLevel _level;
    
    std::ofstream _file;
};

    
} // namespace log
} // namespace tw
