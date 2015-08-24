#include <vector>

#include <tw/common_strat/defs.h>
#include <tw/generated/common_trade_blocks.h>
#include <tw/common_trade/cramStopProcessor.h>
#include <tw/price/defs.h>
#include <tw/generated/bars.h>

#include "../unit_test_price_lib/instr_helper.h"

#include <gtest/gtest.h>

namespace tw {
namespace common_trade {
    
const double EPSILON = 0.0001;

typedef tw::price::Ticks TPrice;
typedef tw::price::Size TSize;

typedef CramStopProcessor TImpl;
typedef CramStopLossParamsWire TParams;

typedef tw::common_trade::Bar TBar;
typedef std::vector<TBar> TBars;

static TParams getParams() {
    TParams p;
    
    p._initialCramStopEvalTicks.set(2);
    p._initialCramPopTicks.set(7);
    p._initialCramOffsetTicks.set(1);
    p._initialCramVolGood.set(20);
    p._initialCramRatio = 1.78;
    p._cramSlideRatio = 1.82;
    p._firmPriceQty.set(48);
    p._firmPriceOffsetTicks.set(4);
    
    return p;
}

TEST(CommonTradeLibTestSuit, cramStopProcessor_test_buy)
{
    std::string reason;
    TParams params;
    
    TImpl impl(params);
    TParams& p = impl.getParams();
    
    ASSERT_TRUE(!impl.isEnabled());
    ASSERT_TRUE(!p._initialCramStopSet);
    
    p = getParams();
    ASSERT_TRUE(impl.isEnabled());
    ASSERT_TRUE(!p._initialCramStopSet);
    
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
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setBid(TPrice(100), TSize(20), 0, 1);
    quote.setAsk(TPrice(101), TSize(20), 0, 1);
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "quote._book[0]._bid._price < info._fill._price+_p._initialCramStopEvalTicks");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_TRUE(!info._stop.isValid());
   
    // Test initialCramPopTicks
    //
    quote.setBid(TPrice(107), TSize(20), 0, 1);
    quote.setAsk(TPrice(108), TSize(20), 0, 1);
    
    ASSERT_TRUE(impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "isStopSlideTriggered(!info._stop.isValid() || info._stop < stop) for: stop=99,fill=InV,0,Unknown,,0,0, -- quote=1,20,107|108,20,1 :: quote._book[0]._bid._price >= info._fill._price+_p._initialCramPopTicks");
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(107));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(108));
    ASSERT_EQ(info._stop, TPrice(99));
    
    info._stop.clear();    
    impl.clear();
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_TRUE(!p._topOfBook._bid.isValid());
    ASSERT_TRUE(!p._topOfBook._ask.isValid());
    
    // Test _initialCramStopEvalTicks, _initialCramVolGood and _initialCramRatio
    //
    quote.setBid(TPrice(102), TSize(20), 0, 1);
    quote.setAsk(TPrice(103), TSize(20), 0, 1);
    quote.setTrade(TPrice(102), TSize(1));
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "vol on ask < _p._initialCramVolGood");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(102));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(1));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(0));
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setTrade(TPrice(103), TSize(10));
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "vol on ask < _p._initialCramVolGood");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(102));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(1));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(10));
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setTrade(TPrice(104), TSize(5));
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "vol on ask < _p._initialCramVolGood");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(102));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(1));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(15));
    ASSERT_TRUE(!info._stop.isValid());
    
    // Test resetting counters
    //
    quote.setTrade(TPrice(101), TSize(5));
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "quote._trade._price < _p._topOfBook._bid._price");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice());
    ASSERT_EQ(p._topOfBook._bid._size, TSize());
    ASSERT_EQ(p._topOfBook._ask._price, TPrice());
    ASSERT_EQ(p._topOfBook._ask._size, TSize());
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setTrade(TPrice(102), TSize(1));
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "vol on ask < _p._initialCramVolGood");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(102));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(1));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(0));
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setBid(TPrice(103), TSize(20), 0, 1);
    quote.setAsk(TPrice(104), TSize(20), 0, 1);
    quote.setTrade(TPrice(103), TSize(12));
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "vol on ask < _p._initialCramVolGood");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(12));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(104));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(0));
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setTrade(TPrice(104), TSize(21));
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "cramSlideRatio < _p._initialCramRatio");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(12));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(104));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(21));
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setTrade(TPrice(104), TSize(1));
    
    ASSERT_TRUE(impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "isStopSlideTriggered(!info._stop.isValid() || info._stop < stop) for: stop=99,fill=InV,0,Unknown,,0,0, -- quote=1,20,103|104,20,1 :: cramSlideRatio >= _p._initialCramRatio");
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(12));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(104));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(22));
    ASSERT_EQ(info._stop, TPrice(99));
    
    quote.setBid(TPrice(102), TSize(21), 2, 1);
    quote.setBid(TPrice(103), TSize(49), 1, 1);
    quote.setBid(TPrice(104), TSize(70), 0, 1);
    
    quote.setAsk(TPrice(105), TSize(20), 0, 1);
    quote.setAsk(TPrice(106), TSize(20), 1, 1);
    quote.setAsk(TPrice(107), TSize(20), 2, 1);
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "info._stop.isValid() && info._stop <= stop");
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(12));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(104));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(22));
    ASSERT_EQ(info._stop, TPrice(99));
    
    quote.setBid(TPrice(102), TSize(21), 2, 1);
    quote.setBid(TPrice(103), TSize(29), 1, 1);
    quote.setBid(TPrice(104), TSize(40), 0, 1);
    
    quote.setAsk(TPrice(105), TSize(20), 0, 1);
    quote.setAsk(TPrice(106), TSize(20), 1, 1);
    quote.setAsk(TPrice(107), TSize(20), 2, 1);
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_TRUE(std::string::npos != reason.find("stop not recalculated"));
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(12));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(104));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(22));
    ASSERT_EQ(info._stop, TPrice(99));
    
    
    quote.setBid(TPrice(105), TSize(21), 2, 1);
    quote.setBid(TPrice(107), TSize(49), 1, 1);
    quote.setBid(TPrice(108), TSize(70), 0, 1);
    
    quote.setAsk(TPrice(109), TSize(20), 0, 1);
    quote.setAsk(TPrice(110), TSize(20), 1, 1);
    quote.setAsk(TPrice(111), TSize(20), 2, 1);
    
    ASSERT_TRUE(impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "isStopSlideTriggered(!info._stop.isValid() || info._stop < stop) for: stop=103,fill=99,0,Unknown,,0,0, -- quote=1,70,108|109,20,1 :: quote._book[i]._bid._size > _p._firmPriceQty && cramSlideRatio > _p._cramSlideRatio :: i=1,quote[i]=1,49,107|110,20,1,quote[i+1]=1,21,105|111,20,1");
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(12));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(104));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(22));
    ASSERT_EQ(info._stop, TPrice(103));
    
    p._cramSlideRatio = 0.0;
    
    quote.setBid(TPrice(105), TSize(21), 2, 1);
    quote.setBid(TPrice(107), TSize(49), 1, 1);
    quote.setBid(TPrice(108), TSize(70), 0, 1);
    
    quote.setAsk(TPrice(109), TSize(20), 0, 1);
    quote.setAsk(TPrice(110), TSize(20), 1, 1);
    quote.setAsk(TPrice(111), TSize(20), 2, 1);
    
    ASSERT_TRUE(impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "isStopSlideTriggered(!info._stop.isValid() || info._stop < stop) for: stop=104,fill=103,0,Unknown,,0,0, -- quote=1,70,108|109,20,1 :: quote._book[i]._bid._size > _p._firmPriceQty :: i=0,quote[i]=1,70,108|109,20,1");
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(12));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(104));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(22));
    ASSERT_EQ(info._stop, TPrice(104));
    
    quote.clear();
    
}

