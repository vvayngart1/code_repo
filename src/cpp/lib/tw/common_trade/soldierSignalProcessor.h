#pragma once

#include <tw/common/defs.h>
#include <tw/price/quote.h>
#include <tw/log/defs.h>
#include <tw/common_trade/bars_manager.h>

namespace tw {
namespace common_trade {
    
typedef tw::common_trade::TBar TBar;
typedef tw::common_trade::TBars TBars;

class SoldierSignalProcessor {
public:
    SoldierSignalProcessor(SoldierSignalParamsWire& p) : _p(p) {
        _p._useBackBarEntryLevel = tw::price::Ticks(0);
    }
    
public:
    SoldierSignalParamsWire& getParams() {
        return _p;
    }
    
    const SoldierSignalParamsWire& getParams() const {
        return _p;
    }
    
public:     
    tw::price::Ticks getLevel() {
        return _p._useBackBarEntryLevel;    
    }
     
    bool isSignalTriggered(const TBars& bars,tw::channel_or::eOrderSide& side,std::string& reason) {
        side = tw::channel_or::eOrderSide::kUnknown;
        reason.clear();
        
        // guide to fields
        // ---------------------------------------------------------
        // <useBackBarCutoffTicks       type="tw::price::Ticks"        desc="disabled if set to '0'"/>
        // <useBackBarPayupTicks        type="tw::price::Ticks"        desc="how much to payup from open of third soldier bar"/>
        // <useBackBarTimeLimit         type="uint32_t"                desc="how many seconds to wait from time of signal to get filled at entry level"/>
        // <minTotalDisplaceTicks       type="tw::price::Ticks"/>
        // <minTicksBetterClose         type="tw::price::Ticks"/>
        // <rejWickToBodyRatio          type="float"/>
        // <rejSmallerBar               type="bool"/>
        // <useBackBarEntryLevel        type="tw::price::Ticks"        serializable='false'/>
        
        tw::common_str_util::TFastStream s;
        s.setPrecision(2);
        
    // ==> prelim. need 3 bars to proceed with soldier evaluation
        if ( bars.size() < 4 ) {
            s << "bars.size() < 4 -- bars.size()=" << bars.size();
            reason = s.str();
            return false;
        }
        
        const TBar& bar1 = bars[bars.size()-4];
        const TBar& bar2 = bars[bars.size()-3];
        const TBar& bar3 = bars[bars.size()-2];
        
        if (!bar3._close.isValid()) {
            s << "bar3 not valid; exiting";
            reason = s.str();
            return false;
        }
        
        if (!bar2._close.isValid()) {
            s << "bar2 not valid; exiting";
            reason = s.str();
            return false;
        }
        
        if (!bar1._close.isValid()) {
            s << "bar1 not valid; exiting";
            reason = s.str();
            return false;
        }
        
        tw::price::Ticks wick;   
        double wickToBodyRatio = 0.0;
        
        if (bar3._close > bar3._open) {
            // :: up bar
            wick = bar3._high - bar3._close;
        } else {
            // :: down bar
            wick = bar3._close - bar3._low;
        }
        
    // ==> 0. final wick flaw
        tw::price::Ticks body = bar3._range - wick;
        wickToBodyRatio = wick.toDouble() / (body.get() > 0 ? body.toDouble() : 1.0);
        if ( wickToBodyRatio > _p._rejWickToBodyRatio ) {
            s << "wickToBodyRatio > _p._wickToBodyRatio -- wick=" << wick << ",body=" << body << ",wickToBodyRatio=" << wickToBodyRatio << ",bar3=" << bar3.toString();
            reason = s.str();
            return false;
        }
        
        // arithmetic requirement on bar1, bar2
        tw::price::Ticks disp3 = bar3._close - bar3._open;
        tw::price::Ticks disp2 = bar2._close - bar2._open;
        tw::price::Ticks disp1 = bar1._close - bar1._open;
        tw::price::Ticks disp_total = bar3._close - bar1._open;

    // ==> 1. transition bar flaw
        if (0 == disp3) {
            s << "disp3=" << disp3 << " is transition bar; exiting signal logic, bar3=" << bar3.toString();
            reason = s.str();
            return false;
        }
        
    // ==> 2. soldier trend flaw
        if (0 < disp3) {
            side = tw::channel_or::eOrderSide::kBuy;
            if (0 >= disp2) {
                s << "disp2=" << disp2 << "<=0, <BUY> trend defined by disp3=" << disp3 << " violated by bar2=" << bar2.toString();
                reason = s.str();
                return false;
            }
            
            if (0 >= disp1) {
                s << "disp1=" << disp1 << "<=0, <BUY> trend defined by disp3=" << disp3 << " violated by bar1=" << bar1.toString();
                reason = s.str();
                return false;
            }
            
            if (disp_total < _p._minTotalDisplaceTicks) {
                s << "disp_total=" << disp_total << " < " << _p._minTotalDisplaceTicks;
                reason = s.str();
                
                return false;
            }
            
            // UP SOLDIER DOES NOT BUY OPEN HIGHER THAN ENTRY LEVEL + PAYUP
            if ( disp_total > _p._useBackBarCutoffTicks ) {
                _p._useBackBarEntryLevel = bar3._open + _p._useBackBarPayupTicks;
            } else {
                _p._useBackBarEntryLevel = tw::price::Ticks(0);
            }
        } else {
            side = tw::channel_or::eOrderSide::kSell;
            if (0 <= disp2) {
                s << "disp2=" << disp2 << ">=0, <SELL> trend defined by disp3=" << disp3 << " violated by bar2=" << bar2.toString();
                reason = s.str();
                return false;
            }
            
            if (0 <= disp1) {
                s << "disp1=" << disp1 << ">=0, <SELL> trend defined by disp3=" << disp3 << " violated by bar1=" << bar1.toString();
                reason = s.str();
                return false;
            }
            
            if (disp_total > -_p._minTotalDisplaceTicks) {
                s << "disp_total=" << disp_total << " > -" << _p._minTotalDisplaceTicks;
                reason = s.str();
                return false;
            }
            
            // DOWN SOLDIER DOES NOT SELL OPEN LOWER THAN ENTRY LEVEL - PAYUP
            if ( disp_total < -_p._useBackBarCutoffTicks ) {
                _p._useBackBarEntryLevel = bar3._open - _p._useBackBarPayupTicks;
            } else {
                _p._useBackBarEntryLevel = tw::price::Ticks(0);
            }
        }
        
        if (0 < disp3) {
    // ==> 3(up). bar 2 flaw
            if (bar2._close < bar1._close + _p._minTicksBetterBar) {
                s << "bar2._close=" << bar2._close << " < bar1._close=" << bar1._close << " + " << _p._minTicksBetterBar;
                reason = s.str();
                return false;
            }

    // ==> 4(up). bar 3 flaw
            if (bar3._close < bar2._close + _p._minTicksBetterBar) {
                s << "bar3._close=" << bar3._close << " < bar2._close=" << bar2._close << " + " << _p._minTicksBetterBar;
                reason = s.str();
                return false;
            }    
        } else {
    // ==> 3(down). bar 2 flaw
            if (bar2._close > bar1._close - _p._minTicksBetterBar) {
                s << "bar2._close=" << bar2._close << " > bar1._close=" << bar1._close << " - " << _p._minTicksBetterBar;
                reason = s.str();
                return false;
            }

    // ==> 4(down). bar 3 flaw
            if (bar3._close > bar2._close - _p._minTicksBetterBar) {
                s << "bar3._close=" << bar3._close << " > bar2._close=" << bar2._close << " - " << _p._minTicksBetterBar;
                reason = s.str();
                return false;
            } 
        }
        
    // ==> 5. final bar size flaw
        if (_p._rejSmallerBar) {
            if (0 < disp3) {
                if ( (disp3 < disp2) && (disp3 < disp1) ) {
                    s << "disp3=" << disp3 << " smaller magnitude than {disp1=" << disp1 << ",disp2=" << disp2 << "} for bar3=" << bar3.toString();

                    reason = s.str();
                    return false;
                }    
            } else {
                if ( (disp3 > disp2) && (disp3 > disp1) ) {
                    s << "disp3=" << disp3 << " smaller magnitude than {disp1=" << disp1 << ",disp2=" << disp2 << "} for bar3=" << bar3.toString();

                    reason = s.str();
                    return false;
                }
            }
        }
        
        s << "SoldierSignalProcessor::isSignalTriggered()==true -- wick=" << wick 
          << ",body=" << body
          << ",wick=" << wick
          << ",wickToBodyRatio=" << wickToBodyRatio
          << ",disp1=" << disp1
          << ",disp2=" << disp2
          << ",disp3=" << disp3
          << ",disp_total=" << disp_total
          << ",side=" << side.toString()
          << ",_p._useBackBarEntryLevel=" << _p._useBackBarEntryLevel
          << ",trigger_bar=" << bar3.toString();
        
        reason = s.str();
        return true;
    }
    
private:
    SoldierSignalParamsWire _p;
};

} // common_trade
} // tw

