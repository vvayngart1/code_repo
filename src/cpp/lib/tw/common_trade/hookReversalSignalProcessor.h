#pragma once

#include <tw/common/defs.h>
#include <tw/price/quote.h>
#include <tw/log/defs.h>
#include <tw/common_trade/bars_manager.h>
#include <tw/common_trade/keltnerBandProcessor.h>
#include <tw/common_trade/atrPricingProcessor.h>

namespace tw {
namespace common_trade {
    
typedef tw::common_trade::TBar TBar;
typedef tw::common_trade::TBars TBars;
typedef tw::common_trade::TBarPatterns TBarPatterns;
typedef tw::common_trade::BarPattern TBarPattern;

typedef ATRPricingProcessor TParent;

class HookReversalSignalProcessor : public TParent {
public:
    HookReversalSignalProcessor(ATRPricingParamsWire& p1,
                                BaseReversalParamsWire& p2,
                                HookReversalParamsWire& p3) : TParent(p1),
                                                              _base_p(p2),
                                                              _p(p3) {
        
    }
    
    void clear() {
        TParent::clear();
        _base_p._brReason.clear();
    }
    
public:
    HookReversalParamsWire& getParams() {
        return _p;
    }
    
    const HookReversalParamsWire& getParams() const {
        return _p;
    }
    
    BaseReversalParamsWire& getBaseParams() {
        return _base_p;
    }
    
    const BaseReversalParamsWire& getBaseParams() const {
        return _base_p;
    }
    
public:     
    bool isEnabled() const {
        return _p._hrEnabled;
    }
     
