#include <vector>

#include <tw/common_strat/defs.h>
#include <tw/generated/common_trade_blocks.h>
#include <tw/common_trade/trailingStopProcessor.h>
#include <tw/price/defs.h>

#include "../unit_test_price_lib/instr_helper.h"

#include <gtest/gtest.h>

namespace tw {
namespace common_trade {
    
const double EPSILON = 0.0001;

typedef tw::price::Ticks TPrice;
typedef tw::price::Size TSize;

typedef TrailingStopProcessor TImpl;

TEST(CommonTradeLibTestSuit, trailingStopProcessor_test_isStopSlideTriggered)
{
    TrailingStopLossParamsWire params;
    std::string reason;
    
    FillInfo info;    
    info._fill._price.set(100);
    
    TImpl impl(params);
    TrailingStopLossParamsWire& p = impl.getParams();
    
    ASSERT_TRUE(!impl.isEnabled());
    
    // Set quote
    //
    tw::price::QuoteStore::TQuote quote;
    quote.setInstrument(InstrHelper::get6CU2());
    
    // Test buy entry order stop trailing
    //
    info._fill._side = tw::channel_or::eOrderSide::kBuy;
    info._stop.set(95);
    
    p._beginTrailingStopTicks.set(5);
    p._trailingStopOffsetTicks.set(1);
    
    quote.setBid(TPrice(104), TSize(20), 0, 1);
    quote.setAsk(TPrice(105), TSize(30), 0, 1);    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(95));
    
    
    quote.setBid(TPrice(105), TSize(20), 0, 1);
    quote.setAsk(TPrice(106), TSize(30), 0, 1);    
    ASSERT_TRUE(impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(104));
    
    
    quote.setBid(TPrice(103), TSize(20), 0, 1);
    quote.setAsk(TPrice(104), TSize(30), 0, 1);    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(104));
    
    
    quote.setBid(TPrice(107), TSize(20), 0, 1);
    quote.setAsk(TPrice(108), TSize(30), 0, 1);    
    ASSERT_TRUE(impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(106));
    
    
    quote.setBid(TPrice(104), TSize(20), 0, 1);
    quote.setAsk(TPrice(105), TSize(30), 0, 1);    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(106));
    
    
    p._beginTrailingStopTicks.set(0);
    quote.setBid(TPrice(108), TSize(20), 0, 1);
    quote.setAsk(TPrice(109), TSize(30), 0, 1);    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(106));
    
    
    p._beginTrailingStopTicks.set(5);
    quote.setBid(TPrice(108), TSize(20), 0, 1);
    quote.setAsk(TPrice(109), TSize(30), 0, 1);    
    ASSERT_TRUE(impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(107));
    
    
    // Test sell entry order stop trailing
    //
    info._fill._side = tw::channel_or::eOrderSide::kSell;
    info._stop.set(105);
    
    p._beginTrailingStopTicks.set(5);
    p._trailingStopOffsetTicks.set(1);
    
    quote.setBid(TPrice(95), TSize(20), 0, 1);
    quote.setAsk(TPrice(96), TSize(30), 0, 1);    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(105));
    
    
    quote.setBid(TPrice(94), TSize(20), 0, 1);
    quote.setAsk(TPrice(95), TSize(30), 0, 1);    
    ASSERT_TRUE(impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(96));
    
    
    quote.setBid(TPrice(96), TSize(20), 0, 1);
    quote.setAsk(TPrice(97), TSize(30), 0, 1);    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(96));
    
    
    quote.setBid(TPrice(92), TSize(20), 0, 1);
    quote.setAsk(TPrice(93), TSize(30), 0, 1);    
    ASSERT_TRUE(impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(94));
    
    
    quote.setBid(TPrice(95), TSize(20), 0, 1);
    quote.setAsk(TPrice(96), TSize(30), 0, 1);    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(94));
    
    
    p._beginTrailingStopTicks.set(0);
    quote.setBid(TPrice(91), TSize(20), 0, 1);
    quote.setAsk(TPrice(92), TSize(30), 0, 1);    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(94));
    
    
    p._beginTrailingStopTicks.set(5);
    quote.setBid(TPrice(91), TSize(20), 0, 1);
    quote.setAsk(TPrice(92), TSize(30), 0, 1);    
    ASSERT_TRUE(impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(93));
    
}


} // common_trade
} // tw
