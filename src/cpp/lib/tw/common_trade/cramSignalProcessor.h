#pragma once

#include <tw/common/defs.h>
#include <tw/price/quote.h>
#include <tw/log/defs.h>

#include <tw/common_trade/bars_manager.h>

namespace tw {
namespace common_trade {
    
typedef tw::common_trade::TBar TBar;
typedef tw::common_trade::TBars TBars;
  
class CramSignalProcessor {
public:
    CramSignalProcessor(CramSignalParamsWire& p) : _p(p) {
        clear();
    }
    
public:
    CramSignalParamsWire& getParams() {
        return _p;
    }
    
    const CramSignalParamsWire& getParams() const {
        return _p;
    }
    
    void clear() {
        _p._barIndex = 0;
        _p._cramPrice.clear();
    }
    
public:
    bool isEnabled() const {
        if ( !_p._cramBarFirmPriceQty.isValid() || 0 == _p._cramBarFirmPriceQty.get() )
            return false;
        
        return true;
    }
    
    bool calcStop(FillInfo& info,
                  const TBars& bars,
                  std::string& reason) {
        tw::common_str_util::TFastStream s;
        s.setPrecision(2);
        
        if ( !checkRange(bars, info._barIndex2) ) {
            s << "checkRange(bars, info._barIndex2) -- info="  << info.toString() << ",bars.size()=" << bars.size();
            reason = s.str();
            return false;
        }
        
        tw::price::Ticks stop;
        const TBar& bar = bars[info._barIndex2-1];
        if ( tw::channel_or::eOrderSide::kBuy == info._fill._side ) {
            stop = bar._low;
            if ( _p._initialStopTicks.isValid() ) {
                tw::price::Ticks stop2 = info._fill._price - _p._initialStopTicks;
                if ( stop < stop2 )
                    stop = stop2;
            }
            
            if ( info._stop.isValid() && info._stop >= stop ) {
                s << "info._stop.isValid() && info._stop >= stop -- stop=" << stop << ",info="  << info.toString() << ",bar=" << bar.toString();
                reason = s.str();
                return false;
            }
        } else {
            stop = bar._high;
            if ( _p._initialStopTicks.isValid() ) {
                tw::price::Ticks stop2 = info._fill._price + _p._initialStopTicks;
                if ( stop > stop2 )
                    stop = stop2;
            }
            
            if ( info._stop.isValid() && info._stop <= stop ) {
                s << "info._stop.isValid() && info._stop <= stop -- stop=" << stop << ",info="  << info.toString() << ",bar=" << bar.toString();
                reason = s.str();
                return false;
            }
        }
        
        info._stop = stop;        
        
        s << "CramSignalProcessor::calcStop()==true -- stop=" << info._stop
          << ",bar=" << bar.toString();
                
        reason = s.str();
        
        return true;
    }
    
