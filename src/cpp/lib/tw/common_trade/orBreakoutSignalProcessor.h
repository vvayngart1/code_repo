#pragma once

#include <tw/common/defs.h>
#include <tw/price/quote.h>
#include <tw/log/defs.h>

#include <tw/common_trade/bars_manager.h>

namespace tw {
namespace common_trade {
    
typedef tw::common_trade::TBar TBar;
typedef tw::common_trade::TBars TBars;
  
class ORBreakoutSignalProcessor {
public:
    ORBreakoutSignalProcessor(ORBreakoutSignalParamsWire& p) : _p(p) {
        clear();
        
        std::string t = tw::common::THighResTime::sqlString(0, true).substr(0,10) + " " + _p._orOpenTime;
        tw::common::THighResTime openTime = tw::common::THighResTime::parseSqlTime(t);
        _p_runtime._orOpenTimeMinutesFromMidnight = openTime.getUnitsFromMidnight(tw::common::eHighResTimeUnits::kMins);
        
        t = tw::common::THighResTime::sqlString(0, true).substr(0,10) + " " + _p._orCloseTime;
        tw::common::THighResTime closeTime = tw::common::THighResTime::parseSqlTime(t);
        _p_runtime._orCloseTimeMinutesFromMidnight = closeTime.getUnitsFromMidnight(tw::common::eHighResTimeUnits::kMins);
        
        if ( closeTime < openTime )
            _p_runtime._orCloseTimeMinutesFromMidnight += tw::common::THighResTime::daysToMinutes(1);
    }
    
public:
    ORBreakoutSignalParamsWire& getParams() {
        return _p;
    }
    
    const ORBreakoutSignalParamsWire& getParams() const {
        return _p;
    }
    
    ORBreakoutSignalParamsRuntime& getParamsRuntime() {
        return _p_runtime;
    }
    
    const ORBreakoutSignalParamsRuntime& getParamsRuntime() const {
        return _p_runtime;
    }
    
    void clear() {
        _p_runtime.clear();
        clearRuntime();
    }
    
    void clearRuntime(uint32_t lastSignalExitBarIndex = 0) {
        _p_runtime._orCountFromBarIndex = 0;
        _p_runtime._orCountBarsClosingInside = 0;
        _p_runtime._orIsSignalOn = false;
        _p_runtime._orOrderSide = tw::channel_or::eOrderSide::kUnknown;
        _p_runtime._orBreakoutPrice.clear();
        if ( 0 < lastSignalExitBarIndex )
            _p_runtime._orLastSignalExitBarIndex = lastSignalExitBarIndex;
        
        if ( 0 < _p._orMinBarsClosingInside ) {
            _p_runtime._orBreakoutTries = 0;
            _p_runtime._orOpenBarIndex = 0;
            _p_runtime._orOpenBarHigh.clear();
            _p_runtime._orOpenBarLow.clear();
            _p_runtime._orTotalVolInsideRange.clear();
            clearVolume();
        }
        
        clearBuffers();
    }
    
    void clearBuffers() {
        _p_runtime._orReason.clear();
        
        _s.clear();
        _s.setPrecision(2);
    }
    
    void clearVolume() {
        _p_runtime._orTotalVolFor.clear();
        _p_runtime._orTotalVolAgainst.clear();
    }
    
    bool isSignalOn() const {
        return _p_runtime._orIsSignalOn;
    }
    
    bool isBarsToExitExceeded(const TBars& bars) {
        if ( 0 < _p._orBarsToExit && ((bars.size()-_p_runtime._orLastSignalBarIndex) > (_p_runtime._orCountBarsClosingInside+1)) ) {
            _s.clear();
            _s << "isBarsToExitExceeded()==true -- ((bars.size()-_p_runtime._orLastSignalBarIndex) > (_p_runtime._orCountBarsClosingInside+1)): bars.size()=" << bars.size()
               << ",_p_runtime._orLastSignalBarIndex" << _p_runtime._orLastSignalBarIndex
               << ",_p_runtime._orCountBarsClosingInside" << _p_runtime._orCountBarsClosingInside;
            _p_runtime._orReason = _s.str();
            return true;
        }
            
        return false;
    }
    
public:
    bool isSignalTriggered(const tw::price::Quote& quote,
                           const TBars& bars,
                           const TBars& orCloseOutsideRangeBars) {
        bool status = doIsSignalTriggered(quote, bars, orCloseOutsideRangeBars);
        if ( quote.isNormalTrade() )
            _p_runtime._orLastProcessedBarIndex = bars.size();
        return status;
    }
    
