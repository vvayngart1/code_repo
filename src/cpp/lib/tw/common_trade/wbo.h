#pragma once

#include <tw/common/defs.h>
#include <tw/price/quote.h>
#include <tw/instr/instrument_manager.h>
#include <tw/log/defs.h>
#include <tw/common_trade/itv.h>

namespace tw {
namespace common_trade {
    
class Wbo_calc1 {
public:
    double operator()(const tw::price::Quote& quote, const tw::instr::InstrumentConstPtr& instrument, double points) {
        double b1 = instrument->_tc->toExchangePrice(quote._book[0]._bid._price);
        double o1 = instrument->_tc->toExchangePrice(quote._book[0]._ask._price);
        
        double wbo_initial = 0.5*b1 + 0.5*o1 - 0.5*points;
        double wbo_multiplier = quote._book[0]._bid._size.toDouble()/(quote._book[0]._bid._size.toDouble()+quote._book[0]._ask._size.toDouble());
        
        return (wbo_initial + wbo_multiplier*points);
    }
};

class Wbo_calc2 {
public:
    double operator()(const tw::price::Quote& quote, const tw::instr::InstrumentConstPtr& instrument, double points) {
        double b1 = instrument->_tc->toExchangePrice(quote._book[0]._bid._price);
        double o1 = instrument->_tc->toExchangePrice(quote._book[0]._ask._price);
        
        double wbo_initial = b1;
        double wbo_multiplier = quote._book[0]._bid._size.toDouble()/(quote._book[0]._bid._size.toDouble()+quote._book[0]._ask._size.toDouble());
        double wbo_width = o1 - b1;
        
        return (wbo_initial + wbo_multiplier*wbo_width);
    }
};

template <typename TCalc>
class Wbo : public ITv {
    typedef ITv Parent;
    
public:
    Wbo() {
        clear();
    }
    
    ~Wbo() {
        clear();
    }
    
    Wbo(const Wbo& rhs) {
        *this = rhs;
    }
    
    Wbo& operator=(const Wbo& rhs) {
        if ( this != &rhs ) {
            static_cast<Parent&>(*this) = static_cast<const Parent&>(rhs);
            _points = rhs._points;
        }
        
        return *this;
    }
    
    void clear() {
        Parent::clear();
        _points = 1;
    }
    
    virtual bool setInstrument(tw::instr::InstrumentConstPtr instrument) {
        if ( ! Parent::setInstrument(instrument) ) 
            return false;
        
        if ( 0 == _instrument->_tickDenominator )
            return false;
        
        _points = static_cast<double>(_instrument->_tickNumerator)/static_cast<double>(_instrument->_tickDenominator);
        return true;
    }
    
public:
    void onQuote(const tw::price::Quote& quote) {
        if ( !_instrument ) {
            if ( !setInstrument(tw::instr::InstrumentManager::instance().getByKeyId(quote._instrumentId)) ) {            
                LOGGER_ERRO << "Severe error - unknown or invalid instrument for id: " << quote._instrumentId << "\n";
                return;
            }
            _points = static_cast<double>(_instrument->_tickNumerator)/static_cast<double>(_instrument->_tickDenominator);
        }

        if ( !quote._book[0].isValid() )
            return;
        
       _tv = _calc(quote, _instrument, _points);
       _isTvSet = true;
    }
    
private:
    TCalc _calc;
    double _points;
};

} // common_trade
} // tw
