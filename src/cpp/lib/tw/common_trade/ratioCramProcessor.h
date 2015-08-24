#pragma once

#include <tw/common/defs.h>
#include <tw/price/quote.h>
#include <tw/log/defs.h>

#include <map>

namespace tw {
namespace common_trade {

typedef tw::price::Ticks TPrice;
typedef tw::price::Size TSize;

class RatioCramProcessor {
public:
    RatioCramProcessor(RatioCramParamsWire& p) : _p(p) {
    }
    
    void clear() {
         _p._rcReason.clear();
        _s.clear();
        _s.setPrecision(2);
    }
    
public:
    RatioCramParamsWire& getParams() {
        return _p;
    }
    
    const RatioCramParamsWire& getParams() const {
        return _p;
    }
    
public:
    bool isEnabled() const {
        return (_p._rcInitialTicks.get() > -1);
    }
   
    bool isStopSlideTriggered(FillInfo& info,
                              const tw::price::Quote& quote,
                              const TPrice& icebergPrice) {
        if ( !isEnabled() )
            return false;
        
        clear();
        
        if ( icebergPrice.isValid() )
            _p._rcIcebergPrice = icebergPrice;
        
        const tw::price::PriceLevel& top = quote._book[0];
        if ( !top.isValid() ) {
            _p._rcReason = "!quote._book[0].isValid()";
            return false;
        }
        
        if ( _p._rcTop._bid._price != top._bid._price || _p._rcTop._ask._price != top._ask._price ) {
            _p._rcTop.clear();
            _p._rcTop._bid._price = top._bid._price;
            _p._rcTop._ask._price = top._ask._price;
        }
        
        if ( !quote.isNormalTrade() ) {
            _p._rcReason = "!quote.isNormalTrade()";
            return false;
        }
        
        if ( _p._rcIcebergPrice.isValid() && (top._bid._price == _p._rcIcebergPrice || top._ask._price == _p._rcIcebergPrice) ) {
            _s << "quote._book[0]._bid._price == _p._rcIcebergPrice || quote._book[0]._ask._price == _p._rcIcebergPrice -- "
               << "_p._rcIcebergPrice=" << _p._rcIcebergPrice
               << ",quote._book[0]=" << top.toShortString();
            
            _p._rcReason = _s.str();
            return false;
        }
        
        if ( quote._trade._price <= _p._rcTop._bid._price )
            _p._rcTop._bid._size += quote._trade._size;
        else if ( quote._trade._price >= _p._rcTop._ask._price )
            _p._rcTop._ask._size += quote._trade._size;
        
        tw::price::Ticks stop;
        double ratio = 0;
        switch ( info._fill._side ) {
            case tw::channel_or::eOrderSide::kBuy:
            {
                if ( top._bid._price < info._fill._price+_p._rcInitialTicks ) {
                    _p._rcReason = "quote._book[0]._bid._price < info._fill._price+_p._rcInitialTicks";
                    return false;
                }
                
                stop = _p._rcTop._bid._price - _p._rcForStopTicks;
                if ( info._stop.isValid() && info._stop >= stop )  {
                    _s << "stop requirements not calculated since info._stop.isValid() && info._stop >= stop -- "
                       << "stop=" << stop
                       << ",_p._rcTop=" << _p._rcTop.toShortString()
                       << ",info=" << info.toString();
                    
                    _p._rcReason = _s.str();
                    return false;
                }
                
                if ( _p._rcTop._ask._size < _p._rcVolFor ) {
                    _s << "_p._rcTop._ask._size < _p._rcVolFor -- "
                       << "_p._rcTop._ask._size=" << _p._rcTop._ask._size
                       << ",_p._rcVolFor=" << _p._rcVolFor
                       << ",_p._rcTop=" << _p._rcTop.toShortString();
                            
                    _p._rcReason = _s.str();
                    return false;
                }
                
                ratio = (_p._rcTop._bid._size.get() > 0) ? (_p._rcTop._ask._size.toDouble()/_p._rcTop._bid._size.toDouble()) : _p._rcTop._ask._size.toDouble();
                if ( ratio < _p._rcRatioFor ) {
                    _s << "ratio < _p._rcRatioFor -- "
                       << "ratio=" << ratio
                       << ",_p._rcRatioFor=" << _p._rcRatioFor
                       << ",_p._rcTop=" << _p._rcTop.toShortString()
                       << ",info=" << info.toString() ;
                            
                    _p._rcReason = _s.str();
                    return false;
                }
                
                _s << "!info._stop.isValid() || info._stop < stop for: ";
            }
                break;
            case tw::channel_or::eOrderSide::kSell:
            {
                if ( top._ask._price > info._fill._price-_p._rcInitialTicks ) {
                    _p._rcReason = "quote._book[0]._ask._price > info._fill._price-_p._rcInitialTicks";
                    return false;
                }
                
                stop = _p._rcTop._ask._price + _p._rcForStopTicks;
                if ( info._stop.isValid() && info._stop <= stop )  {
                    _s << "stop requirements not calculated since info._stop.isValid() && info._stop <= stop -- "
                       << "stop=" << stop
                       << ",_p._rcTop=" << _p._rcTop.toShortString()
                       << ",info=" << info.toString();
                    
                    _p._rcReason = _s.str();
                    return false;
                }
                
                if ( _p._rcTop._bid._size < _p._rcVolFor ) {
                    _s << "_p._rcTop._bid._size < _p._rcVolFor -- "
                       << "_p._rcTop._bid._size=" << _p._rcTop._bid._size
                       << ",_p._rcVolFor=" << _p._rcVolFor
                       << ",_p._rcTop=" << _p._rcTop.toShortString();
                            
                    _p._rcReason = _s.str();
                    return false;
                }
                
                ratio = (_p._rcTop._ask._size.get() > 0) ? (_p._rcTop._bid._size.toDouble()/_p._rcTop._ask._size.toDouble()) : _p._rcTop._bid._size.toDouble();
                if ( ratio < _p._rcRatioFor ) {
                    _s << "ratio < _p._rcRatioFor -- "
                       << "ratio=" << ratio
                       << ",_p._rcRatioFor=" << _p._rcRatioFor
                       << ",_p._rcTop=" << _p._rcTop.toShortString()
                       << ",info=" << info.toString() ;
                            
                    _p._rcReason = _s.str();
                    return false;
                }
                
                _s << "!info._stop.isValid() || info._stop > stop for: ";
                
            }
                break;
            default:
                _s << "info._fill._side == " << info._fill._side.toString()
                   << ",info=" + info.toString();
                
                _p._rcReason = _s.str();
                return false;
        }

        _s << "stop=" << stop 
           << ",ratio=" << ratio
           << ",info=" << info.toString() 
           << ",_p._rcTop=" << _p._rcTop.toShortString() 
           << " -- quote=" << quote._book[0].toShortString();

        info._stop = stop;
        _p._rcReason = _s.str();
        return true;
    }
    
private:
    RatioCramParamsWire& _p;
    tw::common_str_util::FastStream<1024*4> _s;
};

} // common_trade
} // tw