TEST(CommonTradeLibTestSuit, cramStopProcessor_test_initFirmPriceQty_buy)
{
    std::string reason;
    TParams params;
    
    TImpl impl(params);
    TParams& p = impl.getParams();
    
    ASSERT_TRUE(!impl.isEnabled());
    ASSERT_TRUE(!p._initialCramStopSet);
    
    p = getParams();
    p._initialCramFirmPriceQty.set(21);
    ASSERT_TRUE(impl.isEnabled());
    ASSERT_TRUE(!p._initialCramStopSet);
    
    // Set quote
    //
    tw::price::QuoteStore::TQuote quote;
    quote.setInstrument(InstrHelper::get6CU2());
    
    // Set fill info
    //
    FillInfo info;    
    info._fill._side = tw::channel_or::eOrderSide::kBuy;
    info._fill._price.set(100);                
    
    // Test _initialCramFirmPriceQty
    //
    quote.setBid(TPrice(102), TSize(20), 0, 1);
    quote.setAsk(TPrice(103), TSize(20), 0, 1);
    quote.setTrade(TPrice(102), TSize(1));
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "initialCramFirmPriceQty > quote._book[0]._bid._size");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(102));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(1));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(0));
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setBid(TPrice(102), TSize(21), 0, 1);
    quote.setAsk(TPrice(103), TSize(20), 0, 1);
    quote.setTrade(TPrice(103), TSize(10));
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "vol on ask < _p._initialCramVolGood");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(102));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(1));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(10));
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setTrade(TPrice(104), TSize(5));
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "vol on ask < _p._initialCramVolGood");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(102));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(1));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(15));
    ASSERT_TRUE(!info._stop.isValid());
    
    // Test _initialCramFirmPriceQty when quote > fillPrice+_initialCramStopEvalTicks
    //
    quote.setBid(TPrice(103), TSize(21), 0, 1);
    quote.setAsk(TPrice(104), TSize(20), 0, 1);
    quote.setTrade(TPrice(103), TSize(12));
    
    ASSERT_TRUE(impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "isStopSlideTriggered(!info._stop.isValid() || info._stop < stop) for: stop=99,fill=InV,0,Unknown,,0,0, -- quote=1,21,103|104,20,1 :: _p._initialCramFirmPriceQty > quote._book[0]._bid._size && quote._book[0]._bid._price > info._fill._price+_p._initialCramStopEvalTicks");
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(12));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(104));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(0));
    ASSERT_EQ(info._stop, TPrice(99));
    
    quote.clear();
    
}

