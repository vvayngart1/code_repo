#pragma once

#include <tw/common/defs.h>
#include <tw/price/quote.h>
#include <tw/log/defs.h>

#include <map>

namespace tw {
namespace common_trade {

typedef tw::price::Ticks TPrice;
typedef tw::price::Size TSize;

class RatioCramRiskDefenseProcessor {
public:
    RatioCramRiskDefenseProcessor(RatioCramRiskDefenseParamsWire& p) : _p(p) {
    }
    
    void clear() {
         _p._rcrdReason.clear();
        _s.clear();
        _s.setPrecision(2);
    }
    
public:
    RatioCramRiskDefenseParamsWire& getParams() {
        return _p;
    }
    
    const RatioCramRiskDefenseParamsWire& getParams() const {
        return _p;
    }
    
public:
    bool isEnabled() const {
        return (_p._rcrdVolAgainst.get() > -1);
    }
   
    bool isStopSlideTriggered(FillInfo& info, const tw::price::Quote& quote) {
        if ( !isEnabled() )
            return false;
        
        clear();
        
        const tw::price::PriceLevel& top = quote._book[0];
        if ( !top.isValid() ) {
            _p._rcrdReason = "!quote._book[0].isValid()";
            return false;
        }
        
        if ( _p._rcrdTop._bid._price != top._bid._price || _p._rcrdTop._ask._price != top._ask._price ) {
            _p._rcrdTop.clear();
            _p._rcrdTop._bid._price = top._bid._price;
            _p._rcrdTop._ask._price = top._ask._price;
        }
        
        if ( !quote.isNormalTrade() ) {
            _p._rcrdReason = "!quote.isNormalTrade()";
            return false;
        }
        
        if ( quote._trade._price <= _p._rcrdTop._bid._price )
            _p._rcrdTop._bid._size += quote._trade._size;
        else if ( quote._trade._price >= _p._rcrdTop._ask._price )
            _p._rcrdTop._ask._size += quote._trade._size;
        
        tw::price::Ticks stop;
        double ratio = 0;
        switch ( info._fill._side ) {
            case tw::channel_or::eOrderSide::kBuy:
            {
                if ( top._bid._price > info._fill._price-_p._rcrdInitialTicks ) {
                    _p._rcrdReason = "quote._book[0]._bid._price > info._fill._price-_p._rcrdInitialTicks";
                    return false;
                }
                
                stop = _p._rcrdTop._bid._price - _p._rcrdAgainstStopTicks;
                if ( info._stop.isValid() && info._stop >= stop )  {
                    _s << "stop requirements not calculated since info._stop.isValid() && info._stop >= stop -- "
                       << "stop=" << stop
                       << ",_p._rcrdTop=" << _p._rcrdTop.toShortString()
                       << ",info=" << info.toString();
                    
                    _p._rcrdReason = _s.str();
                    return false;
                }
                
                if ( _p._rcrdTop._bid._size < _p._rcrdVolAgainst ) {
                    _s << "_p._rcrdTop._bid._size < _p._rcrdVolAgainst -- "
                       << "_p._rcrdTop._bid._size=" << _p._rcrdTop._bid._size
                       << ",_p._rcrdVolAgainst=" << _p._rcrdVolAgainst
                       << ",_p._rcrdTop=" << _p._rcrdTop.toShortString();
                            
                    _p._rcrdReason = _s.str();
                    return false;
                }
                
                ratio = (_p._rcrdTop._ask._size.get() > 0) ? (_p._rcrdTop._bid._size.toDouble()/_p._rcrdTop._ask._size.toDouble()) : _p._rcrdTop._bid._size.toDouble();
                if ( ratio < _p._rcrdRatioAgainst ) {
                    _s << "ratio < _p._rcrdRatioAgainst -- "
                       << "ratio=" << ratio
                       << ",_p._rcrdRatioAgainst=" << _p._rcrdRatioAgainst
                       << ",_p._rcrdTop=" << _p._rcrdTop.toShortString()
                       << ",info=" << info.toString() ;
                            
                    _p._rcrdReason = _s.str();
                    return false;
                }
                
                _s << "!info._stop.isValid() || info._stop < stop against: ";
            }
                break;
            case tw::channel_or::eOrderSide::kSell:
            {
                if ( top._ask._price < info._fill._price+_p._rcrdInitialTicks ) {
                    _p._rcrdReason = "quote._book[0]._ask._price < info._fill._price+_p._rcrdInitialTicks";
                    return false;
                }
                
                stop = _p._rcrdTop._ask._price + _p._rcrdAgainstStopTicks;
                if ( info._stop.isValid() && info._stop <= stop )  {
                    _s << "stop requirements not calculated since info._stop.isValid() && info._stop <= stop -- "
                       << "stop=" << stop
                       << ",_p._rcrdTop=" << _p._rcrdTop.toShortString()
                       << ",info=" << info.toString();
                    
                    _p._rcrdReason = _s.str();
                    return false;
                }
                
                if ( _p._rcrdTop._ask._size < _p._rcrdVolAgainst ) {
                    _s << "_p._rcrdTop._ask._size < _p._rcrdVolAgainst -- "
                       << "_p._rcrdTop._ask._size=" << _p._rcrdTop._ask._size
                       << ",_p._rcrdVolAgainst=" << _p._rcrdVolAgainst
                       << ",_p._rcrdTop=" << _p._rcrdTop.toShortString();
                            
                    _p._rcrdReason = _s.str();
                    return false;
                }
                
                ratio = (_p._rcrdTop._bid._size.get() > 0) ? (_p._rcrdTop._ask._size.toDouble()/_p._rcrdTop._bid._size.toDouble()) : _p._rcrdTop._ask._size.toDouble();
                if ( ratio < _p._rcrdRatioAgainst ) {
                    _s << "ratio < _p._rcrdRatioAgainst -- "
                       << "ratio=" << ratio
                       << ",_p._rcrdRatioAgainst=" << _p._rcrdRatioAgainst
                       << ",_p._rcrdTop=" << _p._rcrdTop.toShortString()
                       << ",info=" << info.toString() ;
                            
                    _p._rcrdReason = _s.str();
                    return false;
                }
                
                _s << "!info._stop.isValid() || info._stop > stop against: ";
                
            }
                break;
            default:
                _s << "info._fill._side == " << info._fill._side.toString()
                   << ",info=" + info.toString();
                
                _p._rcrdReason = _s.str();
                return false;
        }

        _s << "stop=" << stop 
           << ",ratio=" << ratio
           << ",info=" << info.toString() 
           << ",_p._rcrdTop=" << _p._rcrdTop.toShortString() 
           << " -- quote=" << quote._book[0].toShortString();

        info._stop = stop;
        _p._rcrdReason = _s.str();
        return true;
    }
    
private:
    RatioCramRiskDefenseParamsWire& _p;
    tw::common_str_util::FastStream<1024*4> _s;
};

} // common_trade
} // tw
