#pragma once

#include <tw/common_trade/ipnl.h>

#include <vector>

namespace tw {
namespace common_trade {
    
class PnLComposite : public IPnL {
    typedef IPnL Parent;
    typedef std::vector<Parent*> TComponents;    
    
public:
    PnLComposite() {
        clear();
    }
    
    ~PnLComposite() {
        clear();
    }
    
    PnLComposite(const PnLComposite& rhs) {
        *this = rhs;
    }
    
    PnLComposite& operator=(const PnLComposite& rhs) {
        if ( this != &rhs ) {
             static_cast<Parent&>(*this) = static_cast<const Parent&>(rhs);
             _components = rhs._components;
             _observer = rhs._observer;
        }
        
        return *this;
    }
    
    void clear() {
        Parent::clear();
        _components.clear();
        _observer = NULL;
    }
    
public:
    void addComponent(IPnL* c) {
        if ( !c )
            return;
        
        _components.push_back(c);
    }
    
public:
    // IPnL interface
    //
    
    // If at least one component is valid, return true, otherwise, false
    //
    virtual bool isValid() const {
        if ( _components.empty() )
            return false;
        
        TComponents::const_iterator iter = _components.begin();
        TComponents::const_iterator end = _components.end();
        for ( ; iter != end; ++iter ) {
            if ( (*iter)->isValid() ) 
                return true;
        }
        
        return false;
    }
    
    virtual bool isForMe(const tw::instr::Instrument::TKeyId& instrumentId) const {
        TComponents::const_iterator iter = _components.begin();
        TComponents::const_iterator end = _components.end();
        for ( ; iter != end; ++iter ) {
            if ( (*iter)->isForMe(instrumentId) )
                return true;
        }
        
        return false;
    }
    
    virtual void onTv(const ITv& tv) {
        TComponents::iterator iter = _components.begin();
        TComponents::iterator end = _components.end();
        for ( ; iter != end; ++iter ) {
            IPnL& c = *(*iter);
            if ( c.isForMe(tv.getInstrumentId()) ) {
                preProcess(c);                
                c.onTv(tv);
                postProcess(c);
            }
        }
    }
    
    void onFill(const tw::channel_or::Fill& fill) {
        TComponents::iterator iter = _components.begin();
        TComponents::iterator end = _components.end();
        for ( ; iter != end; ++iter ) {
            IPnL& c = *(*iter);
            if ( c.isForMe(fill._instrumentId) ) {
                preProcess(c);                
                c.onFill(fill);
                postProcess(c);
            }
        }
    }
    
public:
    virtual void preProcess(const IPnL& c) {
        _pnl._unrealizedPnL -= c.getPnLInfo()._unrealizedPnL;
        _pnl._realizedPnL -= c.getPnLInfo()._realizedPnL;
        _pnl._position -= c.getPnLInfo()._position;
        
        _fees -= c.getFeesPaid();
                
        if ( _observer )            
            _observer->preProcess(c);
    }
    
    virtual void postProcess(const IPnL& c) {
        _pnl._unrealizedPnL += c.getPnLInfo()._unrealizedPnL;
        _pnl._realizedPnL += c.getPnLInfo()._realizedPnL;
        _pnl._position += c.getPnLInfo()._position;
        
        Parent::updateRealizedPnL();
        Parent::updateUnrealizedPnL(!isFlat());
        
        _fees += c.getFeesPaid();
                
        if ( _observer )            
            _observer->postProcess(c);
    }
    
    virtual bool isFlat() const {
        if ( _pnl._position != 0)
            return false;
        
        TComponents::const_iterator iter = _components.begin();
        TComponents::const_iterator end = _components.end();
        for ( ; iter != end; ++iter ) {
            if ( !(*iter)->isFlat() )
                return false;
        }
        
        return true;
    }
    
private:
    TComponents _components;
};

} // common_trade
} // tw
