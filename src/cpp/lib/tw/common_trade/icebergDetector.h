#pragma once

#include <tw/common/defs.h>
#include <tw/price/quote.h>
#include <tw/log/defs.h>

#include <map>

namespace tw {
namespace common_trade {

typedef tw::price::Ticks TPrice;
typedef tw::price::Size TSize;

class IcebergDetector {
public:
    typedef std::map<TPrice, IcebergInfo> TInfos; 
    
public:
    IcebergDetector(IcebergDetectorParamsWire& p) : _p(p) {
        if ( _p._idResetCountAwayTicks.get() < 1 ) {
            LOGGER_ERRO << "_p._idResetCountAwayTicks < 1, setting to 1, _p._idResetCountAwayTicks=" << _p._idResetCountAwayTicks << "\n";
            _p._idResetCountAwayTicks.set(1);
        }
    }
    
    void clear() {
         _p._idReason.clear();
        _s.clear();
        _s.setPrecision(2);
    }
    
    void resetIceberg() {
        clear();
        if ( _p._idIsIcebergDetected && _p._idCurrIcebergInfo._price.isValid() )
            resetCounts(getInfo(_p._idCurrIcebergInfo._price));
        
        _p._idIsIcebergDetected = false;
        _p._idCurrIcebergInfo.clear();
    }
    
    void resetAll() {
        clear();
        _p._idIsIcebergDetected = false;
        _p._idIsLastEventTrade = false;
        _p._idCurrIcebergInfo.clear();
        
        _infos.clear();
    }
    
    std::string printInfos() {
        clear();
        
        TInfos::iterator iter = _infos.begin();
        TInfos::iterator end = _infos.end();
        for ( ; iter != end; ++iter ) {
            if ( iter != _infos.begin() )
                _s << ",";
            
            _s << "info=" << iter->second.toString();
        }
        
        return _s.str();
    }
    
public:
    IcebergDetectorParamsWire& getParams() {
        return _p;
    }
    
    const IcebergDetectorParamsWire& getParams() const {
        return _p;
    }
    
    TInfos& getInfos() {
        return _infos;
    } 
    
    const TInfos& getInfos() const {
        return _infos;
    }
    
public:
    bool isEnabled() const {
        return (_p._idRatio > 0);
    }
   
    bool isStopSlideTriggered(FillInfo& info) {
        if ( !isEnabled() )
            return false;
        
        clear();
        TPrice stop;
        
        if ( !_p._idIsIcebergDetected ) {
            _p._idReason = "_p._idIsIcebergDetected==false";
            return false;
        }
        
        if ( tw::channel_or::eOrderSide::kUnknown == _p._idCurrIcebergInfo._side ) {
            _p._idReason = "tw::channel_or::eOrderSide::kUnknown == _p._idCurrIcebergInfo._icebergSide";
            return false;
        }
        
        if ( !_p._idCurrIcebergInfo._price.isValid() ) {
            _p._idReason = "_p._idCurrIcebergInfo._price.isValid()==false";
            return false;
        }
        
        switch ( info._fill._side ) {
            case tw::channel_or::eOrderSide::kBuy:
                stop = ( tw::channel_or::eOrderSide::kBuy == _p._idCurrIcebergInfo._side ) ? (_p._idCurrIcebergInfo._price - _p._idSlideStopForTicks) : (_p._idCurrIcebergInfo._price - _p._idSlideStopAgainstTicks); 
                if ( info._stop.isValid() && info._stop >= stop )  {
                    _p._idReason = "info._stop.isValid() && info._stop >= stop";
                    return false;
                }
                break;
            case tw::channel_or::eOrderSide::kSell:
                stop = ( tw::channel_or::eOrderSide::kSell == _p._idCurrIcebergInfo._side ) ? (_p._idCurrIcebergInfo._price + _p._idSlideStopForTicks) : (_p._idCurrIcebergInfo._price + _p._idSlideStopAgainstTicks);
                if ( info._stop.isValid() && info._stop <= stop )  {
                    _p._idReason = "info._stop.isValid() && info._stop <= stop";
                    return false;
                }
                break;
            default:
                _p._idReason = "tw::channel_or::eOrderSide::kUnknown == info._fill._side";
                return false;
        }
        
        info._stop = stop;
        _s << "CalcStop()==true: stop=" << stop << ",_p._idCurrIcebergInfo=" << _p._idCurrIcebergInfo.toString();
        _p._idReason = _s.str();
        return true;
    }
    
public:
    void onQuote(const tw::price::Quote& quote) {
        if ( !quote.isChanged() )
            return;
        
        if ( !isEnabled() )
            return;
        
        clear();
        _p._idIsIcebergDetected = false;
        _p._idCurrIcebergInfo.clear();
        
        bool initialUpdate = _infos.empty();
        
        for ( size_t i = 0; i < tw::price::Quote::SIZE; ++i ) {
            if ( initialUpdate || quote.isLevelUpdate(i) ) {
                const tw::price::PriceLevel& level = quote._book[i];
                if ( level.isValid() ) {                
                    IcebergInfo& bid = getInfo(level._bid._price);
                    IcebergInfo& ask = getInfo(level._ask._price);
                    processUpdate(bid, level._bid._size, tw::channel_or::eOrderSide::kBuy);
                    processUpdate(ask, level._ask._size, tw::channel_or::eOrderSide::kSell);
                }
            }
        }
        
        if ( quote.isNormalTrade() ) {
            processTrade(getInfo(quote._trade._price), quote._trade._size);
            
            _p._idIsLastEventTrade = true;
            return;
        }
        
        detectIceberg(quote);        
        
        _p._idIsLastEventTrade = false;
        checkAndResetInfos(quote);
    }
    
private:
    IcebergInfo& getInfo(const TPrice& price) {
        TInfos::iterator iter = _infos.find(price);
        if ( iter == _infos.end() ) {
            iter = _infos.insert(TInfos::value_type(price, IcebergInfo())).first;
            iter->second._price = price;
        }
        
        return iter->second;
    }
    
