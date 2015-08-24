
#include <tw/common/defs.h>
#include <tw/price/quote.h>
#include <tw/log/defs.h>
#include <tw/common_trade/bars_manager.h>
#include <tw/common/high_res_time.h>

namespace tw {
namespace common_trade {
    
typedef tw::common_trade::TBar TBar;
typedef tw::common_trade::TBars TBars;

class TransitionSignalProcessor {
public:
    TransitionSignalProcessor(TransitionSignalParamsWire& p) : _p(p) {
        clear();
        _p._tSidePrev = tw::channel_or::eOrderSide::kUnknown;
        _p._tBarIndexPrev = 0;
    }
    
public:
    TransitionSignalParamsWire& getParams() {
        return _p;
    }
    
    const TransitionSignalParamsWire& getParams() const {
        return _p;
    }
    
    TransitionSignalParamsRuntime& getParamsRuntime() {
        return _p_runtime;
    }
    
    const TransitionSignalParamsRuntime& getParamsRuntime() const {
        return _p_runtime;
    }
    
    void clear() {
        // do not clear -- is needed by Scalper
        // _p_runtime._tStopPrice.clear();
        
        _p._tSignalTwiceRejected = false;
        _p._tBuyAllowed = true;
        _p._tSellAllowed = true;
        _p._tCramCompleted = false;
        _p._top.clear();
        _p._fRatio = 0;
        _p._fLocated = false;
        _p._bLocated = false;
        _p._fMap.clear();
        _p._tBarLocated = false;
        _p._tBar.clear();
        _p._tBarIndex = 0;
        _p._tHigh.clear();
        _p._tLow.clear();
        
        // read related logic
        _p._tReadTradedAbove = tw::price::Size(0);
        _p._tReadTradedBelow = tw::price::Size(0);
        _p._tReadRatio = 0;
    }
    
    std::string print_failSignals() {
        std::string contents = "";
        std::set<eFailSignalType>::iterator it = _p._failSignals.begin();
        std::set<eFailSignalType>::iterator end = _p._failSignals.end();
        for ( ; it != end; ++it) {
            contents += boost::lexical_cast<std::string>(*it) + ",";
        }
        
        return contents;
    }
public:
    // N.B. would like to generalize this method for generalized balancing application
    // even in context of different eEntryStyleType::kTransitionSignal != _p._entryStyle
    
    // performs on the most recently close bar, starting with _p._tBar
    bool identifyFulcrum(const TBars& bars, std::string& reason) {
        bool isInsert;
        tw::common_str_util::FastStream<4096> s;
        s.setPrecision(2);
        
        if (bars.size() < 2)
            return false;
        
    // N.B. we do NOT want to wait for bar to close to identify balancing
        const TBar& closedbar = bars[bars.size() - 2];
        const TBar& bar = bars[bars.size() - 1];
        uint32_t currentindex = bar._index;
        
    // ==> check timeout conditions, which require waiting for a new tBar and a new evaluation therefrom
        if ( closedbar._index > _p._tBarIndex + _p._tFulcrumBarsToPersist ) {
            _p._tBarIndexfTimedOut = _p._tBarIndex;
            isInsert = _p._failSignals.insert(eFailSignalType::kFulcrumBarsToPersistViolation).second;
            s << "isInsert=" << boost::lexical_cast<std::string>(isInsert) << " tFulcrumBarsToPersist violated with closedbar._index=" << closedbar._index << " and _p._tBarIndex=" << _p._tBarIndex;
            reason = s.str();
            clear();
            return false;
        }
        
        // N.B. used of "timed out" here actually refers to an unallowable displacement with respect to _p._tBar
        if ( (closedbar._close > _p._tHigh) || (closedbar._close < _p._tLow) ) {
            _p._tBarIndexfTimedOut = _p._tBarIndex;
            isInsert = _p._failSignals.insert(eFailSignalType::kFulcrumBarCloseViolation).second;
            s << "isInsert=" << boost::lexical_cast<std::string>(isInsert) << " closedbar._close=" << closedbar._close << " outside of _p._tBar range";
            reason = s.str();
            clear();
            return false;
        }
     
        // also to consider: is fulcrum allowed to be outside of _p._tBar range
        // as it stands, we consider levels covered by bars starting with the _p._tBar
        // by construction the bars which occur after _p._tBar might cover other levels        
   
    // ==> 1. identify the fulcrum level
        // initialize values for loop over bars start with _p._tBar
        _p._fLevel = tw::price::Ticks(0);
        tw::price::Size maxVolAtLevel = tw::price::Size(0);
        tw::price::Size volAtLevel = tw::price::Size(0);
        
        _p._fMap.clear();
        uint32_t index = _p._tBarIndex;
        while (index <= currentindex) {
            // locate bar for index
            size_t  i = 0;
            for ( i = 0; i <= bars.size() - 2; ++i ) {
                if (bars[i]._index == index)
                    break;
            }
            
            // now we have located i, to obtain bar for this iteration
            for (std::tr1::unordered_map<price::Ticks::type, tw::common_trade::PriceTradesInfo>::const_iterator it = bars[i]._trades.begin(); it != bars[i]._trades.end(); ++it ) {
                tw::common_trade::PriceTradesInfo pti = it->second;
                tw::price::Ticks price = it->second._price;
                tw::price::Size volBid = it->second._volBid;
                tw::price::Size volAsk = it->second._volAsk;
                volAtLevel = volBid + volAsk;
                
                if (_p._fMap.find(it->first) == _p._fMap.end())
                    _p._fMap.insert(std::make_pair(it->first, volAtLevel));
                else
                    _p._fMap[it->first] += volAtLevel;
            }
            
            for (std::tr1::unordered_map<tw::price::Ticks::type, tw::price::Size>::const_iterator mi=_p._fMap.begin(); mi != _p._fMap.end(); ++mi) {
                volAtLevel = mi->second;
                if ( (volAtLevel > maxVolAtLevel) && (_p._tLow <= mi->first) && (_p._tHigh >= mi->first) ) {
                    maxVolAtLevel = volAtLevel;
                    _p._fLevel.set(mi->first);
                }
            }
            index++;
        }
        
        if (maxVolAtLevel < _p._tFulcrumMinVol) {
            isInsert = _p._failSignals.insert(eFailSignalType::kFulcrumMinVolViolation).second;
            s << "isInsert=" << boost::lexical_cast<std::string>(isInsert) << " maxVolAtLevel = " << maxVolAtLevel << " < " << _p._tFulcrumMinVol;
            reason = s.str();
            return false;
        }
        
    // ==> 2. identify the wings of the fulcrum
        // now _p._fLevel has been determined
        _p._fHigh = _p._fLevel + tw::price::Ticks(1);
        _p._fLow = _p._fLevel - tw::price::Ticks(1);
        
        _p._fLevelVol = tw::price::Size(0);
        _p._fHighVol = tw::price::Size(0);
        _p._fLowVol = tw::price::Size(0);
        _p._fExtendHighVol = tw::price::Size(0);
        _p._fExtendLowVol = tw::price::Size(0);
        
        volAtLevel = tw::price::Size(0);
        index = _p._tBarIndex;
        while (index <= currentindex) {
            // locate bar for index
            size_t i;
            for ( i = 0; i <= bars.size() - 2; ++i ) {
                if (bars[i]._index == index)
                    break;
            }
            
            for (std::tr1::unordered_map<price::Ticks::type, tw::common_trade::PriceTradesInfo>::const_iterator it = bars[i]._trades.begin(); it != bars[i]._trades.end(); ++it ) {
                tw::price::Ticks price = it->second._price;
                tw::price::Size volBid = it->second._volBid;
                tw::price::Size volAsk = it->second._volAsk;

                volAtLevel = volBid + volAsk;
                
                if (_p._fLevel == price)
                    _p._fLevelVol += volAtLevel;
                
                if (_p._fHigh == price) {
                    _p._fHighVol += volAtLevel;
                }

                if (_p._fLow == price) {
                    _p._fLowVol += volAtLevel;
                }
            }
            
            index++;
        }
        
    // ==> 3. consider rejecting the fulcrum wings
        if (tw::price::Size(0) == _p._fLevelVol || tw::price::Size(0) == _p._fHighVol || tw::price::Size(0) == _p._fLowVol) {
            isInsert = _p._failSignals.insert(eFailSignalType::kFulcrumZeroVolViolation).second;
            s << "isInsert=" << boost::lexical_cast<std::string>(isInsert) << " 0 == _p._fLevelVol || _p._fHighVol || 0 == _p._fLowVol";
            reason = s.str();
            return false;
        }
        
        if (_p._fHighVol >= _p._fLowVol) {
            _p._fRatio = boost::lexical_cast<double>(_p._fHighVol) / boost::lexical_cast<double>(_p._fLowVol);
        } else {
            _p._fRatio = boost::lexical_cast<double>(_p._fLowVol) / boost::lexical_cast<double>(_p._fHighVol);
        }
        
        double lowratio = boost::lexical_cast<double>(_p._fLowVol) / boost::lexical_cast<double>(_p._fLevelVol);
        double highratio = boost::lexical_cast<double>(_p._fHighVol) / boost::lexical_cast<double>(_p._fLevelVol);
        
        if ( (_p._tFulcrumLower > lowratio) && (_p._tFulcrumLower > highratio) ) {
            _p._fHigh = _p._fLevel;
            _p._fLow = _p._fLevel;
            _p._fRatio = 1;
            s << "degenerate case: small volume above and below, _p._fHigh and _p._fLow collapsed to _p._fLevel";
            reason = s.str();
            
            _p._fLocated = true;
            return true;
        } else {
            if ( (_p._tFulcrumLower > lowratio ) || (_p._tFulcrumHigher < lowratio ) ) {
                // _p._fLowVol does not have desired volume characteristics
                isInsert = _p._failSignals.insert(eFailSignalType::kFulcrumLowVolViolation).second;
                s << "isInsert=" << boost::lexical_cast<std::string>(isInsert) << " _p._fLowVol fails: lowratio=" << lowratio;
                reason = s.str();
                return false;
            }

            if ( (_p._tFulcrumLower > highratio ) || (_p._tFulcrumHigher < highratio ) ) {
                // _p._fHigh does not have desired volume characteristics
                isInsert = _p._failSignals.insert(eFailSignalType::kFulcrumHighVolViolation).second;
                s << "isInsert=" << boost::lexical_cast<std::string>(isInsert) << " _p._fHighVol fails: highratio=" << highratio;
                reason = s.str();
                return false;
            }

            if (_p._fRatio > _p._tFulcrumRatio) {
                // _p._fHigh and _p._fLow are not close enough in traded volume
                isInsert = _p._failSignals.insert(eFailSignalType::kFulcrumRatioViolation).second;
                s << "isInsert=" << boost::lexical_cast<std::string>(isInsert) << " ratio fails: _p._fRatio=" << _p._fRatio << " > _p._tFulcrumRatio=" << _p._tFulcrumRatio;
                reason = s.str();
                return false;
            }

            _p._fLocated = true;
            return true;
        }
    }
    