TEST(CommonTradeLibTestSuit, cramStopProcessorWithBarsDisabled_test_buy)
{
    std::string reason;
    TParams params;
    
    TImpl impl(params);
    TParams& p = impl.getParams();
    
    ASSERT_TRUE(!impl.isEnabled());
    ASSERT_TRUE(!p._initialCramStopSet);
    
    p = getParams();
    ASSERT_TRUE(impl.isEnabled());
    ASSERT_TRUE(!p._initialCramStopSet);
    
    TBars bars;
    
    // Set quote
    //
    tw::price::QuoteStore::TQuote quote;
    quote.setInstrument(InstrHelper::get6CU2());
    
    // Set fill info
    //
    FillInfo info;    
    info._fill._side = tw::channel_or::eOrderSide::kBuy;
    info._fill._price.set(100);        
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "!quote._book[0].isValid()");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setBid(TPrice(100), TSize(20), 0, 1);
    quote.setAsk(TPrice(101), TSize(20), 0, 1);
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "quote._book[0]._bid._price < info._fill._price+_p._initialCramStopEvalTicks");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_TRUE(!info._stop.isValid());
   
    // Test initialCramPopTicks
    //
    quote.setBid(TPrice(107), TSize(20), 0, 1);
    quote.setAsk(TPrice(108), TSize(20), 0, 1);
    
    ASSERT_TRUE(impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "isStopSlideTriggered(!info._stop.isValid() || info._stop < stop) for: stop=99,fill=InV,0,Unknown,,0,0, -- quote=1,20,107|108,20,1 :: quote._book[0]._bid._price >= info._fill._price+_p._initialCramPopTicks");
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(107));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(108));
    ASSERT_EQ(info._stop, TPrice(99));
    
    info._stop.clear();    
    impl.clear();
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_TRUE(!p._topOfBook._bid.isValid());
    ASSERT_TRUE(!p._topOfBook._ask.isValid());
    
    // Test _initialCramStopEvalTicks, _initialCramVolGood and _initialCramRatio
    //
    quote.setBid(TPrice(102), TSize(20), 0, 1);
    quote.setAsk(TPrice(103), TSize(20), 0, 1);
    quote.setTrade(TPrice(102), TSize(1));
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "vol on ask < _p._initialCramVolGood");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(102));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(1));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(0));
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setTrade(TPrice(103), TSize(10));
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "vol on ask < _p._initialCramVolGood");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(102));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(1));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(10));
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setTrade(TPrice(104), TSize(5));
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "vol on ask < _p._initialCramVolGood");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(102));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(1));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(15));
    ASSERT_TRUE(!info._stop.isValid());
    
    // Test resetting counters
    //
    quote.setTrade(TPrice(101), TSize(5));
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "quote._trade._price < _p._topOfBook._bid._price");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice());
    ASSERT_EQ(p._topOfBook._bid._size, TSize());
    ASSERT_EQ(p._topOfBook._ask._price, TPrice());
    ASSERT_EQ(p._topOfBook._ask._size, TSize());
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setTrade(TPrice(102), TSize(1));
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "vol on ask < _p._initialCramVolGood");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(102));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(1));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(0));
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setBid(TPrice(103), TSize(20), 0, 1);
    quote.setAsk(TPrice(104), TSize(20), 0, 1);
    quote.setTrade(TPrice(103), TSize(12));
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "vol on ask < _p._initialCramVolGood");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(12));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(104));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(0));
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setTrade(TPrice(104), TSize(21));
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "cramSlideRatio < _p._initialCramRatio");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(12));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(104));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(21));
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setTrade(TPrice(104), TSize(1));
    
    ASSERT_TRUE(impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "isStopSlideTriggered(!info._stop.isValid() || info._stop < stop) for: stop=99,fill=InV,0,Unknown,,0,0, -- quote=1,20,103|104,20,1 :: cramSlideRatio >= _p._initialCramRatio");
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(12));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(104));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(22));
    ASSERT_EQ(info._stop, TPrice(99));
    
    quote.setBid(TPrice(102), TSize(21), 2, 1);
    quote.setBid(TPrice(103), TSize(49), 1, 1);
    quote.setBid(TPrice(104), TSize(70), 0, 1);
    
    quote.setAsk(TPrice(105), TSize(20), 0, 1);
    quote.setAsk(TPrice(106), TSize(20), 1, 1);
    quote.setAsk(TPrice(107), TSize(20), 2, 1);
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "info._stop.isValid() && info._stop <= stop");
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(12));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(104));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(22));
    ASSERT_EQ(info._stop, TPrice(99));
    
    quote.setBid(TPrice(102), TSize(21), 2, 1);
    quote.setBid(TPrice(103), TSize(29), 1, 1);
    quote.setBid(TPrice(104), TSize(40), 0, 1);
    
    quote.setAsk(TPrice(105), TSize(20), 0, 1);
    quote.setAsk(TPrice(106), TSize(20), 1, 1);
    quote.setAsk(TPrice(107), TSize(20), 2, 1);
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_TRUE(std::string::npos != reason.find("stop not recalculated"));
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(12));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(104));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(22));
    ASSERT_EQ(info._stop, TPrice(99));
    
    
    quote.setBid(TPrice(105), TSize(21), 2, 1);
    quote.setBid(TPrice(107), TSize(49), 1, 1);
    quote.setBid(TPrice(108), TSize(70), 0, 1);
    
    quote.setAsk(TPrice(109), TSize(20), 0, 1);
    quote.setAsk(TPrice(110), TSize(20), 1, 1);
    quote.setAsk(TPrice(111), TSize(20), 2, 1);
    
    ASSERT_TRUE(impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "isStopSlideTriggered(!info._stop.isValid() || info._stop < stop) for: stop=103,fill=99,0,Unknown,,0,0, -- quote=1,70,108|109,20,1 :: quote._book[i]._bid._size > _p._firmPriceQty && cramSlideRatio > _p._cramSlideRatio :: i=1,quote[i]=1,49,107|110,20,1,quote[i+1]=1,21,105|111,20,1");
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(12));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(104));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(22));
    ASSERT_EQ(info._stop, TPrice(103));
    
    p._cramSlideRatio = 0.0;
    
    quote.setBid(TPrice(105), TSize(21), 2, 1);
    quote.setBid(TPrice(107), TSize(49), 1, 1);
    quote.setBid(TPrice(108), TSize(70), 0, 1);
    
    quote.setAsk(TPrice(109), TSize(20), 0, 1);
    quote.setAsk(TPrice(110), TSize(20), 1, 1);
    quote.setAsk(TPrice(111), TSize(20), 2, 1);
    
    ASSERT_TRUE(impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "isStopSlideTriggered(!info._stop.isValid() || info._stop < stop) for: stop=104,fill=103,0,Unknown,,0,0, -- quote=1,70,108|109,20,1 :: quote._book[i]._bid._size > _p._firmPriceQty :: i=0,quote[i]=1,70,108|109,20,1");
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(12));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(104));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(22));
    ASSERT_EQ(info._stop, TPrice(104));
    
    quote.clear();
    
}

