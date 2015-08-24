#include <tw/common/timer_server.h>
#include <tw/common_strat/consumer_proxy.h>

#include "../common_thread/utils.h"

namespace tw {
namespace common {
    
// TimerClientHandler class
//
void TimerClientHandler::onTimeout(const tw::common::THighResTime& now) {
    try {
        uint64_t delta = now - _timestamp;
        if ( delta < _usecs )
            return;

        if ( _cancelled )
            return;

        if ( !_client )
            return;
        
        if ( !tw::common_strat::ConsumerProxy::instance().onTimeout(_client, _id) || _once )
            _cancelled = true; 
        else
            _timestamp = now;
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}
 
// TimerServer class
//
bool TimerServer::isValidTimerId(const TTimerId& id) {
    return (id != TTimerId());
}

void TimerServer::clearTimerId(TTimerId& id) {
    id = TTimerId();
}

TimerServer::TimerServer() {
    clear();
}

TimerServer::~TimerServer() {
    stop();
}
    
void TimerServer::clear() {
    // Lock for thread synchronization
    //
    tw::common_thread::LockGuard<TLock> lock(_lock);
    
    _done = false;
    _resolution = 1;
    _lastId = 0;
    _clients.clear();
}
    
bool TimerServer::init(const tw::common::Settings& settings) {
    // TODO: need to provide resolution from config file
    //    
    return true;
}

bool TimerServer::start() {
    bool status = true;
    try {
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<TLock> lock(_lock);
        
        if ( _thread != NULL ) {
            LOGGER_ERRO << "TimerServer already started" << "\n";
        }                        
        
        _thread = tw::common_thread::ThreadPtr(new tw::common_thread::Thread(boost::bind(&TimerServer::ThreadMain, this)));
        
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

bool TimerServer::stop() {
    try {
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<TLock> lock(_lock);
        
        _done = true;
        if ( _thread != NULL ) {
            _thread->join();
            _thread = tw::common_thread::ThreadPtr();
        }
        
        clear();
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    
    return true;
}
    
bool TimerServer::registerClient(TimerClient* client, const uint32_t msecs, const bool once, TTimerId& id) {
    try {
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<TLock> lock(_lock);
        
        if ( !client ) {
            LOGGER_ERRO << "Client is NULL" << "\n";
            return false;
        }
        
        if ( msecs < _resolution ) {
            LOGGER_WARN << "invalid timeout value (we will notify as soon as we can): " << msecs << " < " << _resolution << "\n";
        }        
        
        TTimerClientHandlerPtr clientHandler(new TimerClientHandler());
        
        clientHandler->_client = client;
        clientHandler->_once = once;
        clientHandler->_usecs = msecs*1000;
        clientHandler->_id = id = ++_lastId;
        clientHandler->_timestamp.setToNow();
        
        _clients[clientHandler->_id] = clientHandler;
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    
    return true;
}

unsigned TimerServer::numberOfClients() {
    tw::common_thread::LockGuard<TLock> lock(_lock);
    return _clients.size(); 
}

void TimerServer::ThreadMain() {
    LOGGER_INFO << "Started" << "\n";
    
    try {
        TClients clients;
        bool anyCancelled = false;
        tw::common::THighResTime timestamp;
        
        while ( !_done ) {
            timestamp.setToNow();
            
            {
                // Lock for thread synchronization
                //
                tw::common_thread::LockGuard<TLock> lock(_lock);
                clients = _clients;
            }
            
            TClients::iterator iter = clients.begin();
            TClients::iterator end = clients.end();
            
            for ( ; iter != end; ++iter ) {
                iter->second->onTimeout(timestamp);
                anyCancelled |= iter->second->_cancelled;
            }
            
            if ( anyCancelled ) {
                anyCancelled = false;
                // Lock for thread synchronization
                //
                tw::common_thread::LockGuard<TLock> lock(_lock);
                
                for ( iter = clients.begin(); iter != end; ++iter ) {
                    if ( iter->second->_cancelled )
                        _clients.erase(iter->second->_id);
                }
            }
            
            // Naive attempt to compensate for processing
            // time taken by the timer server clients
            //
            int32_t resolution = std::max(1, static_cast<int32_t>(_resolution)-static_cast<int32_t>(tw::common::THighResTime::now()-timestamp)/1000);
            tw::common_thread::sleep(resolution);
        }        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    LOGGER_INFO << "Finished" << "\n";
}

} // namespace common
} // namespace tw
