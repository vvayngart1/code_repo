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
  
class ContinuationTradeSignalProcessor {
public:
    ContinuationTradeSignalProcessor(ContinuationTradeSignalParamsWire& p) : _p(p) {
        _p_runtime._ctLastSignalBarIndex = 0;
        _p_runtime._ctLastProcessedBarIndex = 0;
        clear();
    }
    
public:
    ContinuationTradeSignalParamsWire& getParams() {
        return _p;
    }
    
    const ContinuationTradeSignalParamsWire& getParams() const {
        return _p;
    }
    
    ContinuationTradeSignalParamsRuntime& getParamsRuntime() {
        return _p_runtime;
    }
    
    const ContinuationTradeSignalParamsRuntime& getParamsRuntime() const {
        return _p_runtime;
    }
    
    void clear() {
        changeState(eContinuationTradeSignalState::kUnknown);
        _p_runtime._ctMaxPayupPrice.clear();
        _p_runtime._ctMaxFlowbackPrice.clear();
        _p_runtime._ctStopPrice.clear();
        _p_runtime._ctSide = tw::channel_or::eOrderSide::kUnknown;
        _p_runtime._ctReason.clear();
        _s.clear();
        _s.setPrecision(2);
    }
    
    bool isConfirmContinuationBarFailed(const TBars& bars) {
        if ( 0 < _p._ctConfirmContinuationBarIndex ) {
            uint32_t barToCheckIndex = _p_runtime._ctLastSignalBarIndex + _p._ctConfirmContinuationBarIndex;
            if ( barToCheckIndex < (bars.size()-1) ) {
                if ( checkRange(bars, _p_runtime._ctLastSignalBarIndex-1) && checkRange(bars, barToCheckIndex) ) {
                    const TBar& signalBar = bars[_p_runtime._ctLastSignalBarIndex-1];
                    const TBar& barToCheck = bars[barToCheckIndex];
                    bool closedBetter = (tw::channel_or::eOrderSide::kBuy == _p_runtime._ctSide ? barToCheck._close > signalBar._high : barToCheck._close < signalBar._low);
                    if ( !closedBetter ) {
                        _p_runtime._ctReason = "isConfirmContinuationBarFailed()==true: signalBar=" + signalBar.toString() + ",barToCheck=" + barToCheck.toString();
                        return true;
                    }
                } else {
                    LOGGER_ERRO << "checkRange(bars, _p_runtime._ctLastSignalBarIndex-1) && checkRange(bars, barToCheckIndex) failed -  _p_runtime._ctLastSignalBarIndex=" << _p_runtime._ctLastSignalBarIndex
                                << ",barToCheckIndex=" << barToCheckIndex 
                                << "\n";
                }
            }
        }
        
        return false;
    }
    
public:
    void monitorSignal(const tw::price::Quote& quote,
                       const TBars& bars,
                       const tw::price::Ticks& activePrice) {
        _s.clear();
        _s.setPrecision(2);
        if ( !checkRange(bars, _p_runtime._ctLastSignalBarIndex) )  {
            _s << "Severe error: !checkRange(bars, _p_runtime._ctLastSignalBarIndex): _p_runtime._ctLastSignalBarIndex="  << _p_runtime._ctLastSignalBarIndex;            
            
            _p_runtime._ctReason = _s.str();
            changeState(eContinuationTradeSignalState::kCancelled);
            return;
        }

        // Turn off signal after 1 minute
        //
        if ( _p_runtime._ctLastSignalBarIndex < bars.size()-1 ) {
            _s << "_p_runtime._ctLastSignalBarIndex < bars.size()-1: _p_runtime._ctLastSignalBarIndex="  << _p_runtime._ctLastSignalBarIndex
               << ",bars.size()=" << bars.size();            
            
            _p_runtime._ctReason = _s.str();
            changeState(eContinuationTradeSignalState::kCancelled);
            return;
        }
        
        if ( !activePrice.isValid() ) {
            _p_runtime._ctReason = "!activePrice.isValid()";
            return;
        }
        tw::price::Size volumeFor;
        tw::price::Size volumeAgainst;
        tw::price::Ticks gapOnOpen;
        tw::price::Ticks limitPrice;
        std::string limitName;
                
        const TBar& bar = bars[_p_runtime._ctLastSignalBarIndex-1];
        const TBar& currBar = bars.back();
        switch(_p_runtime._ctSignalState) {
            case eContinuationTradeSignalState::kSignaled:
            {
                if ( tw::channel_or::eOrderSide::kSell == _p_runtime._ctSide ) {
                    // Check circuit breakers
                    //
                    if ( _p._ctFlowbackEnabled ) {
                        // Turn off signal if upper limit is exceeded
                        //
                        limitPrice = bar._close + quote._instrument->_tc->nearestTick(bar._atr);
                        limitName = "ATR";
                        
                        if ( limitPrice > bar._high ) {
                            limitPrice = bar._high;
                            limitName = "bar._high";
                        }
                    } else {
                        limitPrice = _p_runtime._ctMaxFlowbackPrice;
                        limitName = "_p_runtime._ctMaxFlowbackPrice";
                    }
                    
                    if ( upperLimitExceeded(quote, bar, ((quote._trade._price.isValid() && quote._book[0]._bid._price.isValid()) ? (quote._trade._price > quote._book[0]._bid._price ? quote._trade._price : quote._book[0]._bid._price) : (quote._trade._price.isValid() ? quote._trade._price : quote._book[0]._bid._price)), limitPrice, limitName) )
                        return;
                    
                    if ( lowerLimitExceeded(quote, bar, activePrice, _p_runtime._ctMaxPayupPrice, "_p_runtime._ctMaxPayupPrice") )
                        return;
                    
                    gapOnOpen = !_p._ctInitiateOnGap ? tw::price::Ticks(0) : (bar._index+1 != currBar._index ? tw::price::Ticks(0) : bar._close - currBar._open);
                    if ( gapOnOpen.get() < 1 ) {
                        if ( _p._ctFlowbackEnabled && (quote._book[0]._ask._price.isValid() && quote._book[0]._ask._price > _p_runtime._ctMaxFlowbackPrice) )
                            _p_runtime._ctFlowbackOccurred = true;
                        
                        if ( _p_runtime._ctFlowbackOccurred ) {
                            volumeFor = calcDownVolume(currBar, quote._book[0]._bid._price, quote._book[0]._bid._price);
                            volumeAgainst = calcUpVolume(currBar, quote._book[0]._bid._price+tw::price::Ticks(1), currBar._high);
                        } else {
                            volumeFor = calcDownVolume(bar, currBar, _p_runtime._ctMaxPayupPrice);
                            volumeAgainst = calcUpVolume(bar, currBar, _p_runtime._ctMaxFlowbackPrice);
                        }
                    }
                } else {
                    // Check circuit breakers
                    //
                    if ( _p._ctFlowbackEnabled ) {
                        // Turn off signal if upper limit is exceeded
                        //
                        limitPrice = bar._close - quote._instrument->_tc->nearestTick(bar._atr);
                        limitName = "ATR";
                        
                        if ( limitPrice < bar._low ) {
                            limitPrice = bar._low;
                            limitName = "bar._low";
                        }
                    } else {
                        limitPrice = _p_runtime._ctMaxFlowbackPrice;
                        limitName = "_p_runtime._ctMaxFlowbackPrice";
                    }
                    
                    if ( lowerLimitExceeded(quote, bar, ((quote._trade._price.isValid() && quote._book[0]._ask._price.isValid()) ? (quote._trade._price < quote._book[0]._ask._price ? quote._trade._price : quote._book[0]._ask._price) : (quote._trade._price.isValid() ? quote._trade._price : quote._book[0]._ask._price)), limitPrice, limitName) )
                        return;
                    
                    if ( upperLimitExceeded(quote, bar, activePrice, _p_runtime._ctMaxPayupPrice, "_p_runtime._ctMaxPayupPrice") )
                        return;
                    
                    gapOnOpen = !_p._ctInitiateOnGap ? tw::price::Ticks(0) : (bar._index+1 != currBar._index ? tw::price::Ticks(0) : currBar._open - bar._close);
                    if ( gapOnOpen.get() < 1 ) {
                        if ( _p._ctFlowbackEnabled && (quote._book[0]._bid._price.isValid() && quote._book[0]._bid._price < _p_runtime._ctMaxFlowbackPrice) )
                            _p_runtime._ctFlowbackOccurred = true;
                        
                        if ( _p_runtime._ctFlowbackOccurred ) {
                            volumeFor = calcUpVolume(currBar, quote._book[0]._ask._price, quote._book[0]._ask._price);
                            volumeAgainst = calcDownVolume(currBar, quote._book[0]._ask._price-tw::price::Ticks(1), currBar._low);
                        } else {
                            volumeFor = calcUpVolume(bar, currBar, _p_runtime._ctMaxPayupPrice);
                            volumeAgainst = calcDownVolume(bar, currBar, _p_runtime._ctMaxFlowbackPrice);
                        }
                    }
                }
                
                if ( gapOnOpen.get() > 0 ) {
                    _s << " ==> gapOnOpen > 0: gapOnOpen=" << gapOnOpen
                       << ",bar=" << bar.toString()
                       << ",currBar=" << currBar.toString();
                    
                    _p_runtime._ctReason += _s.str();
                    changeState(eContinuationTradeSignalState::kValidated);
                    return;
                }
                
                if ( !volumeFor.isValid() || !volumeAgainst.isValid() )
                    return;
                
                if ( volumeFor < _p._ctMinContinuationQtyFor )
                    return;
                
                double ratio = volumeFor.toDouble() / (0 == volumeAgainst.get() ? 1.0 : volumeAgainst.toDouble());
                if ( ratio < _p._ctRatioFor )
                    return;
                
                _s << " ==> ratio >= _p._ctRatioFor -- volumeFor=" << volumeFor
                   << ",volumeAgainst=" << volumeAgainst
                   << ",ratio=" << ratio
                   << ",_p._ctMinContinuationQtyFor=" << _p._ctMinContinuationQtyFor
                   << ",_p._ctRatioFor=" << _p._ctRatioFor;
                 
                _p_runtime._ctReason += _s.str();
                changeState(eContinuationTradeSignalState::kValidated);
            }
                break;                
            case eContinuationTradeSignalState::kValidated:
                tw::channel_or::eOrderSide::kSell == _p_runtime._ctSide ? lowerLimitExceeded(quote, bar, activePrice, _p_runtime._ctMaxPayupPrice, "_p_runtime._ctMaxPayupPrice") : upperLimitExceeded(quote, bar, activePrice, _p_runtime._ctMaxPayupPrice, "_p_runtime._ctMaxPayupPrice");                
                break;
            default:
                break;
        }
    }
    
