#pragma once

#include <tw/common/singleton.h>
#include <tw/common/timer_server.h>
#include <tw/common_trade/wbo_manager.h>
#include <tw/common_thread/utils.h>

#include <deque>
#include <map>

namespace tw {
namespace common_trade {

template <typename TRouter>    
class MktVelsManagerImpl : public tw::common::TimerClient {
public:    
    typedef std::deque<double> TWbos;
    typedef std::map<tw::instr::Instrument::TKeyId, TWbos> TInstrumentWbos;
    
    typedef std::pair<bool, double> TResult;
    
public:
    MktVelsManagerImpl() {
        clear();
    }
    
    ~MktVelsManagerImpl() {
    }
    
    void clear() {
        _verbose = false;
        _maxQueueSize = 10*60*10; // (10 per sec) * (60 per min) * (10 minutes)
        _cycleMSec = 100; // NOTE: for now, all price vels are recorded at 100 ms intervals
        _instrumentsWbos.clear();
    }
    
public:
    void setVerbose(bool v) {
        _verbose = v;
    }
    
    void setMaxQueueSize(uint32_t v) {
        _maxQueueSize = v;
    }
    
    bool subscribe(const tw::instr::Instrument::TKeyId& x) {
        if ( _instrumentsWbos.empty() ) {
            uint32_t delta = calcCycleTimeout();
            // !!!! - TODO: time server is 100 ms resolution, so naive attempt to align
            // our timeouts on 100 ms boundary - this is done only once, so 100 ms
            // CAN drift throughout the day - need to rethink
            //
            if ( delta > _cycleMSec )
                tw::common_thread::sleep(delta%_cycleMSec);
            
            TRouter::registerTimerClient(this, _cycleMSec, true, _timerId);
        }
        
        TInstrumentWbos::iterator iter = _instrumentsWbos.find(x);
        if ( iter != _instrumentsWbos.end() ) {
            LOGGER_WARN << "Trying to subscribe to already subscribed symbol: " << x << "\n";
            return true;
        }
        
        tw::price::QuoteStore::instance().subscribeFront(x, this);
        iter = _instrumentsWbos.insert(TInstrumentWbos::value_type(x, TWbos())).first;
        
        return true;
    }
    
    bool isSubscribed(const tw::instr::Instrument::TKeyId& x) {
       return (_instrumentsWbos.find(x) != _instrumentsWbos.end());
    }
    
public:
    void onQuote(const tw::price::Quote& quote) {
        if ( tw::price::Quote::kSuccess != quote._status )
            return;
        
        if ( quote.isLevelUpdate(0) )
            tw::common_trade::WboManager::instance().onQuote(quote);
    }
    
    // TimerClient interface
    //
    virtual bool onTimeout(const tw::common::TTimerId& id) {
        if ( _verbose )
            LOGGER_INFO << "Updating vels" << "\n";
        
        TInstrumentWbos::iterator iter = _instrumentsWbos.begin();
        TInstrumentWbos::iterator end = _instrumentsWbos.end();
        
        for ( ; iter != end; ++iter ) {
            tw::common_trade::WboManager::TTv& tv = tw::common_trade::WboManager::instance().getOrCreateTv(iter->first);
            iter->second.push_front(tv.getTvInTicks());
            if ( iter->second.size() > _maxQueueSize )
                iter->second.pop_back();
        }        
        
        TRouter::registerTimerClient(this, calcCycleTimeout(), true, _timerId);
        return false;
    }
    
    const TWbos& getWBos(const tw::instr::Instrument::TKeyId& x) const {
        TInstrumentWbos::const_iterator iter = _instrumentsWbos.find(x);
        if ( iter != _instrumentsWbos.end() )
            return iter->second;
        
        return _wbosNull;
    }
    
    TResult getVelDelta(const tw::instr::Instrument::TKeyId& x, uint32_t timeDeltaMs, size_t offsetFromCurrent) const {
        TResult y;       
        
        TInstrumentWbos::const_iterator iter = _instrumentsWbos.find(x);
        if ( iter != _instrumentsWbos.end() && offsetFromCurrent < iter->second.size() ) {
            size_t count = timeDeltaMs/_cycleMSec;
            count = std::min(count+offsetFromCurrent, iter->second.size()-1);
            if ( count > 0 && (count - offsetFromCurrent) > 0 ) {
                y.first = true;
                
                double tv1 = iter->second[count];
                double tv2 = iter->second[offsetFromCurrent];
                
                if ( tv1 != 0.0 && tv2 != 0.0 )
                    y.second = tv2 - tv1;
            }
        }
        
        return y;
    }
    
    double getAvgVel(const tw::instr::Instrument::TKeyId& x, uint32_t timeSliceMs, uint32_t timeDeltaMs, size_t offsetFromCurrent) const {
        double y = 0.0;
        
        int32_t count = timeSliceMs/timeDeltaMs-1;
        size_t step = timeDeltaMs/_cycleMSec;
        size_t items = 0;
        for ( int32_t i = 0; i < count; ++i ) {
            TResult r = getVelDelta(x, timeDeltaMs, offsetFromCurrent+step*i);
            if ( !r.first )
                break;
            
            y += r.second;
            ++items;
        }
        
        if ( items > 2 )
            y /= items;
        
        return y;
    }
    
    uint32_t calcCycleTimeout() const {                
        uint32_t msecFromMidnight = TRouter::getMSecFromMidnight();
        uint32_t msecInCycle = msecFromMidnight % _cycleMSec;
        uint32_t delta = _cycleMSec-msecInCycle;

        return ((delta < _cycleMSec) ? (_cycleMSec+delta) : delta);
    }
    
private:
    bool _verbose;
    uint32_t _maxQueueSize;
    uint32_t _cycleMSec;
    
    tw::common::TTimerId _timerId;
    TInstrumentWbos _instrumentsWbos;
    TWbos _wbosNull;
};


class MktVelsManager : public tw::common::Singleton<MktVelsManager> {
public:
    typedef MktVelsManagerImpl<MktVelsManager> TImpl;
    typedef TImpl::TWbos TWbos;
    typedef TImpl::TResult TResult;
    
public:
    MktVelsManager() {
        clear();
    }
    
    ~MktVelsManager() {
    }
    
    void clear() {
        _impl.clear();
    }
    
 public:
    void setVerbose(bool v) {
        _impl.setVerbose(v);
    }
    
    void setMaxQueueSize(uint32_t v) {
        _impl.setMaxQueueSize(v);
    }
    
    bool subscribe(const tw::instr::Instrument::TKeyId& x) {
       return _impl.subscribe(x);
    }
    
    bool isSubscribed(const tw::instr::Instrument::TKeyId& x) {
       return _impl.isSubscribed(x);
    }
    
    const TWbos& getWBos(const tw::instr::Instrument::TKeyId& x) const {
        return _impl.getWBos(x);
    }
    
    TResult getVelDelta(const tw::instr::Instrument::TKeyId& x, uint32_t timeDeltaMs, size_t offsetFromCurrent) const {
        return _impl.getVelDelta(x, timeDeltaMs, offsetFromCurrent);
    }
    
    double getAvgVel(const tw::instr::Instrument::TKeyId& x, uint32_t timeSliceMs, uint32_t timeDeltaMs, size_t offsetFromCurrent) const {
        return _impl.getAvgVel(x, timeSliceMs, timeDeltaMs, offsetFromCurrent);
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
