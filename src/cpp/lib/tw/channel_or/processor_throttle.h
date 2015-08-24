#pragma once

#include <tw/common/timer_server.h>
#include <tw/common/command.h>
#include <tw/common/singleton.h>
#include <tw/common/settings.h>

#include <tw/generated/channel_or_defs.h>
#include <tw/generated/risk_defs.h>

#include <map>

namespace tw {
namespace channel_or {

// NOTE: ProcessorThrottle has the following algorithm
//      - Once new orders/modifies exceed their respective max MPS (messages per
//        second), ProcessorThrottle will reject all subsequent new orders/modifies
//        until such time as ProcessorThrottle is reset through a command
//      - Once cancels exceed their respective max MPS, ProcessorThrottle will reject
//        all subsequent cancels during the second in which violation occurred
//
class ProcessorThrottle : public tw::common::Singleton<ProcessorThrottle>,
                          public tw::common::TimerClient {
public:
    ProcessorThrottle();    
    ~ProcessorThrottle();
    
    void clear();
    
    bool isEnabled() const {
        return _enabled;
    }
    
    bool init(const tw::risk::Account& account);
    
public:
    // tw::common::TimerClient interface
    //
    virtual bool onTimeout(const tw::common::TTimerId& id);
    
public:
    // ProcessorOut interface
    //
    bool init(const tw::common::Settings& settings);
    bool start();
    void stop();    
    
    bool sendNew(const TOrderPtr& order, Reject& rej);    
    bool sendMod(const TOrderPtr& order, Reject& rej);    
    bool sendCxl(const TOrderPtr& order, Reject& rej);    
    void onCommand(const tw::common::Command& command);
    
    bool rebuildOrder(const TOrderPtr& order, Reject& rej) { return true; }
    void recordFill(const Fill& fill) {}
    void rebuildPos(const PosUpdate& update) {}
    
    void onAlert(const Alert& alert) {}
    void onRebuildOrderRej(const TOrderPtr& order, const Reject& rej) {}
    
    void onNewRej(const TOrderPtr& order, const Reject& rej) {}    
    void onModRej(const TOrderPtr& order, const Reject& rej) {}    
    void onCxlRej(const TOrderPtr& order, const Reject& rej) {}
    
private:
    Reject getRej(eRejectReason reason) {
        tw::channel_or::Reject rej;
        
        rej._rejType = eRejectType::kInternal;
        rej._rejSubType = eRejectSubType::kProcessorThrottle;
        rej._rejReason = reason;
        
        return rej;
    }
    
    void setCounters(const TOrderPtr& order);
    
private:
    tw::common::TTimerId _timerId;
    bool _enabled;
    tw::risk::ThrottleCounters _counters;
    tw::risk::Account _account;
};

    
} // namespace channel_or
} // namespace tw
