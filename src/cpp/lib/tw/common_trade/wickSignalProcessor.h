#pragma once

#include <tw/common/defs.h>
#include <tw/price/quote.h>
#include <tw/log/defs.h>

#include <tw/common_trade/bars_manager.h>

namespace tw {
namespace common_trade {
    
typedef tw::common_trade::TBar TBar;
typedef tw::common_trade::TBars TBars;
typedef tw::common_trade::BarPattern TBarPattern;
typedef tw::price::Ticks TPrice;

class WickSignalProcessor {
public:
    WickSignalProcessor(WickSignalParamsWire& p) : _p(p) {
    }
    
public:
    WickSignalParamsWire& getParams() {
        return _p;
    }
    
    const WickSignalParamsWire& getParams() const {
        return _p;
    }
    
public:
    bool isSignalValid(const TBars& bars,                           
                       const TPrice& activePrice,
                       const tw::channel_or::eOrderSide side,
                       bool& placeOrder,
                       std::string& reason) {
        reason.clear();
        placeOrder = false;
        
        if ( !_p._validateSignal ) {
            reason = "!_p._validateSignal";
            placeOrder = true;
            return true;
        }
        
        tw::common_str_util::TFastStream s;
        s.setPrecision(2);
        
        if ( !activePrice.isValid() ) {
            reason = "!activePrice.isValid()";
            return true;
        }
        
        if ( bars.size() < 2 ) {
            s << "bars.size() < 2 -- bars.size()=" << bars.size();
            
            reason = s.str();
            return false;
        }
        
        const TBar& currBar = bars[bars.size()-1];
        if ( !currBar._close.isValid() ) {
            reason = "!currBar._close.isValid()";
            return true;
        }
        
        const TBar& signalBar = bars[bars.size()-2];
        if ( !signalBar._close.isValid() ) {
            reason = "!signalBar._close.isValid()";
            return false;
        }
        
        switch ( side ) {
            case tw::channel_or::eOrderSide::kBuy:
                if ( currBar._low < (signalBar._close-_p._maxAwayFromCloseTicks) ) {
                    s << "currBar._low < (signalBar._close-_p._maxAwayFromCloseTicks) -- currBar=" << currBar.toString() << ",signalBar=" << signalBar.toString();
                    
                    reason = s.str();
                    return false;
                }
                
                if ( activePrice > signalBar._close ) {
                    s << "activePrice > signalBar._close -- activePrice=" << activePrice << ",signalBar=" << signalBar.toString();
                    
                    reason = s.str();
                    placeOrder = true;
                }
                break;
            case tw::channel_or::eOrderSide::kSell:
                if ( currBar._high > (signalBar._close+_p._maxAwayFromCloseTicks) ) {
                    s << "currBar._high > (signalBar._close+_p._maxAwayFromCloseTicks) -- currBar=" << currBar.toString() << ",signalBar=" << signalBar.toString();
                    
                    reason = s.str();
                    return false;
                }
                
                if ( activePrice < signalBar._close ) {
                    s << "activePrice < signalBar._close -- activePrice=" << activePrice << ",signalBar=" << signalBar.toString();
                    
                    reason = s.str();
                    placeOrder = true;
                }
                break;
            default:
                reason = "tw::channel_or::eOrderSide::kUnknown == side";
                return false;
        }        
        
        return true;
    }
    
