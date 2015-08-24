#pragma once

#include <tw/common/singleton.h>
#include <tw/common/timer_server.h>
#include <tw/price/quote_store.h>
#include <tw/common_trade/bars_storage.h>
#include <tw/common_trade/atr.h>
#include <tw/channel_or/uuid_factory.h>

#include <tw/generated/bars.h>

#include <map>

#include <tw/common_thread/utils.h>

namespace tw {
namespace common_trade {
    
typedef HLOCInfo THLOCInfo;
typedef Bar TBar;
typedef std::vector<Bar> TBars;
typedef std::vector<BarPattern> TBarPatterns;
typedef std::pair<uint32_t, TBarPatterns> TBarPatternsInfo;
typedef std::tr1::unordered_map<tw::price::Ticks::type, PriceTradesInfo> TTrades;
typedef std::map<tw::instr::Instrument::TKeyId, TTrades> TInstrumentTrades;

static bool isHLOCValid(const THLOCInfo& x) {
    return (x._high.isValid() && x._low.isValid() && x._open.isValid() && x._close.isValid() && x._range.isValid());
}

static double calculateAtr(const TBars& bars, uint32_t atrNumOfPeriods, bool updateLastBar) {
    double atrValue = 0.0;
    if ( 0 < atrNumOfPeriods ) {
        tw::common_trade::Atr atr;
        atr.setNumOfPeriods(atrNumOfPeriods);
        TBars::const_reverse_iterator riter = bars.rbegin();
        TBars::const_reverse_iterator rend = bars.rend();
        
        if ( !updateLastBar && riter != rend && !riter->_formed )
            ++riter;
        
        for ( uint32_t i = atrNumOfPeriods; riter != rend && i > 0; ++riter, --i ) {
            TBars::const_reverse_iterator riterPrev = riter + 1;
            
            if ( (*riter)._range.isValid() ) {
                if ( riterPrev == rend || !(*riterPrev)._range.isValid() ) {
                    atr.addRange((*riter)._range);
                } else {
                    tw::price::Ticks high = (*riter)._high > (*riterPrev)._close ? (*riter)._high : (*riterPrev)._close;
                    tw::price::Ticks low = (*riter)._low < (*riterPrev)._close ? (*riter)._low : (*riterPrev)._close;
                    atr.addRange(high-low);
                }
            }
        }
        
        atrValue = atr.getAtr();
        if ( updateLastBar ) {
            TBar& lastBar = const_cast<TBars&>(bars).back();
            lastBar._atrNumOfPeriods = atrNumOfPeriods;
            lastBar._atr = atrValue;
        }
    }
    
    return atrValue;
}

static void updateTrade(tw::price::Size& volBid, tw::price::Size& volAsk, const tw::price::Quote& quote) {
    switch ( quote._trade._aggressorSide ) {
        case tw::price::Trade::kAggressorSideBuy:
            volAsk += quote._trade._size;
            break;
        case tw::price::Trade::kAggressorSideSell:
            volBid += quote._trade._size;
            break;
        default:
            if ( quote._trade._price <= quote._book[0]._bid._price )
                volBid += quote._trade._size;
            else if ( quote._trade._price >= quote._book[0]._ask._price )
                volAsk += quote._trade._size;
            break;
    }
}

static void updateTradesMaps(TTrades& trades, const tw::price::Quote& quote) {
    TTrades::iterator iter = trades.find(quote._trade._price.get());
    if ( iter == trades.end() ) {
        iter = trades.insert(TTrades::value_type(quote._trade._price.get(), PriceTradesInfo())).first;
        iter->second._price = quote._trade._price;
    }

    PriceTradesInfo& x = iter->second;
    updateTrade(x._volBid, x._volAsk, quote);
}

static void updateTrades(Bar& bar, const tw::price::Quote& quote) {
    updateTrade(bar._volBid, bar._volAsk, quote);
    updateTradesMaps(bar._trades, quote);
}

static bool onQuote(const tw::price::Quote& quote, Bar& bar) {
    if ( tw::price::Quote::kSuccess != quote._status || !quote.isNormalTrade() )
        return false;
    
    if ( 1 == (++bar._numOfTrades) ) {
        bar._high = bar._low = bar._open = quote._trade._price;
    } else {
        if ( quote._trade._price > bar._high )
            bar._high = quote._trade._price;
        else if ( quote._trade._price < bar._low )
            bar._low = quote._trade._price;
    }

    bar._close = quote._trade._price;
    bar._volume += quote._trade._size;
    bar._range = bar._high - bar._low;

    if ( bar._close > bar._open )
        bar._dir = ePatternDir::kUp;
    else if ( bar._close < bar._open )
        bar._dir = ePatternDir::kDown;
    else
        bar._dir = ePatternDir::kUnknown;

    updateTrades(bar, quote);
    return true;
}
    
class BarPatterns {
public:
    typedef std::map<tw::instr::Instrument::TKeyId, TBarPatterns> TInstrumentPatterns;
    
public:
    BarPatterns(ePatternsType type) {
        _type = type;
        clear();
    }
    
    virtual ~BarPatterns() {
    }
    
    void clear() {
        _instrumentsPatterns.clear();
        _instrumentsPatternsDynamic.clear();
    }
    
public:
    virtual void processPatterns(const TBars& bars, bool isDynamic) = 0;
    
public:
    ePatternsType getType() const {
        return _type;
    }
    
    const TBarPatterns& getBarPatterns(const tw::instr::Instrument::TKeyId& x) const {
        TInstrumentPatterns::const_iterator iter = _instrumentsPatterns.find(x);
        if ( iter != _instrumentsPatterns.end() )
            return iter->second;
        
        return _barPatternsNull;
    }
    
    const TBarPatterns& getBarPatternsDynamic(const tw::instr::Instrument::TKeyId& x) const {
        TInstrumentPatterns::const_iterator iter = _instrumentsPatternsDynamic.find(x);
        if ( iter != _instrumentsPatternsDynamic.end() )
            return iter->second;
        
        return _barPatternsNull;
    }
    
protected:
    ePatternDir getPatternDir(const THLOCInfo& hloc) const {
        if ( hloc._open < hloc._close )
            return ePatternDir::kUp;
        
        if ( hloc._open > hloc._close )
            return ePatternDir::kDown;        
        
        return ePatternDir::kUnknown;
    }
    
