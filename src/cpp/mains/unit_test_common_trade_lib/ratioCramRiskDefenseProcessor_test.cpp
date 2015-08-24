#include <vector>

#include <tw/common_strat/defs.h>
#include <tw/generated/common_trade_blocks.h>
#include <tw/common_trade/ratioCramRiskDefenseProcessor.h>
#include <tw/price/defs.h>

#include "../unit_test_price_lib/instr_helper.h"

#include <gtest/gtest.h>

namespace tw {
namespace common_trade {
    
typedef tw::price::Ticks TPrice;
typedef tw::price::Size TSize;

typedef RatioCramRiskDefenseProcessor TImpl;
typedef RatioCramRiskDefenseParamsWire TParams;


static TParams getParams(TPrice initialTicks,
                         TSize volAgainst,
                         float ratioAgainst,
                         TPrice againstStopTicks) {
    TParams p;
    
    p._rcrdInitialTicks = initialTicks;
    p._rcrdVolAgainst = volAgainst;
    p._rcrdRatioAgainst = ratioAgainst;
    p._rcrdAgainstStopTicks = againstStopTicks;
    
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

TEST(CommonTradeLibTestSuit, RatioCramRiskDefenseProcessor_test)
{
    TParams params;
    params._rcrdVolAgainst.set(-1);
    
    TImpl impl(params);
    ASSERT_TRUE(!impl.isEnabled());
    
    TParams& p = impl.getParams();
    p = getParams(TPrice(1), TSize(20), 2.5, TPrice(2));
    ASSERT_TRUE(impl.isEnabled());
    
    FillInfo info;
    
    // Set quote
    //
    tw::price::QuoteStore::TQuote quote;
    quote.setInstrument(InstrHelper::get6CU2());
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote));
    ASSERT_EQ(info._stop, TPrice());
    ASSERT_EQ(p._rcrdReason, "!quote._book[0].isValid()");
    