    bool extendWings(const TBars& bars, std::string& reason) {
        // continue to populate _p._fMap
        // we are evaluating ONLY prices which occur within the range of the _p._tBar: [_p._tBar._low, _p._tBar._high]
        tw::common_str_util::FastStream<4096> s;
        s.setPrecision(2);
        
        if (bars.size() < 2)
            return false;
        
        const TBar& bar = bars[bars.size() - 1];
        uint32_t currentindex = bar._index;
        
        tw::price::Size volAtLevel;
        tw::price::Ticks tlevel;
        tw::price::Ticks price;
        
        _p._fMap.clear();
        uint32_t index = _p._tBarIndex;
        while (index <= currentindex) {
            // locate bar for index
            size_t  i = 0;
            for ( i = 0; i <= bars.size() - 1; ++i ) {
                if (bars[i]._index == index)
                    break;
            }
            
            // N.B. here we continue to update _p._fMap, even after _p._fLocated=1           
            volAtLevel = tw::price::Size(0);
            // now we have located i, to obtain bar for this iteration
            for (std::tr1::unordered_map<price::Ticks::type, tw::common_trade::PriceTradesInfo>::const_iterator it = bars[i]._trades.begin(); it != bars[i]._trades.end(); ++it ) {
                tw::common_trade::PriceTradesInfo pti = it->second;
                tw::price::Ticks price = it->second._price;
                tw::price::Size volBid = it->second._volBid;
                tw::price::Size volAsk = it->second._volAsk;
                volAtLevel = volBid + volAsk;
                
                if (_p._fMap.find(it->first) == _p._fMap.end())
                    _p._fMap.insert(std::make_pair(it->first, volAtLevel));
                else
                    _p._fMap[it->first] += volAtLevel;
            }
            
            index++;
        }
        
        // now _p._fMap has been completed, use that information to possibly extend the wings
        //
        
        // start from _p._fLow
        tlevel = _p._fLow - tw::price::Ticks(1);
        while (tlevel >= _p._tLow) {
            volAtLevel = tw::price::Size(0);
            for (std::tr1::unordered_map<tw::price::Ticks::type, tw::price::Size>::const_iterator mi=_p._fMap.begin(); mi != _p._fMap.end(); ++mi) {
                // need to increment over prices contained in _p._tBar
                price.set(mi->first);
                if (price == tlevel) {
                    volAtLevel = mi->second;
                }
            }
            
            // breakout down
            if (volAtLevel < _p._tBreakoutMinVol) {
                // no significant trading has occurred at level yet
            } else {
                // significant trading has occurred at this level since start of _p._tBar
                _p._bLocated = true;

                // reset the low value lower...
                _p._fLow = tlevel;
                _p._fExtendLowVol = volAtLevel;

                // N.B. have to start the read logic tally again with respect to newly defined low wing
                _p._tReadTradedBelow = tw::price::Size(0);
                reason = "new _p._fLow=" + _p._fLow.toString() + " ";
            }    

            tlevel -= tw::price::Ticks(1);
        }

        // start from _p._fHigh
        tlevel = _p._fHigh + tw::price::Ticks(1);
        while (tlevel <= _p._tHigh) {
            volAtLevel = tw::price::Size(0);
            for (std::tr1::unordered_map<tw::price::Ticks::type, tw::price::Size>::const_iterator mi=_p._fMap.begin(); mi != _p._fMap.end(); ++mi) {
                // need to increment over prices contained in _p._tBar
                price.set(mi->first);
                if (price == tlevel) {
                    volAtLevel = mi->second;
                }
            }
            
            // breakout up
            if (volAtLevel < _p._tBreakoutMinVol) {
                // no significant trading has occurred at level yet
            } else {
                // significant trading has occurred at this level since start of _p._tBar
                _p._bLocated = true;

                // reset the high value higher...
                _p._fHigh = tlevel;
                _p._fExtendHighVol = volAtLevel;

                // N.B. have to start the read logic tally again with respect to newly defined high wing
                _p._tReadTradedAbove = tw::price::Size(0);
                reason = "new _p._fHigh=" + _p._fHigh.toString() + " ";
            }

            tlevel += tw::price::Ticks(1);
        }
            
        return true;
    }
    