    void addPattern(TBarPatterns& patterns, ePatternType type, const THLOCInfo& hloc, uint32_t index) {
        if ( !isHLOCValid(hloc) )
            return;
        
        BarPattern& pattern = (*(patterns.insert(patterns.end(), BarPattern())));
        pattern._isDirty = true;
        
        static_cast<HLOCInfo&>(pattern) = hloc;
        pattern._type = type;
        pattern._parentType = _type;
        
        pattern._dir = getPatternDir(hloc);
        pattern._firstBarIndex = pattern._lastBarIndex = index;
        pattern._highBarIndex = pattern._lowBarIndex = index;
        pattern._index = patterns.size();
    }
    
    void updatePattern(BarPattern& pattern, const THLOCInfo& hloc, uint32_t index) {
        if ( !isHLOCValid(hloc) )
            return;
        
        pattern._isDirty = true;
        pattern._volume += hloc._volume;
        pattern._numOfTrades += hloc._numOfTrades;

        if ( pattern._high < hloc._high ) {
            pattern._high = hloc._high;
            pattern._highBarIndex = index;
        } else if ( hloc._high == pattern._high && index < pattern._highBarIndex ) {
            pattern._highBarIndex = index;
        }  

        if ( pattern._low > hloc._low ) {
            pattern._low = hloc._low;
            pattern._lowBarIndex = index;
        } else if ( hloc._low == pattern._low && index < pattern._lowBarIndex ) {
            pattern._lowBarIndex = index;
        }
        
        if ( index > pattern._lastBarIndex ) {
            pattern._lastBarIndex = index;
            pattern._close = hloc._close;
        } else if ( index < pattern._firstBarIndex ) {
            pattern._firstBarIndex = index;
            pattern._open = hloc._open;
        }
        
        pattern._range = pattern._high - pattern._low;
    }
    
    bool isExtremeBroken(const THLOCInfo& v1, const THLOCInfo& v2) {
        return ((ePatternDir::kUp == v1._dir && v2._low < v1._low) || (ePatternDir::kDown == v1._dir && v2._high > v1._high));
    }
    
