#pragma once

#include <tw/common/defs.h>
#include <tw/common_trade/itv.h>
#include <tw/generated/instrument.h>
#include <tw/generated/channel_or_defs.h>
#include <tw/generated/risk_defs.h>

namespace tw {
namespace common_trade {
    
class IPnL {
public:
    IPnL() {
        clear();
    }
    
    virtual ~IPnL() {
        clear();
    }
    
    IPnL(const IPnL& rhs) {
        *this = rhs;
    }
    
    IPnL& operator=(const IPnL& rhs) {
        if ( this != &rhs ) {
            _pnl = rhs._pnl;
            _fees = rhs._fees;
            _observer = rhs._observer;
        }
        
        return *this;
    }
    
    void clear() {
        _pnl.clear();
        _fees.clear();
        _observer = NULL;
    }
    
    std::string getHeader() const {
        return std::string(_pnl.getHeader() + " -- " + _fees.getHeader());
    }
    
    std::string toString() const {
        return std::string(_pnl.toString() + " -- " + _fees.toString());
    }
    
public:
    tw::price::Size getPosition() const { 
        return _pnl._position;
    }
    
    double getPnL() const { 
        return getUnrealizedPnL() + getRealizedPnL();
    }
    
    const tw::risk::PnL& getPnLInfo() const {
        return _pnl;
    }
    
    double getUnrealizedPnL() const {
        return _pnl._unrealizedPnL;
    }
    
    double getRealizedPnL() const {
        return _pnl._realizedPnL;
    }
    
    double getMaxUnrealizedPnL() const {
        return _pnl._maxUnrealizedPnL;
    }
    
    double getMaxRealizedPnL() const {
        return _pnl._maxRealizedPnL;
    }
    
    double getUnrealizedDrawdown() const {        
        return _pnl._maxUnrealizedDrawdown;
    }
    
    double getRealizedDrawdown() const {
        return _pnl._maxRealizedDrawdown;
    }
    
    const tw::instr::Fees& getFeesPaid() const {
        return _fees;
    }
    
    void setObserver(IPnL* o) {
        _observer = o;
    }
    
public:
    virtual bool isValid() const = 0;
    virtual bool isForMe(const tw::instr::Instrument::TKeyId& instrumentId) const = 0;
    virtual void onTv(const ITv& tv) = 0;    
    virtual void onFill(const tw::channel_or::Fill& fill) = 0;
    
public:
    virtual void preProcess(const IPnL& c) {
        _pnl -= c.getPnLInfo();
        _fees -= c.getFeesPaid();
                
        if ( _observer )            
            _observer->preProcess(c);
    }
    
    virtual void postProcess(const IPnL& c) {
        _pnl += c.getPnLInfo();
        _fees += c.getFeesPaid();
                
        if ( _observer )            
            _observer->postProcess(c);
    }
    
    virtual bool isFlat() const {
        return ( 0 == _pnl._position.get() );
    }
    
protected:
    void updateRealizedPnL() {
        if ( _pnl._realizedPnL > _pnl._maxRealizedPnL ) {
            _pnl._maxRealizedPnL = _pnl._realizedPnL;
            _pnl._maxRealizedDrawdown = 0.0;
        } else {
            _pnl._maxRealizedDrawdown = _pnl._maxRealizedPnL - _pnl._realizedPnL;
        }
    }
    
    void updateUnrealizedPnL(bool ignorePos = false) {
        if ( _pnl._position == 0 && !ignorePos ) {
            _pnl._unrealizedPnL = _pnl._maxUnrealizedPnL = _pnl._maxUnrealizedDrawdown = 0.0;
            return;
        }
        
        if ( _pnl._unrealizedPnL > _pnl._maxUnrealizedPnL ) {
            _pnl._maxUnrealizedPnL = _pnl._unrealizedPnL;
            _pnl._maxUnrealizedDrawdown = 0.0;
        } else {
            _pnl._maxUnrealizedDrawdown = _pnl._maxUnrealizedPnL - _pnl._unrealizedPnL;
        }
    }
    
protected:    
    tw::risk::PnL _pnl;
    tw::instr::Fees _fees;
    IPnL* _observer;
};

} // common_trade
} // tw
