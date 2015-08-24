#pragma once

#include <boost/thread.hpp>

namespace tw {
namespace common_thread {
    class LockNull {
    public:
        void lock() {            
        }
        
        bool try_lock() {
            return true;
        }
        
        void unlock() {            
        }
    };
    
    typedef boost::recursive_mutex Lock;
    
    template <typename TLock>
    class LockGuard : public boost::lock_guard<TLock> {
        public:
            LockGuard(TLock& lock) : boost::lock_guard<TLock>(lock) {                
            }
    };
    
} // common_thread
} // tw