    bool isSignalTriggered(const TBars& bars,
                           const TBarPattern& pattern,
                           const TBarPattern& trend,
                           eTradeBias tradeBias) {
        const size_t currBarIndex = bars.size()-1;
        if ( currBarIndex == _p_runtime._ctLastSignalBarIndex )
            return true;
        
        if ( currBarIndex == _p_runtime._ctLastProcessedBarIndex ) {
            if ( !_p_runtime._ctReason.empty() )
                _p_runtime._ctReason.clear();
            return false;
        }
        
        clear();
        
        _p_runtime._ctLastProcessedBarIndex = currBarIndex;
        if ( bars.size() < 3 ) {
            _p_runtime._ctReason = "bars.size() < 3";
            return false;
        }
        
        const TBar& bar = bars[bars.size()-2];
        
        if ( tw::common_trade::ePatternDir::kUnknown == bar._dir ) {
            _s << "tw::common_trade::ePatternDir::kUnknown == bar._dir bar=" << bar.toString();
            
            _p_runtime._ctReason = _s.str();
            return false;
        }
        
        if ( !bar._range.isValid() || bar._range < _p._ctMinDispTicks ) {
            _s << "!bar._range.isValid() || bar._range < _p._ctMinDispTicks -- _p._ctMinDispTicks=" << _p._ctMinDispTicks << ",bar=" << bar.toString();

            _p_runtime._ctReason = _s.str();
            return false;
        }
        
        if ( _p._ctMinVol.isValid() && _p._ctMinVol.get() > 0 && _p._ctMinVol > bar._volume ) {
            _s << "_p._ctMinVol > bar._volume -- _p._ctMinVol=" << _p._ctMinVol << ",bar=" << bar.toString();

            _p_runtime._ctReason = _s.str();
            return false;
        }
        
        if ( _p._ctMinDispATRMult > 0 ) {
            double minDispATR = _p._ctMinDispATRMult * bar._atr;
            if ( bar._range.toDouble() < minDispATR ) {
                _s << "bar._range.toDouble() < minDispATR -- _p._ctMinDispATRMult=" << _p._ctMinDispATRMult 
                   << ",minDispATR=" << minDispATR
                   << ",bar=" << bar.toString();
                
                _p_runtime._ctReason = _s.str();
                return false;
            }
        }

        if ( _p._ctAtrRequired > bar._atr ) {
            _s << "_p._ctAtrRequired > bar._atr -- _p._ctAtrRequired=" << _p._ctAtrRequired << ",bar=" << bar.toString();

            _p_runtime._ctReason = _s.str();
            return false;
        }

        tw::price::Ticks disp = (tw::common_trade::ePatternDir::kDown == bar._dir) ? (bar._high-bar._close) : (bar._close-bar._low);
        if ( !disp.isValid() || 0 == disp.get() ) {
            _s << "body=0 -- bar=" << bar.toString();

            _p_runtime._ctReason = _s.str();
            return false;
        }

        double bodyToBarRatio = disp.toDouble() / bar._range.toDouble();
        if ( bodyToBarRatio < _p._ctBodyToBarRatio ) {
            _s << "bodyToBarRatio < _p._ctBodyToBarRatio -- bodyToBarRatio=" << bodyToBarRatio << ",_p._ctBodyToBarRatio=" << _p._ctBodyToBarRatio << ",bar=" << bar.toString();

            _p_runtime._ctReason = _s.str();
            return false;
        }

        switch ( pattern._dir ) {
            case tw::common_trade::ePatternDir::kDown:
            {
                if ( _p._ctReqBarInSwingDir && tw::common_trade::ePatternDir::kDown != bar._dir ) {
                    _s << "_p._ctReqBarInSwingDir && tw::common_trade::ePatternDir::kDown != bar._dir -- pattern=" << pattern.toString() << ",bar=" << bar.toString();

                    _p_runtime._ctReason = _s.str();
                    return false;   
                }

                if ( _p._ctCheckSwingHighLow && bar._index != pattern._lowBarIndex ) {
                    _s << "_p._ctCheckSwingHighLow && bar._index != pattern._lowBarIndex -- pattern=" << pattern.toString() << ",bar=" << bar.toString();

                    _p_runtime._ctReason = _s.str();
                    return false;   
                }
            }
                break;
            case tw::common_trade::ePatternDir::kUp:
            {
                if ( _p._ctReqBarInSwingDir && tw::common_trade::ePatternDir::kUp != bar._dir ) {
                    _s << "_p._ctReqBarInSwingDir && tw::common_trade::ePatternDir::kUp != bar._dir -- pattern=" << pattern.toString() << ",bar=" << bar.toString();

                    _p_runtime._ctReason = _s.str();
                    return false;   
                }

                if ( _p._ctCheckSwingHighLow && bar._index != pattern._highBarIndex ) {
                    _s << "_p._ctCheckSwingHighLow && bar._index != pattern._highBarIndex -- pattern=" << pattern.toString() << ",bar=" << bar.toString();

                    _p_runtime._ctReason = _s.str();
                    return false;   
                }
            }
                break;
            default:
                if ( _p._ctReqBarInSwingDir || _p._ctCheckSwingHighLow ) {
                    _s << "tw::common_trade::ePatternDir::kUnknown == pattern._dir -- pattern=" << pattern.toString();

                    _p_runtime._ctReason = _s.str();
                    return false;   
                }
                break;
        }

        switch ( trend._dir ) {
            case tw::common_trade::ePatternDir::kDown:
            {
                if ( _p._ctReqBarInTrendDir ) {
                    if ( _p._ctReqTrendExtreme ) {
                        if ( tw::common_trade::ePatternDir::kDown != bar._dir ) {
                            _s << "tw::common_trade::ePatternDir::kDown != bar._dir -- "
                               << "trend=" << trend.toString() 
                               << ",bar=" << bar.toString();

                            _p_runtime._ctReason = _s.str();
                            return false; 
                        }
                        
                        if ( trend._lowBarIndex != bar._index ) {
                            _s << "trend._lowBarIndex != bar._index -- "
                               << "trend=" << trend.toString() 
                               << ",bar=" << bar.toString();

                            _p_runtime._ctReason = _s.str();
                            return false;   
                        }
                    } else if ( tw::common_trade::ePatternDir::kDown != bar._dir ) {
                        double breakTrendWatermark = trend._low.toDouble() + trend._range.toDouble()*_p._ctSwingBreakTrendRatio;
                        if ( breakTrendWatermark > pattern._high.toDouble() ) {
                            _s << "_p._ctReqBarInTrendDir && tw::common_trade::ePatternDir::kDown != bar._dir) && (breakTrendWatermark > pattern._high.toDouble()) -- breakTrendWatermark=" << breakTrendWatermark 
                               << ",_p._ctSwingBreakTrendRatio=" << _p._ctSwingBreakTrendRatio 
                               << ",pattern=" << pattern.toString() 
                               << ",trend=" << trend.toString() 
                               << ",bar=" << bar.toString();

                            _p_runtime._ctReason = _s.str();
                            return false;   
                        }
                    }
                }
            }
                break;
            case tw::common_trade::ePatternDir::kUp:
            {
                if ( _p._ctReqBarInTrendDir ) {
                    if ( _p._ctReqTrendExtreme ) {
                        if ( tw::common_trade::ePatternDir::kUp != bar._dir ) {
                            _s << "tw::common_trade::ePatternDir::kUp != bar._dir -- "
                               << "trend=" << trend.toString() 
                               << ",bar=" << bar.toString();

                            _p_runtime._ctReason = _s.str();
                            return false; 
                        }
                        
                        if ( trend._highBarIndex != bar._index ) {
                            _s << "trend._highBarIndex != bar._index -- "
                               << "trend=" << trend.toString() 
                               << ",bar=" << bar.toString();

                            _p_runtime._ctReason = _s.str();
                            return false;   
                        }
                    } else if ( tw::common_trade::ePatternDir::kUp != bar._dir ) {
                        double breakTrendWatermark = trend._high.toDouble() - trend._range.toDouble()*_p._ctSwingBreakTrendRatio;
                        if ( breakTrendWatermark < pattern._low.toDouble() ) {
                            _s << "_p._ctReqBarInTrendDir && tw::common_trade::ePatternDir::kUp != bar._dir) && (breakTrendWatermark < pattern._low.toDouble()) -- breakTrendWatermark=" << breakTrendWatermark 
                               << ",_p._ctSwingBreakTrendRatio=" << _p._ctSwingBreakTrendRatio 
                               << ",pattern=" << pattern.toString() 
                               << ",trend=" << trend.toString() 
                               << ",bar=" << bar.toString();

                            _p_runtime._ctReason = _s.str();
                            return false;   
                        }
                    }
                }
            }
                break;
            default:
                if ( _p._ctReqBarInTrendDir ) {
                    _s << "tw::common_trade::ePatternDir::kUnknown == trend._dir -- trend=" << trend.toString();

                    _p_runtime._ctReason = _s.str();
                    return false;   
                }
                break;
        }
        
        _p_runtime._ctSide = (tw::common_trade::ePatternDir::kDown == bar._dir) ? tw::channel_or::eOrderSide::kSell : tw::channel_or::eOrderSide::kBuy;
        bool counterTradeBias = false;
        if ( tw::channel_or::eOrderSide::kBuy == _p_runtime._ctSide ) {
            switch (tradeBias) {
                case eTradeBias::kLong:
                case eTradeBias::kNeutral:
                    break;
                default:
                    counterTradeBias = true;
                    break;
            }
        } else {
            switch (tradeBias) {
                case eTradeBias::kShort:
                case eTradeBias::kNeutral:
                    break;
                default:
                    counterTradeBias = true;
                    break;
            }
        }
        
        if ( counterTradeBias ) {
            _s << "signal NOT triggered because of tradeBias -- tradeBias=" << tradeBias.toString()
               << ",_p_runtime._ctSide=" << _p_runtime._ctSide.toString();
            _p_runtime._ctReason = _s.str();
            _p_runtime._ctSide = tw::channel_or::eOrderSide::kUnknown;
            return false;
        }
        
        if ( _p._ctReqBarFlatClose ) {
            if ( (tw::common_trade::ePatternDir::kUp == bar._dir && bar._close != bar._high) || (tw::common_trade::ePatternDir::kDown == bar._dir && bar._close != bar._low) ) {
                _s << "signal NOT triggered because bar doesn't have flat close";
                _p_runtime._ctReason = _s.str();
                _p_runtime._ctSide = tw::channel_or::eOrderSide::kUnknown;
                return false;
            }
        }
        
        _p_runtime._ctMaxPayupPrice = (tw::common_trade::ePatternDir::kDown == bar._dir) ? (bar._close-_p._ctMaxPayupTicks) : (bar._close+_p._ctMaxPayupTicks);
        _p_runtime._ctMaxFlowbackPrice = (tw::common_trade::ePatternDir::kDown == bar._dir) ? (bar._close+_p._ctMaxFlowbackTicks) : (bar._close-_p._ctMaxFlowbackTicks);
        
        if ( _p._ctStopTicks.isValid() && (0 < _p._ctStopTicks.get()) )
            _p_runtime._ctStopPrice = (tw::common_trade::ePatternDir::kDown == bar._dir) ? (bar._high+_p._ctStopTicks) : (bar._low-_p._ctStopTicks);
        else
            _p_runtime._ctStopPrice = (tw::common_trade::ePatternDir::kDown == bar._dir) ? (_p_runtime._ctMaxFlowbackPrice+_p._ctMaxFlowbackStopTicks) : (_p_runtime._ctMaxFlowbackPrice-_p._ctMaxFlowbackStopTicks);
        
        _p_runtime._ctLastSignalBarIndex = currBarIndex;
        _p_runtime._ctSignalState = eContinuationTradeSignalState::kSignaled;
        
        _s << "ContinuationTradeSignalProcessor::isSignalTriggered()==true -- _p_runtime._ctSide=" << _p_runtime._ctSide.toString()
           << ",_p_runtime._ctMaxPayupPrice=" << _p_runtime._ctMaxPayupPrice
           << ",_p_runtime._ctMaxFlowbackPrice=" << _p_runtime._ctMaxFlowbackPrice
           << ",_p_runtime._ctStopPrice=" << _p_runtime._ctStopPrice
           << ",_p_runtime._ctLastSignalBarIndex=" << _p_runtime._ctLastSignalBarIndex
           << ",_p._ctMinDispTicks=" << _p._ctMinDispTicks
           << ",_p._ctBodyToBarRatio=" << _p._ctBodyToBarRatio
           << ",_p._ctAtrRequired=" << _p._ctAtrRequired
           << ",_p._ctMinVol=" << _p._ctMinVol
           << ",trigger_bar=" << bar.toString()
           << ",pattern=" << pattern.toString() 
           << ",trend=" << trend.toString();
        
        _p_runtime._ctReason = _s.str();
        return true;
    }

private:
    void changeState(eContinuationTradeSignalState state) {
        _p_runtime._ctSignalState = state;
        _p_runtime._ctFlowbackOccurred = false;
    }
    
