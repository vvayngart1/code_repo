#pragma once

#include <tw/common_thread/locks.h>

#include <deque>

namespace tw {
namespace common_thread {   
    
template < typename TValue, typename TContainerType = std::deque<TValue> >
class ThreadPipe : private boost::noncopyable {
    typedef boost::mutex TLock;
    typedef boost::mutex::scoped_lock TLockGuard;
    typedef boost::condition_variable TEvent;
    
public:
    typedef TContainerType TContainer;
    
public:
    ThreadPipe () {
        _isDone = false;
    }
    
    ~ThreadPipe() {
        stop();
    }
    
    bool empty() const {
        TLockGuard lock(_lock);
        return _container.empty();
    }
    
    size_t size() const {
        TLockGuard lock(_lock);
        return _container.size();
    }
    
    TContainer getAllItems() const {
        TLockGuard lock(_lock);
        return _container;
    }
    
    void clear() {
        TLockGuard lock(_lock);
        _container.clear();
    }
    
    bool isStopped() const {
        TLockGuard lock(_lock);
        return _isDone;
    }
    
public:    
    void push(const TValue& value) {
        {
            TLockGuard lock(_lock);
            _container.push_back(value);
        }
        signal();
    }
    
    void push_front(const TValue& value) {
        {
            TLockGuard lock(_lock);
            _container.push_front(value);
        }
        signal();
    }
    
    void pop() {
         TLockGuard lock(_lock);
         if ( !_container.empty() )
             _container.pop_front();
    }
    
    bool try_read(TValue& value, bool remove=true) {
        TLockGuard lock(_lock);
        if ( _container.empty() )
            return false;
        
        getValue(value, remove);
        return true;
    }
    
    void read(TValue& value, bool remove=true) {
        TLockGuard lock(_lock);
        while ( !_isDone && _container.empty() ) {
            wait(lock);
        }
        
        if ( _container.empty() )
            return;
        
        getValue(value, remove);
    }
    
    void stop() {
        {
            TLockGuard lock(_lock);
            _isDone = true;
        }
        signal();
    }

private:
    void getValue(TValue& value, bool remove=true) {
         value = _container.front();
         if ( remove )
             _container.pop_front();
    } 
    
    void signal() {
        _event.notify_one();
    }
    
    void wait(TLockGuard& lock) {
        _event.wait(lock);
    }

private:        
    bool _isDone;
    mutable TLock _lock;
    TEvent _event;
    TContainer _container;
};
    
} // common_thread
} // tw

