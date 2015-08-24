#pragma once

#include <tw/common/uuid.h>
#include <tw/common_thread/queue_locked.h>
#include <tw/common_thread/thread.h>
#include <tw/common/singleton.h>

namespace tw {
namespace channel_or {
    
typedef tw::common::TUuidBuffer TUuidBuffer;
    
class UuidFactory : public tw::common::Singleton<UuidFactory> {
    static const uint32_t DEFAULT_SIZE = 1024;
    
public:
    UuidFactory();
    ~UuidFactory();
    
public:
    bool start(uint32_t size = DEFAULT_SIZE);
    void stop();
    
    TUuidBuffer get();
    
private:
    void ThreadMain();
    
private:
    typedef tw::common_thread::QueueLocked<TUuidBuffer> TQueue;
    
    bool _done;
    uint32_t _size;
    TQueue _queue;
    tw::common_thread::ThreadPtr _thread;    
    tw::common::TUuidGenerator _uuidGenenerator;
    
};
    
} // namespace channel_or
} // namespace tw