    bool checkRange(const TBars& bars, uint32_t index) {
        if ( index < 1 || index > bars.size() ) {
            LOGGER_ERRO << "index < 1 || index > bars.size() -- " << index << " :: " << bars.size() << "\n";
            return false;
        }

        return true;
    }
    
    tw::price::Size calcUpVolume(const TBar& currBar, const tw::price::Ticks& fromLimit, const tw::price::Ticks& toLimit) {
        tw::price::Size volume;
        if ( !fromLimit.isValid() || !toLimit.isValid() )
            return volume;
        
        std::tr1::unordered_map<tw::price::Ticks::type, tw::common_trade::PriceTradesInfo>::const_iterator iter;
        for ( tw::price::Ticks i = fromLimit; i <= toLimit; ++i ) {                    
             iter = currBar._trades.find(i.get());
             if ( iter != currBar._trades.end() )
                 volume += iter->second._volAsk;
        }
        
        return volume;
    }
    
    tw::price::Size calcDownVolume(const TBar& currBar, const tw::price::Ticks& fromLimit, const tw::price::Ticks& toLimit) {
        tw::price::Size volume;
        if ( !fromLimit.isValid() || !toLimit.isValid() )
            return volume;
        
        std::tr1::unordered_map<tw::price::Ticks::type, tw::common_trade::PriceTradesInfo>::const_iterator iter;
        for ( tw::price::Ticks i = fromLimit; i >= toLimit; --i ) {                    
            iter = currBar._trades.find(i.get());
            if ( iter != currBar._trades.end() )
                    volume += iter->second._volBid;
        }
        
        return volume;
    }
    
