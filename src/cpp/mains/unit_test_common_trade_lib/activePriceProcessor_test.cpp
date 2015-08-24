#include <vector>

#include <tw/common_strat/defs.h>
#include <tw/generated/common_trade_blocks.h>
#include <tw/common_trade/activePriceProcessor.h>
#include <tw/price/defs.h>

#include "../unit_test_price_lib/instr_helper.h"

#include <gtest/gtest.h>

namespace tw {
namespace common_trade {
    
const double EPSILON = 0.0001;

typedef tw::price::Ticks TPrice;
typedef tw::price::Size TSize;

typedef ActivePriceProcessor TImpl;
typedef TImpl::TInfo TInfo;

TEST(CommonTradeLibTestSuit, activePriceProcessor_test_calcPrice_buy)
{
    ActivePriceParamsWire params;
    
    TImpl impl(params);
    ActivePriceParamsWire& p = impl.getParams();
    
    ASSERT_TRUE(!impl.isEnabled());
    
    TInfo info;
    std::string reason;
    
    // Set quote
    //
    tw::price::QuoteStore::TQuote quote;
    quote.setInstrument(InstrHelper::getNQU2());
    
    // Test buys
    //
    
    // Test not enabled
    //
    info = impl.calcPrice(quote, true, reason);
    ASSERT_TRUE(!info.first.isValid());
    ASSERT_TRUE(!info.second);
    ASSERT_EQ(reason, "");
    
    quote.setBid(TPrice(104), TSize(20), 0, 1);
    info = impl.calcPrice(quote, true, reason);
    ASSERT_TRUE(!info.first.isValid());
    ASSERT_TRUE(!info.second);
    ASSERT_EQ(reason, "");
    
    // Test enabled logic
    //
    p._leanQty.set(100);
    
    // Test active side is invalid
    //
    quote.setBid(TPrice(104), TSize(20), 0, 1);
    info = impl.calcPrice(quote, true, reason);
    ASSERT_EQ(info.first, TPrice(105));
    ASSERT_TRUE(info.second);
    ASSERT_EQ(reason, "!active.isValid()");
    
    // Test active side is gapped
    //
    quote.setAsk(TPrice(106), TSize(20), 0, 1);
    info = impl.calcPrice(quote, true, reason);
    ASSERT_EQ(info.first, TPrice(105));
    ASSERT_TRUE(info.second);
    ASSERT_EQ(reason, "activePrice != active._price: active=1,20,106, activePrice=105");
    
    // Test paranoid qty
    //
    quote.setBid(TPrice(104), TSize(1001), 0, 5);
    quote.setAsk(TPrice(105), TSize(300), 0, 1);
    
    quote._book[0]._bid._numOrders = 10;
    quote._book[0]._ask._numOrders = 10;
    
    // Paranoid qty disabled
    //
    info = impl.calcPrice(quote, true, reason);
    ASSERT_EQ(info.first, TPrice(104));
    ASSERT_TRUE(!info.second);
    ASSERT_EQ(reason, "");
    
    // Paranoid qty enabled
    //
    p._paranoidQty.set(1000);
    info = impl.calcPrice(quote, true, reason);
    ASSERT_EQ(info.first, TPrice(105));
    ASSERT_TRUE(info.second);
    ASSERT_EQ(reason, "passiveSize > _p._paranoidQty: 1001 > 1000");
    
    // Test lean qty
    //
    p._paranoidQty.set(0);
    p._leanQty.set(100);
    
    // Test avgParticipantQty - lean qty violated
    //
    p._avgParticipantQty.set(5);
    info = impl.calcPrice(quote, true, reason);
    ASSERT_EQ(info.first, TPrice(105));
    ASSERT_TRUE(info.second);
    ASSERT_EQ(reason, "activeSize < leanQty: 50 < 100");
    
    // Lean qty not violated
    //
    p._avgParticipantQty.set(0);
    info = impl.calcPrice(quote, true, reason);
    ASSERT_EQ(info.first, TPrice(104));
    ASSERT_TRUE(!info.second);
    ASSERT_EQ(reason, "");
    
    // Lean qty violated
    //
    quote.setBid(TPrice(104), TSize(101), 0, 5);
    
    p._leanQty.set(301);
    info = impl.calcPrice(quote, true, reason);
    ASSERT_EQ(info.first, TPrice(105));
    ASSERT_TRUE(info.second);
    ASSERT_EQ(reason, "activeSize < leanQty: 300 < 301");
    
    // Test passiveSmallThreshold and leanQtySmall
    //
    p._passiveSmallThreshold.set(102);
    p._leanQtySmall.set(299);
    info = impl.calcPrice(quote, true, reason);
    ASSERT_EQ(info.first, TPrice(104));
    ASSERT_TRUE(!info.second);
    ASSERT_EQ(reason, "");
    
    p._passiveSmallThreshold.set(102);
    p._leanQtySmall.set(302);
    info = impl.calcPrice(quote, true, reason);
    ASSERT_EQ(info.first, TPrice(105));
    ASSERT_TRUE(info.second);
    ASSERT_EQ(reason, "activeSize < leanQty: 300 < 302");
    
    p._passiveSmallThreshold.set(0);
    p._leanQtySmall.set(0);
    
    // Test lean ratio
    //
    quote.setBid(TPrice(104), TSize(1001), 0, 5);
    p._leanQty.set(100);
    p._leanRatio = 0.25;
    quote._book[0]._bid._numOrders = 50;
    quote._book[0]._ask._numOrders = 10;
    
    // Test avgParticipantQty - lean ratio violated
    //
    p._avgParticipantQty.set(10);
    info = impl.calcPrice(quote, true, reason);
    ASSERT_EQ(info.first, TPrice(105));
    ASSERT_TRUE(info.second);
    ASSERT_EQ(reason, "passiveSize*_p._leanRatio > activeSize: 500*0.25(125) > 100");
    
    // Lean ratio not violated
    //
    p._avgParticipantQty.set(0);
    info = impl.calcPrice(quote, true, reason);
    ASSERT_EQ(info.first, TPrice(104));
    ASSERT_TRUE(!info.second);
    ASSERT_EQ(reason, "");
    
    // Lean ratio violated
    //
    p._leanRatio = 0.45;
    info = impl.calcPrice(quote, true, reason);
    ASSERT_EQ(info.first, TPrice(105));
    ASSERT_TRUE(info.second);
    ASSERT_EQ(reason, "passiveSize*_p._leanRatio > activeSize: 1001*0.45(450.45) > 300");
    
    // Test ignoreRatioQty
    //
    
    // ignoreRatioQty is bigger than lean qty
    //
    p._ignoreRatioQty.set(350);
    info = impl.calcPrice(quote, true, reason);
    ASSERT_EQ(info.first, TPrice(105));
    ASSERT_TRUE(info.second);
    ASSERT_EQ(reason, "passiveSize*_p._leanRatio > activeSize: 1001*0.45(450.45) > 300");
    
    // ignoreRatioQty is bigger than lean qty
    //
    p._ignoreRatioQty.set(250);
    info = impl.calcPrice(quote, true, reason);
    ASSERT_EQ(info.first, TPrice(104));
    ASSERT_TRUE(!info.second);
    ASSERT_EQ(reason, "");
    
    // Test prev quote
    //
    p._leanQty.set(100);
    p._leanRatio = 0;
    
    quote.setBid(TPrice(104), TSize(1001), 0, 10);
    quote.setAsk(TPrice(105), TSize(240), 0, 50);
    
    
    p._leanQtyDelta = 0.3;
    
    info = impl.calcPrice(quote, true, reason);
    ASSERT_EQ(info.first, TPrice(104));
    ASSERT_TRUE(!info.second);
    ASSERT_EQ(reason, "");    
    
    quote.setAsk(TPrice(105), TSize(300), 0, 10);
    
    // Test leanQtyDelta - lean leanQtyDelta violated
    //
    p._avgParticipantQty.set(10);
    info = impl.calcPrice(quote, true, reason);
    ASSERT_EQ(info.first, TPrice(105));
    ASSERT_TRUE(info.second);
    ASSERT_EQ(reason, "prevActiveSize*(1-_p._leanQtyDelta) > activeSize: 500*0.7(350) > 100");
    
    // Test leanQtyDelta is not violated
    //
    p._avgParticipantQty.set(0);
    info = impl.calcPrice(quote, true, reason);
    ASSERT_EQ(info.first, TPrice(104));
    ASSERT_TRUE(!info.second);
    ASSERT_EQ(reason, "");
    
    // Test leanQtyDelta is violated
    //
    p._leanQtyDelta = 0.18;
    quote.setAsk(TPrice(105), TSize(240), 0, 1);
    info = impl.calcPrice(quote, true, reason);
    ASSERT_EQ(info.first, TPrice(105));
    ASSERT_TRUE(info.second);
    ASSERT_EQ(reason, "prevActiveSize*(1-_p._leanQtyDelta) > activeSize: 300*0.82(246) > 240");
    
    
}


TEST(CommonTradeLibTestSuit, activePriceProcessor_test_calcPrice_sell)
{
    ActivePriceParamsWire params;
    
    TImpl impl(params);
    ActivePriceParamsWire& p = impl.getParams();
    
    ASSERT_TRUE(!impl.isEnabled());
    
    TInfo info;
    std::string reason;
    
    // Set quote
    //
    tw::price::QuoteStore::TQuote quote;
    quote.setInstrument(InstrHelper::getNQU2());
    
    // Test buys
    //
    
    // Test not enabled
    //
    info = impl.calcPrice(quote, false, reason);
    ASSERT_TRUE(!info.first.isValid());
    ASSERT_TRUE(!info.second);
    ASSERT_EQ(reason, "");
    
    quote.setAsk(TPrice(109), TSize(20), 0, 1);
    info = impl.calcPrice(quote, false, reason);
    ASSERT_TRUE(!info.first.isValid());
    ASSERT_TRUE(!info.second);
    ASSERT_EQ(reason, "");
    
    // Test enabled logic
    //
    p._leanQty.set(100);
    
    // Test active side is invalid
    //
    quote.setAsk(TPrice(109), TSize(20), 0, 1);
    info = impl.calcPrice(quote, false, reason);
    ASSERT_EQ(info.first, TPrice(108));
    ASSERT_TRUE(info.second);
    ASSERT_EQ(reason, "!active.isValid()");
    
    // Test active side is gapped
    //
    quote.setBid(TPrice(107), TSize(20), 0, 1);
    info = impl.calcPrice(quote, false, reason);
    ASSERT_EQ(info.first, TPrice(108));
    ASSERT_TRUE(info.second);
    ASSERT_EQ(reason, "activePrice != active._price: active=1,20,107, activePrice=108");
    
    // Test paranoid qty
    //
    quote.setBid(TPrice(108), TSize(300), 0, 1);
    quote.setAsk(TPrice(109), TSize(1001), 0, 1);
    
    quote._book[0]._bid._numOrders = 10;
    quote._book[0]._ask._numOrders = 10;
    
    // Paranoid qty disabled
    //
    info = impl.calcPrice(quote, false, reason);
    ASSERT_EQ(info.first, TPrice(109));
    ASSERT_TRUE(!info.second);
    ASSERT_EQ(reason, "");
    
    // Paranoid qty enabled
    //
    p._paranoidQty.set(1000);
    info = impl.calcPrice(quote, false, reason);
    ASSERT_EQ(info.first, TPrice(108));
    ASSERT_TRUE(info.second);
    ASSERT_EQ(reason, "passiveSize > _p._paranoidQty: 1001 > 1000");
    
    // Test lean qty
    //
    p._paranoidQty.set(0);
    p._leanQty.set(100);
    
    // Test avgParticipantQty - lean qty violated
    //
    p._avgParticipantQty.set(5);
    info = impl.calcPrice(quote, false, reason);
    ASSERT_EQ(info.first, TPrice(108));
    ASSERT_TRUE(info.second);
    ASSERT_EQ(reason, "activeSize < leanQty: 50 < 100");
    
    // Lean qty not violated
    //
    p._avgParticipantQty.set(0);
    info = impl.calcPrice(quote, false, reason);
    ASSERT_EQ(info.first, TPrice(109));
    ASSERT_TRUE(!info.second);
    ASSERT_EQ(reason, "");
    
    // Lean qty violated
    //
    p._leanQty.set(301);
    info = impl.calcPrice(quote, false, reason);
    ASSERT_EQ(info.first, TPrice(108));
    ASSERT_TRUE(info.second);
    ASSERT_EQ(reason, "activeSize < leanQty: 300 < 301");
    
    // Test lean ratio
    //
    p._leanQty.set(100);
    
    // Lean ratio not violated
    //
    p._leanRatio = 0.25;
    info = impl.calcPrice(quote, false, reason);
    ASSERT_EQ(info.first, TPrice(109));
    ASSERT_TRUE(!info.second);
    ASSERT_EQ(reason, "");
    
    // Lean ratio violated
    //
    p._leanRatio = 0.45;
    info = impl.calcPrice(quote, false, reason);
    ASSERT_EQ(info.first, TPrice(108));
    ASSERT_TRUE(info.second);
    ASSERT_EQ(reason, "passiveSize*_p._leanRatio > activeSize: 1001*0.45(450.45) > 300");
    
    // Test ignoreRatioQty
    //
    
    // ignoreRatioQty is bigger than lean qty
    //
    p._ignoreRatioQty.set(350);
    info = impl.calcPrice(quote, false, reason);
    ASSERT_EQ(info.first, TPrice(108));
    ASSERT_TRUE(info.second);
    ASSERT_EQ(reason, "passiveSize*_p._leanRatio > activeSize: 1001*0.45(450.45) > 300");
    
    // ignoreRatioQty is bigger than lean qty
    //
    p._ignoreRatioQty.set(250);
    info = impl.calcPrice(quote, false, reason);
    ASSERT_EQ(info.first, TPrice(109));
    ASSERT_TRUE(!info.second);
    ASSERT_EQ(reason, "");
    
    // Test prev quote
    //
    p._leanQty.set(100);
    p._leanRatio = 0;
    
    quote.setBid(TPrice(108), TSize(240), 0, 1);
    quote.setAsk(TPrice(109), TSize(1001), 0, 1);
    
    // Test leanQtyDelta is not violated
    //
    p._leanQtyDelta = 0.3;
    info = impl.calcPrice(quote, false, reason);
    ASSERT_EQ(info.first, TPrice(109));
    ASSERT_TRUE(!info.second);
    ASSERT_EQ(reason, "");
    
    
    quote.setBid(TPrice(108), TSize(300), 0, 1);
    info = impl.calcPrice(quote, false, reason);
    ASSERT_EQ(info.first, TPrice(109));
    ASSERT_TRUE(!info.second);
    ASSERT_EQ(reason, "");
    
    
    // Test leanQtyDelta is violated
    //
    p._leanQtyDelta = 0.18;
    quote.setBid(TPrice(108), TSize(240), 0, 1);
    info = impl.calcPrice(quote, false, reason);
    ASSERT_EQ(info.first, TPrice(108));
    ASSERT_TRUE(info.second);
    ASSERT_EQ(reason, "prevActiveSize*(1-_p._leanQtyDelta) > activeSize: 300*0.82(246) > 240");
    
    
}


} // common_trade
} // tw