    TInstrumentPatterns::iterator getInstrumentPattern(uint32_t id, bool isDynamic) {
        TInstrumentPatterns::iterator iter;
        if ( isDynamic ) {
            iter = _instrumentsPatternsDynamic.find(id);
            if ( iter == _instrumentsPatternsDynamic.end() ) {
                iter = _instrumentsPatternsDynamic.insert(TInstrumentPatterns::value_type(id, TBarPatterns())).first;
                
            } else {
                TInstrumentPatterns::iterator iter2 = _instrumentsPatterns.find(id);
                if ( iter2 == _instrumentsPatterns.end() || iter2->second.empty() )
                    iter->second.clear();
                else
                    iter->second = iter2->second;
            }
        } else {
            iter = _instrumentsPatterns.find(id);
            if ( iter == _instrumentsPatterns.end() ) {
                iter = _instrumentsPatterns.insert(TInstrumentPatterns::value_type(id, TBarPatterns())).first;
            }
        }
        
        return iter;
    }
    
protected:
    ePatternsType _type;
    TBarPatterns _barPatternsNull;
    TInstrumentPatterns _instrumentsPatterns;
    TInstrumentPatterns _instrumentsPatternsDynamic;
};

class BarPatternsSimple : public BarPatterns {
public:
    BarPatternsSimple() : BarPatterns(ePatternsType::kSimple) {
    }
    
public:
    virtual void processPatterns(const TBars& bars, bool isDynamic) {
        const TBar& bar = bars.back();
        
        if ( bar._numOfTrades == 0 || !bar._close.isValid() ) {
            if ( bar._numOfTrades > 0 )
                LOGGER_ERRO << "bar._numOfTrades > 0 for: " << bar.toString() << "\n";
            return;
        }
        
        TInstrumentPatterns::iterator iter = getInstrumentPattern(bar._instrument->_keyId, isDynamic);
        TBarPatterns& patterns = iter->second;
        
        // Differentiate between different bar patterns
        //
        
        // No trades - hitch bar
        //
        if ( 0 == bar._numOfTrades ) {
            if ( patterns.empty() || ePatternType::kHitch != patterns.back()._type )
                addPattern(patterns, ePatternType::kTransitional, bar, bars.size());
            else 
                updatePattern(patterns.back(), bar, bars.size());
            
            return;
        }
          
        // close == open - transitional bar
        //
        if ( bar._close == bar._open ) {
            addPattern(patterns, ePatternType::kTransitional, bar, bars.size());
            return;
        }
        
        // Check if the last pattern is the swing
        //
        TBarPatterns::reverse_iterator riter = patterns.rbegin();
        TBarPatterns::reverse_iterator rend = patterns.rend();
        for ( ; riter != rend; ++riter) {
            BarPattern& pattern = *riter;
            if ( ePatternType::kSwing == pattern._type ) {
                const ePatternDir dir = getPatternDir(bar);
                if ( dir == pattern._dir ) {
                    if ( dir == ePatternDir::kUp ) {
                        if ( bar._high > pattern._high && bar._low > pattern._low ) {
                            if (  pattern._index == patterns.back()._index )
                                updatePattern(pattern, bar, bars.size());
                            else
                                addPattern(patterns, ePatternType::kSwing, bar, bars.size());
                        } else {
                            addPattern(patterns, ePatternType::kHitch, bar, bars.size());
                        }

                        return;
                    }

                    if ( bar._high < pattern._high && bar._low < pattern._low ) {
                        if ( pattern._index == patterns.back()._index )
                            updatePattern(pattern, bar, bars.size());
                        else
                            addPattern(patterns, ePatternType::kSwing, bar, bars.size());
                    } else {
                        addPattern(patterns, ePatternType::kHitch, bar, bars.size());
                    }

                    return;
                }

                const TBar& bar2 = bars[pattern._lastBarIndex-1];
                if ( dir == ePatternDir::kDown ) {
                    if ( bar._close < bar2._low )
                        addPattern(patterns, ePatternType::kSwing, bar, bars.size());
                    else
                        addPattern(patterns, ePatternType::kHitch, bar, bars.size());

                    return;
                }

                if ( bar._close > bar2._high )
                    addPattern(patterns, ePatternType::kSwing, bar, bars.size());
                else
                    addPattern(patterns, ePatternType::kHitch, bar, bars.size());

                return;
            }
        }
        
        addPattern(patterns, ePatternType::kSwing, bar, bars.size());
    }
};

class BarPatternsSwingLevel1 : public BarPatterns {
public:
    BarPatternsSwingLevel1(BarPatternsSimple& simple,
                           int32_t simpleHitchMaxBars) : BarPatterns(ePatternsType::kSwingLevel1),
                                                        _simple(simple),
                                                        _simpleHitchMaxBars(simpleHitchMaxBars) {
        
    }
    
public:
    virtual void processPatterns(const TBars& bars, bool isDynamic) {
        const TBar& bar = bars.back();
        
        if ( bar._numOfTrades == 0 || !bar._close.isValid() ) {
            if ( bar._numOfTrades > 0 )
                LOGGER_ERRO << "bar._numOfTrades > 0 for: " << bar.toString() << "\n";
            return;
        }

        TInstrumentPatterns::iterator iter = getInstrumentPattern(bar._instrument->_keyId, isDynamic);
        TBarPatterns& patterns = iter->second;
        
        const TBarPatterns& simplePatterns = isDynamic ? _simple.getBarPatternsDynamic(bar._instrument->_keyId) : _simple.getBarPatterns(bar._instrument->_keyId);
        if ( simplePatterns.empty() )
            return;
        
        const BarPattern& lastSimpleBarPattern = simplePatterns.back();
        switch ( lastSimpleBarPattern._type ) {
            case ePatternType::kSwing:
                processSwing(bars, simplePatterns, lastSimpleBarPattern, patterns, isDynamic);
                break;
            case ePatternType::kHitch:
            case ePatternType::kTransitional:
                processHitchOrTransitional(bars, simplePatterns, lastSimpleBarPattern, patterns);
                break;
            default:
                break;
        }
    }
    
protected:
    virtual void processSwing(const TBars& bars, const TBarPatterns& simplePatterns, const BarPattern& lastSimpleBarPattern, TBarPatterns& patterns, bool isDynamic) {
        uint32_t index = bars.size();
        {
            if ( patterns.empty() ) {
                // Go back and add all hitch/transitional bars if any
                //
                if ( 1 == simplePatterns.size() ) {
                    addPattern(patterns, ePatternType::kSwing, lastSimpleBarPattern, index);
                } else {
                    for ( uint32_t i = simplePatterns.size()-1; i > 0; --i ) {
                        const BarPattern& simpleBarPattern = simplePatterns[i-1];
                        if ( patterns.empty() ) {
                            addPattern(patterns, ePatternType::kUnknown, simpleBarPattern, simpleBarPattern._lastBarIndex);
                        } else {
                            updatePattern(patterns.back(), simpleBarPattern, simpleBarPattern._lastBarIndex); 
                        }
                    }
                    
                    if ( patterns.empty() ) {
                        addPattern(patterns, ePatternType::kSwing, lastSimpleBarPattern, lastSimpleBarPattern._lastBarIndex);
                    } else {
                        BarPattern& lastPattern = patterns.back();
                        
                        if ( (ePatternDir::kUp == lastSimpleBarPattern._dir && lastPattern._high > lastSimpleBarPattern._high) || (ePatternDir::kDown == lastSimpleBarPattern._dir && lastPattern._low < lastSimpleBarPattern._low) ) {
                            patterns.clear();
                        } else {
                            updatePattern(lastPattern, lastSimpleBarPattern, lastSimpleBarPattern._lastBarIndex);
                            lastPattern._dir = lastSimpleBarPattern._dir;
                            lastPattern._type = ePatternType::kSwing;
                        }
                    }
                }
                return;
            }
            
            // Process hitch swings if configured
            //
            BarPattern& lastPattern = patterns.back();
            if ( lastPattern._simpleHitchCount > 0 ) {
                // Add new simple swing
                //
                addPattern(patterns, ePatternType::kSwing, lastSimpleBarPattern, index);
                
                // Check if need to add any hitch/transitional previous simple patterns
                //
                BarPattern& currPattern = patterns.back();
                for ( uint32_t i = index-1; i > lastPattern._simpleSwingLastBarIndex; --i ) {
                    updatePattern(currPattern, bars[i-1], i);
                }
                
                return;
            }            
            
            if ( lastPattern._lastBarIndex != (index-1) ) {
                uint32_t lastBarIndex = lastPattern._lastBarIndex;
                // Add new simple swing
                //
                addPattern(patterns, ePatternType::kSwing, lastSimpleBarPattern, index);
                
                // Check if need to add any hitch/transitional previous simple patterns
                //
                BarPattern& currPattern = patterns.back();
                for ( uint32_t i = index-1; i > lastBarIndex+1; --i ) {
                    const TBar& bar = bars[i-1];
                    if ( (ePatternDir::kUp == currPattern._dir && bar._low > currPattern._low) || (ePatternDir::kDown == currPattern._dir && bar._high < currPattern._high) )
                        return;

                    updatePattern(currPattern, bar, i);
                }
                
                return;
            }

            if ( lastSimpleBarPattern._dir == lastPattern._dir ) {
                updatePattern(lastPattern, bars.back(), index);
                return;
            }
        }
        
        addPattern(patterns, ePatternType::kSwing, lastSimpleBarPattern, index);
        BarPattern& lastPattern = patterns[patterns.size()-2];
        
        uint32_t prevHighLowIndex = ( ePatternDir::kUp == lastPattern._dir ) ? lastPattern._highBarIndex : lastPattern._lowBarIndex;
        if ( prevHighLowIndex >= index || 1 > prevHighLowIndex )
            return;
        
        for ( uint32_t i = lastPattern._lastBarIndex; i >= prevHighLowIndex; --i ) {
            updatePattern(patterns.back(), bars[i-1], i);
        }
    }
    
    virtual void processHitchOrTransitional(const TBars& bars, const TBarPatterns& simplePatterns, const BarPattern& lastSimpleBarPattern, TBarPatterns& patterns) {
        if ( patterns.empty() )
            return;
        
        BarPattern& lastPattern = patterns.back();
        if ( (_simpleHitchMaxBars < 0) || (0 == lastPattern._simpleHitchCount) ) {
            if ( _simpleHitchMaxBars > -1 ) {
                lastPattern._simpleSwingLastBarIndex = lastPattern._lastBarIndex;
                ++lastPattern._simpleHitchCount;
            }
            
            // Check if need to skip hitch or transitional
            //
            if ( isExtremeBroken(lastPattern, lastSimpleBarPattern) )
                return;

            if ( lastSimpleBarPattern._lastBarIndex > (lastPattern._lastBarIndex+1) )
                return;
            
            updatePattern(lastPattern, lastSimpleBarPattern, bars.size());
            return;
        }
    }
    
protected:
    BarPatternsSimple& _simple;
    int32_t _simpleHitchMaxBars;
};

class BarPatternsSwingLevel2 : public BarPatternsSwingLevel1 {
    typedef BarPatternsSwingLevel1 TParent;
    
public:
    BarPatternsSwingLevel2(BarPatternsSimple& simple,
                           BarPatternsSwingLevel1& level1,
                           int32_t simpleHitchMaxBars) : TParent(simple, simpleHitchMaxBars),
                                                         _level1(level1) {
       _type = ePatternsType::kSwingLevel2; 
    }
    
protected:
    virtual void processSwing(const TBars& bars, const TBarPatterns& simplePatterns, const BarPattern& lastSimpleBarPattern, TBarPatterns& patterns, bool isDynamic) {
        doProcessSwing(bars, simplePatterns, lastSimpleBarPattern, patterns);
        
        if ( patterns.empty() )
            return;
        
        BarPattern& lastPattern = patterns.back();
        const TBarPatterns& level1 = isDynamic ? _level1.getBarPatternsDynamic(lastPattern._instrument->_keyId) : _level1.getBarPatterns(lastPattern._instrument->_keyId);
        
        if ( level1.empty() )
            return;
        
        if ( lastPattern._trendSwingIndexes.empty() && lastPattern._counterTrendSwingIndexes.empty() ) {
            for ( size_t i = 0; i < level1.size(); ++i ) {
                const BarPattern& currLevel1 = level1[i];
                if ( currLevel1._firstBarIndex >= lastPattern._firstBarIndex )
                    processTrendSwingIndex(currLevel1, lastPattern);
            }
            
            return;
        }
        
        processTrendSwingIndex(level1.back(), lastPattern);
    }
    