    tw::price::Size calcUpVolume(const TBar& signalBar, const TBar& currBar, const tw::price::Ticks& limit) {
        tw::price::Size volume;
        std::tr1::unordered_map<tw::price::Ticks::type, tw::common_trade::PriceTradesInfo>::const_iterator iter;
        for ( tw::price::Ticks i = signalBar._close; i <= limit; ++i ) {                    
             iter = currBar._trades.find(i.get());
             if ( iter != currBar._trades.end() ) {
                 if ( signalBar._close < i )
                     volume += iter->second._volBid + iter->second._volAsk;
                 else if ( !_p._ctIgnoreCloseVol )
                     volume += iter->second._volAsk;
             }
        }
        
        return volume;
    }
    
    tw::price::Size calcDownVolume(const TBar& signalBar, const TBar& currBar, const tw::price::Ticks& limit) {
        tw::price::Size volume;
        std::tr1::unordered_map<tw::price::Ticks::type, tw::common_trade::PriceTradesInfo>::const_iterator iter;
        for ( tw::price::Ticks i = signalBar._close; i >= limit; --i ) {                    
            iter = currBar._trades.find(i.get());
            if ( iter != currBar._trades.end() ) {
                if ( signalBar._close > i )
                    volume += iter->second._volBid + iter->second._volAsk;
                else if ( !_p._ctIgnoreCloseVol )
                     volume += iter->second._volBid;
            }
        }
        
        return volume;
    }
    
