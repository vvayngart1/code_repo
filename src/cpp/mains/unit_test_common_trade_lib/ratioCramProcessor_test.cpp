#include <vector>

#include <tw/common_strat/defs.h>
#include <tw/generated/common_trade_blocks.h>
#include <tw/common_trade/ratioCramProcessor.h>
#include <tw/price/defs.h>

#include "../unit_test_price_lib/instr_helper.h"

#include <gtest/gtest.h>

namespace tw {
namespace common_trade {
    
typedef tw::price::Ticks TPrice;
typedef tw::price::Size TSize;

typedef RatioCramProcessor TImpl;
typedef RatioCramParamsWire TParams;


static TParams getParams(TPrice initialTicks,
                         TSize volFor,
                         float ratioFor,
                         TPrice forStopTicks) {
    TParams p;
    
    p._rcInitialTicks = initialTicks;
    p._rcVolFor = volFor;
    p._rcRatioFor = ratioFor;
    p._rcForStopTicks = forStopTicks;
    
    return p;
}

static bool testTop(const tw::price::PriceLevel& top,
                    const TPrice& bidPrice,
                    const TSize& bidSize,
                    const TPrice& askPrice,
                    const TSize& askSize) {
    EXPECT_EQ(top._bid._price, bidPrice);
    if ( top._bid._price != bidPrice )
        return false;
    
    EXPECT_EQ(top._bid._size, bidSize);
    if ( top._bid._size != bidSize )
        return false;
    
    EXPECT_EQ(top._ask._price, askPrice);
    if ( top._ask._price != askPrice )
        return false;
    
    EXPECT_EQ(top._ask._size, askSize);
    if ( top._ask._size != askSize )
        return false;
    
    return true;
}

TEST(CommonTradeLibTestSuit, RatioCramProcessor_test)
{
    TParams params;
    params._rcInitialTicks.set(-1);
    
    TImpl impl(params);
    ASSERT_TRUE(!impl.isEnabled());
    
    TParams& p = impl.getParams();
    p = getParams(TPrice(3), TSize(20), 2.5, TPrice(2));
    ASSERT_TRUE(impl.isEnabled());
    
    FillInfo info;
    TPrice icebergPrice;
    
    // Set quote
    //
    tw::price::QuoteStore::TQuote quote;
    quote.setInstrument(InstrHelper::get6CU2());
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, icebergPrice));
    ASSERT_EQ(info._stop, TPrice());
    ASSERT_EQ(p._rcReason, "!quote._book[0].isValid()");
    