    void monitorSignal(const tw::price::Quote& quote,
                       const TBars& bars,
                       const tw::price::Ticks& activePrice) {
        clearBuffers();
        
        if ( !isSignalOn() )
            return;
        
        if ( bars.empty() )
            return;
        
        const TBar& currBar = bars.back();
        if ( 0 == _p._orMinBarsClosingInside ) {        
            if ( currBar._index - _p_runtime._orOpenBarIndex > _p._orMaxBarsToBreakout ) {
                clearRuntime();
                _s << "currBar._index - _p_runtime._orOpenBarIndex > _p._orMaxBarsToBreakout: " << currBar._index << " - " << _p_runtime._orOpenBarIndex << " > " << _p._orMaxBarsToBreakout;            
                _p_runtime._orReason = _s.str();

                return;
            }
        } else {
            if ( currBar._index - (_p_runtime._orOpenBarIndex+_p._orMinBarsClosingInside) > _p._orMaxBarsToBreakout ) {
                clearRuntime();
                _s << "currBar._index - (_p_runtime._orOpenBarIndex+_p._orMinBarsClosingInside) > _p._orMaxBarsToBreakout: " << currBar._index << " - " << (_p_runtime._orOpenBarIndex+_p._orMinBarsClosingInside) << " > " << _p._orMaxBarsToBreakout;            
                _p_runtime._orReason = _s.str();

                return;
            }
        }
        
        if ( !activePrice.isValid() ) {
            _p_runtime._orReason = "!activePrice.isValid()";
            return;
        }
        
        switch ( _p_runtime._orOrderSide ) {
            case tw::channel_or::eOrderSide::kBuy:
                if ( activePrice > (_p_runtime._orBreakoutPrice + _p._orInitiateRangeBreakoutTicksLimit) ) {
                    _s << " ==> price > limit: " << activePrice << " > " << (_p_runtime._orBreakoutPrice + _p._orInitiateRangeBreakoutTicksLimit)
                       << ",side="  << _p_runtime._orOrderSide.toString()
                       << ",quote=" << quote.toShortString();
                    
                    _p_runtime._orReason = _s.str();
                    clearRuntime();
                     
                    return;
                }
                break;
            case tw::channel_or::eOrderSide::kSell:
                if ( activePrice < (_p_runtime._orBreakoutPrice - _p._orInitiateRangeBreakoutTicksLimit) ) {
                    _s << " ==> price < limit: " << activePrice << " < " << (_p_runtime._orBreakoutPrice - _p._orInitiateRangeBreakoutTicksLimit)
                       << ",side="  << _p_runtime._orOrderSide.toString()
                       << ",quote=" << quote.toShortString();
                     
                    _p_runtime._orReason = _s.str();
                    clearRuntime(); 
                               
                    return;
                }
                break;
            default:
                break;
        }
    }
    
public:
    void calcInitialStopPrice(FillInfo& info) {
        if ( !isSignalOn() )
            return;
        
        if ( _p._orInitialStopBehindRangeTicks.get() < 0 )
            return;
        
        tw::price::Ticks stop;
        switch ( info._fill._side ) {
            case tw::channel_or::eOrderSide::kBuy:
                stop = _p_runtime._orOpenBarLow - _p._orInitialStopBehindRangeTicks;
                if ( info._stop.isValid() && stop <= info._stop )
                    return;
                
                break;
            case tw::channel_or::eOrderSide::kSell:
                stop = _p_runtime._orOpenBarHigh + _p._orInitialStopBehindRangeTicks;
                if ( info._stop.isValid() && stop >= info._stop )
                    return;
                
                break;
            default:
                return;
        }
        
        if ( !stop.isValid() )
            return;
        
        info._stop = stop;
    }
    