    bool upperLimitExceeded(const tw::price::Quote& quote, const TBar& bar, const tw::price::Ticks& price, const tw::price::Ticks& limit, const std::string& limitName) {
        if ( limit.isValid() && price.isValid() && price > limit ) {
            _s << " ==> price > " << limitName << " -- "
               << limitName << "=" << limit
               << ",side="  << _p_runtime._ctSide.toString()
               << ",quote=" << quote.toShortString()
               << ",bar=" << bar.toString();
            
            _p_runtime._ctReason = _s.str();
            changeState(eContinuationTradeSignalState::kCancelled);
            return true;
        }
        
        return false;
    }
    
    bool lowerLimitExceeded(const tw::price::Quote& quote, const TBar& bar, const tw::price::Ticks& price, const tw::price::Ticks& limit, const std::string& limitName) {
        if ( limit.isValid() && price.isValid() && price < limit ) {
            _s << " ==> price < " << limitName << " -- "
               << limitName << "=" << limit
               << ",side="  << _p_runtime._ctSide.toString()
               << ",quote=" << quote.toShortString()
               << ",bar=" << bar.toString();
            
            _p_runtime._ctReason = _s.str();
            changeState(eContinuationTradeSignalState::kCancelled);
            return true;
        }
        
        return false;
    }
    
private:
    ContinuationTradeSignalParamsWire& _p;
    ContinuationTradeSignalParamsRuntime _p_runtime;
    tw::common_str_util::TFastStream _s;
};

} // common_trade
} // tw