    bool isReadCompleted(const TBars& bars, const tw::price::Quote& quote, std::string& reason, std::string& flag, tw::price::Ticks highprice, tw::price::Ticks lowprice) {
        bool isInsert;
        bool result = false;
        tw::common_str_util::FastStream<4096> s;
        s.setPrecision(2);
        std::string fulcrum_memo = "";
        if (highprice > lowprice)
            fulcrum_memo = " fulcrum read";
        
        const TBar& bar = bars[bars.size()-1];
        if ( quote._trade._price.isValid() && quote.isNormalTrade() ) {
            if ( (quote._trade._price >= highprice + _p._tReadMaxTicks) ) {
                // should not allow BUY SIDE to take off this tBar                
                isInsert = _p._failSignals.insert(eFailSignalType::kReadAboveViolation).second;
                s << "isInsert=" << isInsert << fulcrum_memo << " for kReadAboveViolation; " + print_failSignals() + "; read above violation: quote._trade._price=" << quote._trade._price.toString() << " >= " << highprice.toString() << " + " << _p._tReadMaxTicks << "\n";
                reason = s.str();
                _p._tBuyAllowed = false;
                return false;
            } else if ( (quote._trade._price <= lowprice - _p._tReadMaxTicks) ) {
                // should not allow SELL SIDE to take off this tBar
                isInsert = _p._failSignals.insert(eFailSignalType::kReadBelowViolation).second;
                s << "isInsert=" << isInsert << fulcrum_memo << " for kReadBelowViolation; " + print_failSignals() + "; read below violation: quote._trade._price=" << quote._trade._price.toString() << " <= " << lowprice.toString() << " - " << _p._tReadMaxTicks << "\n";
                reason = s.str();
                _p._tSellAllowed = false;
                return false;
            }
            
            std::string incrementnotation = "";
            if ( (quote._trade._price > highprice) && (quote._trade._price < highprice + _p._tReadMaxTicks) ) {
                if ( (tw::price::Trade::kAggressorSideBuy == quote._trade._aggressorSide) || ( (tw::price::Trade::kAggressorSideUnknown == quote._trade._aggressorSide) && (quote._trade._price == quote._book[0]._ask._price) ) ) {
                    _p._tReadTradedAbove += quote._trade._size;
                    incrementnotation = "added above " + quote._trade._size.toString();
                } else {
                    incrementnotation = "no add above";
                }
                
                if (_p._tReadTradedBelow > 0)
                    _p._tReadRatio = _p._tReadTradedAbove / _p._tReadTradedBelow;
                else
                    _p._tReadRatio = _p._tReadTradedAbove;
                
                if ( (_p._tReadTradedAbove > _p._tReadMinTraded) && (_p._tReadRatio > _p._tReadMinRatio) ) {
                    s << "read above SATISFIED: " << fulcrum_memo << " increment: " << incrementnotation << ", _p._tReadTradedAbove=" << _p._tReadTradedAbove << ", quote._trade._price=" << quote._trade._price.toString() << ", quote._book[0]._ask._price=" << quote._book[0]._ask._price.toString() << ", quote._trade._size=" << quote._trade._size << ", quote._trade._aggressorSide=" << quote._trade._aggressorSide << " and bar=" << bar.toString();
                    reason = s.str();
                    
                    // N.B. set stop price behind highprice 
                    _p_runtime._tStopPrice = lowprice - _p._tStopTicks;
                    _p_runtime._tLimitPrice = highprice + _p._tReadMaxTicks;
                    result = true;
                } else {
                    isInsert = _p._failSignals.insert(eFailSignalType::kReadAboveNotSatisfied).second;
                    s << "isInsert=" << isInsert << fulcrum_memo << " for kReadAboveNotSatisfied; " + print_failSignals() + "; read above not satisfied: increment: " << incrementnotation << ", _p._tReadTradedAbove=" << _p._tReadTradedAbove << ", quote._trade._price=" << quote._trade._price.toString() << ", quote._book[0]._ask._price=" << quote._book[0]._ask._price.toString() << ", quote._trade._size=" << quote._trade._size << ", quote._trade._aggressorSide=" << quote._trade._aggressorSide << " and bar=" << bar.toString();
                    reason = s.str();
                    return false;
                }
            } else if ( (quote._trade._price < lowprice) && (quote._trade._price > lowprice - _p._tReadMaxTicks) ) {
                if ( (tw::price::Trade::kAggressorSideSell == quote._trade._aggressorSide) || ( (tw::price::Trade::kAggressorSideUnknown == quote._trade._aggressorSide) && (quote._trade._price == quote._book[0]._bid._price) ) ) {
                    _p._tReadTradedBelow += quote._trade._size;
                    incrementnotation = "added below " + quote._trade._size.toString();
                } else {
                    incrementnotation = "no add below";
                }
                
                if (_p._tReadTradedAbove > 0)
                    _p._tReadRatio = _p._tReadTradedBelow / _p._tReadTradedAbove;
                else
                    _p._tReadRatio = _p._tReadTradedBelow;
                
                if ( (_p._tReadTradedBelow > _p._tReadMinTraded) && (_p._tReadRatio > _p._tReadMinRatio) ) {
                    s << "read below SATISFIED: " << fulcrum_memo << " increment: " << incrementnotation << ", _p._tReadTradedBelow=" << _p._tReadTradedBelow << ", quote._trade._price=" << quote._trade._price.toString() << ", quote._book[0]._bid._price=" << quote._book[0]._bid._price << ", quote._trade._size=" << quote._trade._size << ", quote._trade._aggressorSide=" << quote._trade._aggressorSide << " and bar=" << bar.toString();
                    reason = s.str();
                    
                    // N.B. set stop price behind highprice 
                    _p_runtime._tStopPrice = highprice + _p._tStopTicks;
                    _p_runtime._tLimitPrice = lowprice - _p._tReadMaxTicks;
                    result = true;
                } else {
                    isInsert = _p._failSignals.insert(eFailSignalType::kReadBelowNotSatisfied).second;
                    s << "isInsert=" << isInsert << fulcrum_memo << " for kReadBelowNotSatisfied; " + print_failSignals() + "; read below not satisfied: increment: " << incrementnotation << ", _p._tReadTradedBelow=" << _p._tReadTradedBelow << ", quote._trade._price=" << quote._trade._price.toString() << ", quote._book[0]._bid._price=" << quote._book[0]._bid._price << ", quote._trade._size=" << quote._trade._size << ", quote._trade._aggressorSide=" << quote._trade._aggressorSide << " and bar=" << bar.toString(); 
                    reason = s.str();
                    return false;
                }
            }
        }
        
        return result;
    }
    
