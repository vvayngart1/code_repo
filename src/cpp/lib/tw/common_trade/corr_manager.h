#pragma once

#include <tw/common/singleton.h>
#include <tw/common/timer_server.h>
#include <tw/common_trade/Defs.h>
#include <tw/price/quote_store.h>

#include <tw/generated/bars.h>

#include <map>

namespace tw {
namespace common_trade {

template <typename TRouter>    
class CorrManagerImpl : public tw::common::TimerClient {
public:
    typedef BarCorr TBar;
    typedef std::vector<BarCorr> TBars;
    typedef std::map<tw::instr::Instrument::TKeyId, TBars> TInstrumentBars;
    
public:
    CorrManagerImpl() {
        clear();
    }
    
    ~CorrManagerImpl() {
    }
    
    void clear() {
        _verbose = false;
        _cycleMSec = 1000;
        _instrumentsBars.clear();
    }
    
public:
    void setVerbose(bool v) {
        _verbose = v;
    }
    
    void setCycleTime(uint32_t cycleMSec) {
        _cycleMSec = cycleMSec;
    }
    
    bool subscribe(const tw::instr::Instrument::TKeyId& x) {
        if ( _instrumentsBars.empty() && !TRouter::registerTimerClient(this, calcCycleTimeout(), true, _timerId) )
            return false;
        
        TInstrumentBars::iterator iter = _instrumentsBars.find(x);
        if ( iter != _instrumentsBars.end() ) {
            LOGGER_WARN << "Trying to subscribe to already subscribed symbol: " << x << "\n";
            return true;
        }                
            
        tw::price::QuoteStore::instance().subscribeFront(x, this);
        iter = _instrumentsBars.insert(TInstrumentBars::value_type(x, TBars())).first;
        createNewBar(iter);        
        
        return true;
    }
    
public:    
    void onQuote(const tw::price::Quote& quote) {
        if ( tw::price::Quote::kSuccess != quote._status || !quote.isNormalTrade() )
            return;
        
        TInstrumentBars::iterator iter = _instrumentsBars.find(quote._instrumentId);
        if ( iter == _instrumentsBars.end() || iter->second.empty() )
            return;
        
        TBar& bar = iter->second.back();
        if ( 1 == (++bar._numOfTrades) ) {
            bar._high = bar._low = bar._open = quote._trade._price;
        } else {
            if ( quote._trade._price > bar._high )
                bar._high = quote._trade._price;
            else if ( quote._trade._price < bar._low )
                bar._low = quote._trade._price;
        }
        
        bar._lastValidPrice = bar._close = quote._trade._price;
        bar._volume += quote._trade._size;
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
                iter->second.back()._formed = true;
                if ( _verbose )
                    LOGGER_INFO << iter->second.back().toString() << "\n";
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
    
    uint32_t calcCycleTimeout() const {
        uint32_t msecFromMidnight = TRouter::getMSecFromMidnight();
        uint32_t msecInCycle = msecFromMidnight % _cycleMSec;
        uint32_t delta = _cycleMSec-msecInCycle;
        if ( delta < 100 )
            delta = 100;

        return (0 == delta ? _cycleMSec : delta);
    }
    
public:
    double calcCorrelation(const tw::instr::Instrument::TKeyId& x,      // leader
                           const tw::instr::Instrument::TKeyId& y,      // lagger
                           uint32_t count,
                           uint32_t offset,
                           uint32_t relOffset,
                           bool checkIfBarTraded = false) {
        if ( relOffset > count )
            return 0.0;
        
        int32_t effectiveCount = static_cast<int32_t>(count) - static_cast<int32_t>(relOffset);
        if ( 2 > effectiveCount )
            return 0.0;
        
        TBars xBars = getBars(x);
        size_t xSize = xBars.size();
        if ( xSize < (effectiveCount+offset) )
            return 0.0;
        
        TBars yBars = getBars(y);
        size_t ySize = yBars.size();
        if ( ySize < (effectiveCount+offset) )
            return 0.0;
        
        _calc.clear();
        
        size_t xIndex = xSize - count - offset; 
        size_t yIndex = ySize - count - offset; 
        for ( int32_t i = 0; i < effectiveCount; ++i ) {
            const TBar& xBar = xBars[xIndex+i];
            if ( !xBar._close.isValid() && !xBar._lastValidPrice.isValid() )
                return 0.0;
            
            if ( checkIfBarTraded && !xBar._close.isValid() )
                return 0.0;
            
            _calc.getX().getValues().push_back(xBar._close.isValid() ? xBar._close.toDouble() : xBar._lastValidPrice.toDouble());
            
            const TBar& yBar = yBars[yIndex+relOffset+i];
            if ( !yBar._close.isValid() && !yBar._lastValidPrice.isValid() )
                return 0.0;
            
            if ( checkIfBarTraded && !yBar._close.isValid() )
                return 0.0;
            
            _calc.getY().getValues().push_back(yBar._close.isValid() ? yBar._close.toDouble() : yBar._lastValidPrice.toDouble());
        }
        
        return _calc.calcCorrel();
    }
    
private:
    void createNewBar(TInstrumentBars::iterator& iter) {            
        iter->second.push_back(TBar());
        iter->second.back()._open_timestamp.setToNow();
        iter->second.back()._instrument = tw::instr::InstrumentManager::instance().getByKeyId(iter->first);
        if ( iter->second.back()._instrument ) {
            iter->second.back()._displayName = iter->second.back()._instrument->_displayName;
            iter->second.back()._exchange = iter->second.back()._instrument->_exchange;
        }
        iter->second.back()._duration = _cycleMSec;
        if ( iter->second.size() > 1 )
            iter->second.back()._lastValidPrice = iter->second[iter->second.size()-2]._lastValidPrice;
    }
    
private:
    bool _verbose;
    uint32_t _cycleMSec;
    
    tw::common::TTimerId _timerId;
    TInstrumentBars _instrumentsBars;
    TBars _barsNull;
    FinCalc _calc;
};


class CorrManager : public tw::common::Singleton<CorrManager> {
public:
    typedef CorrManagerImpl<CorrManager> TImpl;
    typedef TImpl::TBar TBar;
    typedef TImpl::TBars TBars;
    typedef TImpl::TInstrumentBars TInstrumentBars;
    
public:
    CorrManager() {
        clear();
    }
    
    ~CorrManager() {
    }
    
    void clear() {
        _impl.clear();
    }
    
 public:
    void setVerbose(bool v) {
        _impl.setVerbose(v);
    }
    
    void setCycleTime(uint32_t cycleMSec) {
        _impl.setCycleTime(cycleMSec);
    }
    
    bool subscribe(const tw::instr::Instrument::TKeyId& x) {
       return _impl.subscribe(x);
    }
    
    const TBars& getBars(const tw::instr::Instrument::TKeyId& x) const {
        return _impl.getBars(x);
    }
    
    double calcCorrelation(const tw::instr::Instrument::TKeyId& x,
                           const tw::instr::Instrument::TKeyId& y,
                           uint32_t count,
                           uint32_t offset,
                           uint32_t relOffset) {
        return _impl.calcCorrelation(x, y, count, offset, relOffset);
    }
    
public:
    static bool registerTimerClient(TImpl* impl, const uint32_t msecs, const bool once, tw::common::TTimerId& id) {
        return tw::common::TimerServer::instance().registerClient(impl, msecs, once, id);
    }

    static uint32_t getMSecFromMidnight() {
        return tw::common::THighResTime::now().getUsecsFromMidnight() / 1000;
    }
    
    
private:
    TImpl _impl;
};

} // common_trade
} // tw
