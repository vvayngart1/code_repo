#pragma once

#include <tw/common_thread/locks.h>

#include <deque>

namespace tw {
namespace common_thread {

	/*
	The thread that intends to modify the variable has to
	acquire a std::mutex (typically via std::lock_guard)
	perform the modification while the lock is held
	execute notify_one or notify_all on the std::condition_variable (the lock does not need to be held for notification)
	Even if the shared variable is atomic, it must be modified under the mutex in order to correctly publish the modification to the waiting thread.
	Any thread that intends to wait on std::condition_variable has to
	acquire a std::unique_lock<std::mutex>, on the same mutex as used to protect the shared variable
	execute wait, wait_for, or wait_until. The wait operations atomically release the mutex and suspend the execution of the thread.
	When the condition variable is notified, a timeout expires, or a spurious wakeup occurs, the thread is awakened, and the mutex is atomically reacquired.
	The thread should then check the condition and resume waiting if the wake up was spurious.
	*/

	/*
	WOULD USE LMAX disruptor now instead of arrays based queues (ABD): http://lmax-exchange.github.io/disruptor/files/Disruptor-1.0.pdf
	Concurrent execution of code is about two things, mutual exclusion and visibility of change
	Mutual exclusion is about managing contended updates to some resource. 
	Visibility of change is about controlling when such changes are made visible to other threads

	Memory barriers are used by processors to indicate sections of code where the ordering of memory updates is important.
	They are the means by which hardware ordering and visibility of change is achieved between threads.

	At the heart of the disruptor mechanism sits a pre-allocated bounded data structure in the form of a ring-buffer. Data is
	added to the ring buffer through one or more producers and processed by one or more consumers

	All memory for the ring buffer is pre-allocated on start up. A ring-buffer can store either an array of pointers to entries or
	an array of structures representing the entries

	When designing the LMAX financial
	exchange our profiling showed that taking a queue based approach resulted in queuing costs dominating the total
	execution costs for processing a transaction.
	*/

	/*
	https://en.wikipedia.org/wiki/Transmission_Control_Protocol
	https://en.wikipedia.org/wiki/TCP_tuning
	*/
    
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

