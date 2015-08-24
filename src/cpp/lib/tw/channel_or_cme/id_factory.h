#pragma once

#include <tw/common/defs.h>
#include <tw/common_thread/queue_locked.h>
#include <tw/common_thread/thread.h>
#include <tw/common_thread/locks.h>
#include <tw/common/singleton.h>

namespace tw {
namespace channel_or_cme {
    
class Id {
public:
    typedef uint64_t Type;
    static const Type MAX_VALUE;
    
public:
    Id();
    Id(const Id& rhs);
    Id& operator=(const Id& rhs);
    
    void clear();
    
public:
    void generate(Type lastId);
    const char* c_str() const;
    
private:    
    char _buffer[21];
    char* _ptrToData;        
};
    
class IdFactory : public tw::common::Singleton<IdFactory> {
    static const uint32_t DEFAULT_SIZE = 1024;
    
public:
    IdFactory();
    ~IdFactory();
    
public:
    bool start(const Id::Type lastId = Id::MAX_VALUE, const uint32_t size = DEFAULT_SIZE);
    void stop();
    
    Id get();
    
private:
    Id::Type operator++();
    void ThreadMain();
    
private:
    typedef tw::common_thread::Lock TLock;
    typedef tw::common_thread::QueueLocked<Id> TQueue;
    
    bool _done;
    uint32_t _size;
    Id::Type _lastId;
    TQueue _queue;
    
    TLock _lock;
    tw::common_thread::ThreadPtr _thread;    
};
    
} // namespace channel_or_cme
} // namespace tw