    bool isSignalTriggered(const TBars& bars,
                           const TBarPatterns& swings,
                           const TBarPatterns& trends,
                           const KeltnerBandInfo& keltnerBandInfo) {
        if ( !isEnabled() )
            return false;
        
        clear();
        
        if ( bars.size() < 2 || swings.empty() || trends.empty() ) {
            _base_p._brReason = "bars.size() < 2 || swings.empty() || trends.empty()";
            return false;
        }
        
        const TBar& bar = bars[bars.size()-2];                
        if ( bar._index == _base_p._brLastProcessedBarIndex )
            return false;
        
        _base_p._brLastProcessedBarIndex = bar._index;
         
        switch ( _p._hrState ) {
            case eHookReversalState::kUnknown:
            {
                const TBarPattern& barPattern = swings.back();
                if ( !isHLOCValid(bar) || !isHLOCValid(barPattern) || ePatternDir::kUnknown == barPattern._dir ) {
                     TParent::_s << "!isHLOCValid(bar) || !isHLOCValid(barPattern) || ePatternDir::kUnknown == barPattern._dir -- bar=" << bar.toString() << ",barPattern=" << barPattern.toString();
                    return copyReasonAndReturn();
                }
                
                if ( bar._dir != barPattern._dir ) {
                     TParent::_s << "bar._dir != barPattern._dir -- bar=" << bar.toString() << ",barPattern=" << barPattern.toString();
                    return copyReasonAndReturn();
                }
                
                if ( bar._range < _p._hrFirstBarMinDispTicks ) {
                     TParent::_s << "bar._range < _p._hrFirstBarMinDispTicks -- _p._hrFirstBarMinDispTicks=" << _p._hrFirstBarMinDispTicks << ",bar=" << bar.toString();
                    return copyReasonAndReturn();
                }
                
                if ( eHookReversalEntryStyle::kInTrend == _p._hrEntryStyle && barPattern._dir == trends.back()._dir ) {
                     TParent::_s << "eHookReversalEntryStyle::kInTrend == _p._hrEntryStyle && barPattern._dir == trends.back()._dir -- barPattern=" << barPattern.toString() << ",trend=" << trends.back().toString();
                    return copyReasonAndReturn();
                }
                
                if ( ePatternDir::kUp == barPattern._dir ) {
                    if ( barPattern._highBarIndex != bar._index ) {
                         TParent::_s << "barPattern._highBarIndex != bar._index -- bar=" << bar.toString() << ",barPattern=" << barPattern.toString();
                        return copyReasonAndReturn();
                    }
                    
                    if ( !(bar._high.toDouble() > keltnerBandInfo._upperValue) && eHookReversalEntryStyle::kKeltnerBand == _p._hrEntryStyle ) {
                         TParent::_s << "!(bar._high.toDouble() > keltnerBandInfo._upperValue) -- bar=" << bar.toString() << ",keltnerBandInfo=" << keltnerBandInfo.toString();
                        return copyReasonAndReturn();
                    }
                    
                    double bodyToBarRatio = (bar._close-bar._open).toDouble()/bar._range.toDouble();
                    if ( bodyToBarRatio < _p._hrFirstBarBodyToBarRatio ) {
                         TParent::_s << "bodyToBarRatio < _p._hrFirstBarBodyToBarRatio -- bar=" << bar.toString() << ",bodyToBarRatio" << bodyToBarRatio << ",_p._hrFirstBarBodyToBarRatio=" << _p._hrFirstBarBodyToBarRatio;
                        return copyReasonAndReturn();
                    }
                } else {
                    if ( barPattern._lowBarIndex != bar._index ) {
                         TParent::_s << "barPattern._lowBarIndex != bar._index -- bar=" << bar.toString() << ",barPattern=" << barPattern.toString();
                        return copyReasonAndReturn();
                    }
                    
                    if ( !(bar._low.toDouble() < keltnerBandInfo._lowerValue) && eHookReversalEntryStyle::kKeltnerBand == _p._hrEntryStyle ) {
                         TParent::_s << "!(bar._low.toDouble() < keltnerBandInfo._lowerValue) -- bar=" << bar.toString() << ",keltnerBandInfo=" << keltnerBandInfo.toString();
                        return copyReasonAndReturn();
                    }
                    
                    double bodyToBarRatio = (bar._open-bar._close).toDouble()/bar._range.toDouble();
                    if ( bodyToBarRatio < _p._hrFirstBarBodyToBarRatio ) {
                         TParent::_s << "bodyToBarRatio < _p._hrFirstBarBodyToBarRatio -- bar=" << bar.toString() << ",bodyToBarRatio" << bodyToBarRatio << ",_p._hrFirstBarBodyToBarRatio=" << _p._hrFirstBarBodyToBarRatio;
                        return copyReasonAndReturn();
                    }
                }
                
                _base_p._brTriggerBarIndex = bar._index;
                _base_p._brTriggerBarPatternIndex = barPattern._index;
                _p._hrState = eHookReversalState::kFirstBarSatisfied;
                
                TParent::_s << "Changed state to: " << _p._hrState.toString() << " -- bar=" << bar.toString() << ",barPattern=" << barPattern.toString();
            }
                return copyReasonAndReturn();
            case eHookReversalState::kFirstBarSatisfied:
            {
                const TBarPattern& barPattern = swings.back();
                if ( barPattern._index != _base_p._brTriggerBarPatternIndex ) {
                    clearState();
                    TParent::_s << "barPattern._index != _base_p._brTriggerBarPatternIndex -- barPattern._index=" << barPattern._index << ",_base_p._brTriggerBarPatternIndex=" << _base_p._brTriggerBarPatternIndex << ",barPattern=" << barPattern.toString();
                    return copyReasonAndReturn();
                }
                
                bool isTransitionBar = false;
                if ( isHLOCValid(bar) && bar._open == bar._close ) {
                    isTransitionBar = true;
                    TParent::_s << "transitionBar: bar._open==bar._close <==> ";
                }
                
                if ( ePatternDir::kUp == barPattern._dir ) {
                    if ( barPattern._highBarIndex != bar._index && !isTransitionBar ) {
                        clearState();
                        TParent::_s << "barPattern._highBarIndex != bar._index && !isTransitionBar -- bar=" << bar.toString() << ",barPattern=" << barPattern.toString();
                        return copyReasonAndReturn();
                    }
                    
                    double bodyToBarRatio = (bar._close-bar._open).toDouble()/bar._range.toDouble();
                    if ( bodyToBarRatio > _p._hrSecondBarBodyToBarRatio ) {
                        TParent::_s << "Not satisfied as second bar(bodyToBarRatio > _p._hrSecondBarBodyToBarRatio) -- bodyToBarRatio=" << bodyToBarRatio
                                    << ",_p._hrSecondBarBodyToBarRatio=" << _p._hrSecondBarBodyToBarRatio 
                                    << ",bar=" << bar.toString() 
                                    << " <==> ";
                        clearState();
                        return isSignalTriggered(bars, swings, trends, keltnerBandInfo);
                    }
                } else {
                    if ( barPattern._lowBarIndex != bar._index && !isTransitionBar ) {
                        clearState();
                        TParent::_s << "barPattern._lowBarIndex != bar._index && !isTransitionBar -- bar=" << bar.toString() << ",barPattern=" << barPattern.toString();
                        return copyReasonAndReturn();
                    }
                    
                    double bodyToBarRatio = (bar._open-bar._close).toDouble()/bar._range.toDouble();
                    if ( bodyToBarRatio > _p._hrSecondBarBodyToBarRatio ) {
                        TParent::_s << "Not satisfied as second bar(bodyToBarRatio > _p._hrSecondBarBodyToBarRatio) -- bodyToBarRatio=" << bodyToBarRatio
                                    << ",_p._hrSecondBarBodyToBarRatio=" << _p._hrSecondBarBodyToBarRatio 
                                    << ",bar=" << bar.toString() 
                                    << " <==> ";
                        clearState();
                        return isSignalTriggered(bars, swings, trends, keltnerBandInfo);
                    }
                }
                
                const TBar& firstBar = bars[bar._index-2];
                if ( (firstBar._close-firstBar._open).abs() < (bar._close-bar._open).abs() ) {
                    TParent::_s << "Not satisfied as second bar((firstBar._close-firstBar._open).abs() < (bar._close-bar._open).abs()) -- bar=" << bar.toString()
                                << ",firstBar=" << firstBar.toString() 
                                << " <==> ";
                    clearState();
                    return isSignalTriggered(bars, swings, trends, keltnerBandInfo);
                }
                
                _p._hrState = eHookReversalState::kSecondBarSatisfied;
                
                TParent::_s << "Changed state to: " << _p._hrState.toString() << " -- bar=" << bar.toString() << ",barPattern=" << barPattern.toString();
            }
                return copyReasonAndReturn();
            case eHookReversalState::kSecondBarSatisfied:
            {
                const TBar& firstBar = bars[bar._index-3];
                const TBar& secondBar = bars[bar._index-2];
                
                if ( firstBar._dir == bar._dir ) {
                    TParent::_s << "Not satisfied as third bar(firstBar._dir == bar._dir) -- bar=" << bar.toString()
                                << ",firstBar=" << firstBar.toString() 
                                << " <==> ";
                    clearState();
                    return isSignalTriggered(bars, swings, trends, keltnerBandInfo);
                }
                
                if ( ePatternDir::kUp == firstBar._dir ) {
                    double breakLevel = firstBar._high.toDouble() - _p._hrThirdBarRetracePercent * firstBar._range.toDouble();
                    if ( breakLevel < bar._close.toDouble() ) {
                        TParent::_s << "Not satisfied as third bar(breakLevel < bar._close.toDouble()) -- breakLevel=" << breakLevel
                                    << ",_p._hrThirdBarRetracePercent=" << _p._hrThirdBarRetracePercent 
                                    << ",bar=" << bar.toString() 
                                    << " <==> ";
                        clearState();
                        return isSignalTriggered(bars, swings, trends, keltnerBandInfo);
                    }
                    
                    if ( bar._high > secondBar._high ) {
                        TParent::_s << "Not satisfied as third bar(bar._high > secondBar._high) -- bar=" << bar.toString()
                                    << ",secondBar=" << secondBar.toString() 
                                    << " <==> ";
                        clearState();
                        return isSignalTriggered(bars, swings, trends, keltnerBandInfo);
                    }
                    
                    TParent::_p._atrOrderSide = tw::channel_or::eOrderSide::kSell;
                } else {
                    double breakLevel = firstBar._low.toDouble() + _p._hrThirdBarRetracePercent * firstBar._range.toDouble();
                    if ( breakLevel > bar._close.toDouble() ) {
                        TParent::_s << "Not satisfied as third bar(breakLevel > bar._close.toDouble()) -- breakLevel=" << breakLevel
                                    << ",_p._hrThirdBarRetracePercent=" << _p._hrThirdBarRetracePercent 
                                    << ",bar=" << bar.toString() 
                                    << " <==> ";
                        clearState();
                        return isSignalTriggered(bars, swings, trends, keltnerBandInfo);
                    }
                    
                    if ( bar._low < secondBar._low ) {
                        TParent::_s << "Not satisfied as third bar(bar._low < secondBar._low) -- bar=" << bar.toString()
                                    << ",secondBar=" << secondBar.toString() 
                                    << " <==> ";
                        clearState();
                        return isSignalTriggered(bars, swings, trends, keltnerBandInfo);
                    }
                    
                    TParent::_p._atrOrderSide = tw::channel_or::eOrderSide::kBuy;
                }
                
                if ( !calcPriceOffsets(keltnerBandInfo._atr) ) {
                    clearState();
                    return copyReasonAndReturn();
                }
                
                TParent::_s << "HookReversalSignalProcessor: isSignalTriggered()=true -- _p._atrOrderSide=" << TParent::_p._atrOrderSide.toString()
                            << ",_p._atrEntryOrderPriceOffset=" << TParent::_p._atrEntryOrderPriceOffset
                            << ",_p._atrExitOrderPriceOffset=" << TParent::_p._atrExitOrderPriceOffset
                            << ",firstBar=" << firstBar.toString() 
                            << ",secondBar=" << secondBar.toString() 
                            << ",thirdBar=" << bar.toString();
            }
                break;
        }
        
        _base_p._brReason =  TParent::_s.str();
        clearState(false);
        return true;
    }
    