TEST(CommonTradeLibTestSuit, cramStopProcessorWithBarsEnabled_test_buy)
{
    std::string reason;
    TParams params;
    
    TImpl impl(params);
    TParams& p = impl.getParams();
    
    ASSERT_TRUE(!impl.isEnabled());
    ASSERT_TRUE(!p._initialCramStopSet);
    
    p = getParams();
    p._minBarDispToCramTicks.set(10);
    p._cramSlideRatio = 0.0;
    
    ASSERT_TRUE(impl.isEnabled());
    ASSERT_TRUE(!p._initialCramStopSet);
    
    TBars bars;
    
    // Set quote
    //
    tw::price::QuoteStore::TQuote quote;
    quote.setInstrument(InstrHelper::get6CU2());
    
    // Set fill info
    //
    FillInfo info;    
    info._fill._side = tw::channel_or::eOrderSide::kBuy;
    info._fill._price.set(100);
    info._barIndex = info._barIndex2 = 1;
    
    bars.resize(1);
    
    // Set initialCramStopSet==true
    //
    quote.setBid(TPrice(103), TSize(20), 0, 1);
    quote.setAsk(TPrice(104), TSize(20), 0, 1);
    quote.setTrade(TPrice(103), TSize(12));
    
    bars.back()._numOfTrades = 20;
    bars.back()._high.set(103);
    bars.back()._low.set(100);
    bars.back()._open.set(100);
    bars.back()._close = bars.back()._high;
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    quote.setTrade(TPrice(104), TSize(22));    
    ASSERT_TRUE(impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "isStopSlideTriggered(!info._stop.isValid() || info._stop < stop) for: stop=99,fill=InV,0,Unknown,,1,1, -- quote=1,20,103|104,20,1 :: cramSlideRatio >= _p._initialCramRatio");
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(12));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(104));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(22));
    ASSERT_EQ(info._stop, TPrice(99));
    
    quote.setBid(TPrice(105), TSize(49), 0, 1);
    quote.setAsk(TPrice(106), TSize(20), 0, 1);
    quote.setTrade(TPrice(106), TSize(1));
    
    bars.back()._high.set(106);    
    bars.back()._close = bars.back()._high;
    
    bars.back()._dir = tw::common_trade::ePatternDir::kDown;
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "bar is not in the direction of trade: bar=0,,,Unknown,106,100,100,106,InV,0,20,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,,info._fill._side=Buy");
    ASSERT_TRUE(p._initialCramStopSet);
    
    bars.back()._dir = tw::common_trade::ePatternDir::kUp;
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "currBar._high < info._fill._price+_p._initialCramPopTicks");
    ASSERT_TRUE(p._initialCramStopSet);
    
    // Test initial bar cram logic
    //
    quote.setBid(TPrice(106), TSize(49), 0, 1);
    quote.setAsk(TPrice(107), TSize(20), 0, 1);
    quote.setTrade(TPrice(107), TSize(1));
    
    bars.back()._high.set(107);    
    bars.back()._close = bars.back()._high;
    
    ASSERT_TRUE(impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "isStopSlideTriggered(!info._stop.isValid() || info._stop < stop) for: stop=102,fill=99,0,Unknown,,1,1, -- quote=1,49,106|107,20,1 :: quote._book[i]._bid._size > _p._firmPriceQty :: i=0,quote[i]=1,49,106|107,20,1");
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(12));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(104));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(22));
    ASSERT_EQ(info._stop, TPrice(102));    
    
    // Test next to initial bar cram logic
    //
    bars.resize(2);
    
    quote.setBid(TPrice(114), TSize(49), 0, 1);
    quote.setAsk(TPrice(115), TSize(20), 0, 1);
    quote.setTrade(TPrice(115), TSize(1));
    
    bars.back()._numOfTrades = 20;
    bars.back()._high.set(115);
    bars.back()._low.set(115);
    bars.back()._open.set(115);    
    bars.back()._close = bars.back()._high;
    bars.back()._dir = tw::common_trade::ePatternDir::kUp;
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "range < _p._minBarDispToCramTicks");
    ASSERT_TRUE(p._initialCramStopSet);
    
    quote.setBid(TPrice(116), TSize(49), 0, 1);
    quote.setAsk(TPrice(117), TSize(20), 0, 1);
    quote.setTrade(TPrice(117), TSize(1));    
    
    bars.back()._high.set(117);
    bars.back()._close = bars.back()._high;
    
    ASSERT_TRUE(impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "isStopSlideTriggered(!info._stop.isValid() || info._stop < stop) for: stop=112,fill=102,0,Unknown,,1,1, -- quote=1,49,116|117,20,1 :: quote._book[i]._bid._size > _p._firmPriceQty :: i=0,quote[i]=1,49,116|117,20,1");
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(103));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(12));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(104));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(22));
    ASSERT_EQ(info._stop, TPrice(112));    
    
    quote.clear();
    
}