    bool isSignalTriggered(const tw::price::Quote& quote,
                           const TBars& bars,
                           tw::channel_or::eOrderSide& side,
                           std::string& reason) {
        side = tw::channel_or::eOrderSide::kUnknown;
        reason.clear();
        
        tw::common_str_util::TFastStream s;
        s.setPrecision(2);
        
        if ( bars.empty() ) {
            s << "bars.empty()";
            
            reason = s.str();
            return false;
        }
        
        const TBar& bar = bars[bars.size()-1];
        if ( bar._range < _p._minCramBarTicks ) {
            s << "bar._range < _p._minCramBarTicks -- _p._minCramBarTicks=" << _p._minCramBarTicks << ",bar=" << bar.toString();
            
            reason = s.str();
            return false;
        }
        
        if ( _p._lastSignalBarIndex == bar._index ) {
            s << "_p._lastSignalIndex == bar._index -- _p._lastSignalBarIndex=" << _p._lastSignalBarIndex << ",bar=" << bar.toString();
            
            reason = s.str();
            return false;
        }
        
        if ( bar._index != _p._barIndex ) {
            clear();
            _p._barIndex = bar._index; 
        }
        
        tw::price::Ticks cramPrice;
        switch ( bar._dir ) {
            case tw::common_trade::ePatternDir::kUp:
            {
                for ( size_t i = 0; i < tw::price::Quote::SIZE; ++i ) {
                    if ( quote._book[i]._bid.isValid() && quote._book[i]._bid._size > _p._cramBarFirmPriceQty ) {
                        cramPrice = quote._book[i]._bid._price;
                        if ( !_p._cramPrice.isValid() || cramPrice > _p._cramPrice ) {
                            double cramPriceDepthPercent = (bar._high - cramPrice).toDouble() / bar._range.toDouble();
                            if ( cramPriceDepthPercent  <= _p._cramPriceDepthPercent ) {
                                s << "quote._book[i]._bid._size > _p._cramBarFirmPriceQty :: cramPrice=" << cramPrice << ",cramPriceDepthPercent=" << cramPriceDepthPercent << ",quote[" << i << "]=" + quote._book[i].toShortString() << " -- ";
                                _p._cramPrice = cramPrice;
                                break;
                            }
                        }
                    }
                }
                
                if ( !_p._cramPrice.isValid() ) {
                    s << "!_p._cramPrice.isValid()";            
                    reason = s.str();
                    return false;   
                }
                
                if ( !quote._trade._price.isValid() ) {
                    reason = "!quote._trade._price.isValid()";            
                    return false;   
                }
                
                if ( quote._trade._price >= _p._cramPrice ) {
                    s << "quote._trade._price >= _p._cramPrice - _p._cramPrice=" << _p._cramPrice << ",quote._trade=" << quote._trade.toString() << ",bar=" << bar.toString();            
                    reason = s.str();
                    return false;
                }
                
                side = tw::channel_or::eOrderSide::kSell;
            }
                break;
            case tw::common_trade::ePatternDir::kDown:
            {
                for ( size_t i = 0; i < tw::price::Quote::SIZE; ++i ) {
                    if ( quote._book[i]._ask.isValid() && quote._book[i]._ask._size > _p._cramBarFirmPriceQty ) {
                        cramPrice = quote._book[i]._ask._price;
                        if ( !_p._cramPrice.isValid() || cramPrice < _p._cramPrice ) {
                            double cramPriceDepthPercent = (cramPrice - bar._low).toDouble() / bar._range.toDouble();
                            if ( cramPriceDepthPercent  <= _p._cramPriceDepthPercent ) {
                                s << "quote._book[i]._ask._size > _p._cramBarFirmPriceQty :: cramPrice=" << cramPrice << ",cramPriceDepthPercent=" << cramPriceDepthPercent << ",quote[" << i << "]=" + quote._book[i].toShortString() << " -- ";
                                _p._cramPrice = cramPrice;
                                break;
                            }
                        }
                    }
                }
                
                if ( !_p._cramPrice.isValid() ) {
                    s << "!_p._cramPrice.isValid()";            
                    reason = s.str();
                    return false;   
                }
                
                if ( !quote._trade._price.isValid() ) {
                    reason = "!quote._trade._price.isValid()";            
                    return false;   
                }
                
                if ( quote._trade._price <= _p._cramPrice ) {
                    s << "quote._trade._price <= _p._cramPrice - _p._cramPrice=" << _p._cramPrice << ",quote._trade=" << quote._trade.toString() << ",bar=" << bar.toString();            
                    reason = s.str();
                    return false;
                }
                
                side = tw::channel_or::eOrderSide::kBuy;
            }                          
                break;
            default:
                s << "tw::common_trade::ePatternDir::kUnknown == bar._dir -- bar=" << bar.toString();
                reason = s.str();
                return false;
        }
        
        s << "CramSignalProcessor::isSignalTriggered()==true -- _p._cramPrice=" << _p._cramPrice
          << ",trigger_bar=" << bar.toString();
                
        _p._lastSignalBarIndex = bar._index;
        reason = s.str();
        return true;
    }
    
private:
    bool checkRange(const TBars& bars, uint32_t index) {
        if ( index < 1 || index > bars.size() ) {
            LOGGER_ERRO << "index < 1 || index > bars.size() -- " << index << " :: " << bars.size() << "\n";
            return false;
        }

        return true;
    }
    
private:
    CramSignalParamsWire& _p;
};

} // common_trade
} // tw
