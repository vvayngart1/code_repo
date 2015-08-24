#pragma once

#include <tw/common/defs.h>
#include <tw/common/timer_server.h>
#include <tw/price/quote.h>
#include <tw/log/defs.h>

namespace tw {
namespace common_trade {

class DefenseBarsTrailingStopProcessor {
public:
    DefenseBarsTrailingStopProcessor(BarsTrailingStopLossParamsWire& p1,
                                     DefenseBarsTrailingStopLossParamsWire& p2) : _p1(p1),
                                                                                  _p2(p2){
    }
    
public:
    DefenseBarsTrailingStopLossParamsWire& getParams() {
        return _p2;
    }
    
    const DefenseBarsTrailingStopLossParamsWire& getParams() const {
        return _p2;
    }
    
public:
    bool isEnabled() const {
        if ( 0 > _p2._dbBarsCountToStart )
            return false;
        
        return true;
    }
    
    template <typename TBars>
    bool isStopSlideRecalculated(FillInfo& info,
                                 const TBars& bars,
                                 std::string& reason) {
        if ( !isEnabled() )
            return false;
        
        if ( (info._barIndex2+_p2._dbBarsCountToStart) != (bars.size()-1) )
            return false;
        
        const typename TBars::value_type& currBar = bars[bars.size()-2];
        if ( !currBar._formed || 0 == currBar._numOfTrades )
            return false;
        
        tw::price::Ticks stop;
        tw::common_str_util::TFastStream s;
        
        switch ( info._fill._side ) {
            case tw::channel_or::eOrderSide::kBuy:
            {
                if ( ePatternDir::kDown != currBar._dir )
                    return false;
                
                if ( eDefenseBarsTrailingStopMode::kCurrentBar == _p2._dbMode ) {
                    stop = currBar._close - _p1._barTrailingStopOffsetTicks;
                    
                    s << "isStopSlideRecalculated()==true -- "
                      << "fillInfo=" << info.toString()
                      << "_p2._dbMode=" << _p2._dbMode.toString()
                      << "currBar=" << currBar.toString();
                } else {
                    tw::price::Ticks worstPrice;
                    for ( uint32_t i = info._barIndex2; i < bars.size()-1; ++i ) {
                        const typename TBars::value_type& prevBar = bars[i-1];
                        if ( !worstPrice.isValid() || worstPrice > prevBar._low )
                            worstPrice = prevBar._low;
                    }
                    
                    if ( !worstPrice.isValid() )
                        return false;
                    
                    stop = worstPrice - _p1._barTrailingStopOffsetTicks;
                    
                    s << "isStopSlideRecalculated()==true -- "
                      << "fillInfo=" << info.toString()
                      << "_p2._dbMode=" << _p2._dbMode.toString()
                      << "worstPrice=" << worstPrice;
                }
                
                if ( info._stop.isValid() && stop <= info._stop )
                    return false;
            }
                break;
            case tw::channel_or::eOrderSide::kSell:
            {
                if ( ePatternDir::kUp != currBar._dir )
                    return false;
                
                if ( eDefenseBarsTrailingStopMode::kCurrentBar == _p2._dbMode ) {
                    stop = currBar._close + _p1._barTrailingStopOffsetTicks;
                    
                    s << "isStopSlideRecalculated()==true -- "
                      << "fillInfo=" << info.toString()
                      << "_p2._dbMode=" << _p2._dbMode.toString()
                      << "currBar=" << currBar.toString();
                } else {
                    tw::price::Ticks worstPrice;
                    for ( uint32_t i = info._barIndex2; i < bars.size()-1; ++i ) {
                        const typename TBars::value_type& prevBar = bars[i-1];
                        if ( !worstPrice.isValid() || worstPrice < prevBar._high )
                            worstPrice = prevBar._high;
                    }
                    
                    if ( !worstPrice.isValid() )
                        return false;
                    
                    stop = worstPrice + _p1._barTrailingStopOffsetTicks;
                    
                    s << "isStopSlideRecalculated()==true -- "
                      << "fillInfo=" << info.toString()
                      << "_p2._dbMode=" << _p2._dbMode.toString()
                      << "worstPrice=" << worstPrice;
                }
                
                if ( info._stop.isValid() && stop >= info._stop )
                    return false;
            }

                 break;
            default:
                return false;
        }
        
        info._stop = stop;
        reason = s.str();
        return true;
    }
    
private:
    BarsTrailingStopLossParamsWire& _p1;
    DefenseBarsTrailingStopLossParamsWire& _p2;
};

} // common_trade
} // tw
