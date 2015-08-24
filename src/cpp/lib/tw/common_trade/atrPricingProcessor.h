#pragma once

#include <tw/common/defs.h>
#include <tw/price/quote.h>
#include <tw/log/defs.h>
#include <tw/common_trade/bars_manager.h>

namespace tw {
namespace common_trade {
    
typedef tw::common_trade::TBar TBar;
typedef tw::common_trade::TBars TBars;
typedef tw::common_trade::TBarPatterns TBarPatterns;
typedef tw::common_trade::BarPattern TBarPattern;

class ATRPricingProcessor {
public:
    ATRPricingProcessor(ATRPricingParamsWire& p) : _p(p) {
    }
    
    void clear() {
        _s.clear();
    }
    
public:
    ATRPricingParamsWire& getParams() {
        return _p;
    }
    
    const ATRPricingParamsWire& getParams() const {
        return _p;
    }
    
    bool isExitConfigured() const {
        return (0.0 < _p._atrMultiplierExit);
    }
    
public:
    bool calcPriceOffsets(const float& atr) {
        double atrExitOrderPriceOffset = _p._atrMultiplierExit * atr;
        _p._atrExitOrderPriceOffset.set(static_cast<int32_t>(atrExitOrderPriceOffset));
        if ( 0 == _p._atrExitOrderPriceOffset.get() && 0.0 != _p._atrMultiplierExit ) {
            _s << "0 == _p._atrExitOrderPriceOffset.get() && 0.0 != _p._atrMultiplierExit";
            return false;
        }

        double atrEntryOrderPriceOffset = _p._atrMultiplierEntry * atr;
        _p._atrEntryOrderPriceOffset.set(static_cast<int32_t>(atrEntryOrderPriceOffset));
        
        return true;
    }
    
    tw::price::Ticks getEnterOrderPrice(const tw::price::Ticks& price) {
        if ( 0.0 == _p._atrMultiplierEntry )
            return price;
        
        return (tw::channel_or::eOrderSide::kBuy == _p._atrOrderSide) ? (price-_p._atrEntryOrderPriceOffset) : (price+_p._atrEntryOrderPriceOffset);
    }
    
    tw::price::Ticks getExitOrderProfitTicks() {
        if ( 0.0 == _p._atrMultiplierExit )
            return tw::price::Ticks(0);
        
        return _p._atrExitOrderPriceOffset;
    }
    
    bool shouldModifyEnterOrder(const tw::price::Ticks& price, tw::price::Ticks& newPrice) const {
        if ( 0.0 == _p._atrMultiplierEntry )
            return true;
        
        if ( eATRPricingPayUpStyle::kDynamic != _p._atrPayUpStyleEntry )
            return false;
        
        newPrice = (tw::channel_or::eOrderSide::kBuy == _p._atrOrderSide) ? (newPrice-_p._atrEntryOrderPriceOffset) : (newPrice+_p._atrEntryOrderPriceOffset);
        if ( ((tw::channel_or::eOrderSide::kBuy == _p._atrOrderSide) && (newPrice <= price)) || ((tw::channel_or::eOrderSide::kSell == _p._atrOrderSide) && (newPrice >= price)) )
            return false;
        
        return true;
    }
    
    bool shouldModifyExitOrder(const tw::price::Ticks& price, tw::channel_or::eOrderSide side, tw::price::Ticks& newPrice) const {
        if ( 0.0 == _p._atrMultiplierExit )
            return false;
        
        if ( eATRPricingPayUpStyle::kDynamic != _p._atrPayUpStyleExit )
            return false;
        
        newPrice = (tw::channel_or::eOrderSide::kBuy == side) ? (newPrice-_p._atrExitOrderPriceOffset) : (newPrice+_p._atrExitOrderPriceOffset);
        if ( ((tw::channel_or::eOrderSide::kBuy == side) && (newPrice <= price)) || ((tw::channel_or::eOrderSide::kSell == side) && (newPrice >= price)) )
            return false;
        
        return true;
    }
    
protected:
    ATRPricingParamsWire& _p;
    tw::common_str_util::TFastStream _s;
};

} // common_trade
} // tw