    void processTrendSwingIndex(const BarPattern& currLevel1, BarPattern& lastPattern) {
        if ( currLevel1._dir == lastPattern._dir ) {
            if ( lastPattern._trendSwingIndexes.empty() || lastPattern._trendSwingIndexes.back() != currLevel1._index )
                lastPattern._trendSwingIndexes.push_back(currLevel1._index);
        } else {
            if ( lastPattern._counterTrendSwingIndexes.empty() || lastPattern._counterTrendSwingIndexes.back() != currLevel1._index )
                lastPattern._counterTrendSwingIndexes.push_back(currLevel1._index);
        }
    }
        
    void doProcessSwing(const TBars& bars, const TBarPatterns& simplePatterns, const BarPattern& lastSimpleBarPattern, TBarPatterns& patterns) {
        uint32_t index = bars.size();
        if ( patterns.empty() ) {
            // Go back and add all hitch/transitional bars if any
            //
            if ( 1 == simplePatterns.size() ) {
                addPattern(patterns, ePatternType::kTrend, lastSimpleBarPattern, index);
            } else {
                for ( uint32_t i = simplePatterns.size()-1; i > 0; --i ) {
                    const BarPattern& simpleBarPattern = simplePatterns[i-1];
                    if ( patterns.empty() ) {
                        addPattern(patterns, ePatternType::kUnknown, simpleBarPattern, simpleBarPattern._lastBarIndex);
                    } else {
                        updatePattern(patterns.back(), simpleBarPattern, simpleBarPattern._lastBarIndex); 
                    }
                }

                if ( patterns.empty() ) {
                    addPattern(patterns, ePatternType::kTrend, lastSimpleBarPattern, lastSimpleBarPattern._lastBarIndex);
                } else {
                    BarPattern& lastPattern = patterns.back();

                    if ( (ePatternDir::kUp == lastSimpleBarPattern._dir && lastPattern._high > lastSimpleBarPattern._high) || (ePatternDir::kDown == lastSimpleBarPattern._dir && lastPattern._low < lastSimpleBarPattern._low) ) {
                        patterns.clear();
                    } else {
                        updatePattern(lastPattern, lastSimpleBarPattern, lastSimpleBarPattern._lastBarIndex);
                        lastPattern._dir = lastSimpleBarPattern._dir;
                        lastPattern._type = ePatternType::kTrend;
                    }
                }
            }
            return;
        }

        {
            BarPattern& lastPattern = patterns.back();
            if ( lastSimpleBarPattern._dir == lastPattern._dir ) {
                updatePattern(lastPattern, bars.back(), index);
                return;
            }

            if ( !isExtremeBroken(lastPattern, bars.back()) ) {
                updatePattern(lastPattern, bars.back(), bars.size());
                return;
            }
        }
        
        addPattern(patterns, ePatternType::kTrend, bars.back(), index);
        BarPattern& lastPattern = patterns[patterns.size()-2];
        
        uint32_t prevHighLowIndex = ( ePatternDir::kUp == lastPattern._dir ) ? lastPattern._highBarIndex : lastPattern._lowBarIndex;
        if ( prevHighLowIndex >= index || 1 > prevHighLowIndex )
            return;
        
        BarPattern& currPattern = patterns.back();
        for ( uint32_t i = lastPattern._lastBarIndex; i >= prevHighLowIndex; --i ) {
            const TBar& bar = bars[i-1];
            if ( !isExtremeBroken(lastPattern, bar) )
                updatePattern(currPattern, bar, i);
        }
    }
    
    virtual void processHitchOrTransitional(const TBars& bars, const TBarPatterns& simplePatterns, const BarPattern& lastSimpleBarPattern, TBarPatterns& patterns) {
        if ( patterns.empty() )
            return;
        
        // Check if need to skip hitch or transitional
        //
        BarPattern& lastPattern = patterns.back();
        if ( isExtremeBroken(lastPattern, lastSimpleBarPattern) )
            return;
        
        updatePattern(patterns.back(), lastSimpleBarPattern, bars.size());
    }
    
private:
    BarPatternsSwingLevel1& _level1;
};

template <typename TRouter>    
class BarsManagerImpl : public tw::common::TimerClient {
public:
    typedef std::map<tw::instr::Instrument::TKeyId, TBars> TInstrumentBars;
    typedef std::pair<uint32_t, TBars> TBarsInfo;
    typedef std::map<tw::instr::Instrument::TKeyId, TBarsInfo> TInstrumentBarsInfo;
    
public:
    BarsManagerImpl(uint32_t barLength=60, int32_t simpleHitchMaxBars=-1, uint32_t atrPeriods=0) : _barPatternsSwingLevel1(_barPatternsSimple, simpleHitchMaxBars),
                                                                                                   _barPatternsSwingLevel2(_barPatternsSimple, _barPatternsSwingLevel1, simpleHitchMaxBars) {
        clear();
        _barLength=barLength;
        _source.clear();
        _simpleHitchMaxBars=simpleHitchMaxBars;
        _atrNumOfPeriods = atrPeriods;
    }
    
    ~BarsManagerImpl() {
    }
    
    void clear() {
        _verbose = false;
        _instrumentsBars.clear();
        _instrumentsBarsInfo.clear();
        _instrumentTrades.clear();
        
        _barPatternsSimple.clear();
        _barPatternsSwingLevel1.clear();
        _barPatternsSwingLevel2.clear();
    }
    
public:
    void setVerbose(bool v) {
        _verbose = v;
    }
    