TEST(CommonTradeLibTestSuit, cramStopProcessor_test_sell)
{
    std::string reason;
    TParams params;
    
    TImpl impl(params);
    TParams& p = impl.getParams();
    
    ASSERT_TRUE(!impl.isEnabled());
    ASSERT_TRUE(!p._initialCramStopSet);
    
    p = getParams();
    ASSERT_TRUE(impl.isEnabled());
    ASSERT_TRUE(!p._initialCramStopSet);
    
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
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setBid(TPrice(99), TSize(20), 0, 1);
    quote.setAsk(TPrice(100), TSize(20), 0, 1);
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "quote._book[0]._ask._price > info._fill._price-_p._initialCramStopEvalTicks");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_TRUE(!info._stop.isValid());
   
    // Test initialCramPopTicks
    //
    quote.setBid(TPrice(92), TSize(20), 0, 1);
    quote.setAsk(TPrice(93), TSize(20), 0, 1);
    
    ASSERT_TRUE(impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "isStopSlideTriggered(!info._stop.isValid() || info._stop > stop) for: stop=101,fill=InV,0,Unknown,,0,0, -- quote=1,20,92|93,20,1 :: quote._book[0]._ask._price <= info._fill._price-_p._initialCramPopTicks");
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(92));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(93));
    ASSERT_EQ(info._stop, TPrice(101));
    
    info._stop.clear();    
    impl.clear();
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_TRUE(!p._topOfBook._bid.isValid());
    ASSERT_TRUE(!p._topOfBook._ask.isValid());
    
    // Test _initialCramStopEvalTicks, _initialCramVolGood and _initialCramRatio
    //
    quote.setBid(TPrice(97), TSize(20), 0, 1);
    quote.setAsk(TPrice(98), TSize(20), 0, 1);
    quote.setTrade(TPrice(98), TSize(1));
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "vol on bid < _p._initialCramVolGood");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(0));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(98));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(1));
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setTrade(TPrice(97), TSize(10));
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "vol on bid < _p._initialCramVolGood");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(10));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(98));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(1));
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setTrade(TPrice(96), TSize(5));
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "vol on bid < _p._initialCramVolGood");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(15));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(98));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(1));
    ASSERT_TRUE(!info._stop.isValid());
    
    // Test resetting counters
    //
    quote.setTrade(TPrice(99), TSize(5));
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "quote._trade._price > _p._topOfBook._ask._price");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice());
    ASSERT_EQ(p._topOfBook._bid._size, TSize());
    ASSERT_EQ(p._topOfBook._ask._price, TPrice());
    ASSERT_EQ(p._topOfBook._ask._size, TSize());
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setTrade(TPrice(98), TSize(1));
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "vol on bid < _p._initialCramVolGood");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(0));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(98));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(1));
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setBid(TPrice(96), TSize(20), 0, 1);
    quote.setAsk(TPrice(97), TSize(20), 0, 1);
    quote.setTrade(TPrice(97), TSize(12));
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "vol on bid < _p._initialCramVolGood");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(96));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(0));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(12));
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setTrade(TPrice(96), TSize(21));
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "cramSlideRatio < _p._initialCramRatio");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(96));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(21));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(12));
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setTrade(TPrice(96), TSize(1));
    
    ASSERT_TRUE(impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "isStopSlideTriggered(!info._stop.isValid() || info._stop > stop) for: stop=101,fill=InV,0,Unknown,,0,0, -- quote=1,20,96|97,20,1 :: cramSlideRatio >= _p._initialCramRatio");
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(96));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(22));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(12));
    ASSERT_EQ(info._stop, TPrice(101));
    
    quote.setBid(TPrice(93), TSize(20), 2, 1);
    quote.setBid(TPrice(94), TSize(20), 1, 1);
    quote.setBid(TPrice(95), TSize(20), 0, 1);
    
    quote.setAsk(TPrice(96), TSize(70), 0, 1);
    quote.setAsk(TPrice(97), TSize(49), 1, 1);
    quote.setAsk(TPrice(98), TSize(21), 2, 1);
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "info._stop.isValid() && info._stop <= stop");
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(96));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(22));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(12));
    ASSERT_EQ(info._stop, TPrice(101));
    
    quote.setBid(TPrice(93), TSize(20), 2, 1);
    quote.setBid(TPrice(94), TSize(20), 1, 1);
    quote.setBid(TPrice(95), TSize(20), 0, 1);
    
    quote.setAsk(TPrice(96), TSize(40), 0, 1);
    quote.setAsk(TPrice(97), TSize(29), 1, 1);
    quote.setAsk(TPrice(98), TSize(21), 2, 1);
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_TRUE(std::string::npos != reason.find("stop not recalculated"));
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(96));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(22));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(12));
    ASSERT_EQ(info._stop, TPrice(101));    
    
    quote.setBid(TPrice(89), TSize(20), 2, 1);
    quote.setBid(TPrice(90), TSize(20), 1, 1);
    quote.setBid(TPrice(91), TSize(20), 0, 1);
    
    quote.setAsk(TPrice(92), TSize(70), 0, 1);
    quote.setAsk(TPrice(93), TSize(49), 1, 1);
    quote.setAsk(TPrice(95), TSize(21), 2, 1);
    
    
    ASSERT_TRUE(impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "isStopSlideTriggered(!info._stop.isValid() || info._stop > stop) for: stop=97,fill=101,0,Unknown,,0,0, -- quote=1,20,91|92,70,1 :: quote._book[i]._ask._size > _p._firmPriceQty && cramSlideRatio > _p._cramSlideRatio :: i=1,quote[i]=1,20,90|93,49,1,quote[i+1]=1,20,89|95,21,1");
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(96));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(22));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(12));
    ASSERT_EQ(info._stop, TPrice(97));
    
    p._cramSlideRatio = 0.0;
    
    quote.setBid(TPrice(89), TSize(20), 2, 1);
    quote.setBid(TPrice(90), TSize(20), 1, 1);
    quote.setBid(TPrice(91), TSize(20), 0, 1);
    
    quote.setAsk(TPrice(92), TSize(70), 0, 1);
    quote.setAsk(TPrice(93), TSize(49), 1, 1);
    quote.setAsk(TPrice(95), TSize(21), 2, 1);
    
    ASSERT_TRUE(impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "isStopSlideTriggered(!info._stop.isValid() || info._stop > stop) for: stop=96,fill=97,0,Unknown,,0,0, -- quote=1,20,91|92,70,1 :: quote._book[i]._ask._size > _p._firmPriceQty :: i=0,quote[i]=1,20,91|92,70,1");
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(96));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(22));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(12));
    ASSERT_EQ(info._stop, TPrice(96));
    
    quote.clear();
}