    bool isInitialStopSlideRecalculated(FillInfo& info,
                                        const TBars& bars) {
        if ( 0 < _p._orMinBarsClosingInside )
            return false;
        
        if ( !isSignalOn() )
            return false;
        
        if ( !checkRange(bars, _p_runtime._orLastSignalBarIndex) )  {            
            return false;
        }
        
        const TBar& signalBar = bars[_p_runtime._orLastSignalBarIndex-1];
        if ( !signalBar._formed )
            return false;
        
        tw::price::Ticks stop;
        switch ( info._fill._side ) {
            case tw::channel_or::eOrderSide::kBuy:
                stop = signalBar._low - _p._orFirstSlideTrailingStopTicks;
                if ( info._stop.isValid() && stop <= info._stop )
                    return false;
                
                break;
            case tw::channel_or::eOrderSide::kSell:
                stop = signalBar._high + _p._orFirstSlideTrailingStopTicks;
                if ( info._stop.isValid() && stop >= info._stop )
                    return false;
                
                break;
            default:
                return false;
        }
        
        if ( !stop.isValid() )
            return false;
        
        info._stop = stop;
        return true;
    }
    
private:
    bool doIsSignalTriggered(const tw::price::Quote& quote,
                             const TBars& bars,
                             const TBars& orCloseOutsideRangeBars) {
        clearBuffers();
        
        if ( isSignalOn() )
            return true;
        
        if ( !quote.isNormalTrade() )
            return false;
        
        const size_t currBarIndex = bars.size();
        uint32_t minNumberOfBars = (0 == _p._orMinBarsClosingInside) ? 2 : 3;
        if ( currBarIndex < minNumberOfBars ) {
            if ( currBarIndex != _p_runtime._orLastProcessedBarIndex )
                _p_runtime._orReason = "currBarIndex < " + boost::lexical_cast<std::string>(minNumberOfBars);
            
            return false;
        }
        
        if ( orCloseOutsideRangeBars.size() < minNumberOfBars ) {
            if ( currBarIndex != _p_runtime._orLastProcessedBarIndex )
                _p_runtime._orReason = "currBarIndex < " + boost::lexical_cast<std::string>(minNumberOfBars);
            
            return false;
        }
        
        if ( 0 == _p._orMinBarsClosingInside ) {
            if ( !doIsSignalTriggeredOnOpenRangeBreakout(quote, bars, currBarIndex) )
                return false;
        } else {
            if ( !doIsSignalTriggeredOnGenericRangeBreakout(quote, bars, orCloseOutsideRangeBars, currBarIndex) )
                return false;
        }
        
        _p_runtime._orReason = _s.str();
        _p_runtime._orLastSignalBarIndex = currBarIndex;
        return true;
    }
    