    bool isInitialCramCompleted(const tw::channel_or::eOrderSide side, const tw::price::Ticks level, const tw::price::Quote& quote, std::string& reason) {
        bool isInsert;
        switch (side) {
            case tw::channel_or::eOrderSide::kBuy:
                // step 1.
                if ( quote._book[0]._bid._price < level + _p._tCramStopEvalTicks ) {
                    isInsert = _p._failSignals.insert(eFailSignalType::kUpTBarStopViolation).second;
                    reason = "isInsert=" + boost::lexical_cast<std::string>(isInsert) + " for kUpTBarStopViolation; " + print_failSignals() + "; quote._book[0]._bid._price < " + level.toString() + " +_p._tCramStopEvalTicks";
                    return false;
                }
                
                // step 2.
                if ( quote._book[0]._bid._price > level + _p._tCramMaxTicks ) {
                    isInsert = _p._failSignals.insert(eFailSignalType::kUpTBarMaxViolation).second;
                    clear();
                    reason = "isInsert=" + boost::lexical_cast<std::string>(isInsert) + " for kUpTBarMaxViolation; " + print_failSignals() + "; quote._book[0]._bid._price > " + level.toString() + " +_p._tCramMaxTicks";
                    return false;
                }
                
                // step 3.
                if ( !_p._top._bid._price.isValid() || _p._top._bid._price != quote._book[0]._bid._price ) {
                    // investigating top object:
                    _p._top.clear();
                    _p._top._bid._price = quote._book[0]._bid._price;
                    _p._top._ask._price = quote._book[0]._ask._price;
                }
                
                if ( _p._tCramFirmPriceQty.get() > 0 ) {
                    // step 4a, 4b.
                    if ( _p._tCramFirmPriceQty > quote._book[0]._bid._size ) {
                        // do not return here because need to next check cram ratio
                        reason = "tCramFirmPriceQty > quote._book[0]._bid._size=" + quote._book[0]._bid._size.toString();
                    } else if ( quote._book[0]._bid._price > level + _p._tCramStopEvalTicks ) {
                        _p._tCramCompleted = true;
                        reason = "FIRM SATISFIED && quote._book[0]._bid._price > " + level.toString() + " + _p._tCramStopEvalTicks";
                    }
                }
                
                // old way to use quote.isNormalTrade()
                if ( quote._trade._price.isValid() ) {
                    if ( quote._trade._price > _p._top._bid._price ) {
                        _p._top._ask._size += quote._trade._size;
                    } else if ( quote._trade._price == _p._top._bid._price ) {
                        _p._top._bid._size += quote._trade._size;
                    } else {
                        // step 4c.
                        isInsert = _p._failSignals.insert(eFailSignalType::kUpTBarLowPrice).second;
                        _p._top.clear();
                        reason += "isInsert=" + boost::lexical_cast<std::string>(isInsert) + " for kUpTBarLowPrice; " + print_failSignals() + "; AND quote._trade._price=" + quote._trade._price.toString() + " < _p._top._bid._price=" + _p._top._bid._price.toString();
                        return false;
                    }
                    
                    if ( !_p._tCramCompleted ) {
                        // step 5a.
                        if ( _p._top._ask._size < _p._tCramVolGood ) {
                            isInsert = _p._failSignals.insert(eFailSignalType::kUpTBarVolNotSatisfied).second;
                            reason += "isInsert=" + boost::lexical_cast<std::string>(isInsert) + " for kUpTBarVolNotSatisfied; " + print_failSignals() + "; AND _p._top._ask._size=" + _p._top._ask._size.toString() + " < _p._tCramVolGood=" + _p._tCramVolGood.toString();
                            return false;
                        }
                        
                        // step 5b.
                        double cramSlideRatio = (0 == _p._top._bid._size.get() ? _p._top._ask._size.toDouble() : (_p._top._ask._size.toDouble()/_p._top._bid._size.toDouble()));
                        if ( cramSlideRatio < _p._tCramRatio ) {
                            isInsert = _p._failSignals.insert(eFailSignalType::kUpTBarRatioNotSatisfied).second;
                            reason += "isInsert=" + boost::lexical_cast<std::string>(isInsert) + " for kUpTBarRatioNotSatisfied; " + print_failSignals() + "; AND cramSlideRatio < _p._tCramRatio";
                            return false;
                        }
                        
                        _p._tCramCompleted = true;
                        reason += " AND cramSlideRatio >= _p._tCramRatio";
                    }
                } else {
                    isInsert = _p._failSignals.insert(eFailSignalType::kUpTBarTradePriceInvalid).second;
                    reason += " isInsert=" + boost::lexical_cast<std::string>(isInsert) + " for kUpTBarTradePriceInvalid; " + print_failSignals() + "; AND quote._trade._price=" + quote._trade._price.toString() + " not valid for side=" + side.toString();
                    return false;
                }
                break;
            case tw::channel_or::eOrderSide::kSell:
                // step 1.
                if ( quote._book[0]._ask._price > level - _p._tCramStopEvalTicks ) {
                    isInsert = _p._failSignals.insert(eFailSignalType::kDownTBarStopViolation).second;
                    reason = " isInsert=" + boost::lexical_cast<std::string>(isInsert) + " for kDownTBarStopViolation; " + print_failSignals() + "; AND quote._book[0]._ask._price > " + level.toString() + " - _p._tCramStopEvalTicks";
                    return false;
                }
                
                // step 2.
                if ( quote._book[0]._ask._price < level - _p._tCramMaxTicks ) {
                    // be VERY careful with use of clear method:
                    isInsert = _p._failSignals.insert(eFailSignalType::kDownTBarMaxViolation).second;
                    clear();
                    reason = "isInsert=" + boost::lexical_cast<std::string>(isInsert) + " for kDownTBarMaxViolation; " + print_failSignals() + "; AND quote._book[0]._ask._price < " + level.toString() + " - _p._tCramMaxTicks";
                    return false;
                }
                
                // step 3.
                if ( !_p._top._ask._price.isValid() || _p._top._ask._price != quote._book[0]._ask._price ) {
                    // investigating top object:
                    _p._top.clear();
                    _p._top._bid._price = quote._book[0]._bid._price;
                    _p._top._ask._price = quote._book[0]._ask._price;
                }
                
                if ( _p._tCramFirmPriceQty.get() > 0 ) {
                    // step 4a, 4b.
                    if ( _p._tCramFirmPriceQty > quote._book[0]._ask._size ) {
                        // do not return here, because need to next check cram ratio
                        reason = "tCramFirmPriceQty > quote._book[0]._ask._size=" + quote._book[0]._ask._size.toString();
                    } else if ( quote._book[0]._ask._price < level - _p._tCramStopEvalTicks ) {
                        _p._tCramCompleted = true;
                        reason = "FIRM SATISFIED && quote._book[0]._ask._price < " + level.toString() + " - _p._tCramStopEvalTicks";
                    }
                }
                
                // old way to use quote.isNormalTrade()
                if ( quote._trade._price.isValid() ) {
                    if ( quote._trade._price < _p._top._ask._price ) {
                        _p._top._bid._size += quote._trade._size;
                    } else if ( quote._trade._price == _p._top._ask._price ) {
                        _p._top._ask._size += quote._trade._size;
                    } else {
                        // step 4c.
                        isInsert = _p._failSignals.insert(eFailSignalType::kDownTBarHighPrice).second;
                        _p._top.clear();
                        reason += " isInsert=" + boost::lexical_cast<std::string>(isInsert) + " for kDownTBarHighPrice; " + print_failSignals() + "; AND quote._trade._price=" + quote._trade._price.toString() + " > _p._top._ask._price=" + _p._top._ask._price.toString();
                        return false;
                    }
                    
                    if ( !_p._tCramCompleted ) {
                        // step 5a.
                        if ( _p._top._bid._size < _p._tCramVolGood ) {
                            isInsert = _p._failSignals.insert(eFailSignalType::kDownTBarVolNotSatisfied).second;
                            reason += " isInsert=" + boost::lexical_cast<std::string>(isInsert) + " for kDownTBarVolNotSatisfied; " + print_failSignals() + "; AND _p._top._bid._size=" + _p._top._bid._size.toString() + " < _p._tCramVolGood=" + _p._tCramVolGood.toString();
                            return false;
                        }
                        
                        // step 5b.
                        double cramSlideRatio = (0 == _p._top._ask._size.get() ? _p._top._bid._size.toDouble() : (_p._top._bid._size.toDouble()/_p._top._ask._size.toDouble()));
                        if ( cramSlideRatio < _p._tCramRatio ) {
                            isInsert = _p._failSignals.insert(eFailSignalType::kDownTBarRatioNotSatisfied).second;
                            reason += " isInsert=" + boost::lexical_cast<std::string>(isInsert) + " for kDownTBarRatioNotSatisfied; " + print_failSignals() + "; AND cramSlideRatio < _p._tCramRatio";
                            return false;
                        }
                        
                        _p._tCramCompleted = true;
                        reason += " AND cramSlideRatio >= _p._tCramRatio";
                    }
                } else {
                    isInsert = _p._failSignals.insert(eFailSignalType::kDownTBarTradePriceInvalid).second;
                    reason += " isInsert=" + boost::lexical_cast<std::string>(isInsert) + " for kDownTBarTradePriceInvalid; " + print_failSignals() + "; AND quote._trade._price=" + quote._trade._price.toString() + " not valid for side=" + side.toString();
                    return false;
                }
                break;
            default:
                // do nothing:
                break;
        }
        
        return true;
    }
    