    quote.setBid(TPrice(107), TSize(20), 0, 1);
    quote.setAsk(TPrice(108), TSize(20), 0, 1);
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, icebergPrice));
    ASSERT_TRUE(testTop(p._rcTop, TPrice(107), TSize(0), TPrice(108), TSize(0)));
    ASSERT_EQ(info._stop, TPrice());
    ASSERT_EQ(p._rcReason, "!quote.isNormalTrade()");
    
    quote.setTrade(TPrice(108), TSize(1));
    p._rcTop._ask._size.set(10);

    icebergPrice.set(107);
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, icebergPrice));
    ASSERT_TRUE(testTop(p._rcTop, TPrice(107), TSize(), TPrice(108), TSize(10)));
    ASSERT_EQ(info._stop, TPrice());
    ASSERT_EQ(p._rcReason, "quote._book[0]._bid._price == _p._rcIcebergPrice || quote._book[0]._ask._price == _p._rcIcebergPrice -- _p._rcIcebergPrice=107,quote._book[0]=1,20,107|108,20,1");
    
    icebergPrice.set(108);
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, icebergPrice));
    ASSERT_TRUE(testTop(p._rcTop, TPrice(107), TSize(), TPrice(108), TSize(10)));
    ASSERT_EQ(info._stop, TPrice());
    ASSERT_EQ(p._rcReason, "quote._book[0]._bid._price == _p._rcIcebergPrice || quote._book[0]._ask._price == _p._rcIcebergPrice -- _p._rcIcebergPrice=108,quote._book[0]=1,20,107|108,20,1");
    
    icebergPrice.set(106);
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, icebergPrice));
    ASSERT_TRUE(testTop(p._rcTop, TPrice(107), TSize(), TPrice(108), TSize(11)));
    ASSERT_EQ(info._stop, TPrice());
    ASSERT_EQ(p._rcReason, "info._fill._side == Unknown,info=InV,0,Unknown,,0,0,");
    
    // Test buys
    //
    
    // Set fill info
    //
    p._rcTop._bid._size.set(20);
    info._fill._side = tw::channel_or::eOrderSide::kBuy;
    info._fill._price.set(107);  
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, icebergPrice));
    ASSERT_TRUE(testTop(p._rcTop, TPrice(107), TSize(20), TPrice(108), TSize(12)));
    ASSERT_EQ(info._stop, TPrice());
    ASSERT_EQ(p._rcReason, "quote._book[0]._bid._price < info._fill._price+_p._rcInitialTicks");
    
    info._fill._price.set(104);
    info._stop.set(105);
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, icebergPrice));
    ASSERT_TRUE(testTop(p._rcTop, TPrice(107), TSize(20), TPrice(108), TSize(13)));
    ASSERT_EQ(info._stop, TPrice(105));
    ASSERT_EQ(p._rcReason, "stop requirements not calculated since info._stop.isValid() && info._stop >= stop -- stop=105,_p._rcTop=0,20,107|108,13,0,info=105,0,Unknown,,0,0,");
    
    info._stop.set(94);
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, icebergPrice));
    ASSERT_TRUE(testTop(p._rcTop, TPrice(107), TSize(20), TPrice(108), TSize(14)));
    ASSERT_EQ(info._stop, TPrice(94));
    ASSERT_EQ(p._rcReason, "_p._rcTop._ask._size < _p._rcVolFor -- _p._rcTop._ask._size=14,_p._rcVolFor=20,_p._rcTop=0,20,107|108,14,0");
    
    quote.setTrade(TPrice(108), TSize(20));
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, icebergPrice));
    ASSERT_TRUE(testTop(p._rcTop, TPrice(107), TSize(20), TPrice(108), TSize(34)));
    ASSERT_EQ(info._stop, TPrice(94));
    ASSERT_EQ(p._rcReason, "ratio < _p._rcRatioFor -- ratio=1.7,_p._rcRatioFor=2.5,_p._rcTop=0,20,107|108,34,0,info=94,0,Unknown,,0,0,");
    
    quote.setTrade(TPrice(108), TSize(20));
    ASSERT_TRUE(impl.isStopSlideTriggered(info, quote, icebergPrice));
    ASSERT_TRUE(testTop(p._rcTop, TPrice(107), TSize(20), TPrice(108), TSize(54)));
    ASSERT_EQ(info._stop, TPrice(105));
    ASSERT_EQ(p._rcReason, "!info._stop.isValid() || info._stop < stop for: stop=105,ratio=2.7,info=94,0,Unknown,,0,0,,_p._rcTop=0,20,107|108,54,0 -- quote=1,20,107|108,20,1");
    
    // Test resetting counts
    //
    quote.clearFlag();
    quote.setBid(TPrice(107), TSize(20), 0, 1);
    quote.setAsk(TPrice(109), TSize(20), 0, 1);

    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, icebergPrice));
    ASSERT_TRUE(testTop(p._rcTop, TPrice(107), TSize(0), TPrice(109), TSize(0)));
    ASSERT_EQ(info._stop, TPrice(105));
    ASSERT_EQ(p._rcReason, "!quote.isNormalTrade()");
    
    quote.setBid(TPrice(108), TSize(20), 0, 1);
    quote.setAsk(TPrice(109), TSize(20), 0, 1);
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, icebergPrice));
    ASSERT_TRUE(testTop(p._rcTop, TPrice(108), TSize(0), TPrice(109), TSize(0)));
    ASSERT_EQ(info._stop, TPrice(105));
    ASSERT_EQ(p._rcReason, "!quote.isNormalTrade()");
    
    quote.setBid(TPrice(107), TSize(20), 0, 1);
    quote.setAsk(TPrice(108), TSize(20), 0, 1);
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, icebergPrice));
    ASSERT_TRUE(testTop(p._rcTop, TPrice(107), TSize(0), TPrice(108), TSize(0)));
    ASSERT_EQ(info._stop, TPrice(105));
    ASSERT_EQ(p._rcReason, "!quote.isNormalTrade()");
    
    
    // Test sells
    //
    
    // Set fill info
    //
    info._fill._side = tw::channel_or::eOrderSide::kSell;
    info._fill._price.set(108);
    info._stop.clear();
    
    quote.setTrade(TPrice(107), TSize(1));
    
    p._rcTop._bid._size.set(11);
    p._rcTop._ask._size.set(20);
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, icebergPrice));
    ASSERT_TRUE(testTop(p._rcTop, TPrice(107), TSize(12), TPrice(108), TSize(20)));
    ASSERT_EQ(info._stop, TPrice());
    ASSERT_EQ(p._rcReason, "quote._book[0]._ask._price > info._fill._price-_p._rcInitialTicks");
    
    info._fill._price.set(111);
    info._stop.set(110);
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, icebergPrice));
    ASSERT_TRUE(testTop(p._rcTop, TPrice(107), TSize(13), TPrice(108), TSize(20)));
    ASSERT_EQ(info._stop, TPrice(110));
    ASSERT_EQ(p._rcReason, "stop requirements not calculated since info._stop.isValid() && info._stop <= stop -- stop=110,_p._rcTop=0,13,107|108,20,0,info=110,0,Unknown,,0,0,");
    
    info._stop.set(121);
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, icebergPrice));
    ASSERT_TRUE(testTop(p._rcTop, TPrice(107), TSize(14), TPrice(108), TSize(20)));
    ASSERT_EQ(info._stop, TPrice(121));
    ASSERT_EQ(p._rcReason, "_p._rcTop._bid._size < _p._rcVolFor -- _p._rcTop._bid._size=14,_p._rcVolFor=20,_p._rcTop=0,14,107|108,20,0");
    
    quote.setTrade(TPrice(107), TSize(20));
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, icebergPrice));
    ASSERT_TRUE(testTop(p._rcTop, TPrice(107), TSize(34), TPrice(108), TSize(20)));
    ASSERT_EQ(info._stop, TPrice(121));
    ASSERT_EQ(p._rcReason, "ratio < _p._rcRatioFor -- ratio=1.7,_p._rcRatioFor=2.5,_p._rcTop=0,34,107|108,20,0,info=121,0,Unknown,,0,0,");
    
    quote.setTrade(TPrice(107), TSize(20));
    ASSERT_TRUE(impl.isStopSlideTriggered(info, quote, icebergPrice));
    ASSERT_TRUE(testTop(p._rcTop, TPrice(107), TSize(54), TPrice(108), TSize(20)));
    ASSERT_EQ(info._stop, TPrice(110));
    ASSERT_EQ(p._rcReason, "!info._stop.isValid() || info._stop > stop for: stop=110,ratio=2.7,info=121,0,Unknown,,0,0,,_p._rcTop=0,54,107|108,20,0 -- quote=1,20,107|108,20,1");
    

}

} // common_trade
} // tw