    bool turnSignalOff(const TBars& bars) {
        if ( bars.empty() )
            return false;
        
        const TBar& bar = bars.back();
        int32_t barsCount = bar._index-(_base_p._brTriggerBarIndex+2);
        if ( barsCount > _base_p._brBarsCountToWork ) {
            clear();
              TParent::_s << "HookReversalSignalProcessor: turnSignalOff(barsCount > _base_p._brBarsCountToWork)=true -- barsCount=" << barsCount
                << ",_base_p._brBarsCountToWork=" << _base_p._brBarsCountToWork
                << ",_base_p._brTriggerBarIndex=" << _base_p._brTriggerBarIndex                
                << ",bar=" << bar.toString();
             _base_p._brReason =  TParent::_s.str();
             return true;
        }
        
        return false;
    }
    
private:
    void clearState(bool resetLastProcessedBarIndex = true) {
        if ( resetLastProcessedBarIndex )
            --_base_p._brLastProcessedBarIndex;
        
        _p._hrState = eHookReversalState::kUnknown;
    }
    
    bool copyReasonAndReturn() {
        _base_p._brReason =  TParent::_s.str();
        return false;
    }
    
private:
    BaseReversalParamsWire& _base_p;
    HookReversalParamsWire& _p;
    
};

} // common_trade
} // tw