    bool doIsSignalTriggeredOnOpenRangeBreakout(const tw::price::Quote& quote,
                                                const TBars& bars,
                                                const size_t currBarIndex) {
        // Check to see that if openBar is determined
        //
        const TBar& prevBar = bars[bars.size()-2];
        if ( 0 == _p_runtime._orOpenBarIndex ) {
            uint32_t minFromMidnight = prevBar._open_timestamp.getUnitsFromMidnight(tw::common::eHighResTimeUnits::kMins);
            if ( minFromMidnight != _p_runtime._orOpenTimeMinutesFromMidnight ) {
                 if ( currBarIndex != _p_runtime._orLastProcessedBarIndex ) {
                     _s << "NOT an opening bar: " << prevBar._open_timestamp.toString();
                     _p_runtime._orReason = _s.str();
                 }
                 
                 return false;
            }
            
            _p_runtime._orOpenBarIndex = prevBar._index;
            _p_runtime._orCountFromBarIndex = prevBar._index;
            _p_runtime._orOpenBarHigh = prevBar._high;
            _p_runtime._orOpenBarLow = prevBar._low;
            
            _s << "Opening bar: high=" << _p_runtime._orOpenBarHigh << ",low=" << _p_runtime._orOpenBarLow << ",bar=" << prevBar.toString();
            _p_runtime._orReason = _s.str();
        }
        
        // Check to see if max time for breakout elapsed
        //
        const TBar& currBar = bars[currBarIndex-1];
        if ( currBar._index - _p_runtime._orOpenBarIndex > _p._orMaxBarsToBreakout ) {
            if ( currBarIndex != _p_runtime._orLastProcessedBarIndex ) {
                _s << "currBar._index - _p_runtime._orOpenBarIndex > _p._orMaxBarsToBreakout: " << currBar._index << " - " << _p_runtime._orOpenBarIndex << " > " << _p._orMaxBarsToBreakout;            
                _p_runtime._orReason = _s.str();
            }
            
            return false;
        }
        
        // Check to see if max breakout tries are exceeded
        //
        if ( _p_runtime._orBreakoutTries >= _p._orMaxBreakoutTries ) {
            if ( currBarIndex != _p_runtime._orLastProcessedBarIndex ) {
                _s << "_p_runtime._orBreakoutTries >= _p._orMaxBreakoutTries: " << _p_runtime._orBreakoutTries << " >= " << _p._orMaxBreakoutTries;            
                _p_runtime._orReason = _s.str();
            }
            
            return false;
        }
        
        // Check to see if need to determine next orCountFromBarIndex
        //
        if ( 0 == _p_runtime._orCountFromBarIndex ) {
            if (  prevBar._close >= _p_runtime._orOpenBarHigh || prevBar._close <= _p_runtime._orOpenBarLow ) {
                if ( currBarIndex != _p_runtime._orLastProcessedBarIndex ) {
                     _s << "NOT a next orCountFromBarIndex bar: " << prevBar.toString();
                     _p_runtime._orReason = _s.str();
                 }
                 
                 return false;
            }
            
            _p_runtime._orCountFromBarIndex = prevBar._index;
        }
        
        // Check to see if need to determine if previous bar close or not
        //
        tw::price::Ticks breakoutPrice;
        if ( (currBar._index - _p_runtime._orCountFromBarIndex) < _p._orBreakoutBarsInsideLimit ) {
            if ( (quote._trade._price - _p_runtime._orOpenBarHigh) >= _p._orInitiateRangeBreakoutTicks ) {
                _p_runtime._orOrderSide = tw::channel_or::eOrderSide::kBuy;
                breakoutPrice = _p_runtime._orOpenBarHigh;
                
                _s << "BUY: (quote._trade._price - _p_runtime._orOpenBarHigh) >= _p._orInitiateRangeBreakoutTicks: " << quote._trade._price << " - " << _p_runtime._orOpenBarHigh << " >= " << _p._orInitiateRangeBreakoutTicks;
            } else if ( (_p_runtime._orOpenBarLow - quote._trade._price) >= _p._orInitiateRangeBreakoutTicks ) {
                _p_runtime._orOrderSide = tw::channel_or::eOrderSide::kSell;
                breakoutPrice = _p_runtime._orOpenBarLow;
                
                _s << "SELL: (_p_runtime._orOpenBarLow - quote._trade._price) >= _p._orInitiateRangeBreakoutTicks: " << _p_runtime._orOpenBarLow << " - " << quote._trade._price << " >= " << _p._orInitiateRangeBreakoutTicks;
            } else {
                return false;
            }
        } else {
            if ( (prevBar._close - _p_runtime._orOpenBarHigh) >= _p._orInitiateRangeBreakoutTicks ) {
                _p_runtime._orOrderSide = tw::channel_or::eOrderSide::kBuy;
                
                _s << "BUY: (prevBar._close - _p_runtime._orOpenBarHigh) >= _p._orInitiateRangeBreakoutTicks: " << prevBar._close << " - " << _p_runtime._orOpenBarHigh << " >= " << _p._orInitiateRangeBreakoutTicks;
            } else if ( (_p_runtime._orOpenBarLow - prevBar._close) >= _p._orInitiateRangeBreakoutTicks ) {
                _p_runtime._orOrderSide = tw::channel_or::eOrderSide::kSell;                
                
                _s << "SELL: (_p_runtime._orOpenBarLow - prevBar._close) >= _p._orInitiateRangeBreakoutTicks: " << _p_runtime._orOpenBarLow << " - " << prevBar._close << " >= " << _p._orInitiateRangeBreakoutTicks;
            } else {
                return false;
            }
            
            breakoutPrice = prevBar._close;
        }
        
        // Check to see that don't trade more than once in the same bar in the same direction
        //
        if ( _p_runtime._orLastSignalExitBarIndex >= currBarIndex && _p_runtime._orLastSignalOrderSide == _p_runtime._orOrderSide ) {
            if ( currBarIndex != _p_runtime._orLastProcessedBarIndex ) {
                _s << " -- _p_runtime._orLastSignalExitBarIndex >= currBarIndex && _p_runtime._orLastSignalOrderSide == _p_runtime._orOrderSide: " << _p_runtime._orLastSignalExitBarIndex 
                   << " >= " << currBarIndex 
                   << " &&  " << _p_runtime._orLastSignalOrderSide.toString()
                   << " ==  " << _p_runtime._orOrderSide.toString()
                   << ",bar=" << currBar.toString();            
                _p_runtime._orReason = _s.str();
            }
            
            _p_runtime._orOrderSide = tw::channel_or::eOrderSide::kUnknown;
            return false;
        }
        
        _p_runtime._orIsSignalOn = true;
        _p_runtime._orBreakoutPrice = breakoutPrice;
        _p_runtime._orLastSignalOrderSide = _p_runtime._orOrderSide;
        
        _s << " -- breakout_try=" << (++_p_runtime._orBreakoutTries) << ",orCountFromBarIndex=" << _p_runtime._orCountFromBarIndex;        
        return true;
    }
    
