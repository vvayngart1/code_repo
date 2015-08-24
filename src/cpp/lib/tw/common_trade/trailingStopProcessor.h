#pragma once

#include <tw/common/defs.h>
#include <tw/price/quote.h>
#include <tw/log/defs.h>

namespace tw {
namespace common_trade {

class TrailingStopProcessor {
public:
    TrailingStopProcessor(TrailingStopLossParamsWire& p) : _p(p) {
    }
    
public:
    TrailingStopLossParamsWire& getParams() {
        return _p;
    }
    
    const TrailingStopLossParamsWire& getParams() const {
        return _p;
    }
    
public:
    bool isEnabled() const {
        if ( !_p._beginTrailingStopTicks.isValid() || 1 > _p._beginTrailingStopTicks.get() || !_p._trailingStopOffsetTicks.isValid() )
            return false;
        
        return true;
    }
    
    bool isStopSlideTriggered(FillInfo& info,
                              const tw::price::Quote& quote,
                              std::string& reason) {
        if ( !isEnabled() )
            return false;

        tw::common_str_util::TFastStream s;
        switch ( info._fill._side ) {
            case tw::channel_or::eOrderSide::kBuy:
            {
                if ( (info._fill._price + _p._beginTrailingStopTicks) > quote._book[0]._bid._price )
                    return false;
                
                tw::price::Ticks stop = quote._book[0]._bid._price - _p._trailingStopOffsetTicks;
                if ( info._stop >= stop )
                    return false;
                
                s << "isStopSlideTriggered(info._stop < stop(quote._book[0]._bid._price - _p._stopOffsetTicks)) for: "                                                 
                  << "stop=" << stop.get() << ",fill=" << info.toString() 
                  << " -- quote=" << quote._book[0].toShortString();

                info._stop = stop;
            }   
                break;
            case tw::channel_or::eOrderSide::kSell:
            {
                if ( (info._fill._price - _p._beginTrailingStopTicks) < quote._book[0]._ask._price )
                    return false;
                
                tw::price::Ticks stop = quote._book[0]._ask._price + _p._trailingStopOffsetTicks;
                if ( info._stop <= stop )
                    return false;
                
                s << "isStopSlideTriggered(info._stop > stop(quote._book[0]._ask._price + _p._stopOffsetTicks)) for: "                                                 
                  << "stop=" << stop.get() << ",fill=" << info.toString() 
                  << " -- quote=" << quote._book[0].toShortString();

                info._stop = stop;
            }
                break;
            default:
                return false;
        }

        reason = s.str();
        return true;
    }
    
private:
    TrailingStopLossParamsWire& _p;
};

} // common_trade
} // tw