    void setSource(const std::string& v) {
        if ( _source.empty() ) {
            _source = v;
            _source += "_" + tw::channel_or::UuidFactory::instance().get().toString();
            TRouter::setDbSource(_source);
        }
    }
    
    bool subscribe(const tw::instr::Instrument::TKeyId& x, uint32_t lookbackMinutes = 0) {
        if ( _instrumentsBars.empty() )
            TRouter::registerTimerClient(this, calcCycleTimeout(), true, _timerId);
        
        TInstrumentBars::iterator iter = _instrumentsBars.find(x);
        if ( iter == _instrumentsBars.end() ) {
            tw::price::QuoteStore::instance().subscribeFront(x, this);
            iter = _instrumentsBars.insert(TInstrumentBars::value_type(x, TBars())).first;
            createNewBar(iter);
            
            _instrumentsBarsInfo[x].first = lookbackMinutes;
            return true;
        }
        
        if ( _instrumentsBarsInfo[x].first > lookbackMinutes && 0 != lookbackMinutes ) {
            LOGGER_WARN << "prevStoredLookbackBarsCount > lookbackBarsCount: " 
                        << "instrumentId=" << x
                        << ",prevStoredLookbackBarsCount=" << _instrumentsBarsInfo[x].first
                        << ",lookbackBarsCount=" << lookbackMinutes
                        << "\n";
            return true;
        }
        
        _instrumentsBarsInfo[x].first = lookbackMinutes;
        return true;
    }
    
    bool start(const tw::common::Settings& settings) {
        LOGGER_INFO << "Restoring bars..." << "\n";
        _settings = settings;
        
        if ( !restoreBars() ) 
            return false;
        
        TInstrumentBarsInfo::iterator iter = _instrumentsBarsInfo.begin();
        TInstrumentBarsInfo::iterator end = _instrumentsBarsInfo.end();
        
        TBar tempBar;
        for ( ; iter != end; ++iter ) {
            TInstrumentBars::iterator iter2 = _instrumentsBars.find(iter->first);
            if ( iter2 == _instrumentsBars.end() ) {
                LOGGER_ERRO << "Failed to find subscription for: " << iter->first << "\n";
                return false;
            }
            
            const TBars& historicalBars = iter->second.second;
            TBars& bars = iter2->second;
            
            for ( size_t i = 0; i < historicalBars.size(); ++i ) {
                tempBar = bars.back();
                bars.back() = historicalBars[i];
                bars.back()._index = tempBar._index;
                bars.back()._instrument = tempBar._instrument;
                
                processPatterns(bars, false);
                
                if ( i < historicalBars.size()-1 )
                    createNewBar(iter2);
            }
            
            outputToDbOnStart(bars);
            
            if ( !historicalBars.empty() )
                LOGGER_WARN << "Restored: " << historicalBars.size() << " bars for lookbackMinutes: " << iter->second.first << " for: " << tempBar._displayName << "\n";
        }
        
        LOGGER_INFO << "Restored bars" << "\n";
        return true;
    }
    
public:    
    void onQuote(const tw::price::Quote& quote) {
        TInstrumentBars::iterator iter = _instrumentsBars.find(quote._instrumentId);
        if ( iter == _instrumentsBars.end() || iter->second.empty() )
            return;
        
        if ( !::tw::common_trade::onQuote(quote, iter->second.back()) )
            return;
        
        updateTradesMaps(_instrumentTrades[quote._instrumentId], quote);
        processPatterns(iter->second, true);
    }
    
    // TimerClient interface
    //
    virtual bool onTimeout(const tw::common::TTimerId& id) {
        if ( _verbose )
            LOGGER_INFO << "Updating bars" << "\n";
        
        TInstrumentBars::iterator iter = _instrumentsBars.begin();
        TInstrumentBars::iterator end = _instrumentsBars.end();
        
        for ( ; iter != end; ++iter ) {
            if ( !iter->second.empty() ) {
                if ( !iter->second.back()._formed ) {
                    iter->second.back()._formed = true;
                    processPatterns(iter->second, false);
                    if ( _verbose )
                        LOGGER_INFO << iter->second.back().toString() << "\n";

                    outputToDb(iter->second);
                }
            }
            
            createNewBar(iter);
        }        
        
        TRouter::registerTimerClient(this, calcCycleTimeout(), true, _timerId);
        return false;
    }
    
    const TBars& getBars(const tw::instr::Instrument::TKeyId& x) const {
        TInstrumentBars::const_iterator iter = _instrumentsBars.find(x);
        if ( iter != _instrumentsBars.end() )
            return iter->second;
        
        return _barsNull;
    }
    
    double getAvgVolByPrice(const tw::instr::Instrument::TKeyId& x) const {
        return getAvgVolByPrice(getTrades(x));
    }
    
    double getAvgVolByPrice(const TTrades& trades) const {
        if ( trades.empty() )
            return 0.0;
        
        TTrades::const_iterator iter = trades.begin();
        TTrades::const_iterator end = trades.end();
        tw::price::Size size = tw::price::Size(0);
        for ( ; iter != end; ++iter ) {
            const PriceTradesInfo& info = iter->second;
            size += info._volBid + info._volAsk;
        }
        
        return (size.toDouble()/trades.size());
    }
    
    tw::price::Size getVolByPrice(const tw::instr::Instrument::TKeyId& x, const tw::price::Ticks& price) const {
        return getVolByPrice(price, getTrades(x));
    }
    
    tw::price::Size getVolByPrice(const tw::price::Ticks& price, const TTrades& trades) const {
        if ( trades.empty() )
            return tw::price::Size(0);
        
        TTrades::const_iterator iter = trades.find(price);
        if ( iter == trades.end() )
            return tw::price::Size(0);
        
        return iter->second._volBid + iter->second._volAsk;
    }
    
    const TTrades& getTrades(const tw::instr::Instrument::TKeyId& x) const {
        TInstrumentTrades::const_iterator iter = _instrumentTrades.find(x);
        if ( iter != _instrumentTrades.end() )
            return iter->second;
            
        return _tradesNull;
    }
    
    tw::price::Size getBackBarsCumVol(const tw::instr::Instrument::TKeyId& x, size_t count, size_t offsetFromCurrent) const {
        tw::price::Size y;
        
        TInstrumentBars::const_iterator iter = _instrumentsBars.find(x);
        if ( iter != _instrumentsBars.end() && offsetFromCurrent < iter->second.size() ) {
            TBars::const_reverse_iterator iterBars = iter->second.rbegin();
            count = std::min(count, iter->second.size()-offsetFromCurrent);
            if ( count > 0 ) {
                for ( size_t i = 0; i < offsetFromCurrent; ++i ) {
                    ++iterBars;
                }

                for ( size_t i = 0; i < count; ++i ) {
                    y += iterBars->_volume;
                    ++iterBars;
                }
            }
        }
        
        return y;
    }
    
