#pragma once

#include <tw/common/defs.h>
#include <tw/common/settings.h>
#include <tw/generated/instrument.h>
#include <tw/price/quote.h>

namespace tw {
namespace channel_pf {
    
class IChannelImpl {
public:
    IChannelImpl() {}
    virtual ~IChannelImpl() {}
    
public:
    virtual bool init(const tw::common::Settings& settings) = 0;
    virtual bool start() = 0;
    virtual void stop() = 0;
    
    virtual bool subscribe(tw::instr::InstrumentConstPtr instrument) = 0;    
    virtual bool unsubscribe(tw::instr::InstrumentConstPtr instrument) = 0;
    
public:
    void checkGappedQuote(const tw::price::Ticks& newPrice,
                          const tw::price::Ticks& oldPrice,
                          tw::price::Quote& quote,
                          uint32_t gap) {
        if ( newPrice.isGapped(oldPrice, gap) )
            quote._status = tw::price::Quote::kPriceGap;
        
        if ( newPrice.isGapped(quote._trade._price, gap) )
            quote._status = tw::price::Quote::kPriceGap;
    }
    
    void checkGappedTrade(const tw::price::Ticks& tradePrice,
                          tw::price::Quote& quote,
                          uint32_t gap) {
        if ( tradePrice.isGapped(quote._book[0]._bid._price, gap) )
            quote._status = tw::price::Quote::kPriceGap;
        
        if ( tradePrice.isGapped(quote._book[0]._ask._price, gap) )
            quote._status = tw::price::Quote::kPriceGap;
    }
};
	
} // channel_pf
} // tw

