#pragma once

#include <tw/common/defs.h>
#include <tw/common/timer_server.h>
#include <tw/price/quote.h>
#include <tw/log/defs.h>

namespace tw {
namespace common_trade {

class BarsTrailingStopProcessor {
public:
    BarsTrailingStopProcessor(BarsTrailingStopLossParamsWire& p) : _p(p) {
    }
    
public:
    BarsTrailingStopLossParamsWire& getParams() {
        return _p;
    }
    
    const BarsTrailingStopLossParamsWire& getParams() const {
        return _p;
    }
    
public:
    bool isEnabled() const {
        if ( 0 > _p._barsCountToStart )
            return false;
        
        return true;
    }
    
    bool isNumOfTicksInFavor(const FillInfo& info,
                             const tw::price::Quote& quote) {
        if ( !isEnabled() )
            return false;
        
        if ( !_p._barTrailingCntToStartTicks.isValid() || 0 > _p._barTrailingCntToStartTicks.get() )
            return true;
        
        if ( !quote._book[0].isValid() )
            return false;
        
        switch ( info._fill._side ) {
            case tw::channel_or::eOrderSide::kBuy:
                if ( quote._book[0]._bid._price < (info._fill._price+_p._barTrailingCntToStartTicks) )
                    return false;
                break;
            case tw::channel_or::eOrderSide::kSell:
                if ( quote._book[0]._ask._price > (info._fill._price-_p._barTrailingCntToStartTicks) )
                    return false;
                break;
            default:
                return false;
        }
        
        return true;
    }
    
    
    template <typename TBars>
    bool isStopSlideRecalculated(FillInfo& info,
                                 const TBars& bars,
                                 std::string& reason) {
        if ( !isEnabled() )
            return false;
        
        if ( (info._barIndex2+_p._barsCountToStart) > (bars.size()-1) )
            return false;
        
        const typename TBars::value_type& currBar = bars[bars.size()-2];
        if ( !currBar._formed || 0 == currBar._numOfTrades )
            return false;
        
        tw::price::Ticks stop;
        tw::common_str_util::TFastStream s;
        bool isTransitionBar = false;
        if ( currBar._open == currBar._close && bars.size()-1 != info._barIndex2 ) {
            isTransitionBar = true;
            s << "isStopSlideRecalculated(TransitionBar: currBar._open==currBar._close -- "
              << "currBar=" << currBar.toString()
              << ")" ;
        }
        
        switch ( info._fill._side ) {
            case tw::channel_or::eOrderSide::kBuy:
            {
                if ( isTransitionBar ) {
                    switch ( _p._transitionBarSlideMode ) {
                        case eTransitionBarSlideMode::kBar:
                            stop = currBar._low - _p._barTrailingStopOffsetTicks;
                            break;
                        case eTransitionBarSlideMode::kOff:
                            return false;
                        case eTransitionBarSlideMode::kClose:
                        default:
                            stop = currBar._close - _p._barTrailingStopOffsetTicks;
                            break;
                    }
                } else {
                    if ( !_p._slideBehindBetterBar ) {
                        if ( s.empty() ) {
                            if ( bars.size()-1 == info._barIndex2 ) {
                                if ( !isStopSlideRecalculatedBehindFirstBar(currBar, info, s) )
                                    return false;
                            } else {
                                const typename TBars::value_type& prevBar = bars[bars.size()-3];
                                if ( 0 == prevBar._numOfTrades || (currBar._close <= prevBar._close) )
                                    return false;

                                s << "isStopSlideRecalculated(currBar._close > prevBar._close -- "
                                  << "currBar=" << currBar.toString()
                                  << "prevBar=" << prevBar.toString()
                                  << ")" ;
                            }
                        }
                    } else {
                        if ( bars.size()-1 == info._barIndex2 ) {
                            if ( !isStopSlideRecalculatedBehindFirstBar(currBar, info, s) )
                                return false;
                        } else {
                            for ( uint32_t i = info._barIndex2; i < bars.size()-1; ++i ) {
                                const typename TBars::value_type& prevBar = bars[i-1];
                                if ( 0 < prevBar._numOfTrades && (currBar._close <= prevBar._close) )
                                    return false;
                            }

                            s << "isStopSlideRecalculated(currBar._close > any prevBar._close -- "
                              << "currBar=" << currBar.toString()
                              << ")" ;
                        }
                    }

                    stop = currBar._low - _p._barTrailingStopOffsetTicks;
                }
                
                if ( info._stop.isValid() && stop <= info._stop )
                    return false;
            }
                break;
            case tw::channel_or::eOrderSide::kSell:
            {
                if ( isTransitionBar ) {
                    switch ( _p._transitionBarSlideMode ) {
                        case eTransitionBarSlideMode::kBar:
                            stop = currBar._high + _p._barTrailingStopOffsetTicks;
                            break;
                        case eTransitionBarSlideMode::kOff:
                            return false;
                        case eTransitionBarSlideMode::kClose:
                        default:
                            stop = currBar._close + _p._barTrailingStopOffsetTicks;
                            break;
                    }
                } else {
                    if ( !_p._slideBehindBetterBar ) {
                        if ( s.empty() ) {
                            if ( bars.size()-1 == info._barIndex2 ) {
                                if ( !isStopSlideRecalculatedBehindFirstBar(currBar, info, s) )
                                    return false;
                            } else {
                                const typename TBars::value_type& prevBar = bars[bars.size()-3];
                                if ( 0 == prevBar._numOfTrades || (currBar._close >= prevBar._close) )
                                    return false;

                                s << "isStopSlideRecalculated(currBar._close < prevBar._close -- "
                                  << "currBar=" << currBar.toString()
                                  << "prevBar=" << prevBar.toString()
                                  << ")" ;
                            }
                        }
                    } else {
                        if ( bars.size()-1 == info._barIndex2 ) {
                            if ( !isStopSlideRecalculatedBehindFirstBar(currBar, info, s) )
                                return false;
                        } else {
                            for ( uint32_t i = info._barIndex2; i < bars.size()-1; ++i ) {
                                const typename TBars::value_type& prevBar = bars[i-1];
                                if ( 0 < prevBar._numOfTrades && (currBar._close >= prevBar._close) )
                                    return false;
                            }

                            s << "isStopSlideRecalculated(currBar._close < any prevBar._close -- "
                              << "currBar=" << currBar.toString()
                              << ")" ;
                        }
                    }

                    stop = currBar._high + _p._barTrailingStopOffsetTicks;
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
    template <typename TBar>
    bool isStopSlideRecalculatedBehindFirstBar(const TBar& currBar,
                                               const FillInfo& info,
                                               tw::common_str_util::TFastStream& s) {
        switch ( info._fill._side ) {
            case tw::channel_or::eOrderSide::kBuy:
            {
                if ( ePatternDir::kDown == currBar._dir )
                    return false;
                
                if ( currBar._close < info._fill._price )
                    return false;
            }
                break;
            case tw::channel_or::eOrderSide::kSell:
            {
                if ( ePatternDir::kUp == currBar._dir )
                    return false;
                
                if ( currBar._close > info._fill._price )
                    return false;
            }
                break;
            default:
                return false;
        }
        
        s << "isStopSlideRecalculated(sliding behind first bar -- "
          << "currBar=" << currBar.toString()
          << ")" ;
        return true;
    }
    
private:
    BarsTrailingStopLossParamsWire& _p;
};

} // common_trade
} // tw