    tw::price::Ticks getBackBarsHigh(const tw::instr::Instrument::TKeyId& x, size_t count) const {
        tw::price::Ticks y;
        
        TInstrumentBars::const_iterator iter = _instrumentsBars.find(x);
        if ( iter != _instrumentsBars.end() ) {
            TBars::const_reverse_iterator iterBars = iter->second.rbegin();
            count = std::min(count, iter->second.size());
            for ( size_t i = 0; i < count; ++i ) {
                if ( !y.isValid() )
                    y = iterBars->_high;
                else
                    y = tw::price::Ticks::max(y, iterBars->_high);
                ++iterBars;
            }
        }
        
        return y;
    }
    
    tw::price::Ticks getBackBarsLow(const tw::instr::Instrument::TKeyId& x, size_t count) const {
        tw::price::Ticks y;
        
        TInstrumentBars::const_iterator iter = _instrumentsBars.find(x);
        if ( iter != _instrumentsBars.end() ) {
            TBars::const_reverse_iterator iterBars = iter->second.rbegin();
            count = std::min(count, iter->second.size());
            for ( size_t i = 0; i < count; ++i ) {
                if ( !y.isValid() )
                    y = iterBars->_low;
                else
                    y = tw::price::Ticks::min(y, iterBars->_low);
                ++iterBars;
            }
        }
        
        return y;
    }
    
    tw::price::Ticks getBackBarsHighLowRangeAvg(const tw::instr::Instrument::TKeyId& x, size_t count, size_t offsetFromCurrent) const {
        tw::price::Ticks y(0);
        
        TInstrumentBars::const_iterator iter = _instrumentsBars.find(x);
        if ( iter != _instrumentsBars.end() && offsetFromCurrent < iter->second.size() ) {
            TBars::const_reverse_iterator iterBars = iter->second.rbegin();
            count = std::min(count, iter->second.size()-offsetFromCurrent);
            if ( count > 0 ) {
                tw::instr::InstrumentPtr instrument = iterBars->_instrument;
                for ( size_t i = 0; i < offsetFromCurrent; ++i ) {
                    ++iterBars;
                }
                
                for ( size_t i = 0; i < count; ++i, ++iterBars) {
                    if ( iterBars->_numOfTrades > 0 )
                        y += iterBars->_high - iterBars->_low;
                }
                
                if ( y.isValid() && instrument )
                    y = instrument->_tc->nearestTick(y.toDouble()/static_cast<double>(count));
            }
        }
        
        return y;
    }
    
    const BarPatternsSimple& getBarPatternsSimple() const {
        return _barPatternsSimple;
    }
    
    const BarPatternsSwingLevel1& getBarPatternsSwingLevel1() const {
        return _barPatternsSwingLevel1;
    }
    
    const BarPatternsSwingLevel2& getBarPatternsSwingLevel2() const {
        return _barPatternsSwingLevel2;
    }
    
public:
    uint32_t calcCycleTimeout() const {
        uint32_t cycleMSec = _barLength * 1000;
        
        uint32_t msecFromMidnight = TRouter::getMSecFromMidnight();
        uint32_t msecInCycle = msecFromMidnight % cycleMSec;
        uint32_t delta = cycleMSec-msecInCycle;

        return (0 == delta ? cycleMSec : delta);
    }
    
private:
    void createNewBar(const TInstrumentBars::iterator& iter) {            
        iter->second.push_back(Bar());
        iter->second.back()._open_timestamp.setToNow();
        iter->second.back()._instrument = tw::instr::InstrumentManager::instance().getByKeyId(iter->first);
        if ( iter->second.back()._instrument ) {
            iter->second.back()._displayName = iter->second.back()._instrument->_displayName;
            iter->second.back()._exchange = iter->second.back()._instrument->_exchange;
        }
        iter->second.back()._duration = _barLength;
        iter->second.back()._index = iter->second.size();
    }        
    
private:
    void processPatterns(const TBars& bars, bool isDynamic) {
        if ( bars.empty() || bars.back()._numOfTrades == 0 || !bars.back()._close.isValid() ) {
            if ( !bars.empty() && bars.back()._numOfTrades > 0 )
                LOGGER_ERRO << "!bars.empty() && bars.back()._numOfTrades > 0 for: " << bars.back().toString() << "\n";
            return;
        }
        
        calculateAtr(bars, _atrNumOfPeriods, true);
        if ( !TRouter::processPatterns() )
            return;
        
        _barPatternsSimple.processPatterns(bars, isDynamic);
        _barPatternsSwingLevel1.processPatterns(bars, isDynamic);
        _barPatternsSwingLevel2.processPatterns(bars, isDynamic);
    }
    
    void outputToDb(const TBars& bars) {
        if ( !TRouter::outputToDb() )
            return;
        
        if ( bars.empty() )
            return;
        
        const TBar& bar = bars.back();
        TRouter::persist(bar);
        
        if ( !TRouter::processPatterns() )
            return;
        
        outputToDb(_barPatternsSimple, bar._instrument->_keyId);
        outputToDb(_barPatternsSwingLevel1, bar._instrument->_keyId);
        outputToDb(_barPatternsSwingLevel2, bar._instrument->_keyId);
    }
    
    void outputToDbOnStart(const TBars& bars) {
        if ( !TRouter::outputToDb() )
            return;
        
        if ( bars.empty() )
            return;
        
        for ( size_t i = 0; i < bars.size(); ++i ) {
            const TBar& bar = bars[i];
            TRouter::persist(bar);
        }
        
        if ( !TRouter::processPatterns() )
            return;
        
        const tw::instr::Instrument::TKeyId& instrumentId = bars.back()._instrument->_keyId;
        // On start up, don't output simple patterns
        //
        outputToDb(_barPatternsSimple, instrumentId);
        outputToDb(_barPatternsSwingLevel1, instrumentId, true);
        outputToDb(_barPatternsSwingLevel2, instrumentId, true);
    }
    
    bool restoreBars() {
        bool status = true;
        switch ( _settings._bars_manager_bars_restore_mode ) {
            case tw::common::eBarsRestoreMode::kDb:
                status = readBarsFromDb();
                break;
            case tw::common::eBarsRestoreMode::kFile:
                status = readBarsFromFile();
                break;
            default:
                break;
        }
        
        return status;
    }
    
