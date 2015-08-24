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

class PiercingEngulfingReversalSignalProcessor : public TParent {
public:
    PiercingEngulfingReversalSignalProcessor(ATRPricingParamsWire& p1,
                                             BaseReversalParamsWire& p2,
                                             PiercingEngulfingReversalParamsWire& p3) : TParent(p1),
                                                                                        _base_p(p2),
                                                                                        _p(p3) {
        
    }
    
    void clear() {
        TParent::clear();
        _base_p._brReason.clear();
    }
    
public:
    PiercingEngulfingReversalParamsWire& getParams() {
        return _p;
    }
    
    const PiercingEngulfingReversalParamsWire& getParams() const {
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
        return _p._perEnabled;
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
         
        switch ( _p._perState ) {
            case ePiercingEngulfingReversalState::kUnknown:
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
                
                if ( bar._range < _p._perFirstBarMinDispTicks ) {
                    TParent::_s << "bar._range < _p._perFirstBarMinDispTicks -- _p._perFirstBarMinDispTicks=" << _p._perFirstBarMinDispTicks << ",bar=" << bar.toString();
                    return copyReasonAndReturn();
                }
                
                if ( ePiercingEngulfingReversalEntryStyle::kInTrend == _p._perEntryStyle && barPattern._dir == trends.back()._dir ) {
                    TParent::_s << "ePiercingEngulfingReversalEntryStyle::kInTrend == _p._perEntryStyle && barPattern._dir == trends.back()._dir -- barPattern=" << barPattern.toString() << ",trend=" << trends.back().toString();
                    return copyReasonAndReturn();
                }
                
                if ( _p._perFirstBarBodyToBarRatio > 0 ) {
                    tw::price::Ticks body = (ePatternDir::kUp == bar._dir ? (bar._close-bar._open) : (bar._open-bar._close));
                    double ratio = body.toDouble() / bar._range.toDouble();
                    if ( ratio < _p._perFirstBarBodyToBarRatio ) {
                        TParent::_s << "ratio < _p._perFirstBarBodyToBarRatio -- ratio=" << ratio 
                                    << ",_p._perFirstBarBodyToBarRatio=" << _p._perFirstBarBodyToBarRatio
                                    << ",bar=" << bar.toString();
                        return copyReasonAndReturn();
                    }
                }
                
                if ( ePatternDir::kUp == barPattern._dir ) {
                    if ( barPattern._highBarIndex != bar._index ) {
                        TParent::_s << "barPattern._highBarIndex != bar._index -- bar=" << bar.toString() << ",barPattern=" << barPattern.toString();
                        return copyReasonAndReturn();
                    }                                        
                } else {
                    if ( barPattern._lowBarIndex != bar._index ) {
                        TParent::_s << "barPattern._lowBarIndex != bar._index -- bar=" << bar.toString() << ",barPattern=" << barPattern.toString();
                        return copyReasonAndReturn();
                    }
                }
                
                _base_p._brTriggerBarIndex = bar._index;
                _p._perState = ePiercingEngulfingReversalState::kFirstBarSatisfied;
                
                TParent::_s << "Changed state to: ePiercingEngulfingReversalState::kFirstBarSatisfied -- bar=" << bar.toString() << ",barPattern=" << barPattern.toString();
            }
                return copyReasonAndReturn();
            case ePiercingEngulfingReversalState::kFirstBarSatisfied:
            {
                const TBar& firstBar = bars[bar._index-2];
                if ( firstBar._dir == bar._dir ) {
                    TParent::_s << "Not satisfied as second bar(firstBar._dir == bar._dir) -- bar=" << bar.toString() << ",firstBar=" << firstBar.toString() << " <==> ";
                    clearState();
                    return isSignalTriggered(bars, swings, trends, keltnerBandInfo);
                }
                
                if ( ePatternDir::kUp == firstBar._dir ) {
                    if ( _p._perReqSecondBarLowerOpen && bar._open <= firstBar._high ) {
                        TParent::_s << "Not satisfied as second bar(_p._perReqSecondBarLowerOpen && bar._open <= firstBar._high) -- bar=" << bar.toString() << ",firstBar=" << firstBar.toString() << " <==> ";
                        clearState();
                        return isSignalTriggered(bars, swings, trends, keltnerBandInfo);
                    }
                    
                    double breakLevel = firstBar._high.toDouble() - _p._perSecondBarRetracePercent * firstBar._range.toDouble();
                    if ( breakLevel < bar._close.toDouble() ) {
                        TParent::_s << "Not satisfied as second bar(breakLevel < bar._close.toDouble()) -- breakLevel=" << breakLevel 
                                    << ",_p._perSecondBarRetracePercent=" << _p._perSecondBarRetracePercent 
                                    << ",bar=" << bar.toString() 
                                    << ",firstBar=" << firstBar.toString() << " <==> ";
                        clearState();
                        return isSignalTriggered(bars, swings, trends, keltnerBandInfo);
                    }
                    
                    TParent::_p._atrOrderSide = tw::channel_or::eOrderSide::kSell;
                } else {
                    if ( _p._perReqSecondBarLowerOpen && bar._open >= firstBar._low ) {
                        TParent::_s << "Not satisfied as second bar(_p._perReqSecondBarLowerOpen && bar._open >= firstBar._low) -- bar=" << bar.toString() << ",firstBar=" << firstBar.toString() << " <==> ";
                        clearState();
                        return isSignalTriggered(bars, swings, trends, keltnerBandInfo);
                    }
                    
                    double breakLevel = firstBar._low.toDouble() + _p._perSecondBarRetracePercent * firstBar._range.toDouble();
                    if ( breakLevel > bar._close.toDouble() ) {
                        TParent::_s << "Not satisfied as second bar(breakLevel > bar._close.toDouble()) -- breakLevel=" << breakLevel 
                                    << ",_p._perSecondBarRetracePercent=" << _p._perSecondBarRetracePercent 
                                    << ",bar=" << bar.toString() 
                                    << ",firstBar=" << firstBar.toString() << " <==> ";
                        clearState();
                        return isSignalTriggered(bars, swings, trends, keltnerBandInfo);
                    }
                    
                    TParent::_p._atrOrderSide = tw::channel_or::eOrderSide::kBuy;
                }
                
                if ( !calcPriceOffsets(keltnerBandInfo._atr) ) {
                    clearState();
                    return copyReasonAndReturn();
                }
                
                TParent::_s << "PiercingEngulfingReversalSignalProcessor: isSignalTriggered()=true -- _p._atrOrderSide=" << TParent::_p._atrOrderSide.toString()
                            << ",_p._atrEntryOrderPriceOffset=" << TParent::_p._atrEntryOrderPriceOffset
                            << ",_p._atrExitOrderPriceOffset=" << TParent::_p._atrExitOrderPriceOffset
                            << ",firstBar=" << firstBar.toString() 
                            << ",secondBar=" << bar.toString();
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
        int32_t barsCount = bar._index-(_base_p._brTriggerBarIndex+1);
        if ( barsCount > _base_p._brBarsCountToWork ) {
            clear();
              TParent::_s << "PiercingEngulfingReversalSignalProcessor: turnSignalOff(barsCount > _base_p._brBarsCountToWork)=true -- barsCount=" << barsCount
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
        
        _p._perState = ePiercingEngulfingReversalState::kUnknown;
    }
    
    bool copyReasonAndReturn() {
        _base_p._brReason =  TParent::_s.str();
        return false;
    }
    
private:
    BaseReversalParamsWire& _base_p;
    PiercingEngulfingReversalParamsWire& _p;
    
};

} // common_trade
} // tw