    bool checkRange(const tw::price::Ticks& range,
                    const size_t currBarIndex) {
        if ( _p._orMaxRangeTicks.get() > 0 && range > _p._orMaxRangeTicks ) {
            clearRuntime();
            if ( currBarIndex != _p_runtime._orLastProcessedBarIndex ) {
                _s << "range > _p._orMaxRangeTicks: " << range << " > " << _p._orMaxRangeTicks;
                _p_runtime._orReason = _s.str();
            }
            return false;
        }
        
        return true;
    }
    
    bool checkAndAssignFirstBarOfRange(const TBar& prevBar,
                                       const size_t currBarIndex) {
        clearRuntime();
        if ( !checkRange(prevBar._range, currBarIndex) )
            return false;

        _p_runtime._orOpenBarIndex = prevBar._index;
        _p_runtime._orOpenBarHigh = prevBar._high;
        _p_runtime._orOpenBarLow = prevBar._low;
        _p_runtime._orTotalVolInsideRange = prevBar._volume;
        
        return true;
    }
    
    void clearVolOutsideRangeRuntime(const tw::price::Quote& quote) {
        _p_runtime._orOrderSide = tw::channel_or::eOrderSide::kUnknown;
        clearVolume();
        _s << "price inside range: price=" << quote._trade._price << ",high=" << _p_runtime._orOpenBarHigh << ",low=" << _p_runtime._orOpenBarLow;           
        _p_runtime._orReason = _s.str();
    }
    
