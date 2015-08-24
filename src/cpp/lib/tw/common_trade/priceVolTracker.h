#pragma once

#include <tw/common/defs.h>
#include <tw/price/quote.h>
#include <tw/log/defs.h>

#include <deque>

namespace tw {
namespace common_trade {
    
typedef std::pair<tw::price::Ticks, tw::price::Size> TPriceSize; 

template <typename THighResTime=tw::common::THighResTime>
struct PriceVolInfo 
{
    PriceVolInfo() {
        clear();
    }
    
    void clear() {
        _bid.first.clear();
        _bid.second.set(0);
        _ask.first.clear();
        _ask.second.set(0);
        
        _timestamp.setToNow();
    }
    
    bool operator==(const tw::price::PriceLevel& rhs) const {
        bool bidsEqual = (_bid.first == rhs._bid._price);
        bool asksEqual = (_ask.first == rhs._ask._price);
        
        return (bidsEqual && asksEqual);
    }
    
    bool operator!=(const tw::price::PriceLevel& rhs) const {
        return !(*this==rhs);
    }
    
    TPriceSize _bid;
    TPriceSize _ask;
    THighResTime _timestamp;
};

template <typename THighResTime=tw::common::THighResTime>
struct TimeVolInfo 
{
    TimeVolInfo() {
        clear();
    }
    
    void clear() {
        _size.clear();
        _isBuy = false;
        _timestamp.setToNow();
    }
    
    tw::price::Size _size;
    bool _isBuy;
    THighResTime _timestamp;
};

template <typename THighResTime=tw::common::THighResTime>
class PriceVolTracker {
public:
    typedef PriceVolInfo<THighResTime> TPriceVolInfo;
    typedef TimeVolInfo<THighResTime> TTimeVolInfo;
    
    typedef std::deque<TPriceVolInfo> TInfos;
    typedef std::deque<TTimeVolInfo> TTimeVolInfos;
    typedef std::pair<tw::price::Size, tw::price::Size> TBuySellVolInfo;
    
public:
    PriceVolTracker() {
        clear();
    }
    
    void clear() {
        _infos.clear();
        _timeVolInfos.clear();
    }
    
    const TInfos& getInfos() const {
        return _infos;
    }
    
    const TTimeVolInfos& getTimeVolInfos() const {
        return _timeVolInfos;
    }
    
public:
    void onQuote(const tw::price::Quote& quote) {        
        if ( quote.isLevelUpdate(0) )
            check(quote._book[0]);
        
        if ( quote.isNormalTrade() ) {
            TTimeVolInfo timeVolCurrInfo;
            timeVolCurrInfo._size = quote._trade._size;
            
            TPriceVolInfo& currInfo = _infos.front();            
            switch ( quote._trade._aggressorSide ) {
                case tw::price::Trade::kAggressorSideBuy:
                    if ( quote._trade._price == currInfo._ask.first ) {
                        currInfo._ask.second += quote._trade._size;
                    } else {
                        TPriceVolInfo info;
                        info._ask.first = quote._trade._price;
                        info._ask.second += quote._trade._size;
                        _infos.push_front(info);
                    }                    
                    timeVolCurrInfo._isBuy = true;
                    break;
                case tw::price::Trade::kAggressorSideSell:
                    if ( quote._trade._price == currInfo._bid.first ) {
                        currInfo._bid.second += quote._trade._size;
                    } else {
                        TPriceVolInfo info;
                        info._bid.first = quote._trade._price;
                        info._bid.second += quote._trade._size;
                        _infos.push_front(info);
                    }                    
                    break;
                default:
                    if ( quote._trade._price == currInfo._bid.first ) {
                        currInfo._bid.second += quote._trade._size;
                    } else if ( quote._trade._price == currInfo._ask.first ) {
                        currInfo._ask.second += quote._trade._size;
                        timeVolCurrInfo._isBuy = true;
                    } else if ( quote._trade._price < currInfo._bid.first ) {
                        TPriceVolInfo info;
                        info._bid.first = quote._trade._price;
                        info._bid.second += quote._trade._size;
                        _infos.push_front(info);
                    } else if ( quote._trade._price > currInfo._ask.first ) {
                        TPriceVolInfo info;
                        info._ask.first = quote._trade._price;
                        info._ask.second += quote._trade._size;
                        _infos.push_front(info);
                        timeVolCurrInfo._isBuy = true;
                    }
                    break;
            }
            
            _timeVolInfos.push_front(timeVolCurrInfo);
        }
    }
    
    TBuySellVolInfo getBuySellVolInfo(int64_t tillUSec) {
        TBuySellVolInfo x;
        THighResTime now = THighResTime::now();
        
        for ( size_t i = 0; i < _timeVolInfos.size(); ++i ) {
            TTimeVolInfo& info = _timeVolInfos[i];
            int64_t delta = now - info._timestamp;
            if ( tillUSec < delta )
                break;
            
            if ( info._isBuy )
                x.first += info._size;
            else
                x.second += info._size;
        }
        
        return x;
    }
    
private:
    void check(const tw::price::PriceLevel& level) {
        if ( _infos.empty() ) {
            add(level);
            return;
        }
        
        TPriceVolInfo& info = _infos.front();
        if ( info != level ) {
            if ( level._bid._price == info._bid.first && !info._ask.first.isValid() ) {
                info._ask.first = level._ask._price;
                return;
            }
            
            if ( level._ask._price == info._ask.first && !info._bid.first.isValid() ) {
                info._bid.first = level._bid._price;
                return;
            }
            
            add(level);
        }
    }
    
    void add(const tw::price::PriceLevel& level) {        
        TPriceVolInfo info;
        info._bid.first = level._bid._price;
        info._ask.first = level._ask._price;

        _infos.push_front(info);
    }
    
private:
    TInfos _infos;
    TTimeVolInfos _timeVolInfos;
};

typedef PriceVolTracker<> TPriceVolTracker;

} // common_trade
} // tw
