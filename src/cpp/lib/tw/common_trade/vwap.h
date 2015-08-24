#pragma once

#include <tw/common/defs.h>
#include <tw/price/quote.h>
#include <tw/instr/instrument_manager.h>
#include <tw/log/defs.h>
#include <tw/common_trade/itv.h>

namespace tw {
namespace common_trade {
    
class Vwap : public ITv {
    typedef ITv Parent;
    
public:
    Vwap() {
        clear();
    }
    
    ~Vwap() {
        clear();
    }
    
    Vwap(const Vwap& rhs) {
        *this = rhs;
    }
    
    Vwap& operator=(const Vwap& rhs) {
        if ( this != &rhs ) {
            static_cast<Parent&>(*this) = static_cast<const Parent&>(rhs);
            _volume = rhs._volume;
        }
        
        return *this;
    }
    
    void clear() {
        Parent::clear();
        _volume = 0.0;
    }
    
public:
    void onQuote(const tw::price::Quote& quote) {
        if ( !_instrument ) {
            if ( !setInstrument(tw::instr::InstrumentManager::instance().getByKeyId(quote._instrumentId)) ) {            
                LOGGER_ERRO << "Severe error - unknown or invalid instrument for id: " << quote._instrumentId << "\n";
                return;
            }
        }

        if ( tw::price::Quote::kSuccess != quote._status || !quote.isTrade() || !quote._trade.isValid() )
            return;
     
        _tv *= _volume;
        _tv += _instrument->_tc->toExchangePrice(quote._trade._price) * quote._trade._size.get();
        _volume += quote._trade._size.get();
        _tv /= _volume;

        _isTvSet = true;
    }
    
private:
    double _volume;
};

} // common_trade
} // tw
