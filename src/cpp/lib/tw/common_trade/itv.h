#pragma once

#include <tw/common/defs.h>
#include <tw/generated/instrument.h>

namespace tw {
namespace common_trade {
    
class ITv {
public:
    ITv() {
        clear();
    }
    
    virtual ~ITv() {
        clear();
    }
    
    ITv(const ITv& rhs) {
        *this = rhs;
    }
    
    ITv& operator=(const ITv& rhs) {
        if ( this != &rhs ) {
            _instrument = rhs._instrument;
            _isTvSet = rhs._isTvSet;
            _tv = rhs._tv;
        }
        
        return *this;
    }
    
    void clear() {
        _instrument.reset();
        _isTvSet = false;
        _tv = 0.0;
    }
    
    // Done for unit testing
    //
    void setTv(double tv) {
        _tv = tv;
        _isTvSet = true;
    }
    
    virtual bool setInstrument(tw::instr::InstrumentConstPtr instrument) {
        if ( !instrument || !instrument->isValid() )            
            return false;
        
        _instrument = instrument;
        return true;        
    }
    
public:
    bool isValid() const {
        if ( !_instrument || !_instrument->isValid() )
            return false;
        
        return _isTvSet;
    }
    
    tw::instr::InstrumentConstPtr getInstrument() const {
        return _instrument;
    }
    
    tw::instr::Instrument::TKeyId getInstrumentId() const {
        if ( isValid() )
            return _instrument->_keyId;
        
        return tw::instr::Instrument::TKeyId();
    }

    double getTv() const { 
        return _tv;
    }
    
    double getTvInTicks() const {
        if ( !isValid() )
            return 0.0;
        
        return _instrument->_tc->fractionalTicks(_tv);;
    }
   
protected:
    tw::instr::InstrumentConstPtr _instrument;
    bool _isTvSet;
    double _tv;
};

} // common_trade
} // tw