    bool doIsSignalTriggeredOnGenericRangeBreakout(const tw::price::Quote& quote,
                                                   const TBars& bars,
                                                   const TBars& orCloseOutsideRangeBars,
                                                   const size_t currBarIndex) {
        const TBar& prevBar = bars[bars.size()-2];
        uint32_t minFromMidnight = prevBar._open_timestamp.getUnitsFromMidnight(tw::common::eHighResTimeUnits::kMins);
        if ( minFromMidnight < _p_runtime._orOpenTimeMinutesFromMidnight || minFromMidnight > _p_runtime._orCloseTimeMinutesFromMidnight  ) {
            if ( currBarIndex != _p_runtime._orLastProcessedBarIndex ) {
                if ( minFromMidnight < _p_runtime._orOpenTimeMinutesFromMidnight )
                    _s << "Before open time: " << _p._orOpenTime << " :: " << prevBar._open_timestamp.toString();
                else
                    _s << "After close time: " << _p._orCloseTime << " :: " << prevBar._open_timestamp.toString();
                    
                _p_runtime._orReason = _s.str();
            }
            
            return false;
        }
        
        if ( 0 == _p_runtime._orOpenBarIndex ) {
            if ( !checkAndAssignFirstBarOfRange(prevBar, currBarIndex) )
                return false;
        } else if ( currBarIndex != _p_runtime._orLastProcessedBarIndex ) {
            if ( prevBar._close > _p_runtime._orOpenBarHigh || prevBar._close < _p_runtime._orOpenBarLow ) {
                if ( !checkAndAssignFirstBarOfRange(prevBar, currBarIndex) )
                    return false;
            } else {
                if ( prevBar._high > _p_runtime._orOpenBarHigh )
                    _p_runtime._orOpenBarHigh = prevBar._high;
                
                if ( prevBar._low < _p_runtime._orOpenBarLow )
                    _p_runtime._orOpenBarLow = prevBar._low;
                
                if ( !checkRange(_p_runtime._orOpenBarHigh-_p_runtime._orOpenBarLow, currBarIndex) )
                    return false;
                
                 ++_p_runtime._orCountBarsClosingInside;
                 _p_runtime._orTotalVolInsideRange += prevBar._volume;
            }
        }
        
        if ( _p_runtime._orCountBarsClosingInside < _p._orMinBarsClosingInside ) {
            if ( currBarIndex != _p_runtime._orLastProcessedBarIndex ) {
                _s << "orCountBarsClosingInside < orMinBarsClosingInside: " << _p_runtime._orCountBarsClosingInside << " < " << _p._orMinBarsClosingInside << " -- high=" << _p_runtime._orOpenBarHigh << ",low=" << _p_runtime._orOpenBarLow << ",bar=" << prevBar.toString();
                _p_runtime._orReason = _s.str();
            }
            
            return false;
        }
        
        if ( _p_runtime._orCountBarsClosingInside == _p._orMinBarsClosingInside && currBarIndex != _p_runtime._orLastProcessedBarIndex ) {
            _s << "Range conditions completed: high=" << _p_runtime._orOpenBarHigh << ",low=" << _p_runtime._orOpenBarLow << ",lastBar=" << prevBar.toString();
            _p_runtime._orReason = _s.str();
        }
        
        if ( _p._orMinVolRequired.get() > 0 && _p_runtime._orTotalVolInsideRange < _p._orMinVolRequired ) {
            if ( currBarIndex != _p_runtime._orLastProcessedBarIndex ) {
                _s << "_p_runtime._orTotalVolInsideRange < _p._orMinVolRequired: " << _p_runtime._orTotalVolInsideRange << " < " << _p._orMinVolRequired << ",high=" << _p_runtime._orOpenBarHigh << ",low=" << _p_runtime._orOpenBarLow << ",lastBar=" << prevBar.toString();
                _p_runtime._orReason = _s.str();
            }
            
            return false;
        }
        
        if ( _p_runtime._orCountBarsClosingInside == _p._orMinBarsClosingInside && currBarIndex != _p_runtime._orLastProcessedBarIndex ) {
            _s << "Range and volume conditions completed: high=" << _p_runtime._orOpenBarHigh << ",low=" << _p_runtime._orOpenBarLow << ",_p_runtime._orTotalVolInsideRange=" << _p_runtime._orTotalVolInsideRange;
            _p_runtime._orReason = _s.str();
        }
        
        
        // Check to see if max time for breakout elapsed
        //
        const TBar& currBar = bars[currBarIndex-1];
        if ( currBar._index - (_p_runtime._orOpenBarIndex+_p._orMinBarsClosingInside) > _p._orMaxBarsToBreakout ) {
            if ( currBarIndex != _p_runtime._orLastProcessedBarIndex ) {
                clearRuntime();
                _s << "currBar._index - (_p_runtime._orOpenBarIndex+_p._orMinBarsClosingInside) > _p._orMaxBarsToBreakout: " << currBar._index << " - " << (_p_runtime._orOpenBarIndex+_p._orMinBarsClosingInside) << " > " << _p._orMaxBarsToBreakout;            
                _p_runtime._orReason = _s.str();
            }
            
            return false;
        }
        
        // Check to see if max breakout tries are exceeded
        //
        if ( _p_runtime._orBreakoutTries >= _p._orMaxBreakoutTries ) {
            if ( currBarIndex != _p_runtime._orLastProcessedBarIndex ) {
                clearRuntime();
                _s << "_p_runtime._orBreakoutTries >= _p._orMaxBreakoutTries: " << _p_runtime._orBreakoutTries << " >= " << _p._orMaxBreakoutTries;            
                _p_runtime._orReason = _s.str();
            }
            
            return false;
        }
        
        // Check breakout volume requirement
        //
        if ( _p._orMinVolOutsideRange.get() > 0 ) {
            if ( tw::channel_or::eOrderSide::kUnknown == _p_runtime._orOrderSide ) {
                if ( (quote._trade._price - _p_runtime._orOpenBarHigh) > tw::price::Ticks(0) ) {
                    _p_runtime._orOrderSide = tw::channel_or::eOrderSide::kBuy;
                } else if ( (_p_runtime._orOpenBarLow - quote._trade._price) > tw::price::Ticks(0) ) {
                    _p_runtime._orOrderSide = tw::channel_or::eOrderSide::kSell;
                } else {
                    if ( currBarIndex != _p_runtime._orLastProcessedBarIndex ) {
                        _s << "price inside range: price=" << quote._trade._price << ",high=" << _p_runtime._orOpenBarHigh << ",low=" << _p_runtime._orOpenBarLow;
                        _p_runtime._orReason = _s.str();
                    }
                    return false;
                }
                
                _s << "price outside range: price=" << quote._trade._price << ",high=" << _p_runtime._orOpenBarHigh << ",low=" << _p_runtime._orOpenBarLow << ",_p_runtime._orOrderSide=" << _p_runtime._orOrderSide.toString();
                _p_runtime._orReason = _s.str();
            }
            
            tw::price::Size volBid;
            tw::price::Size volAsk;
            if ( tw::channel_or::eOrderSide::kBuy == _p_runtime._orOrderSide ) {
                if ( quote._trade._price < _p_runtime._orOpenBarHigh ) {                        
                    clearVolOutsideRangeRuntime(quote);
                    return false;
                }
                
                tw::common_trade::updateTrade(volBid, volAsk, quote);
                _p_runtime._orTotalVolAgainst += volBid;
                if ( quote._trade._price > _p_runtime._orOpenBarHigh )
                    _p_runtime._orTotalVolFor += volAsk;
                    
            } else {
                if ( quote._trade._price > _p_runtime._orOpenBarLow ) {                        
                    clearVolOutsideRangeRuntime(quote);
                    return false;
                }
                
                tw::common_trade::updateTrade(volBid, volAsk, quote);
                _p_runtime._orTotalVolAgainst += volAsk;
                if ( quote._trade._price < _p_runtime._orOpenBarLow )
                    _p_runtime._orTotalVolFor += volBid;
            }
            
            double ratio = (0 == _p_runtime._orTotalVolAgainst.get()) ? _p_runtime._orTotalVolFor.toDouble() : (_p_runtime._orTotalVolFor.toDouble()/_p_runtime._orTotalVolAgainst.toDouble());
            if ( _p._orMinVolOutsideRange > (_p_runtime._orTotalVolFor+_p_runtime._orTotalVolAgainst) || ratio < _p._orMinVolRatioFor ) {
                _s << "_p._orMinVolOutsideRange > (_p_runtime._orTotalVolFor+_p_runtime._orTotalVolAgainst) || ratio < _p._orMinVolRatioFor: _p_runtime._orTotalVolFor=" << _p_runtime._orTotalVolFor
                   << ",p_runtime._orTotalVolAgainst=" << _p_runtime._orTotalVolAgainst
                   << ",ratio=" << ratio;
                _p_runtime._orReason = _s.str();
                return false;
            }
        
            _s << "_p._orMinVolOutsideRange <= (_p_runtime._orTotalVolFor+_p_runtime._orTotalVolAgainst) && ratio >= _p._orMinVolRatioFor: _p_runtime._orTotalVolFor=" << _p_runtime._orTotalVolFor
                   << ",p_runtime._orTotalVolAgainst=" << _p_runtime._orTotalVolAgainst
                   << ",ratio=" << ratio;
            _p_runtime._orReason = _s.str();
        } 
        
        // Check to see if need to determine of previous bar close or not
        //
        if ( !_p._orReqCloseOutsideRangeToInitiate ) {
            if ( (quote._trade._price - _p_runtime._orOpenBarHigh) >= _p._orInitiateRangeBreakoutTicks ) {
                _p_runtime._orOrderSide = tw::channel_or::eOrderSide::kBuy;
                _p_runtime._orBreakoutPrice = _p_runtime._orOpenBarHigh;
                _p_runtime._orIsSignalOn = true;
                
                _s << "BUY: (quote._trade._price - _p_runtime._orOpenBarHigh) >= _p._orInitiateRangeBreakoutTicks: " << quote._trade._price << " - " << _p_runtime._orOpenBarHigh << " >= " << _p._orInitiateRangeBreakoutTicks;
            } else if ( (_p_runtime._orOpenBarLow - quote._trade._price) >= _p._orInitiateRangeBreakoutTicks ) {
                _p_runtime._orOrderSide = tw::channel_or::eOrderSide::kSell;
                _p_runtime._orBreakoutPrice = _p_runtime._orOpenBarLow;
                _p_runtime._orIsSignalOn = true;
                
                _s << "SELL: (_p_runtime._orOpenBarLow - quote._trade._price) >= _p._orInitiateRangeBreakoutTicks: " << _p_runtime._orOpenBarLow << " - " << quote._trade._price << " >= " << _p._orInitiateRangeBreakoutTicks;
            } else {
                return false;
            }
        } else {
            if ( (prevBar._close - _p_runtime._orOpenBarHigh) >= _p._orInitiateRangeBreakoutTicks ) {
                _p_runtime._orOrderSide = tw::channel_or::eOrderSide::kBuy;
                
                _s << "BUY: (prevBar._close - _p_runtime._orOpenBarHigh) >= _p._orInitiateRangeBreakoutTicks: " << prevBar._close << " - " << _p_runtime._orOpenBarHigh << " >= " << _p._orInitiateRangeBreakoutTicks;
            } else if ( (_p_runtime._orOpenBarLow - prevBar._close) >= _p._orInitiateRangeBreakoutTicks ) {
                _p_runtime._orOrderSide = tw::channel_or::eOrderSide::kSell;                
                
                _s << "SELL: (_p_runtime._orOpenBarLow - prevBar._close) >= _p._orInitiateRangeBreakoutTicks: " << _p_runtime._orOpenBarLow << " - " << prevBar._close << " >= " << _p._orInitiateRangeBreakoutTicks;
            } else {
                return false;
            }
            
            _p_runtime._orBreakoutPrice = prevBar._close;
            _p_runtime._orIsSignalOn = true;
        }
        
        _s << " -- breakout_try=" << (++_p_runtime._orBreakoutTries) 
           << ",barsClosingInside=" << _p_runtime._orCountBarsClosingInside
           << ",totalVolInsideRange=" << _p_runtime._orTotalVolInsideRange
           << ",insideRangeInTicks=" << (_p_runtime._orOpenBarHigh-_p_runtime._orOpenBarLow);
        return true;
    }
    
    bool checkRange(const TBars& bars, uint32_t index) {
        if ( index < 1 || index > bars.size() ) {
            LOGGER_ERRO << "index < 1 || index > bars.size() -- " << index << " :: " << bars.size() << "\n";
            return false;
        }

        return true;
    }

private:
    ORBreakoutSignalParamsWire& _p;
    ORBreakoutSignalParamsRuntime _p_runtime;
    tw::common_str_util::TFastStream _s;
};

} // common_trade
} // tw