TEST(CommonTradeLibTestSuit, cramStopProcessor_test_initFirmPriceQty_sell)
{
    std::string reason;
    TParams params;
    
    TImpl impl(params);
    TParams& p = impl.getParams();
    
    ASSERT_TRUE(!impl.isEnabled());
    ASSERT_TRUE(!p._initialCramStopSet);
    
    p = getParams();
    p._initialCramFirmPriceQty.set(21);
    ASSERT_TRUE(impl.isEnabled());
    ASSERT_TRUE(!p._initialCramStopSet);
    
    // Set quote
    //
    tw::price::QuoteStore::TQuote quote;
    quote.setInstrument(InstrHelper::get6CU2());
    
    // Set fill info
    //
    FillInfo info;    
    info._fill._side = tw::channel_or::eOrderSide::kSell;
    info._fill._price.set(100);
    
    // Test _initialCramFirmPriceQty
    //
    quote.setBid(TPrice(97), TSize(20), 0, 1);
    quote.setAsk(TPrice(98), TSize(20), 0, 1);
    quote.setTrade(TPrice(98), TSize(1));
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "initialCramFirmPriceQty > quote._book[0]._ask._size");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(0));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(98));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(1));
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setBid(TPrice(97), TSize(20), 0, 1);
    quote.setAsk(TPrice(98), TSize(21), 0, 1);
    quote.setTrade(TPrice(97), TSize(10));
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "vol on bid < _p._initialCramVolGood");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(10));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(98));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(1));
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setTrade(TPrice(96), TSize(5));
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "vol on bid < _p._initialCramVolGood");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(15));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(98));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(1));
    ASSERT_TRUE(!info._stop.isValid());
    
    // Test _initialCramFirmPriceQty when quote > fillPrice+_initialCramStopEvalTicks
    //
    quote.setBid(TPrice(96), TSize(20), 0, 1);
    quote.setAsk(TPrice(97), TSize(21), 0, 1);
    quote.setTrade(TPrice(97), TSize(12));
    
    ASSERT_TRUE(impl.isStopSlideTriggered(info, quote, reason));
    ASSERT_EQ(reason, "isStopSlideTriggered(!info._stop.isValid() || info._stop > stop) for: stop=101,fill=InV,0,Unknown,,0,0, -- quote=1,20,96|97,21,1 :: _p._initialCramFirmPriceQty > quote._book[0]._ask._size && quote._book[0]._ask._price < info._fill._price-_p._initialCramStopEvalTicks");
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(96));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(0));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(12));
    ASSERT_EQ(info._stop, TPrice(101));
    
    quote.clear();
}

