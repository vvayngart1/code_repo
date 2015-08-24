#include <tw/channel_or/uuid_factory.h>
#include <tw/common_thread/utils.h>
#include <tw/log/defs.h>

namespace tw {
namespace channel_or {
    
UuidFactory::UuidFactory() : _done(false),
                             _size(0),
                             _queue(),
                             _thread() {
    
}

UuidFactory::~UuidFactory() {
    stop();
}

bool UuidFactory::start(uint32_t size) {
    bool status = true;
    try {
        if ( _thread != NULL ) {
            LOGGER_ERRO << "Factory already started" << "\n";
        }
        
        for ( uint32_t i = _queue.size(); i < size; ++ i ) {
            _queue.push(_uuidGenenerator.generateUuid());
        }
        
        _size = size;
        _thread = tw::common_thread::ThreadPtr(new tw::common_thread::Thread(boost::bind(&UuidFactory::ThreadMain, this)));
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

void UuidFactory::stop() {
    try {
        _size = 0;
        _done = true;
        if ( _thread != NULL ) {
            _thread->join();
            _thread = tw::common_thread::ThreadPtr();
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

TUuidBuffer UuidFactory::get() {
    TUuidBuffer next;
    
    try {
        if ( !_queue.pop(next) ) {
            // Should not be here - flood of requests?
            //
            next = _uuidGenenerator.generateUuid();
            
            LOGGER_ERRO << "Queue is empty - generated uuid in get()"  << "\n";
        }        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    return next;
}

void UuidFactory::ThreadMain() {
    LOGGER_INFO << "Started" << "\n";
    
    try {
        while ( !_done ) {
            while ( !_done && (_queue.size() < _size) ) {
                _queue.push(_uuidGenenerator.generateUuid());
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
    
} // namespace channel_or
} // namespace tw
