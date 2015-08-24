#pragma once

#include <tw/common_thread/locks.h>

namespace tw {
namespace common {    

class SignalCatcher {
public:
    SignalCatcher();
    ~SignalCatcher();
    
public:    
    void run();
    
public:
    void handle_stop(int32_t v);
    
private:
    typedef boost::mutex TLock;
    typedef boost::mutex::scoped_lock TLockGuard;
    typedef boost::condition_variable TEvent;
    
    mutable TLock _lock;
    TEvent _event;
};

} // namespace common
} // namespace tw
