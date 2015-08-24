#pragma once

#include <tw/common/defs.h>
#include <tw/price/quote.h>
#include <tw/log/defs.h>

namespace tw {
namespace common_trade {

class ActivePriceProcessor {
public:
    typedef std::pair<tw::price::Ticks, bool> TInfo;
    
public:
    ActivePriceProcessor(ActivePriceParamsWire& p) : _p(p) {
    }
    
    void clear() {
        _p._quote.clear();
    }
    
public:
    ActivePriceParamsWire& getParams() {
        return _p;
    }
    
    const ActivePriceParamsWire& getParams() const {
        return _p;
    }
    
public:
    bool isEnabled() const {
        if ( !_p._leanQty.isValid() || 1 > _p._leanQty.get() )
            return false;
        
        return true;
    }
    
    void setQuote(const tw::price::Quote& quote) {
        _p._quote = quote;
    }
    
    TInfo calcPrice(const tw::price::Quote& quote, bool isBuy, std::string& reason) {
        static tw::price::Ticks oneTick(1);
        
        if ( !isEnabled() )
            return TInfo();
        
        const tw::price::PriceLevel& curr = quote._book[0];
        const tw::price::PriceLevel& prev = _p._quote._book[0];
        
        TInfo info = ((isBuy) ? calcPrice(curr._bid, curr._ask, prev._ask, oneTick, reason) : calcPrice(curr._ask, curr._bid, prev._bid, -oneTick, reason));
        _p._quote = quote;
        
        return info;
    }
    
private:
    TInfo calcPrice(const tw::price::PriceSizeNumOfOrders& passive,
                    const tw::price::PriceSizeNumOfOrders& active,
                    const tw::price::PriceSizeNumOfOrders& prevActive,
                    const tw::price::Ticks& crossAmount,
                    std::string& reason) {
        TInfo info;
        reason.clear();
        _s.clear();        
        
        if ( !passive.isValid() ) {
            reason = "passive level is not valid";
            return info;
        }
        
        tw::price::Ticks activePrice = passive._price + crossAmount;
        if ( !active.isValid() ) {
            info.first = activePrice;
            info.second = true;
            reason = "!active.isValid()";
            return info;
        }
        
        if ( activePrice != active._price ) {
            info.first = activePrice;
            info.second = true;
            reason = "activePrice != active._price: active=" + active.toShortString() + ", activePrice=" + activePrice.toString();
            return info;
        }
        
        tw::price::Size passiveSize = (0 < _p._avgParticipantQty.get()) ? tw::price::Size(_p._avgParticipantQty.get()*passive._numOrders) : passive._size;
        tw::price::Size activeSize = (0 < _p._avgParticipantQty.get()) ? tw::price::Size(_p._avgParticipantQty.get()*active._numOrders) : active._size;
        tw::price::Size prevActiveSize = (0 < _p._avgParticipantQty.get()) ? tw::price::Size(_p._avgParticipantQty.get()*prevActive._numOrders) : prevActive._size;
        tw::price::Size leanQty = (0 < _p._passiveSmallThreshold && passiveSize < _p._passiveSmallThreshold) ? _p._leanQtySmall : _p._leanQty;
        
        if ( 0 < _p._paranoidQty && passiveSize > _p._paranoidQty ) {
            info.first = activePrice;
            info.second = true;
            reason = "passiveSize > _p._paranoidQty: " + passiveSize.toString() + " > " + _p._paranoidQty.toString();
            return info;
        }
        
        if ( activeSize < leanQty ) {
            info.first = activePrice;
            info.second = true;
            reason = "activeSize < leanQty: " + activeSize.toString() + " < " + leanQty.toString();
            return info;
        }
        
        if ( passiveSize.toDouble()*_p._leanRatio > activeSize.toDouble() ) {
            if ( (1 > _p._ignoreRatioQty.get()) || (_p._ignoreRatioQty > activeSize) ) {
                info.first = activePrice;
                info.second = true;                
                
                _s.setPrecision(2);
                _s << "passiveSize*_p._leanRatio > activeSize: " << passiveSize.toString() << "*" << _p._leanRatio << "(" << passiveSize.toDouble()*_p._leanRatio << ")" << " > " << activeSize.toString();
                _s.setPrecision(8);
                
                reason = _s.str();
                return info;
            }
        }
        
        if ( prevActive.isValid() && (prevActive._price == activePrice) && (0.0 < _p._leanQtyDelta) && (activeSize < prevActiveSize) ) {
            if ( prevActiveSize.toDouble()*(1-_p._leanQtyDelta) > activeSize.toDouble() ) {
                if ( (1 > _p._ignoreLeanQtyDelta.get()) || (_p._ignoreLeanQtyDelta > activeSize) ) {
                    info.first = activePrice;
                    info.second = true;
                    
                    _s.setPrecision(2);
                    _s << "prevActiveSize*(1-_p._leanQtyDelta) > activeSize: " << prevActiveSize.toString() << "*" << (1- _p._leanQtyDelta) << "(" << prevActiveSize.toDouble()*(1-_p._leanQtyDelta) << ")" << " > " << activeSize.toString();
                    _s.setPrecision(8);
                    
                    reason = _s.str();
                    return info;
                }
            }
        }
        
        info.first = passive._price;
        info.second = false;
        
        return info;
    }
    
private:
    ActivePriceParamsWire& _p;
    tw::common_str_util::TFastStream _s;
};

} // common_trade
} // tw