    bool isSignalTriggered(const TBars& bars,
                           const TBarPattern& pattern,
                           tw::channel_or::eOrderSide& side,
                           std::string& reason) {
        side = tw::channel_or::eOrderSide::kUnknown;
        reason.clear();
        
        tw::common_str_util::TFastStream s;
        s.setPrecision(2);
        
        if ( bars.size() < 3 ) {
            s << "bars.size() < 3 -- bars.size()=" << bars.size();
            
            reason = s.str();
            return false;
        }
        
        if ( pattern._firstBarIndex == pattern._lastBarIndex ) {
            s << "pattern._firstBarIndex == pattern._lastBarIndex -- pattern=" << pattern.toString();
            
            reason = s.str();
            return false;
        }
        
        if ( pattern._range < _p._minPatternTicks ) {
            s << "pattern._range < _p._minPatternTicks -- pattern=" << pattern.toString();
            
            reason = s.str();
            return false;            
        }
        
        const TBar& bar = bars[bars.size()-2];
        if ( bar._range < _p._minWickBarTicks ) {
            s << "bar._range < _p._minWickBarTicks -- bar=" << bar.toString();
            
            reason = s.str();
            return false;
        }
        
        double wickBarToSwingRatio = bar._range.toDouble() / pattern._range.toDouble();
        if ( wickBarToSwingRatio > _p._wickBarToSwingRatio ) {
            s << "wickBarToSwingRatio > _p._wickBarToSwingRatio -- wickBarToSwingRatio=" << wickBarToSwingRatio << ",bar=" << bar.toString() << ".pattern=" << pattern.toString();
            
            reason = s.str();
            return false;
        }
        
        tw::price::Ticks body;
        tw::price::Ticks wick;        
        double wickToBodyRatio = 0.0;
    
        switch ( pattern._dir ) {
            case tw::common_trade::ePatternDir::kDown:
            {
                switch ( _p._wickBarToPatternDir ) {
                    case eWickBarToPatternDir::kSame:
                        if ( tw::common_trade::ePatternDir::kUp == bar._dir ) {
                            s << "eWickBarToPatternDir::kSame && tw::common_trade::ePatternDir::kUp == bar._dir -- bar=" << bar.toString() << "pattern=" << pattern.toString();
                            
                            reason = s.str();
                            return false;
                        }
                        break;
                    case eWickBarToPatternDir::kOpposite:
                        if ( tw::common_trade::ePatternDir::kDown == bar._dir ) {
                            s << "eWickBarToPatternDir::kOpposite && tw::common_trade::ePatternDir::kDown == bar._dir -- bar=" << bar.toString() << "pattern=" << pattern.toString();
                            
                            reason = s.str();
                            return false;
                        }
                        break;
                    default:break;
                }
                
                if ( pattern._lowBarIndex != bar._index ) {
                    s <<  "pattern._lowBarIndex != bar._index -- bar=" << bar.toString() << "pattern=" << pattern.toString();
                    
                    reason = s.str();
                    return false;
                }                
                
                wick = (bar._open > bar._close ? (bar._close-bar._low) : (bar._open-bar._low));
                if ( _p._useBodyInWickToBodyRatio )
                    body = (bar._open-bar._close).abs();                    
                else
                    body = bar._range-wick;
                
                wickToBodyRatio = wick.toDouble() / (body.get() > 0 ? body.toDouble() : 1.0);
                if ( wickToBodyRatio < _p._wickToBodyRatio ) {
                    s << "wickToBodyRatio < _p._wickToBodyRatio -- wick=" << wick << ",body=" << body << ",wickToBodyRatio=" << wickToBodyRatio << ",bar=" << bar.toString();
                    
                    reason = s.str();
                    return false;
                }
                
                if ( _p._wickOutsidePrevHighLow ) {
                    // Make sure that wick's high is lower than any close in the swing
                    //
                    tw::price::Ticks wickHigh = bar._low + wick;
                    for ( uint32_t i = pattern._firstBarIndex; i < bar._index; ++i ) {
                        if ( checkRange(bars, i) && wickHigh > bars[i-1]._close ) {
                            s << "wickHigh > bars[i-1]._close -- wick=" << wick << ",wickHigh=" << wickHigh << ",bar=" << bar.toString() << ",bars[" << i << "]=" << bars[i-1].toString();
                    
                            reason = s.str();
                            return false;
                        }
                    }
                }
                
                side = tw::channel_or::eOrderSide::kBuy;
            }
                break;
            case tw::common_trade::ePatternDir::kUp:
            {
                switch ( _p._wickBarToPatternDir ) {
                    case eWickBarToPatternDir::kSame:
                        if ( tw::common_trade::ePatternDir::kDown == bar._dir ) {
                            s << "eWickBarToPatternDir::kSame && tw::common_trade::ePatternDir::kDown == bar._dir -- bar=" << bar.toString() << "pattern=" << pattern.toString();
                            
                            reason = s.str();
                            return false;
                        }
                        break;
                    case eWickBarToPatternDir::kOpposite:
                        if ( tw::common_trade::ePatternDir::kUp == bar._dir ) {
                            s << "eWickBarToPatternDir::kOpposite && tw::common_trade::ePatternDir::kUp == bar._dir -- bar=" << bar.toString() << "pattern=" << pattern.toString();
                            
                            reason = s.str();
                            return false;
                        }
                        break;
                    default:break;
                }
                
                if ( pattern._highBarIndex != bar._index ) {
                    s << "pattern._highBarIndex != bar._index -- bar=" << bar.toString() << "pattern=" << pattern.toString();
                    
                    reason = s.str();
                    return false;
                }
                
                wick = (bar._open > bar._close ? (bar._high-bar._open) : (bar._high-bar._close));
                if ( _p._useBodyInWickToBodyRatio )
                    body = (bar._open-bar._close).abs();
                else
                    body = bar._range-wick;
                
                wickToBodyRatio = wick.toDouble() / (body.get() > 0 ? body.toDouble() : 1.0);
                if ( wickToBodyRatio < _p._wickToBodyRatio ) {
                    s << "wickToBodyRatio < _p._wickToBodyRatio -- wick=" << wick << ",body=" << body << ",wickToBodyRatio=" << wickToBodyRatio << ",bar=" << bar.toString();
                    
                    reason = s.str();
                    return false;
                }
                
                if ( _p._wickOutsidePrevHighLow ) {
                    // Make sure that wick's low is higher than any close in the swing
                    //
                    tw::price::Ticks wickLow = bar._high - wick;
                    for ( uint32_t i = pattern._firstBarIndex; i < bar._index; ++i ) {
                        if ( checkRange(bars, i) && wickLow < bars[i-1]._close ) {
                            s << "wickLow < bars[i-1]._close -- wick=" << wick << ",wickLow=" << wickLow << ",bar=" << bar.toString() << ",bars[" << i << "]=" << bars[i-1].toString();
                    
                            reason = s.str();
                            return false;
                        }
                    }
                }
                
                side = tw::channel_or::eOrderSide::kSell;
            }                          
                break;
            default:
                s << "tw::common_trade::ePatternDir::kUnknown == pattern._dir -- pattern=" << pattern.toString();
                
                reason = s.str();
                return false;
        }        
        
        
        s << "WickSignalProcessor::isSignalTriggered()==true -- wick=" << wick 
          << ",body=" << body
          << ",wickBarToSwingRatio=" << wickBarToSwingRatio
          << ",wickToBodyRatio=" << wickToBodyRatio
          << ",trigger_bar=" << bar.toString()
          << ",pattern=" << pattern.toString();
                
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
    WickSignalParamsWire _p;
};

} // common_trade
} // tw