TEST(CommonTradeLibTestSuit, cramStopProcessorWithBarsDisabled_test_sell)
{
    std::string reason;
    TParams params;
    
    TImpl impl(params);
    TParams& p = impl.getParams();
    
    ASSERT_TRUE(!impl.isEnabled());
    ASSERT_TRUE(!p._initialCramStopSet);
    
    p = getParams();
    ASSERT_TRUE(impl.isEnabled());
    ASSERT_TRUE(!p._initialCramStopSet);
    
    TBars bars;
    
    // Set quote
    //
    tw::price::QuoteStore::TQuote quote;
    quote.setInstrument(InstrHelper::get6CU2());
    
    // Set fill info
    //
    FillInfo info;    
    info._fill._side = tw::channel_or::eOrderSide::kSell;
    info._fill._price.set(100);        
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "!quote._book[0].isValid()");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setBid(TPrice(99), TSize(20), 0, 1);
    quote.setAsk(TPrice(100), TSize(20), 0, 1);
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "quote._book[0]._ask._price > info._fill._price-_p._initialCramStopEvalTicks");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_TRUE(!info._stop.isValid());
   
    // Test initialCramPopTicks
    //
    quote.setBid(TPrice(92), TSize(20), 0, 1);
    quote.setAsk(TPrice(93), TSize(20), 0, 1);
    
    ASSERT_TRUE(impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "isStopSlideTriggered(!info._stop.isValid() || info._stop > stop) for: stop=101,fill=InV,0,Unknown,,0,0, -- quote=1,20,92|93,20,1 :: quote._book[0]._ask._price <= info._fill._price-_p._initialCramPopTicks");
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(92));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(93));
    ASSERT_EQ(info._stop, TPrice(101));
    
    info._stop.clear();    
    impl.clear();
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_TRUE(!p._topOfBook._bid.isValid());
    ASSERT_TRUE(!p._topOfBook._ask.isValid());
    
    // Test _initialCramStopEvalTicks, _initialCramVolGood and _initialCramRatio
    //
    quote.setBid(TPrice(97), TSize(20), 0, 1);
    quote.setAsk(TPrice(98), TSize(20), 0, 1);
    quote.setTrade(TPrice(98), TSize(1));
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "vol on bid < _p._initialCramVolGood");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(0));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(98));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(1));
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setTrade(TPrice(97), TSize(10));
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "vol on bid < _p._initialCramVolGood");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(10));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(98));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(1));
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setTrade(TPrice(96), TSize(5));
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "vol on bid < _p._initialCramVolGood");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(15));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(98));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(1));
    ASSERT_TRUE(!info._stop.isValid());
    
    // Test resetting counters
    //
    quote.setTrade(TPrice(99), TSize(5));
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "quote._trade._price > _p._topOfBook._ask._price");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice());
    ASSERT_EQ(p._topOfBook._bid._size, TSize());
    ASSERT_EQ(p._topOfBook._ask._price, TPrice());
    ASSERT_EQ(p._topOfBook._ask._size, TSize());
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setTrade(TPrice(98), TSize(1));
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "vol on bid < _p._initialCramVolGood");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(0));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(98));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(1));
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setBid(TPrice(96), TSize(20), 0, 1);
    quote.setAsk(TPrice(97), TSize(20), 0, 1);
    quote.setTrade(TPrice(97), TSize(12));
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "vol on bid < _p._initialCramVolGood");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(96));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(0));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(12));
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setTrade(TPrice(96), TSize(21));
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "cramSlideRatio < _p._initialCramRatio");
    ASSERT_TRUE(!p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(96));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(21));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(12));
    ASSERT_TRUE(!info._stop.isValid());
    
    quote.setTrade(TPrice(96), TSize(1));
    
    ASSERT_TRUE(impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "isStopSlideTriggered(!info._stop.isValid() || info._stop > stop) for: stop=101,fill=InV,0,Unknown,,0,0, -- quote=1,20,96|97,20,1 :: cramSlideRatio >= _p._initialCramRatio");
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(96));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(22));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(12));
    ASSERT_EQ(info._stop, TPrice(101));
    
    quote.setBid(TPrice(93), TSize(20), 2, 1);
    quote.setBid(TPrice(94), TSize(20), 1, 1);
    quote.setBid(TPrice(95), TSize(20), 0, 1);
    
    quote.setAsk(TPrice(96), TSize(70), 0, 1);
    quote.setAsk(TPrice(97), TSize(49), 1, 1);
    quote.setAsk(TPrice(98), TSize(21), 2, 1);
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "info._stop.isValid() && info._stop <= stop");
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(96));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(22));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(12));
    ASSERT_EQ(info._stop, TPrice(101));
    
    quote.setBid(TPrice(93), TSize(20), 2, 1);
    quote.setBid(TPrice(94), TSize(20), 1, 1);
    quote.setBid(TPrice(95), TSize(20), 0, 1);
    
    quote.setAsk(TPrice(96), TSize(40), 0, 1);
    quote.setAsk(TPrice(97), TSize(29), 1, 1);
    quote.setAsk(TPrice(98), TSize(21), 2, 1);
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_TRUE(std::string::npos != reason.find("stop not recalculated"));
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(96));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(22));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(12));
    ASSERT_EQ(info._stop, TPrice(101));    
    
    quote.setBid(TPrice(89), TSize(20), 2, 1);
    quote.setBid(TPrice(90), TSize(20), 1, 1);
    quote.setBid(TPrice(91), TSize(20), 0, 1);
    
    quote.setAsk(TPrice(92), TSize(70), 0, 1);
    quote.setAsk(TPrice(93), TSize(49), 1, 1);
    quote.setAsk(TPrice(95), TSize(21), 2, 1);
    
    
    ASSERT_TRUE(impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "isStopSlideTriggered(!info._stop.isValid() || info._stop > stop) for: stop=97,fill=101,0,Unknown,,0,0, -- quote=1,20,91|92,70,1 :: quote._book[i]._ask._size > _p._firmPriceQty && cramSlideRatio > _p._cramSlideRatio :: i=1,quote[i]=1,20,90|93,49,1,quote[i+1]=1,20,89|95,21,1");
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(96));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(22));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(12));
    ASSERT_EQ(info._stop, TPrice(97));
    
    p._cramSlideRatio = 0.0;
    
    quote.setBid(TPrice(89), TSize(20), 2, 1);
    quote.setBid(TPrice(90), TSize(20), 1, 1);
    quote.setBid(TPrice(91), TSize(20), 0, 1);
    
    quote.setAsk(TPrice(92), TSize(70), 0, 1);
    quote.setAsk(TPrice(93), TSize(49), 1, 1);
    quote.setAsk(TPrice(95), TSize(21), 2, 1);
    
    ASSERT_TRUE(impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "isStopSlideTriggered(!info._stop.isValid() || info._stop > stop) for: stop=96,fill=97,0,Unknown,,0,0, -- quote=1,20,91|92,70,1 :: quote._book[i]._ask._size > _p._firmPriceQty :: i=0,quote[i]=1,20,91|92,70,1");
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(96));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(22));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(12));
    ASSERT_EQ(info._stop, TPrice(96));
    
    quote.clear();
}