    // IMPORTANT: should consider blockOnSignal for at least one minute 
    // to prevent a second initiation on same transition bar
    bool isSignalTriggered(const TBars& bars, const TBarPatterns& swings, const TBarPatterns& trends, tw::channel_or::eOrderSide& side, const tw::price::Quote& quote, std::string& reason) {
        side = tw::channel_or::eOrderSide::kUnknown;
        reason.clear();
        
        tw::common_str_util::TFastStream s;
        s.setPrecision(2);
        
        bool isInsert;
    // ==> prelim. need 3 bars to proceed
        if ( bars.size() < 3 ) {
            s << "bars.size() < 3 -- bars.size()=" << bars.size();
            reason = s.str();
            return false;
        }
        
        const TBar& bar = bars[bars.size() - 2];
        uint32_t barIndex = bar._index;
        if (!bar._close.isValid()) {
            s << "bar not valid; exiting";
            reason = s.str();
            return false;
        }
        
        // take the current swing, trend:
        if (_p._tFilterTrades) {
            if (swings.empty()) {
                s << "no swings available; exiting";
                reason = s.str();
                return false;
            }

            if (trends.empty()) {
                s << "no trends available; exiting";
                reason = s.str();
                return false;
            }   
        }
        
        tw::price::Ticks disp = bar._close - bar._open;
        
    // ==> 1a. transition bar flaw
        if ( (!_p._tBarLocated) && (tw::price::Ticks(0) != disp) ) {
            s << "_p._tBarLocated = " << _p._tBarLocated << " and not a transition, bar=" << bar.toString();
            reason = s.str();
            return false;
        } else if (!_p._tBarLocated && (tw::price::Ticks(0) == disp) ) {
    // ==> 1b. verify _p._tBar of desired characteristics
            if ( (bar._range < _p._tMinRangeTicks) || (bar._range > _p._tMaxRangeTicks) ) {
                isInsert = _p._failSignals.insert(eFailSignalType::kTransBarInvalidRange).second;
                s << "isInsert=" << boost::lexical_cast<std::string>(isInsert) << " candidate transition bar has invalid range, bar=" << bar.toString();
                reason = s.str();
                return false;
            }
            
    // ==> 1c. verify ATR requirement on potential tBar
            _p._atr = bar._atr;
            if (_p._atr < _p._atr_required) {
                isInsert = _p._failSignals.insert(eFailSignalType::kTransBarInvalidAtr).second;
                s << "isInsert=" << boost::lexical_cast<std::string>(isInsert) << " _p._atr = " << _p._atr << " not sufficient for _p._tBar requirement; exiting";
                reason = s.str();
                return false;
            }
            
    // ==> check if this bar already eliminated by tTimeToSignal to serve as tBar
            if (_p._tBarIndexTimedOut == barIndex) {
                isInsert = _p._failSignals.insert(eFailSignalType::kTransBarInvalidIndex).second;
                s << "isInsert=" << boost::lexical_cast<std::string>(isInsert) << " this index already eliminated as possible tBar=" << barIndex;
                reason = s.str();
                return false;
            }
            
    // ==> check if this bar already eliminated by fulcrum logic to serve as tBar
            if (_p._tBarIndexfTimedOut == barIndex) {
                isInsert = _p._failSignals.insert(eFailSignalType::kTransBarInvalidFulcrumIndex).second;
                s << "isInsert=" << boost::lexical_cast<std::string>(isInsert) << " this index already eliminated by fulcrum logic as possible tBar=" << barIndex;
                reason = s.str();
                return false;
            }
            
            std::string swing_string = "";
            std::string trend_string = "";
            if (_p._tFilterTrades) {
                // N.B. do we take the last completed BarPatterns objects, or the current index?
                const BarPattern& swing = swings[swings.size() - 1];
                const BarPattern& trend = trends[trends.size() - 1];
             
                swing_string = swing.toString();
                trend_string = trend.toString();
                // set a default value each time we are evaluating a possible tBar
                _p._tFilterCase = 0;
                
                // CASE 1. TREND, SWING OPPOSITE ==> ONLY ALLOW DIRECTION OF TREND
                if ( tw::common_trade::ePatternDir::kUp == trend._dir && tw::common_trade::ePatternDir::kDown == swing._dir ) {
                    _p._tSellAllowed = false;
                    _p._tFilterCase = 1;
                } else if ( tw::common_trade::ePatternDir::kDown == trend._dir && tw::common_trade::ePatternDir::kUp == swing._dir ) {
                    _p._tBuyAllowed = false;
                    _p._tFilterCase = 1;
                }

                // CASE 2. TREND, SWING SAME WAY, tBar makes new high/low for swing ==> ALLOW EITHER DIRECTION
                // CASE 3. TREND, SWING SAME WAY, tBar does NOT make new high/low for swing ==> ONLY ALLOW DIRECTION OF TREND
                if ( tw::common_trade::ePatternDir::kUp == trend._dir && tw::common_trade::ePatternDir::kUp == swing._dir ) {
                    if (bar._high > swing._high) {
                        _p._tFilterCase = 2;
                    } else if (bar._low < swing._low) {
                        _p._tFilterCase = 2;
                    } else {
                        _p._tSellAllowed = false;
                        _p._tFilterCase = 3;
                    }
                } else if ( tw::common_trade::ePatternDir::kDown == trend._dir && tw::common_trade::ePatternDir::kDown == swing._dir ) {
                    if (bar._high > swing._high) {
                        _p._tFilterCase = 2;
                    } else if (bar._low < swing._low) {
                        _p._tFilterCase = 2;
                    } else {
                        _p._tBuyAllowed = false;
                        _p._tFilterCase = 3;
                    }
                }
            }
            
            // in case of quick, successive trading such as BUYOPEN->SELLOPEN
            if (barIndex != _p._tBarIndex) {
                _p._tTimestamp = tw::common::THighResTime::now();
            }
            
            _p._tDisp = bar._high - bar._low;
            _p._tBarLocated = true;
            _p._tBar = bar;
            _p._tBarIndex = barIndex;
            _p._tHigh = bar._high;
            _p._tLow = bar._low;
            _p._tCramCompleted = false;
            _p._top.clear();
            
    // ==> 1d. check if _p._tBar is a "T" bar (or upside-down "T" bar)
            _p._up_T_active = false;
            _p._down_T_active = false;
            if (_p._tHigh == _p._tBar._close)
                _p._up_T_active = true;
            if (_p._tLow == _p._tBar._close) 
                _p._down_T_active = true;
            
            // we exit here because if we identify the tBar on this quote, we want to process again on next quote, do not insert failSignal here
            switch (_p._tFilterCase) {
                case 0:
                   isInsert = _p._failSignals.insert(eFailSignalType::kTrendSwingCase0).second;
                   break;
                case 1:
                   isInsert = _p._failSignals.insert(eFailSignalType::kTrendSwingCase1).second;
                   break;
                case 2:
                   isInsert = _p._failSignals.insert(eFailSignalType::kTrendSwingCase2).second;
                   break;
                case 3:
                   isInsert = _p._failSignals.insert(eFailSignalType::kTrendSwingCase3).second;
                   break;
                default:
                   isInsert = _p._failSignals.insert(eFailSignalType::kTrendSwingCase0).second;
                   break;                 
            }
            
            if (_p._tFilterTrades)
                s << "isInsert=" << boost::lexical_cast<std::string>(isInsert) << " transition bar located, barLocated=" << _p._tBarLocated << ", barIndex=" << _p._tBarIndex << ", bar=" << _p._tBar.toString() << ", swing=" << swing_string << ", trend=" << trend_string;
            else
                s << "transition bar located, barLocated=" << _p._tBarLocated << ", barIndex=" << _p._tBarIndex << ", bar=" << _p._tBar.toString();
            reason = s.str();
            return false;
        } else if ( _p._tBarLocated && (tw::price::Ticks(0) == disp) ) {
    // ==> 2a. consider extending the length of original _p._tBar
            if ( (barIndex > _p._tBarIndex) && (bars[bars.size() - 3]._close == bars[bars.size() - 3]._open) ) {
                // N.B. do not overwrite _p._tBarIndex even if we do extend the range
                if (bar._high > _p._tHigh)
                    _p._tHigh = bar._high;
                if (bar._low < _p._tLow)
                    _p._tLow = bar._low;
            }

            // N.B. do not allow future signaling off new transition bar which arrives while considering to trade off previous tBar
            if ( barIndex > _p._tBarIndex)  {
               _p._tBarIndexTimedOut = barIndex;
            }
        }
        
    // ==> check tTimeToSignal
        _p._delta = tw::common::THighResTime::now() - _p._tTimestamp;
        if (_p._delta > _p._tTimeToSignal * 1000000) {
            _p._tBarIndexTimedOut = _p._tBarIndex;
            isInsert = _p._failSignals.insert(eFailSignalType::kTimeToSignalViolation).second;
            s << "isInsert=" << boost::lexical_cast<std::string>(isInsert) << " for kTimeToSignalViolation; " + print_failSignals() + "; delta microseconds=" << _p._delta << " since tBar" << "\n";
            reason = s.str();
            clear();
            return false;
        }
        
    // ==> 2b. time lapse condition
        if (barIndex > _p._tBarIndex + _p._tMaxBarsToPersist) {
            isInsert = _p._failSignals.insert(eFailSignalType::kMaxBarsToPersistViolation).second;
            s << "isInsert=" << boost::lexical_cast<std::string>(isInsert) << " for kMaxBarsToPersistViolation; " + print_failSignals() + "; too much time since tBar, barIndex >= " << _p._tBarIndex << " + " << _p._tMaxBarsToPersist << "\n";
            reason = s.str();
            clear();
            return false;
        }
        
        std::string fr;
        if ( !_p._fLocated && (_p._tFulcrumMinVol > 0) ) {
            if ( quote._trade._price.isValid() && quote.isNormalTrade() ) {
                identifyFulcrum(bars, fr);
            }
        }
        
        std::string er;
        if (_p._fLocated) {
            if ( quote._trade._price.isValid() && quote.isNormalTrade() ) {
                extendWings(bars, er);
            }
        }
        
    // ==> fulcrum read logic here (N.B. stop price is set inside read logic if read is satisfied)
        std::string f_read;
        std::string f_flag;
        if (_p._fLocated) {
            // balancing has been achieved, now look for a breakout of range = [_p._fLow, _p._fHigh]
            if ( quote._trade._price.isValid() && quote.isNormalTrade() ) {
                if (quote._trade._price > _p._fHigh) {
                    if ( _p._tBuyAllowed && !_p._down_T_active && isReadCompleted(bars, quote, f_read, f_flag, _p._fHigh, _p._fLow) )
                        side = tw::channel_or::eOrderSide::kBuy;
                } else if (quote._trade._price < _p._fLow) {
                    if ( _p._tSellAllowed && !_p._up_T_active && isReadCompleted(bars, quote, f_read, f_flag, _p._fHigh, _p._fLow) )
                        side = tw::channel_or::eOrderSide::kSell;
                } else {
                    f_read = "NO FULCRUM READ DUE TO TRADE PRICE";
                }
            } else {
                f_read = "NO FULCRUM READ DUE TO NO TRADE";
            }
        }
        f_read = er + fr + " " + f_read;
        
        // REST OF LOGIC ADDRESSES WHAT TO DO AFTER _p._tBar has been identified
        //
        
    // ==> 3. check if read logic is satisfied (N.B. stop price is set inside read logic if read is satisfied)
        std::string r_read;
        std::string r_flag;
        // N.B. do NOT do read logic wrt _p._tBar._close if balancing configuration is active
        if (0 == _p._tFulcrumMinVol) {
            if ( quote._trade._price.isValid() && quote.isNormalTrade() ) {
                if (quote._trade._price > _p._tBar._close) {
                    if ( _p._tBuyAllowed && !_p._down_T_active && isReadCompleted(bars, quote, r_read, r_flag, _p._tBar._close, _p._tBar._close) )
                        side = tw::channel_or::eOrderSide::kBuy;
                } else if (quote._trade._price < _p._tBar._close) {
                    if ( _p._tSellAllowed && !_p._up_T_active && isReadCompleted(bars, quote, r_read, r_flag, _p._tBar._close, _p._tBar._close) )
                        side = tw::channel_or::eOrderSide::kSell;
                } else {
                    r_read = "NO READ DUE TO TRADE PRICE";
                }    
            } else {
                r_read = "NO READ DUE TO NO TRADE";
            }

            if ("exit" == r_flag) {
                reason = r_read;
                clear();
                return false;
            }    
        }
        
        std::string r_buy;
        std::string r_sell;
        if ( _p._tAllowCloseTrades && (tw::channel_or::eOrderSide::kUnknown == side) ) {
            if (_p._tDisp <= _p._tBarLargeDispTicks) {
            // ==> 4a. next close condition (no intervening quotes satisfied)
                if (barIndex > _p._tBarIndex) {
                    if (bar._close > _p._tHigh) {
                        // go with up trend
                        if ( _p._tBuyAllowed && !_p._down_T_active ) {
                            side = tw::channel_or::eOrderSide::kBuy;
                            _p_runtime._tStopPrice = _p._tLow - _p._tStopTicks;
                            _p_runtime._tLimitPrice = _p._tHigh + _p._tReadMaxTicks;
                        }
                    } else if (bar._close < _p._tLow) {
                        // go with down trend
                        if ( _p._tSellAllowed && !_p._up_T_active ) {
                            side = tw::channel_or::eOrderSide::kSell;
                            _p_runtime._tStopPrice = _p._tHigh + _p._tStopTicks;
                            _p_runtime._tLimitPrice = _p._tLow - _p._tReadMaxTicks;
                        }
                    }    
                }
            } else {
            // ==> 4d. here a quote is too far outside the tBar range
                if (quote._book[0]._bid._price > _p._tHigh + _p._tMaxOutsideTicks) {
                    isInsert = _p._failSignals.insert(eFailSignalType::kLargeBarMaxHigh).second;
                    s <<  "isInsert=" << boost::lexical_cast<std::string>(isInsert) << " for kLargeBarMaxHigh; " + print_failSignals() + "; large _p._tDisp case: bid = " << quote._book[0]._bid._price.toString() << " > " << _p._tHigh << " + " << _p._tMaxOutsideTicks;
                    reason = s.str();
                    clear();
                    return false;
                } else  if (quote._book[0]._ask._price < _p._tLow - _p._tMaxOutsideTicks) {
                    isInsert = _p._failSignals.insert(eFailSignalType::kLargeBarMaxLow).second;
                    s <<  "isInsert=" << boost::lexical_cast<std::string>(isInsert) << " for kLargeBarMaxLow; " + print_failSignals() + "; large _p._tDisp case: ask = " << quote._book[0]._ask._price.toString() << " < " << _p._tLow << " - " << _p._tMaxOutsideTicks;
                    reason = s.str();
                    clear();
                    return false;
                }
                
            // ==> 4e. here wait for a clue bar to close inside the tBar range
                if (barIndex > _p._tBarIndex) {
                    if ( (bar._close >= _p._tLow) && (bar._close <= _p._tHigh) ) {
                        // go with up trend 
                        if (bar._close > bar._open) {
                            if ( _p._tBuyAllowed && !_p._down_T_active ) {
                                side = tw::channel_or::eOrderSide::kBuy;
                                _p_runtime._tStopPrice = _p._tLow - _p._tStopTicks;
                                _p_runtime._tLimitPrice = _p._tHigh + _p._tReadMaxTicks;
                            }
                        }
                        
                        // go with down trend
                        if (bar._close < bar._open) {
                            if ( _p._tSellAllowed && !_p._up_T_active ) {
                                side = tw::channel_or::eOrderSide::kSell;
                                _p_runtime._tStopPrice = _p._tHigh + _p._tStopTicks;
                                _p_runtime._tLimitPrice = _p._tLow - _p._tReadMaxTicks;
                            }
                        }
                    } else {
                        isInsert = _p._failSignals.insert(eFailSignalType::kLargeBarCloseOutsideViolation).second;
                        s <<  "isInsert=" << boost::lexical_cast<std::string>(isInsert) << " for kLargeBarCloseOutsideViolation; " + print_failSignals() + "; large _p._tDisp case: bar close out of tBar range, this bar=" << bar.toString();
                        reason = s.str();
                        clear();
                        return false;
                    }    
                }
            }
        }
        
        if (_p._tInitialCramMode) {
            if ( (_p._tDisp <= _p._tBarLargeDispTicks) && (tw::channel_or::eOrderSide::kUnknown == side) ) {
                // ==> 4b. cram condition for when we consider buying
                if (quote._book[0]._bid._price >= _p._tHigh ) {
                    r_sell = "";
                    if (_p._down_T_active)
                        r_buy = "";
                    else if ( _p._tBuyAllowed && _p._up_T_active && isInitialCramCompleted(tw::channel_or::eOrderSide::kBuy, _p._tHigh, quote, r_buy) ){
                        side = tw::channel_or::eOrderSide::kBuy;
                        _p_runtime._tStopPrice = _p._tLow - _p._tStopTicks;
                        _p_runtime._tLimitPrice = _p._tHigh + _p._tReadMaxTicks;
                    }
                // ==> 4c. cram condition for when we consider selling
                } else if (quote._book[0]._ask._price <= _p._tLow ) {
                    r_buy = "";
                    if (_p._up_T_active)
                        r_sell = "";
                    else if ( _p._tSellAllowed && _p._down_T_active && isInitialCramCompleted(tw::channel_or::eOrderSide::kSell, _p._tLow, quote, r_sell) ) {
                        side = tw::channel_or::eOrderSide::kSell;
                        _p_runtime._tStopPrice = _p._tHigh + _p._tStopTicks;
                        _p_runtime._tLimitPrice = _p._tLow - _p._tReadMaxTicks;
                    }
                } else {
                    r_buy = "NOT CHECKING";
                    r_sell = "INITIAL CRAM";
                }    
            }
        }
        
    // ==> 5. check if next close or cram was satisfied
        std::string side_notation;
        if ( tw::channel_or::eOrderSide::kUnknown == side )
            side_notation = "side not set";
        else if ( (_p._tBarIndexPrev == _p._tBarIndex) && (_p._tSidePrev == side) ) {
            _p._tSignalTwiceRejected = true;
            side_notation = "trying to trade same side again off same _p._tBar";
        }
        else
            side_notation = "side OK";
        
        if ( "side OK" != side_notation) {
            s << ",quote._trade._price=" << quote._trade._price
                << ",bid=" << quote._book[0]._bid._price
                << ",ask=" << quote._book[0]._ask._price
                << ",_p._delta=" << _p._delta
                << ",_p._tBarIndexTimedOut=" << _p._tBarIndexTimedOut
                << ",_p._tBarIndexfTimedOut=" << _p._tBarIndexfTimedOut
                << ",_p._tSignalTwiceRejected=" << _p._tSignalTwiceRejected
                << ",up_T_active=" << _p._up_T_active
                << ",down_T_active=" << _p._down_T_active
                << ",_p._tBuyAllowed=" << _p._tBuyAllowed
                << ",_p._tSellAllowed=" << _p._tSellAllowed
                << ",_p._tReadTradedAbove=" << _p._tReadTradedAbove
                << ",_p._tReadTradedBelow=" << _p._tReadTradedBelow
                << ",_p._tFilterCase=" << _p._tFilterCase    
                
                << ",_p._fLevel=" << _p._fLevel
                << ",_p._fHigh=" << _p._fHigh
                << ",_p._fLow=" << _p._fLow
                << ",_p._fLevelVol=" << _p._fLevelVol
                << ",_p._fHighVol=" << _p._fHighVol
                << ",_p._fLowVol=" << _p._fLowVol
                << ",_p._fExtendHighVol=" << _p._fExtendHighVol
                << ",_p._fExtendLowVol=" << _p._fExtendLowVol
                << ",_p._fRatio=" << _p._fRatio
                << ",_p._fLocated=" << _p._fLocated
                << ",_p._bLocated=" << _p._bLocated                

                << ",_p._top._bid._size=" << _p._top._bid._size
                << ",_p._top._bid._price=" << _p._top._bid._price
                << ",_p._top._ask._price=" << _p._top._ask._price
                << ",_p._top._ask._size=" << _p._top._ask._size
                << ",_p._tDisp=" << _p._tDisp
                << ",_p._tCramCompleted=" << _p._tCramCompleted
                << ",_p._tBarLocated=" << _p._tBarLocated
                << ",_p._tBarIndex=" << _p._tBarIndex
                << ",_p._tBarIndexPrev=" << _p._tBarIndexPrev
                << ",_p._tHigh=" << _p._tHigh
                << ",_p._tLow=" << _p._tLow
                << ",_p._tClose=" << _p._tBar._close
                << ",_p._tBar=" << _p._tBar.toString()
                << ",bar=" << bar.toString();      
            
            reason = r_read + " " + f_read + " " + r_buy + " " + r_sell + " " + side_notation + " " + s.str();
            return false;
        }
        
    // ==> 7. process signal
        s << "TransitionSignalProcessor::isSignalTriggered()==true"
          << ",disp=" << disp
          << ",side=" << side.toString()
          << ",_p._atr=" << _p._atr
          << ",quote._trade._price=" << quote._trade._price
          << ",bid=" << quote._book[0]._bid._price
          << ",ask=" << quote._book[0]._ask._price
          << ",_p._delta=" << _p._delta
          << ",_p._tBarIndexTimedOut=" << _p._tBarIndexTimedOut
          << ",_p._tBarIndexfTimedOut=" << _p._tBarIndexfTimedOut
          << ",up_T_active=" << _p._up_T_active
          << ",down_T_active=" << _p._down_T_active
          << ",_p._tBuyAllowed=" << _p._tBuyAllowed
          << ",_p._tSellAllowed=" << _p._tSellAllowed
          << ",_p._tReadTradedAbove=" << _p._tReadTradedAbove
          << ",_p._tReadTradedBelow=" << _p._tReadTradedBelow
          << ",_p._tFilterCase=" << _p._tFilterCase
                
          << ",_p._fLevel=" << _p._fLevel
          << ",_p._fHigh=" << _p._fHigh
          << ",_p._fLow=" << _p._fLow
          << ",_p._fLevelVol=" << _p._fLevelVol
          << ",_p._fHighVol=" << _p._fHighVol
          << ",_p._fLowVol=" << _p._fLowVol
          << ",_p._fExtendHighVol=" << _p._fExtendHighVol
          << ",_p._fExtendLowVol=" << _p._fExtendLowVol
          << ",_p._fRatio=" << _p._fRatio
          << ",_p._fLocated=" << _p._fLocated
          << ",_p._bLocated=" << _p._bLocated           
 
          << ",_p._top._bid._size=" << _p._top._bid._size
          << ",_p._top._bid._price=" << _p._top._bid._price
          << ",_p._top._ask._price=" << _p._top._ask._price
          << ",_p._top._ask._size=" << _p._top._ask._size
          << ",_p._tDisp=" << _p._tDisp
          << ",_p._tCramCompleted=" << _p._tCramCompleted
          << ",_p._tBarLocated=" << _p._tBarLocated
          << ",_p._tBarIndex=" << _p._tBarIndex
          << ",_p._tBarIndexPrev=" << _p._tBarIndexPrev
          << ",_p._tHigh=" << _p._tHigh
          << ",_p._tLow=" << _p._tLow
          << ",_p._tClose=" << _p._tBar._close
          << ",_p._tBar=" << _p._tBar.toString()
          << ",bar=" << bar.toString();
        
        reason = r_read + " " + f_read + " " + r_buy + " " + r_sell + " " + s.str();
        _p._tBarIndexPrev = _p._tBarIndex;
        _p._tSidePrev = side;
        clear();
        return true;
    }
    
private:
    TransitionSignalParamsWire& _p;
    TransitionSignalParamsRuntime _p_runtime;
};

} // common_trade
} // tw



