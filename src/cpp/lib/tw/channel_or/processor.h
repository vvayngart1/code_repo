#pragma once

#include <tw/common/uuid.h>
#include <tw/common/command.h>
#include <tw/generated/channel_or_defs.h>
#include <tw/common/settings.h>

namespace tw {
namespace channel_or {
    
class ProcessorOutNull {
public:
    // Administrative methods
    //
    bool init(const tw::config::Settings& settings) { return true; }
    bool start() { return true; }
    void stop() {}
    
public:
    bool sendNew(const TOrderPtr& order, Reject& rej) { return true; }
    bool sendMod(const TOrderPtr& order, Reject& rej) { return true; }
    bool sendCxl(const TOrderPtr& order, Reject& rej) { return true; }
    
    void onCommand(const tw::common::Command& command) {}
    
    bool rebuildOrder(const TOrderPtr& order, Reject& rej) { return true; }
    void recordFill(const Fill& fill)         {}    
    void rebuildPos(const PosUpdate& update)  {}    
};

class ProcessorInNull {
public:
    void onNewAck(const TOrderPtr& order)                       {}
    void onNewRej(const TOrderPtr& order, const Reject& rej)	{}
    void onModAck(const TOrderPtr& order)			{}
    void onModRej(const TOrderPtr& order, const Reject& rej)	{}
    void onCxlAck(const TOrderPtr& order)                       {}
    void onCxlRej(const TOrderPtr& order, const Reject& rej)	{}
    void onFill(const Fill& fill)                               {}
    
    void onAlert(const Alert& alert) {}
    void onRebuildOrderRej(const TOrderPtr& order, const Reject& rej) {}
};

static ProcessorOutNull nextOutNull;
static ProcessorInNull nextInNull;

template <typename TImpl, typename TProcessorNext = ProcessorOutNull>
class ProcessorOut {
public:
    ProcessorOut(TImpl& impl) : _impl(impl),
                                _next(nextOutNull) {        
    }
    
    ProcessorOut(TImpl& impl,
                   TProcessorNext& next) : _impl(impl),
                                           _next(next) {        
    }
    
public:
    // Administrative methods
    //
    bool init(const tw::common::Settings& settings) {
        if ( !_impl.init(settings) )
            return false;
        
        if ( !_next.init(settings) ) {
            _impl.stop();
            return false;
        }
        
        return true;
    }
    
    bool start() {
        if ( !_impl.start() )
            return false;
        
        if ( !_next.start() ) {
            _impl.stop();
            return false;
        }
        
        return true;
    }
    
    void stop() {
        _impl.stop();
        _next.stop();
    }
    
public:
    bool sendNew(const TOrderPtr& order, Reject& rej) {
        if ( !_impl.sendNew(order, rej) )
            return false;
        
        if ( !_next.sendNew(order,rej) ) {
            _impl.onNewRej(order, rej);
            return false;
        }
        
        return true;
    }
    
    bool sendMod(const TOrderPtr& order, Reject& rej) {
        if ( !_impl.sendMod(order, rej) )
            return false;
        
        if ( !_next.sendMod(order,rej) ) {
            _impl.onModRej(order, rej);
            return false;
        }
        
        return true;
    }
    
    bool sendCxl(const TOrderPtr& order, Reject& rej) {
        if ( !_impl.sendCxl(order, rej) )
            return false;
        
        if ( !_next.sendCxl(order,rej) ) {
            _impl.onCxlRej(order, rej);
            return false;
        }
        
        return true;
    }
    
    void onCommand(const tw::common::Command& command) {
        _impl.onCommand(command);
        _next.onCommand(command);
    }
    
    bool rebuildOrder(const TOrderPtr& order, Reject& rej) {
        if ( !_impl.rebuildOrder(order, rej) )
            return false;
        
        if ( !_next.rebuildOrder(order, rej) ) {
            _impl.onRebuildOrderRej(order, rej);
            return false;
        }
        
        return true;
    }
    
    void recordFill(const Fill& fill) {
        _impl.recordFill(fill);
        _next.recordFill(fill);
    }
    
    void rebuildPos(const PosUpdate& update) {
        _impl.rebuildPos(update);
        _next.rebuildPos(update);
    }
    
private:
    TImpl& _impl;
    TProcessorNext _next;
};

template <typename TImpl, typename TProcessorNext = ProcessorInNull>
class ProcessorIn {
public:
    ProcessorIn(TImpl& impl) : _impl(impl),
                               _next(nextInNull) {        
    }
    
    ProcessorIn(TImpl& impl,
                TProcessorNext& next) : _impl(impl),
                                        _next(next) {        
    }
    
public:
    void onNewAck(const TOrderPtr& order) {
        _impl.onNewAck(order);
        _next.onNewAck(order);
    }
    
    void onNewRej(const TOrderPtr& order, const Reject& rej) {
        _impl.onNewRej(order, rej);
        _next.onNewRej(order, rej);
    }
    
    void onModAck(const TOrderPtr& order) {
        _impl.onModAck(order);
        _next.onModAck(order);
    }
    
    void onModRej(const TOrderPtr& order, const Reject& rej) {
        _impl.onModRej(order, rej);
        _next.onModRej(order, rej);
    }
    
    void onCxlAck(const TOrderPtr& order) {
        _impl.onCxlAck(order);
        _next.onCxlAck(order);
    }
    
    void onCxlRej(const TOrderPtr& order, const Reject& rej) {
        _impl.onCxlRej(order, rej);
        _next.onCxlRej(order, rej);
    }
    
    void onFill(const Fill& fill) {
        _impl.onFill(fill);
        _next.onFill(fill);
    }
    
    void onAlert(const Alert& alert) {
        _impl.onAlert(alert);
        _next.onAlert(alert);
    }
    
    void onRebuildOrderRej(const TOrderPtr& order, const Reject& rej) {
        _impl.onRebuildOrderRej(order, rej);
        _next.onRebuildOrderRej(order, rej);
    }
    
private:
    TImpl& _impl;
    TProcessorNext _next;
};

    
} // namespace channel_or
} // namespace tw
