#pragma once

#include <tw/common/settings.h>
#include <tw/common/singleton.h>
#include <tw/common_thread/locks.h>
#include <tw/common_thread/thread.h>
#include <tw/common/high_res_time.h>
#include <tw/log/defs.h>

#include <boost/bind.hpp>

#include <string>
#include <map>

namespace tw {
namespace common {    

typedef uint32_t TTimerId;

// TimerClient class
//
class TimerClient {
public:
    TimerClient() {
    }
    
    virtual ~TimerClient() {
    }
    
public:
    // Return 'true' to continue firing timeout events,
    // return 'false' to cancel firing timeout events
    //
    virtual bool onTimeout(const TTimerId& id) = 0;
};

// TimerClientHandler struct
//
struct TimerClientHandler {
    TimerClientHandler() : _client(NULL),
                           _cancelled(false),
                           _once(false),
                           _usecs(0),
                           _id(),
                           _timestamp() {
    }
    
    void onTimeout(const tw::common::THighResTime& now);

    TimerClient* _client;
    bool _cancelled;
    bool _once;    
    uint32_t _usecs;
    TTimerId _id;
    tw::common::THighResTime _timestamp;
};

typedef boost::shared_ptr<TimerClientHandler> TTimerClientHandlerPtr;

// TimerServer class
//
class TimerServer : public tw::common::Singleton<TimerServer> {
    typedef std::map<TTimerId, TTimerClientHandlerPtr> TClients;
    
public:
    static bool isValidTimerId(const TTimerId& id);
    static void clearTimerId(TTimerId& id);
    
public:
    TimerServer();
    ~TimerServer();
    
    void clear();
    
public:
    bool init(const tw::common::Settings& settings);    
    bool start();    
    bool stop();
    
public:
    // NOTE: minimum timeout resolution is 100 ms, if becomes
    // too restrictive, change it to lower limit
    //
    bool registerClient(TimerClient* client, const uint32_t msecs, const bool once, TTimerId& id);
    
    unsigned numberOfClients();

private:
    void ThreadMain();
    
private:
    typedef tw::common_thread::Lock TLock;
    
    TLock _lock;    
    bool _done;
    uint32_t _resolution;
    TTimerId _lastId;
    tw::common_thread::ThreadPtr _thread;
    TClients _clients;
};

} // namespace common
} // namespace tw