    bool readBarsFromDb() {
        TInstrumentBarsInfo::iterator iter = _instrumentsBarsInfo.begin();
        TInstrumentBarsInfo::iterator end = _instrumentsBarsInfo.end();
        
        for ( ; iter != end; ++iter ) {
            if ( iter->second.first > 0 ) {
                if ( !TRouter::readBars(iter->first, _barLength, iter->second.first, _atrNumOfPeriods, iter->second.second) ) {
                    LOGGER_ERRO << "Failed to read bars for instrumentId: " << iter->first << "\n";
                    return false;
                }
            }
        }
        
        return true;
    }
    
    void processHistoricalQuote(tw::price::QuoteStore::TQuote& quote, const tw::common::THighResTime& currHistoricalTimestamp) {
        bool needToGetNewBar = false;
        
        TInstrumentBarsInfo::iterator iter = _instrumentsBarsInfo.find(quote._instrumentId);
        if ( iter != _instrumentsBarsInfo.end() ) {
            if ( iter->second.second.empty() ) {
                needToGetNewBar = true;
            } else {
                TBar& bar = iter->second.second.back();
                uint32_t currNumberOfIntervals = static_cast<uint32_t>(currHistoricalTimestamp.getUnitsFromMidnight(tw::common::eHighResTimeUnits::kSecs)/_barLength);
                uint32_t barNumberOfIntervals = static_cast<uint32_t>(bar._open_timestamp.getUnitsFromMidnight(tw::common::eHighResTimeUnits::kSecs)/_barLength);

                if ( currNumberOfIntervals > barNumberOfIntervals ) {
                    tw::common_trade::calculateAtr(iter->second.second, _atrNumOfPeriods, true);
                    bar._formed = true;
                    
                    needToGetNewBar = true;
                }
            }

            if ( needToGetNewBar ) {
                TBar bar;
                bar._index = iter->second.second.size() + 1;
                bar._open_timestamp = currHistoricalTimestamp;
                bar._source = _source;
                bar._displayName = static_cast<tw::price::Quote&>(quote)._instrument->_displayName;
                bar._exchange = static_cast<tw::price::Quote&>(quote)._instrument->_exchange;
                bar._duration = _barLength;                    
                iter->second.second.push_back(bar);
            }

            tw::common_trade::onQuote(quote, iter->second.second.back());
        }
    }
    
    bool readBarsFromFile() {
        static const uint32_t TIMESTAMP_LENGTH = 26;
        static const uint32_t HEADER_LENGTH = 26;
        static const uint32_t BODY_LENGTH = sizeof(tw::price::QuoteWire);
        static const uint32_t FOOTER_LENGTH = sizeof(char);
        static const uint32_t DATA_LENGTH = HEADER_LENGTH+BODY_LENGTH+FOOTER_LENGTH;
        static const std::string NULL_TIMESTAMP = "00000000";

        try {
            // Open file with historical data
            //
            std::string buffer;
            buffer.reserve(DATA_LENGTH);
        
            std::string filename = _settings._publisher_pf_log_dir;
            if ( tw::common::Filesystem::exists_dir(filename) ) {
                filename += std::string("/publisher_pf_") + tw::common::THighResTime::dateISOString() + std::string(".log");
                LOGGER_INFO << "_settings._publisher_pf_log_dir is a directory - constructed filename: " << filename << "\n";
            } else {
                LOGGER_INFO << "_settings._publisher_pf_log_dir is a file - filename: " << filename << "\n";
            }
            
            std::fstream file(filename.c_str(), std::fstream::in);
            if ( !file.good() ) {
                LOGGER_ERRO << "Can't open for reading file: " << filename << "\n";
                return false;
            }                            
            file.seekg(0);
            
            tw::common::THighResTime currHistoricalTimestamp;
            while ( true ) {
                buffer.clear();
                buffer.resize(DATA_LENGTH);
                
                file.read(&buffer[0], DATA_LENGTH);
                if ( !file || file.eof() )
                    break;
                
                if ( file.gcount() != DATA_LENGTH )
                    break;
                
                if ( NULL_TIMESTAMP != buffer.substr(0, NULL_TIMESTAMP.length()) ) {
                    currHistoricalTimestamp = tw::common::THighResTime::parse(buffer.substr(0, TIMESTAMP_LENGTH-2));
                    tw::price::QuoteWire* quoteWire=reinterpret_cast<tw::price::QuoteWire*>(&buffer[HEADER_LENGTH]);
                    tw::price::QuoteStore::TQuote& quote = tw::price::QuoteStore::instance().getQuoteByKeyId(quoteWire->_instrumentId);
                    if ( quote.isValid() ) {
                        static_cast<tw::price::QuoteWire&>(quote) = *quoteWire;
                        processHistoricalQuote(quote, currHistoricalTimestamp);
                    }
                }
            }
            
            // Delete all bars which occurred longer than lookbackMinutes ago
            //
            TInstrumentBarsInfo::iterator iter = _instrumentsBarsInfo.begin();
            TInstrumentBarsInfo::iterator end = _instrumentsBarsInfo.end();        
            
            for ( ; iter != end; ++iter ) {
                TBars& historicalBars = iter->second.second;
                if ( 0 < iter->second.first && iter->second.first < historicalBars.size() ) {
                    historicalBars.erase(historicalBars.begin(), historicalBars.begin()+(historicalBars.size()-iter->second.first-1));
                }
            }
            
        } catch(const std::exception& e) {
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
            return false;
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
            return false;
        }
        
        return true;
    }
    
    void outputToDb(const BarPatterns& v, const tw::instr::Instrument::TKeyId& x, bool onStart=false) {
        const TBarPatterns& barPatterns = v.getBarPatterns(x);
        if ( barPatterns.empty() )
            return;
        
        const BarPattern& pattern = barPatterns.back();
        
        TBarPatterns::const_reverse_iterator riter = barPatterns.rbegin();
        TBarPatterns::const_reverse_iterator rend = barPatterns.rend();
        uint32_t count = 0;
        for ( ; riter != rend && ((*riter)._isDirty); ++riter, ++count ) {
            const_cast<BarPattern&>(*riter)._simpleHitchMaxBars = _simpleHitchMaxBars;
            const_cast<BarPattern&>(*riter)._isDirty = false;
            TRouter::persist(*riter);
        }
        
        if ( _verbose ) {
            if ( onStart )
                LOGGER_WARN << "Restored: " << barPatterns.size() << " " << pattern._parentType.toString() << " barPatterns for: " << pattern._displayName << "\n";
            else
                LOGGER_WARN << "Logged: " << count << " " << pattern._parentType.toString() << " barPatterns for: " << pattern._displayName << "\n";
        }
    }
    
private:
    bool _verbose;
    std::string _source;
    int32_t _simpleHitchMaxBars;
    tw::common::TTimerId _timerId;
    uint32_t _barLength;
    uint32_t _atrNumOfPeriods;
    TInstrumentBars _instrumentsBars;
    TInstrumentBarsInfo _instrumentsBarsInfo;
    TInstrumentTrades _instrumentTrades;
    TBars _barsNull;
    TTrades _tradesNull;
    tw::common::Settings _settings;
    