    void processUpdate(IcebergInfo& info, const TSize& currSize, const tw::channel_or::eOrderSide side) {
        if ( side != info._side)
            resetState(info, side);        
        
        if ( !info._maxQty.isValid() || info._maxQty < currSize )
            info._maxQty = currSize;
    }
    
    void processTrade(IcebergInfo& info, const TSize& tradeSize) {
        if ( !info._volume.isValid() )
            info._volume = tradeSize;
        else
            info._volume += tradeSize;
    }
    
    void detectIceberg(const tw::price::Quote& quote) {
        if ( !_p._idIsLastEventTrade || !quote._trade.isValid() )
            return;
        
        clear();
        
        const tw::price::PriceLevel& level = quote._book[0];
        if ( level._bid._price == quote._trade._price )
            detectIceberg(getInfo(level._bid._price), level._bid._size, quote);
        else if ( level._ask._price == quote._trade._price )
            detectIceberg(getInfo(level._ask._price), level._ask._size, quote);
        else {
            _s << "quote._trade._price != bbo: quote=" << quote.toShortString();
            _p._idReason = _s.str();
        }
    }
    
    void detectIceberg(IcebergInfo& info, const TSize& currSize, const tw::price::Quote& quote) {
        if ( info._maxQty < _p._idMinMaxQty ) {
            _s << "info._maxQty < _p._minMaxQty: _p._idMinMaxQty=" << _p._idMinMaxQty << ",info=" << info.toString() << ",quote=" << quote.toShortString();
            _p._idReason = _s.str();
            return;
        }
        
        if ( info._volume < _p._idMinVolume ) {
            _s << "info.volume < _p._idMinVolume: _p._idMinVolume=" << _p._idMinVolume << ",info=" << info.toString() << ",quote=" << quote.toShortString();
            _p._idReason = _s.str();
            return;
        }
        
        TSize delta = (currSize == info._maxQty) ? TSize(1) : (info._maxQty-currSize);
        double ratio = info._volume.toDouble() / delta.toDouble();
        if ( ratio < _p._idRatio ) {
            _s << "ratio < _p._idRatio: _p._idRatio=" << _p._idRatio << ",ratio=" << ratio << ",info=" << info.toString() << ",quote=" << quote.toShortString();
            _p._idReason = _s.str();
            return;
        }
        
        _p._idIsIcebergDetected = true;
        _p._idCurrIcebergInfo = info;
        _s << "iceberg detected(ratio >= _p._idRatio): _p._idRatio=" << _p._idRatio << ",ratio=" << ratio << ",info=" << info.toString() << ",quote=" << quote.toShortString();
        _p._idReason = _s.str();
    }
    
    void checkAndResetInfos(const tw::price::Quote& quote) {
        const tw::price::PriceLevel& level = quote._book[0];
        if ( !level.isValid() )
            return;
        
        TInfos::iterator iter = _infos.begin();
        TInfos::iterator end = _infos.end();
        for ( ; iter != end; ++iter ) {            
            if ( level._bid._price-_p._idResetCountAwayTicks > iter->first )
                resetCounts(iter->second);
            else if ( level._bid._price >= iter->first && tw::channel_or::eOrderSide::kBuy != iter->second._side )
                resetState(iter->second, tw::channel_or::eOrderSide::kBuy);
            else if ( level._ask._price+_p._idResetCountAwayTicks < iter->first )
                resetCounts(iter->second);
            else if ( level._ask._price <= iter->first && tw::channel_or::eOrderSide::kSell != iter->second._side )
                resetState(iter->second, tw::channel_or::eOrderSide::kSell);
        }
    }
    
    void resetCounts(IcebergInfo& info) {
        info._maxQty.set(0);
        info._volume.set(0);
    }
    
    void resetState(IcebergInfo& info, tw::channel_or::eOrderSide side) {
        resetCounts(info);
        info._side = side;
    }
    
private:
    IcebergDetectorParamsWire& _p;
    tw::common_str_util::FastStream<1024*4> _s;
    std::map<TPrice, IcebergInfo> _infos;
};

} // common_trade
} // tw
