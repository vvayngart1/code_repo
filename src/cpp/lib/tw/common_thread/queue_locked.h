#pragma once

#include <tw/common_thread/locks.h>
#include <queue>

namespace tw {
namespace common_thread {

template<class TItem, class TContainer = ::std::deque<TItem>, typename TLock = Lock>
class QueueLocked {
public:
    typedef ::std::queue<TItem, TContainer> queue_type;
    typedef typename queue_type::size_type size_type;
    typedef typename queue_type::value_type value_type;
    

public:
    explicit QueueLocked(const TContainer& c = TContainer()) : _lock(),
                                                               _q(c) {        
    }

    bool empty() const {
        LockGuard<TLock> guard(_lock);
        return _q.empty();
    }

    size_type size() const {
        LockGuard<TLock> guard(_lock);
        return _q.size();
    }

    bool push(const value_type& x) {
        LockGuard<TLock> guard(_lock);
        _q.push(x);
        return true;
    }

    bool pop(value_type& x) {
        LockGuard<TLock> guard(_lock);
        if (_q.empty())
            return false;
        
        x = _q.front();
        _q.pop();
        return true;
    }
    
    void clear() {
        LockGuard<TLock> guard(_lock);
        while ( _q.size() > 0 )
            _q.pop();
    }
    
private:
    mutable TLock _lock;
    queue_type _q;
};

} // common_thread
} // tw