    BarPatternsSimple _barPatternsSimple;
    BarPatternsSwingLevel1 _barPatternsSwingLevel1;
    BarPatternsSwingLevel2 _barPatternsSwingLevel2;
};
    

class BarsManager {
public:
    typedef BarsManagerImpl<BarsManager> TImpl;
    typedef TImpl::TInstrumentBars TInstrumentBars;
    
public:
    BarsManager(uint32_t barLength=60, int32_t simpleHitchMaxBars=-1, uint32_t atrPeriods=0) : _impl(barLength, simpleHitchMaxBars, atrPeriods) {
        clear();
    }
    
    ~BarsManager() {
    }
    
    void clear() {
        _impl.clear();
    }
    
 public:
    void setVerbose(bool v) {
        _impl.setVerbose(v);
    }
    
    bool subscribe(const tw::instr::Instrument::TKeyId& x, uint32_t lookbackMinutes = 0) {
       return _impl.subscribe(x, lookbackMinutes);
    }
    
    void setSource(const std::string& v) {
        _impl.setSource(v);
    }
    
    const TBars& getBars(const tw::instr::Instrument::TKeyId& x) const {
        return _impl.getBars(x);
    }
    
    double getAvgVolByPrice(const tw::instr::Instrument::TKeyId& x) const {
        return _impl.getAvgVolByPrice(x);
    }
    
    double getAvgVolByPrice(const TTrades& trades) const {
        return _impl.getAvgVolByPrice(trades);
    }
    
    tw::price::Size getVolByPrice(const tw::instr::Instrument::TKeyId& x, const tw::price::Ticks& price) const {
        return _impl.getVolByPrice(x, price);
    }
    
    tw::price::Size getVolByPrice(const tw::price::Ticks& price, const TTrades& trades) const {
        return _impl.getVolByPrice(price, trades);
    }
    
    const TTrades& getTrades(const tw::instr::Instrument::TKeyId& x) const {
        return _impl.getTrades(x);
    }
    
    const BarPatternsSimple& getBarPatternsSimple() const {
        return _impl.getBarPatternsSimple();
    }
    
    const BarPatternsSwingLevel1& getBarPatternsSwingLevel1() const {
        return _impl.getBarPatternsSwingLevel1();
    }
    
    const BarPatternsSwingLevel2& getBarPatternsSwingLevel2() const {
        return _impl.getBarPatternsSwingLevel2();
    }
    
    bool start(const tw::common::Settings& settings) {
        return _impl.start(settings);
    }
    
public:
    static bool registerTimerClient(TImpl* impl, const uint32_t msecs, const bool once, tw::common::TTimerId& id) {
        return tw::common::TimerServer::instance().registerClient(impl, msecs, once, id);
    }

    static uint32_t getMSecFromMidnight() {
        return tw::common::THighResTime::now().getUsecsFromMidnight() / 1000;
    }  
    
public:
    // Bar storage related methods
    //
    static void setDbSource(const std::string& source) {
        BarsStorage::instance().setSource(source);
    }
    
    static bool outputToDb() {
        return BarsStorage::instance().outputToDb();
    }
    
    static bool readBars(uint32_t x, uint32_t barLength, uint32_t lookbackMinutes, uint32_t atrNumOfPeriods, TBars& bars) {
        return BarsStorage::instance().readBars(x, barLength, lookbackMinutes, atrNumOfPeriods, bars);
    }
    
    static bool processPatterns() {
        return BarsStorage::instance().processPatterns();
    }

    static bool isValid() {
        return BarsStorage::instance().isValid();
    }
    
    static bool canPersistToDb() {
        return BarsStorage::instance().canPersistToDb();
    }
    
    template <typename TItemType>
    static bool persist(const TItemType& v) {
        return BarsStorage::instance().persist(v);
    }
    
public:    
    tw::price::Size getBackBarsCumVol(const tw::instr::Instrument::TKeyId& x, size_t count, size_t offsetFromCurrent) const {
        return _impl.getBackBarsCumVol(x, count, offsetFromCurrent);
    }
    
    tw::price::Ticks getBackBarsHigh(const tw::instr::Instrument::TKeyId& x, size_t count) const {
        return _impl.getBackBarsHigh(x, count);
    }
    
    tw::price::Ticks getBackBarsLow(const tw::instr::Instrument::TKeyId& x, size_t count) const {
        return _impl.getBackBarsLow(x, count);
    }
    
    tw::price::Ticks getBackBarsHighLowRangeAvg(const tw::instr::Instrument::TKeyId& x, size_t count, size_t offsetFromCurrent) const {
        return _impl.getBackBarsHighLowRangeAvg(x, count, offsetFromCurrent);
    }
    
private:
    TImpl _impl;
};

class BarsManagerFactory : public tw::common::Singleton<BarsManagerFactory> {
public:
    typedef boost::shared_ptr<BarsManager> TBarsManagerPtr;
    
private:
    typedef std::pair<uint32_t, int32_t> TBarsManagerKey; 
    typedef std::pair<TBarsManagerKey, int32_t> TBarsManagerKeyWithAtr; 
    typedef std::map<TBarsManagerKeyWithAtr, TBarsManagerPtr> TBarsManagers;
    
public:
    TBarsManagerPtr getBarsManager(uint32_t barLength, int32_t simpleHitchMaxBars=-1, uint32_t atrPeriods=0) {
        if ( simpleHitchMaxBars < 0 )
            simpleHitchMaxBars = -1;
        
        TBarsManagerKeyWithAtr key(TBarsManagerKey(barLength, simpleHitchMaxBars), atrPeriods);
        TBarsManagers::iterator iter = _barsManagers.find(key);
        if ( iter == _barsManagers.end() )
            iter = _barsManagers.insert(TBarsManagers::value_type(key, TBarsManagerPtr(new BarsManager(barLength, simpleHitchMaxBars, atrPeriods)))).first;
        
        return iter->second;
    }

    bool start(const tw::common::Settings& settings) {
        TBarsManagers::iterator iter = _barsManagers.begin();
        TBarsManagers::iterator end = _barsManagers.end();
        
        for ( ; iter != end; ++iter ) {
            if ( !iter->second->start(settings) )
                return false;
        }
        
        return true;
    }
    
private:
    TBarsManagers _barsManagers;
};

} // common_trade
} // tw