    quote.setBid(TPrice(107), TSize(20), 0, 1);
    quote.setAsk(TPrice(108), TSize(20), 0, 1);
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote));
    ASSERT_TRUE(testTop(p._rcrdTop, TPrice(107), TSize(0), TPrice(108), TSize(0)));
    ASSERT_EQ(info._stop, TPrice());
    ASSERT_EQ(p._rcrdReason, "!quote.isNormalTrade()");
    
    quote.setTrade(TPrice(107), TSize(1));
    
    p._rcrdTop._bid._size.set(10);
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote));
    ASSERT_TRUE(testTop(p._rcrdTop, TPrice(107), TSize(11), TPrice(108), TSize()));
    ASSERT_EQ(info._stop, TPrice());
    ASSERT_EQ(p._rcrdReason, "info._fill._side == Unknown,info=InV,0,Unknown,,0,0,");
    
    // Test buys
    //
    
    // Set fill info
    //
    p._rcrdTop._ask._size.set(20);
    info._fill._side = tw::channel_or::eOrderSide::kBuy;
    info._fill._price.set(107);  
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote));
    ASSERT_TRUE(testTop(p._rcrdTop, TPrice(107), TSize(12), TPrice(108), TSize(20)));
    ASSERT_EQ(info._stop, TPrice());
    ASSERT_EQ(p._rcrdReason, "quote._book[0]._bid._price > info._fill._price-_p._rcrdInitialTicks");
    
    info._fill._price.set(108);
    info._stop.set(107);
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote));
    ASSERT_TRUE(testTop(p._rcrdTop, TPrice(107), TSize(13), TPrice(108), TSize(20)));
    ASSERT_EQ(info._stop, TPrice(107));
    ASSERT_EQ(p._rcrdReason, "stop requirements not calculated since info._stop.isValid() && info._stop >= stop -- stop=105,_p._rcrdTop=0,13,107|108,20,0,info=107,0,Unknown,,0,0,");
    
    info._stop.set(94);
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote));
    ASSERT_TRUE(testTop(p._rcrdTop, TPrice(107), TSize(14), TPrice(108), TSize(20)));
    ASSERT_EQ(info._stop, TPrice(94));
    ASSERT_EQ(p._rcrdReason, "_p._rcrdTop._bid._size < _p._rcrdVolAgainst -- _p._rcrdTop._bid._size=14,_p._rcrdVolAgainst=20,_p._rcrdTop=0,14,107|108,20,0");
    
    quote.setTrade(TPrice(107), TSize(20));
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote));
    ASSERT_TRUE(testTop(p._rcrdTop, TPrice(107), TSize(34), TPrice(108), TSize(20)));
    ASSERT_EQ(info._stop, TPrice(94));
    ASSERT_EQ(p._rcrdReason, "ratio < _p._rcrdRatioAgainst -- ratio=1.7,_p._rcrdRatioAgainst=2.5,_p._rcrdTop=0,34,107|108,20,0,info=94,0,Unknown,,0,0,");
    
    quote.setTrade(TPrice(107), TSize(20));
    ASSERT_TRUE(impl.isStopSlideTriggered(info, quote));
    ASSERT_TRUE(testTop(p._rcrdTop, TPrice(107), TSize(54), TPrice(108), TSize(20)));
    ASSERT_EQ(info._stop, TPrice(105));
    ASSERT_EQ(p._rcrdReason, "!info._stop.isValid() || info._stop < stop against: stop=105,ratio=2.7,info=94,0,Unknown,,0,0,,_p._rcrdTop=0,54,107|108,20,0 -- quote=1,20,107|108,20,1");
    
    // Test resetting counts
    //
    quote.clearFlag();
    quote.setBid(TPrice(107), TSize(20), 0, 1);
    quote.setAsk(TPrice(109), TSize(20), 0, 1);

    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote));
    ASSERT_TRUE(testTop(p._rcrdTop, TPrice(107), TSize(0), TPrice(109), TSize(0)));
    ASSERT_EQ(info._stop, TPrice(105));
    ASSERT_EQ(p._rcrdReason, "!quote.isNormalTrade()");
    
    quote.setBid(TPrice(108), TSize(20), 0, 1);
    quote.setAsk(TPrice(109), TSize(20), 0, 1);
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote));
    ASSERT_TRUE(testTop(p._rcrdTop, TPrice(108), TSize(0), TPrice(109), TSize(0)));
    ASSERT_EQ(info._stop, TPrice(105));
    ASSERT_EQ(p._rcrdReason, "!quote.isNormalTrade()");
    
    quote.setBid(TPrice(107), TSize(20), 0, 1);
    quote.setAsk(TPrice(108), TSize(20), 0, 1);
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote));
    ASSERT_TRUE(testTop(p._rcrdTop, TPrice(107), TSize(0), TPrice(108), TSize(0)));
    ASSERT_EQ(info._stop, TPrice(105));
    ASSERT_EQ(p._rcrdReason, "!quote.isNormalTrade()");
    
    
    // Test sells
    //
    
    // Set fill info
    //
    info._fill._side = tw::channel_or::eOrderSide::kSell;
    info._fill._price.set(108);
    info._stop.clear();
    
    quote.setTrade(TPrice(108), TSize(1));
    
    p._rcrdTop._bid._size.set(20);
    p._rcrdTop._ask._size.set(11);
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote));
    ASSERT_TRUE(testTop(p._rcrdTop, TPrice(107), TSize(20), TPrice(108), TSize(12)));
    ASSERT_EQ(info._stop, TPrice());
    ASSERT_EQ(p._rcrdReason, "quote._book[0]._ask._price < info._fill._price+_p._rcrdInitialTicks");
    
    info._fill._price.set(107);
    info._stop.set(108);
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote));
    ASSERT_TRUE(testTop(p._rcrdTop, TPrice(107), TSize(20), TPrice(108), TSize(13)));
    ASSERT_EQ(info._stop, TPrice(108));
    ASSERT_EQ(p._rcrdReason, "stop requirements not calculated since info._stop.isValid() && info._stop <= stop -- stop=110,_p._rcrdTop=0,20,107|108,13,0,info=108,0,Unknown,,0,0,");
    
    info._stop.set(121);
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote));
    ASSERT_TRUE(testTop(p._rcrdTop, TPrice(107), TSize(20), TPrice(108), TSize(14)));
    ASSERT_EQ(info._stop, TPrice(121));
    ASSERT_EQ(p._rcrdReason, "_p._rcrdTop._ask._size < _p._rcrdVolAgainst -- _p._rcrdTop._ask._size=14,_p._rcrdVolAgainst=20,_p._rcrdTop=0,20,107|108,14,0");
    
    quote.setTrade(TPrice(108), TSize(20));
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote));
    ASSERT_TRUE(testTop(p._rcrdTop, TPrice(107), TSize(20), TPrice(108), TSize(34)));
    ASSERT_EQ(info._stop, TPrice(121));
    ASSERT_EQ(p._rcrdReason, "ratio < _p._rcrdRatioAgainst -- ratio=1.7,_p._rcrdRatioAgainst=2.5,_p._rcrdTop=0,20,107|108,34,0,info=121,0,Unknown,,0,0,");
    
    quote.setTrade(TPrice(108), TSize(20));
    ASSERT_TRUE(impl.isStopSlideTriggered(info, quote));
    ASSERT_TRUE(testTop(p._rcrdTop, TPrice(107), TSize(20), TPrice(108), TSize(54)));
    ASSERT_EQ(info._stop, TPrice(110));
    ASSERT_EQ(p._rcrdReason, "!info._stop.isValid() || info._stop > stop against: stop=110,ratio=2.7,info=121,0,Unknown,,0,0,,_p._rcrdTop=0,20,107|108,54,0 -- quote=1,20,107|108,20,1");
    

}

} // common_trade
} // tw

