#pragma once

#include <tw/common/defs.h>
#include <tw/price/quote.h>
#include <tw/log/defs.h>

namespace tw {
namespace common_trade {
    
struct StopLossTriggerParams {
    StopLossTriggerParams(StopLossTriggerParamsWire& p) : _p(p) {        
    }
    
    StopLossTriggerParamsWire& _p;
    
    tw::price::Size _stopTriggerQty;
    tw::price::Ticks _stopPrice;
};

class stopTriggerProcessor {
public:
    stopTriggerProcessor(StopLossTriggerParamsWire& p) : _p(p) {
        _p._stopTriggerQty.set(0);
    }
    
public:
    StopLossTriggerParams& getParams() {
        return _p;
    }
    
    const StopLossTriggerParams& getParams() const {
        return _p;
    }
    
public:
    bool isEnabled() const {
        if ( 1 > _p._p._minStopTriggerQty.get() )
            return false;
        
        return true;
    }
    
    bool isStopTriggered(const FillInfo& info,
                         const tw::price::Quote& quote,
                         std::string& reason) {
        if ( !isEnabled() || !quote.isNormalTrade() ) {
            // reason = "!isEnabled() || !quote.isNormalTrade()";
            return false;
        }
        
        if ( info._stop != _p._stopPrice ) {
            _p._stopTriggerQty.set(0);
            _p._stopPrice = info._stop;
        }            
        
        tw::common_str_util::TFastStream s;
        if ( quote._trade._price == info._stop ) {
            _p._stopTriggerQty += quote._trade._size;
            if ( _p._stopTriggerQty < _p._p._minStopTriggerQty ) {
                reason = "_p._stopTriggerQty < _p._p._minStopTriggerQty";
                return false;
            }
            
            s << "isStopTriggered(quote._trade._price == info._stop && _p._stopTriggerQty >= _p._minStopTriggerQty): "
              << "_p._stopTriggerQty=" <<  _p._stopTriggerQty.get()
              << ",_p._minStopTriggerQty=" <<  _p._p._minStopTriggerQty.get();
        }
        
        _p._stopTriggerQty.set(0);
        
        if ( s.empty() ) {
            switch ( info._fill._side ) {
                case tw::channel_or::eOrderSide::kBuy:
                    if ( quote._trade._price > info._stop ) {
                        reason = "quote._trade._price=" + quote._trade._price.toString() + " > info._stop=" + info._stop.toString();
                        return false;
                    }

                    s << "isStopTriggered(quote._trade._price < info._stop)";
                    break;
                case tw::channel_or::eOrderSide::kSell:
                     if ( quote._trade._price < info._stop ) {
                        reason = "quote._trade._price=" + quote._trade._price.toString() + " < info._stop=" + info._stop.toString();
                        return false;
                     }
                     
                     s << "isStopTriggered(quote._trade._price > info._stop)";
                     break;
                default:
                    return false;
            }
        }
        
        reason = s.str();
        return true;
    }
    
private:
    StopLossTriggerParams _p;
};

} // common_trade
} // tw
