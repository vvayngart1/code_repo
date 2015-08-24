#pragma once

#include <tw/common/defs.h>
#include <tw/common_thread/locks.h>

#include <tw/log/defs.h>

namespace tw {
namespace common {
    
template<typename TItem, typename TLock = tw::common_thread::LockNull>
class Pool : public noncopyable {
    static const uint32_t DEFAULT_SIZE = 128;
    
    typedef std::tr1::unordered_set<TItem*, stdext::hash<TItem> > TItems;
    typedef std::vector<TItem*> TItemsArray;
    
public:
    class Deleter {
        friend class Pool;
        
    public:        
        void operator()(TItem* item) {
            const_cast<Pool&>(_parent).release(item);
        }
        
    private:
        Deleter(const Pool& parent) : _parent(parent) {            
        }
        
    private:
        const Pool& _parent;
    };
    
    Deleter getDeleter() const {
        return Deleter(*this);
    }
    
public:
    Pool(uint32_t size = DEFAULT_SIZE) {
        clear();
        
        TItemsArray items;
        items.resize(size);
        std::for_each(items.begin(), items.end(), std::alloc());
        
        _items.insert(items.begin(), items.end());
        _availableItems.insert(_items.begin(), _items.end());
    }
    
    ~Pool() {
        clear();
    }    
    
    void clear() {
        tw::common_thread::LockGuard<TLock> lock(_lock);
        
        TItemsArray items;
        std::copy(_items.begin(), _items.end(), std::back_inserter(items));
        
        _availableItems.clear();
        _items.clear();        
        
        std::for_each(items.begin(), items.end(), std::dealloc());
    }
    
public:
    TItem* obtain() {
        tw::common_thread::LockGuard<TLock> lock(_lock);
        TItem* item = NULL;
        
        if ( _availableItems.empty() ) {
            item = new TItem();
            _items.insert(item);
        } else {
            typename TItems::iterator iter = _availableItems.begin();
            item = *iter;
            _availableItems.erase(item);
        }
        
        return item;
    }
    
    void release(TItem* item) {
        tw::common_thread::LockGuard<TLock> lock(_lock);
        
        if ( NULL == item ) {
            LOGGER_ERRO << "attempting to return NULL to the pool" << "\n";
            return;
        }
        
        if ( !exists(item) ) {
            LOGGER_ERRO << "attempting to return non-managed object to the pool" << "\n";
            return;
        }
        
        if ( available(item) ) {
            LOGGER_ERRO << "attempting to return already returned object to the pool" << "\n";
            return;
        }
        
        item->clear();
        _availableItems.insert(item);
    }
    
    bool exists(TItem* item) const {
        tw::common_thread::LockGuard<TLock> lock(_lock);
        return (_items.find(item) != _items.end());
    }
    
    bool available(TItem* item) const {
        tw::common_thread::LockGuard<TLock> lock(_lock);
        return (_availableItems.find(item) != _availableItems.end());
    }
    
    size_t capacity() const {
        tw::common_thread::LockGuard<TLock> lock(_lock);
        return _items.size();
    }
    
    size_t size() const {
        tw::common_thread::LockGuard<TLock> lock(_lock);
        return _availableItems.size();
    }
    
private:
    mutable TLock _lock;
    TItems _availableItems;
    TItems _items;        
};

} // namespace common
} // namespace tw




