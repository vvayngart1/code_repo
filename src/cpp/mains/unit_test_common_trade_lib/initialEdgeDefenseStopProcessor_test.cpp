#include <vector>

#include <tw/common_strat/defs.h>
#include <tw/generated/common_trade_blocks.h>
#include <tw/common_trade/initialEdgeDefenseStopProcessor.h>
#include <tw/price/defs.h>

#include "../unit_test_price_lib/instr_helper.h"

#include <gtest/gtest.h>

namespace tw {
namespace common_trade {
    
const double EPSILON = 0.0001;

typedef tw::price::Ticks TPrice;
typedef tw::price::Size TSize;

typedef InitialEdgeDefenseStopProcessor TImpl;
typedef InitialEdgeDefenseStopLossParamsWire TParams;

static TParams getParams() {
    TParams p;
    
    p._slideStopOnProfitTicks1.set(1);
    p._slideStopPayupTicks1.set(4);
    p._slideStopOnProfitTicks2.set(2);
    p._slideStopPayupTicks2.set(3);
    
    return p;
}

TEST(CommonTradeLibTestSuit, initialEdgeDefenseStopProcessor_test_buy)
{
    std::string reason;
    TParams params;
    
    TImpl impl(params);
    TParams& p = impl.getParams();
    p = getParams();
    
    // Set quote
    //
    tw::price::QuoteStore::TQuote quote;
    quote.setInstrument(InstrHelper::get6CU2());
    
    // Set fill info
    //
    FillInfo info;    
    info._fill._side = tw::channel_or::eOrderSide::kBuy;
    info._fill._price.set(100);        
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "!quote._book[0].isValid()");    
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setBid(TPrice(100), TSize(20), 0, 1);
    quote.setAsk(TPrice(101), TSize(20), 0, 1);
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "info._fill._price + _p._slideStopOnProfitTicks1 > quote._book[0]._bid._price");
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setBid(TPrice(101), TSize(20), 0, 1);
    quote.setAsk(TPrice(102), TSize(20), 0, 1);
    
    ASSERT_TRUE(impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "isStopSlideTriggered(!info._stop.isValid() || info._stop < stop) for: fill=InV,0,Unknown,,0,0, -- quote=1,20,101|102,20,1 :: info._fill._price + _p._slideStopOnProfitTicks1 <= quote._book[0]._bid._price");
    ASSERT_EQ(info._stop, TPrice(96));
    
    quote.setBid(TPrice(101), TSize(20), 0, 1);
    quote.setAsk(TPrice(102), TSize(20), 0, 1);
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "info._stop.isValid() && info._stop >= stop");
    ASSERT_EQ(info._stop, TPrice(96));
        
    quote.setBid(TPrice(102), TSize(20), 0, 1);
    quote.setAsk(TPrice(103), TSize(20), 0, 1);
    
    ASSERT_TRUE(impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "isStopSlideTriggered(!info._stop.isValid() || info._stop < stop) for: fill=96,0,Unknown,,0,0, -- quote=1,20,102|103,20,1 :: info._fill._price + _p._slideStopOnProfitTicks2 <= quote._book[0]._bid._price");
    ASSERT_EQ(info._stop, TPrice(97));
   
    quote.setBid(TPrice(105), TSize(20), 0, 1);
    quote.setAsk(TPrice(106), TSize(20), 0, 1);
    info._stop.set(99);
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "info._stop.isValid() && info._stop >= info._fill._price - _p._slideStopPayupTicks2");
    ASSERT_EQ(info._stop, TPrice(99));
    
    quote.clear();
    
}

TEST(CommonTradeLibTestSuit, initialEdgeDefenseStopProcessor_test_sell)
{
    std::string reason;
    TParams params;
    
    TImpl impl(params);
    TParams& p = impl.getParams();
    p = getParams();
    
    // Set quote
    //
    tw::price::QuoteStore::TQuote quote;
    quote.setInstrument(InstrHelper::get6CU2());
    
    // Set fill info
    //
    FillInfo info;    
    info._fill._side = tw::channel_or::eOrderSide::kSell;
    info._fill._price.set(100);        
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "!quote._book[0].isValid()");    
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setBid(TPrice(99), TSize(20), 0, 1);
    quote.setAsk(TPrice(100), TSize(20), 0, 1);
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "info._fill._price - _p._slideStopOnProfitTicks1 < quote._book[0]._ask._price");
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setBid(TPrice(98), TSize(20), 0, 1);
    quote.setAsk(TPrice(99), TSize(20), 0, 1);
    
    ASSERT_TRUE(impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "isStopSlideTriggered(!info._stop.isValid() || info._stop > stop) for: fill=InV,0,Unknown,,0,0, -- quote=1,20,98|99,20,1 :: info._fill._price - _p._slideStopOnProfitTicks1 >= quote._book[0]._ask._price");
    ASSERT_EQ(info._stop, TPrice(104));
    
    quote.setBid(TPrice(98), TSize(20), 0, 1);
    quote.setAsk(TPrice(99), TSize(20), 0, 1);
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "info._stop.isValid() && info._stop <= stop");
    ASSERT_EQ(info._stop, TPrice(104));
        
    quote.setBid(TPrice(97), TSize(20), 0, 1);
    quote.setAsk(TPrice(98), TSize(20), 0, 1);
    
    ASSERT_TRUE(impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "isStopSlideTriggered(!info._stop.isValid() || info._stop > stop) for: fill=104,0,Unknown,,0,0, -- quote=1,20,97|98,20,1 :: info._fill._price - _p._slideStopOnProfitTicks2 >= quote._book[0]._ask._price");
    ASSERT_EQ(info._stop, TPrice(103));
   
    quote.setBid(TPrice(94), TSize(20), 0, 1);
    quote.setAsk(TPrice(95), TSize(20), 0, 1);
    info._stop.set(101);
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "info._stop.isValid() && info._stop <= info._fill._price + _p._slideStopPayupTicks2");
    ASSERT_EQ(info._stop, TPrice(101));
    
    quote.clear();
    
}


} // common_trade
} // tw

