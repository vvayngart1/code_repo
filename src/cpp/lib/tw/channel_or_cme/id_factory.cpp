#include <tw/channel_or_cme/id_factory.h>
#include <tw/log/defs.h>
#include <tw/common_thread/utils.h>

#include "tw/common/high_res_time.h"

namespace tw {
namespace channel_or_cme {
    
static const char ALPHABET[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

// class Id
//

const Id::Type Id::MAX_VALUE = std::numeric_limits<Type>::max();

Id::Id() {
    clear();
}

Id::Id(const Id& rhs) {
    *this = rhs;
}

Id& Id::operator=(const Id& rhs) {
    if ( this != &rhs ) {
        std::copy(rhs._buffer, rhs._buffer+sizeof(_buffer), _buffer);
        _ptrToData = (&_buffer[sizeof(_buffer)])-(&rhs._buffer[sizeof(_buffer)]-rhs._ptrToData);
    }
    
    return *this;
}

void Id::clear() {
    std::fill(_buffer, _buffer+sizeof(_buffer), 0);    
    _buffer[sizeof(_buffer)-2] = '0';
    _ptrToData = &_buffer[sizeof(_buffer)-2];
}

const char* Id::c_str() const {
    return _ptrToData;
}

void Id::generate(Id::Type lastId) {
    clear();
    int32_t max_index = sizeof(_buffer)-2;
    int32_t index = 0;
    for ( Id::Type i = lastId; i != (uint64_t)0u && index < max_index ; ++index ) {
        char ch = ALPHABET[i % 36];
        _buffer[max_index - index] = ch;        
        i /= 36;
    }
    
   _ptrToData = _buffer + (max_index - index) + 1;
}
    
// class IdFactory
//
IdFactory::IdFactory() : _done(false),
                         _size(0),
                         _lastId(Id::Type()),
                         _queue(),
                         _thread() {
    
}

IdFactory::~IdFactory() {
    stop();
}

bool IdFactory::start(const Id::Type lastId, const uint32_t size) {
    bool status = true;
    try {
        if ( _thread != NULL ) {
            LOGGER_ERRO << "Factory already started" << "\n";
        }
        
        // TODO: this is implemented not to have persistence of orders
        // works only for one executable with CME connection!!!
        //
        // Gets number of microseconds since the start of the day and uses that
        // as a seed
        //
        if ( Id::MAX_VALUE == lastId ) {
            _lastId = tw::common::THighResTime::now().getUsecsFromMidnight();
            LOGGER_WARN << "Set _lastId to UsecsFromMidnight: "  << _lastId << "\n" << "\n";
        } else {        
            _lastId = lastId;
        }
        
        for ( uint32_t i = _queue.size(); i < size; ++ i ) {
            Id next;
            next.generate(++(*this));
            _queue.push(next);
        }
        
        _size = size;
        _thread = tw::common_thread::ThreadPtr(new tw::common_thread::Thread(boost::bind(&IdFactory::ThreadMain, this)));
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    if ( !status )
        stop();
    
    return status;
}

// TODO: need to implement atomic counter
//
Id::Type IdFactory::operator++() {
    Id::Type value;
    {
        tw::common_thread::LockGuard<TLock> guard(_lock);
        value = ++_lastId;
    }
    
    return value;
    
}

void IdFactory::stop() {
    try {
        _size = 0;
        _done = true;
        if ( _thread != NULL ) {
            _thread->join();
            _thread = tw::common_thread::ThreadPtr();
        }
        _queue.clear();
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

Id IdFactory::get() {
    Id next;
    
    try {
        if ( !_queue.pop(next) ) {
            // Should not be here - flood of requests?
            //
            next.generate(++(*this));
        }        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    return next;
}

void IdFactory::ThreadMain() {
    LOGGER_INFO << "Started" << "\n";
    
    try {
        while ( !_done ) {
            while ( !_done && (_queue.size() < _size) ) {
                Id next;
                next.generate(++(*this));
                _queue.push(next);
                _thread->yield();
            }
            
            // Sleep for 1 ms
            //
            tw::common_thread::sleep(1);
        }        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    LOGGER_INFO << "Finished" << "\n";
}
    
} // namespace channel_or_cme
} // namespace tw
