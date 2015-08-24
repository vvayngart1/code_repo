#pragma once

#include <tw/common/defs.h>
#include <tw/price/quote.h>
#include <tw/log/defs.h>

namespace tw {
namespace common_trade {

typedef tw::common_trade::TBar TBar;
typedef tw::common_trade::TBars TBars;

class MarketActivityProcessor {
public:
    MarketActivityProcessor(MarketActivityParamsWire& p) : _p(p) {
        _p._maIsOn = false;
    }
    
    void clear() {
        if ( _p._maReason.empty() )
            return;
        
        _p._maReason.clear();
        _s.clear();
        _s.setPrecision(2);
    }
    
public:
    MarketActivityParamsWire& getParams() {
        return _p;
    }
    
    const MarketActivityParamsWire& getParams() const {
        return _p;
    }
    
public:
    bool isEnabled() const {
        return _p._maEnabled;
    }
    
    bool isSignalOn() const {
        return _p._maIsOn;
    }
    
    template <typename TBarsManager>
    void processSignal(const TBars& bars, const TBarsManager& barManager) {
        if ( !isEnabled() )
            return;
        
        clear();
        
        const size_t currBarIndex = bars.size()-1;
        if ( currBarIndex == _p._maLastProcessedBarIndex )
            return;
        
        _p._maLastProcessedBarIndex = currBarIndex;
        
        if ( bars.size() < 3 ) {
            _p._maReason = "bars.size() < 3";
            return;
        }
        
        size_t barIndex = bars.size()-2;
        const TBar& bar = bars[barIndex];
        
        if ( !_p._maIsOn ) {
            if ( (bar._range.isValid() && bar._range >= _p._maMinDispTicks) && (!_p._maMinVolToEnter.isValid() || bar._volume >= _p._maMinVolToEnter) ) {
                _s << "_p._maIsOn==true(bar._range >= _p._maMinDispTicks && (!_p._maMinVolToEnter.isValid() || bar._volume >= _p._maMinVolToEnter)) -- _p._ctMinDispTicks=" << _p._maMinDispTicks 
                   << ",_p._maMinVolToEnter=" << _p._maMinVolToEnter
                   << ",bar=" << bar.toString();
                setSignalOn(barIndex);
                return;
            }
            
            if ( bar._atr >= _p._maAtrRequired ) {
                _s << "_p._maIsOn==true(bar._atr >= _p._maAtrRequired) -- _p._maAtrRequired=" << _p._maAtrRequired << ",bar=" << bar.toString();
                setSignalOn(barIndex);
                return;
            }
            
            if ( bar._volume >= _p._maVolToEnter ) {
                _s << "_p._maIsOn==true(bar._volume >= _p._maVolToEnter) -- _p._maVolToEnter=" << _p._maVolToEnter << ",bar=" << bar.toString();
                setSignalOn(barIndex);
                return;
            }
            
            _s << "_p._maIsOn==false(not changed) -- _p._maMinDispTicks=" << _p._maMinDispTicks 
               << ",_p._maAtrRequired=" << _p._maAtrRequired
               << ",_p._maVolToEnter=" << _p._maVolToEnter
               << ",_p._maMinVolToEnter=" << _p._maMinVolToEnter
               << ",bar=" << bar.toString();

            _p._maReason = _s.str();
            
            return;
        } else {
            if ( (_p._maLastSignalOnBarIndex+_p._maVolMultToExitInterval) > (currBarIndex-1) )
                return;
            
            tw::price::Size exitIntervalVolume = barManager.getBackBarsCumVol(bar._instrument->_keyId, _p._maVolMultToExitInterval, 1UL)/tw::price::Size(_p._maVolMultToExitInterval);
            
            uint32_t onIntervalCount = currBarIndex-(_p._maLastSignalOnBarIndex+_p._maVolMultToExitInterval);
            tw::price::Size onIntervalVolume = barManager.getBackBarsCumVol(bar._instrument->_keyId, onIntervalCount, 1UL+_p._maVolMultToExitInterval)/tw::price::Size(onIntervalCount);
            
            double ratio = (exitIntervalVolume.toDouble()/onIntervalVolume.toDouble());
            if ( ratio < _p._maVolMultToExit ) {
                _p._maIsOn = false;
                _s << "_p._maIsOn==false(ratio < _p._maVolMultToExit) -- _p._maVolMultToExit=" << _p._maVolMultToExit 
                   << ",_p._maLastSignalOnBarIndex=" << _p._maLastSignalOnBarIndex
                   << ",_p._maVolMultToExitInterval=" << _p._maVolMultToExitInterval
                   << ",onIntervalCount=" << onIntervalCount
                   << ",exitIntervalVolume=" << exitIntervalVolume
                   << ",onIntervalVolume=" << onIntervalVolume
                   << ",ratio=" << ratio
                   << ",bar=" << bar.toString();
                _p._maReason = _s.str();
                return;
            }
            
            return;
        }
    }
    
private:
    void setSignalOn(size_t barIndex) {
        _p._maIsOn = true;
        _p._maLastSignalOnBarIndex = barIndex;        
        _p._maReason = _s.str();
    }
    
private:
    MarketActivityParamsWire& _p;
    tw::common_str_util::TFastStream _s;
};

} // common_trade
} // tw
