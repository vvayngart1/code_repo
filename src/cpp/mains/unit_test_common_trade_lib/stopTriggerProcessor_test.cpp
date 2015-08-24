#include <vector>

#include <tw/common_strat/defs.h>
#include <tw/generated/common_trade_blocks.h>
#include <tw/common_trade/stopTriggerProcessor.h>
#include <tw/price/defs.h>

#include "../unit_test_price_lib/instr_helper.h"

#include <gtest/gtest.h>

namespace tw {
namespace common_trade {
    
const double EPSILON = 0.0001;

typedef tw::price::Ticks TPrice;
typedef tw::price::Size TSize;

typedef stopTriggerProcessor TImpl;

TEST(CommonTradeLibTestSuit, stopTriggerProcessor_test_isStopTriggered)
{
    StopLossTriggerParamsWire params;
    std::string reason;
    
    FillInfo info;    
    info._fill._price.set(100);    
    
    TImpl impl(params);
    StopLossTriggerParams& p = impl.getParams();
    ASSERT_TRUE(!impl.isEnabled());
    
    p._p._minStopTriggerQty.set(10);
    
    // Set quote
    //
    tw::price::QuoteStore::TQuote quote;
    quote.setInstrument(InstrHelper::get6CU2());
    quote.setBid(TPrice(90), TSize(20), 0, 1);
    quote.setAsk(TPrice(110), TSize(30), 0, 1);        
    
    // Test buy entry order stop trailing
    //
    info._fill._side = tw::channel_or::eOrderSide::kBuy;
    info._stop.set(105);
    
    ASSERT_TRUE(!impl.isStopTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(105));
    ASSERT_EQ(p._stopTriggerQty, tw::price::Size(0));
    
    quote.setTrade(TPrice(106), TSize(4));
    info._stop.set(106);
    
    ASSERT_TRUE(!impl.isStopTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(106));
    ASSERT_EQ(p._stopTriggerQty, tw::price::Size(4));
    ASSERT_EQ(p._stopPrice, tw::price::Ticks(106));
    
    quote.setTrade(TPrice(105), TSize(3));
    info._stop.set(105);
    
    ASSERT_TRUE(!impl.isStopTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(105));
    ASSERT_EQ(p._stopTriggerQty, tw::price::Size(3));
    ASSERT_EQ(p._stopPrice, tw::price::Ticks(105));
    
    quote.setTrade(TPrice(105), TSize(1));
    
    ASSERT_TRUE(!impl.isStopTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(105));
    ASSERT_EQ(p._stopTriggerQty, tw::price::Size(4));
    ASSERT_EQ(p._stopPrice, tw::price::Ticks(105));
    
    quote.setTrade(TPrice(106), TSize(1));
    
    ASSERT_TRUE(!impl.isStopTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(105));
    ASSERT_EQ(p._stopTriggerQty, tw::price::Size(0));
    ASSERT_EQ(p._stopPrice, tw::price::Ticks(105));
    
    quote.setTrade(TPrice(105), TSize(2));
    
    ASSERT_TRUE(!impl.isStopTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(105));
    ASSERT_EQ(p._stopTriggerQty, tw::price::Size(2));
    ASSERT_EQ(p._stopPrice, tw::price::Ticks(105));
    
    quote.setTrade(TPrice(104), TSize(2));
    
    ASSERT_TRUE(impl.isStopTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(105));
    ASSERT_EQ(p._stopTriggerQty, tw::price::Size(0));
    ASSERT_EQ(p._stopPrice, tw::price::Ticks(105));
    
    
    quote.setTrade(TPrice(105), TSize(3));
    
    ASSERT_TRUE(!impl.isStopTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(105));
    ASSERT_EQ(p._stopTriggerQty, tw::price::Size(3));
    ASSERT_EQ(p._stopPrice, tw::price::Ticks(105));
    
    quote.setTrade(TPrice(105), TSize(1));
    
    ASSERT_TRUE(!impl.isStopTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(105));
    ASSERT_EQ(p._stopTriggerQty, tw::price::Size(4));
    ASSERT_EQ(p._stopPrice, tw::price::Ticks(105));
    
    quote.setTrade(TPrice(105), TSize(6));
    
    ASSERT_TRUE(impl.isStopTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(105));
    ASSERT_EQ(p._stopTriggerQty, tw::price::Size(0));
    ASSERT_EQ(p._stopPrice, tw::price::Ticks(105));
    
    
    // Test sell entry order stop trailing
    //
    info._fill._side = tw::channel_or::eOrderSide::kSell;
    info._stop.set(95);
    quote.clearFlag();
    
    ASSERT_TRUE(!impl.isStopTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(95));
    ASSERT_EQ(p._stopTriggerQty, tw::price::Size(0));
    ASSERT_EQ(p._stopPrice, tw::price::Ticks(105));
    
    quote.setTrade(TPrice(95), TSize(3));
    
    ASSERT_TRUE(!impl.isStopTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(95));
    ASSERT_EQ(p._stopTriggerQty, tw::price::Size(3));
    ASSERT_EQ(p._stopPrice, tw::price::Ticks(95));
    
    quote.setTrade(TPrice(95), TSize(1));
    
    ASSERT_TRUE(!impl.isStopTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(95));
    ASSERT_EQ(p._stopTriggerQty, tw::price::Size(4));    
    ASSERT_EQ(p._stopPrice, tw::price::Ticks(95));
    
    quote.setTrade(TPrice(94), TSize(1));
    
    ASSERT_TRUE(!impl.isStopTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(95));
    ASSERT_EQ(p._stopTriggerQty, tw::price::Size(0));
    ASSERT_EQ(p._stopPrice, tw::price::Ticks(95));
    
    quote.setTrade(TPrice(95), TSize(2));
    
    ASSERT_TRUE(!impl.isStopTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(95));
    ASSERT_EQ(p._stopTriggerQty, tw::price::Size(2));
    ASSERT_EQ(p._stopPrice, tw::price::Ticks(95));    
    
    quote.setTrade(TPrice(96), TSize(2));
    
    ASSERT_TRUE(impl.isStopTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(95));
    ASSERT_EQ(p._stopTriggerQty, tw::price::Size(0));
    ASSERT_EQ(p._stopPrice, tw::price::Ticks(95));
    
    quote.setTrade(TPrice(95), TSize(3));
    
    ASSERT_TRUE(!impl.isStopTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(95));
    ASSERT_EQ(p._stopTriggerQty, tw::price::Size(3));
    ASSERT_EQ(p._stopPrice, tw::price::Ticks(95));
    
    quote.setTrade(TPrice(95), TSize(1));
    
    ASSERT_TRUE(!impl.isStopTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(95));
    ASSERT_EQ(p._stopTriggerQty, tw::price::Size(4));
    ASSERT_EQ(p._stopPrice, tw::price::Ticks(95));
    
    quote.setTrade(TPrice(95), TSize(6));
    
    ASSERT_TRUE(impl.isStopTriggered(info, quote, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(95));
    ASSERT_EQ(p._stopTriggerQty, tw::price::Size(0));
    ASSERT_EQ(p._stopPrice, tw::price::Ticks(95));
    
}



} // common_trade
} // tw