TEST(CommonTradeLibTestSuit, cramStopProcessorWithBarsEnabled_test_sell)
{
    std::string reason;
    TParams params;
    
    TImpl impl(params);
    TParams& p = impl.getParams();
    
    ASSERT_TRUE(!impl.isEnabled());
    ASSERT_TRUE(!p._initialCramStopSet);
    
    p = getParams();
    p._minBarDispToCramTicks.set(10);
    p._cramSlideRatio = 0.0;
    
    ASSERT_TRUE(impl.isEnabled());
    ASSERT_TRUE(!p._initialCramStopSet);
    
    TBars bars;
    
    // Set quote
    //
    tw::price::QuoteStore::TQuote quote;
    quote.setInstrument(InstrHelper::get6CU2());
    
    // Set fill info
    //
    FillInfo info;    
    info._fill._side = tw::channel_or::eOrderSide::kSell;
    info._fill._price.set(100);
    info._barIndex = info._barIndex2 = 1;
    
    bars.resize(1);
    
    // Set initialCramStopSet==true
    //
    quote.setBid(TPrice(96), TSize(20), 0, 1);
    quote.setAsk(TPrice(97), TSize(20), 0, 1);
    quote.setTrade(TPrice(97), TSize(12));
    
    bars.back()._numOfTrades = 20;
    bars.back()._high.set(100);
    bars.back()._low.set(97);
    bars.back()._open.set(100);
    bars.back()._close = bars.back()._low;
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    quote.setTrade(TPrice(96), TSize(22));    
    ASSERT_TRUE(impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "isStopSlideTriggered(!info._stop.isValid() || info._stop > stop) for: stop=101,fill=InV,0,Unknown,,1,1, -- quote=1,20,96|97,20,1 :: cramSlideRatio >= _p._initialCramRatio");
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(96));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(22));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(12));
    ASSERT_EQ(info._stop, TPrice(101));
    
    quote.setBid(TPrice(94), TSize(20), 0, 1);
    quote.setAsk(TPrice(95), TSize(49), 0, 1);
    quote.setTrade(TPrice(94), TSize(1));
    
    bars.back()._low.set(94);    
    bars.back()._close = bars.back()._low;
    
    bars.back()._dir = tw::common_trade::ePatternDir::kUp;
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "bar is not in the direction of trade: bar=0,,,Unknown,100,94,100,94,InV,0,20,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,,info._fill._side=Sell");
    ASSERT_TRUE(p._initialCramStopSet);
    
    bars.back()._dir = tw::common_trade::ePatternDir::kDown;
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "currBar._low > info._fill._price-_p._initialCramPopTicks");
    ASSERT_TRUE(p._initialCramStopSet);
    
    // Test initial bar cram logic
    //
    quote.setBid(TPrice(93), TSize(20), 0, 1);
    quote.setAsk(TPrice(94), TSize(49), 0, 1);
    quote.setTrade(TPrice(93), TSize(1));
    
    bars.back()._low.set(93);    
    bars.back()._close = bars.back()._low;
    
    ASSERT_TRUE(impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "isStopSlideTriggered(!info._stop.isValid() || info._stop > stop) for: stop=98,fill=101,0,Unknown,,1,1, -- quote=1,20,93|94,49,1 :: quote._book[i]._ask._size > _p._firmPriceQty :: i=0,quote[i]=1,20,93|94,49,1");
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(96));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(22));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(12));
    ASSERT_EQ(info._stop, TPrice(98));    
    
    // Test next to initial bar cram logic
    //
    bars.resize(2);
    
    quote.setBid(TPrice(85), TSize(20), 0, 1);
    quote.setAsk(TPrice(86), TSize(49), 0, 1);
    quote.setTrade(TPrice(85), TSize(1));
    
    bars.back()._numOfTrades = 20;
    bars.back()._high.set(85);
    bars.back()._low.set(85);
    bars.back()._open.set(85);    
    bars.back()._close = bars.back()._low;
    bars.back()._dir = tw::common_trade::ePatternDir::kDown;
    
    ASSERT_TRUE(!impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "range < _p._minBarDispToCramTicks");
    ASSERT_TRUE(p._initialCramStopSet);
    
    quote.setBid(TPrice(83), TSize(20), 0, 1);
    quote.setAsk(TPrice(84), TSize(49), 0, 1);
    quote.setTrade(TPrice(83), TSize(1));    
    
    bars.back()._low.set(83);
    bars.back()._close = bars.back()._low;
    
    ASSERT_TRUE(impl.isStopSlideWithBarsTriggered(info, quote, bars, reason));
    ASSERT_EQ(reason, "isStopSlideTriggered(!info._stop.isValid() || info._stop > stop) for: stop=88,fill=98,0,Unknown,,1,1, -- quote=1,20,83|84,49,1 :: quote._book[i]._ask._size > _p._firmPriceQty :: i=0,quote[i]=1,20,83|84,49,1");
    ASSERT_TRUE(p._initialCramStopSet);
    ASSERT_EQ(p._topOfBook._bid._price, TPrice(96));
    ASSERT_EQ(p._topOfBook._bid._size, TSize(22));
    ASSERT_EQ(p._topOfBook._ask._price, TPrice(97));
    ASSERT_EQ(p._topOfBook._ask._size, TSize(12));
    ASSERT_EQ(info._stop, TPrice(88));    
    
    quote.clear();
    
}

} // common_trade
} // tw

