#include <vector>

#include <tw/common_strat/defs.h>
#include <tw/generated/common_trade_blocks.h>
#include <tw/common_trade/cramSignalProcessor.h>
#include <tw/price/defs.h>

#include "../unit_test_price_lib/instr_helper.h"

#include <gtest/gtest.h>

namespace tw {
namespace common_trade {
    
typedef tw::price::Ticks TPrice;
typedef tw::price::Size TSize;

typedef tw::common_trade::TBar TBar;
typedef tw::common_trade::TBars TBars;
typedef tw::common_trade::BarPattern TBarPattern;

typedef CramSignalProcessor TImpl;
typedef CramSignalParamsWire TParams;

static TParams getParams(TPrice minCramBarTicks,
                         TSize cramBarFirmPriceQty,
                         float cramPriceDepthPercent,
                         TPrice initialStopTicks) {
    TParams p;
    
    p._minCramBarTicks = minCramBarTicks;
    p._cramBarFirmPriceQty = cramBarFirmPriceQty;
    p._cramPriceDepthPercent = cramPriceDepthPercent;
    p._initialStopTicks = initialStopTicks;
    
    return p;
}

TEST(CommonTradeLibTestSuit, cramSignalProcessor_test_buy)
{
    return;
    
    // Test initialization
    //
    TParams params;    
    TImpl impl(params);
    TParams& p = impl.getParams();
    
    ASSERT_TRUE(!impl.isEnabled());    
    p = getParams(TPrice(10), TSize(41), 0.15, TPrice(5));    
    ASSERT_TRUE(impl.isEnabled());
    
    // Test isSignalTriggered()
    //
    tw::channel_or::eOrderSide side;
    std::string reason;    
    TBars bars;
    
    // Set quote
    //
    tw::price::QuoteStore::TQuote quote;
    quote.setInstrument(InstrHelper::get6CU2());
    
    ASSERT_TRUE(!impl.isSignalTriggered(quote, bars, side, reason));
    ASSERT_EQ(reason, "bars.empty()");
    ASSERT_EQ(p._barIndex, 0);
    ASSERT_TRUE(!p._cramPrice.isValid());
    
    bars.resize(1);
    TBar& bar = bars.back();
    
    bar._index = 1;
    bar._open.set(100);
    bar._close.set(102);
    
    bar._high.set(110);
    bar._low.set(90);
    bar._range.set(2);
    bar._dir = tw::common_trade::ePatternDir::kUp;
    
    ASSERT_TRUE(!impl.isSignalTriggered(quote, bars, side, reason));
    ASSERT_EQ(reason, "bar._range < _p._minCramBarTicks -- _p._minCramBarTicks=10,bar=1,,,Unknown,110,90,100,102,2,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p._barIndex, 0);
    ASSERT_TRUE(!p._cramPrice.isValid());
    
    bar._open.set(95);
    bar._close.set(105);
    bar._range.set(20);
    
    ASSERT_TRUE(!impl.isSignalTriggered(quote, bars, side, reason));
    ASSERT_EQ(reason, "!_p._cramPrice.isValid()");
    ASSERT_EQ(p._barIndex, 1);
    ASSERT_TRUE(!p._cramPrice.isValid());
    
    quote.setAsk(TPrice(108), TSize(20), 0, 1);
    quote.setBid(TPrice(105), TSize(20), 0, 1);
    quote.setBid(TPrice(104), TSize(20), 1, 1);
    quote.setBid(TPrice(103), TSize(42), 2, 1);
    quote.setBid(TPrice(102), TSize(53), 3, 1);
    quote.setBid(TPrice(101), TSize(64), 4, 1);
    
    ASSERT_TRUE(!impl.isSignalTriggered(quote, bars, side, reason));
    ASSERT_EQ(reason, "!_p._cramPrice.isValid()");
    ASSERT_EQ(p._barIndex, 1);
    ASSERT_TRUE(!p._cramPrice.isValid());
    
    p._cramPriceDepthPercent = 0.48;
    
    ASSERT_TRUE(!impl.isSignalTriggered(quote, bars, side, reason));
    ASSERT_EQ(reason, "!quote._trade._price.isValid()");
    ASSERT_EQ(p._barIndex, 1);
    ASSERT_EQ(p._cramPrice, TPrice(103));
    
    quote.setTrade(TPrice(108), TSize(1));

    ASSERT_TRUE(!impl.isSignalTriggered(quote, bars, side, reason));
    ASSERT_EQ(reason, "quote._trade._price >= _p._cramPrice - _p._cramPrice=103,quote._trade=         1@108        :: u,u,u,u,bar=1,,,Unknown,110,90,95,105,20,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p._barIndex, 1);
    ASSERT_EQ(p._cramPrice, TPrice(103));
    
    quote.setAsk(TPrice(108), TSize(20), 0, 1);
    quote.setBid(TPrice(105), TSize(20), 0, 1);
    quote.setBid(TPrice(104), TSize(20), 1, 1);
    quote.setBid(TPrice(103), TSize(20), 2, 1);
    quote.setBid(TPrice(102), TSize(53), 3, 1);
    quote.setBid(TPrice(101), TSize(64), 4, 1);
    
    ASSERT_TRUE(!impl.isSignalTriggered(quote, bars, side, reason));
    ASSERT_EQ(reason, "quote._trade._price >= _p._cramPrice - _p._cramPrice=103,quote._trade=         1@108        :: u,u,u,u,bar=1,,,Unknown,110,90,95,105,20,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p._barIndex, 1);
    ASSERT_EQ(p._cramPrice, TPrice(103));
    
    quote.setAsk(TPrice(108), TSize(20), 0, 1);
    quote.setBid(TPrice(105), TSize(60), 0, 1);
    quote.setBid(TPrice(104), TSize(42), 1, 1);
    quote.setBid(TPrice(103), TSize(20), 2, 1);
    quote.setBid(TPrice(102), TSize(53), 3, 1);
    quote.setBid(TPrice(101), TSize(64), 4, 1);
    
    ASSERT_TRUE(!impl.isSignalTriggered(quote, bars, side, reason));
    ASSERT_EQ(reason, "quote._book[i]._bid._size > _p._cramBarFirmPriceQty :: cramPrice=105,cramPriceDepthPercent=0.25,quote[0]=1,60,105|108,20,1 -- quote._trade._price >= _p._cramPrice - _p._cramPrice=105,quote._trade=         1@108        :: u,u,u,u,bar=1,,,Unknown,110,90,95,105,20,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p._barIndex, 1);
    ASSERT_EQ(p._lastSignalBarIndex, 0);
    ASSERT_EQ(p._cramPrice, TPrice(105));
    
    
    quote.setAsk(TPrice(108), TSize(20), 0, 1);
    quote.setBid(TPrice(104), TSize(60), 0, 1);
    quote.setBid(TPrice(103), TSize(42), 1, 1);
    quote.setBid(TPrice(102), TSize(20), 2, 1);
    quote.setBid(TPrice(101), TSize(53), 3, 1);
    quote.setBid(TPrice(100), TSize(64), 4, 1);
    
    quote.setTrade(TPrice(104), TSize(1));
    
    ASSERT_TRUE(impl.isSignalTriggered(quote, bars, side, reason));
    ASSERT_EQ(reason, "CramSignalProcessor::isSignalTriggered()==true -- _p._cramPrice=105,trigger_bar=1,,,Unknown,110,90,95,105,20,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p._barIndex, 1);
    ASSERT_EQ(p._lastSignalBarIndex, 1);
    ASSERT_EQ(p._cramPrice, TPrice(105));
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kSell);
    
    ASSERT_TRUE(!impl.isSignalTriggered(quote, bars, side, reason));
    ASSERT_EQ(reason, "_p._lastSignalIndex == bar._index -- _p._lastSignalBarIndex=1,bar=1,,,Unknown,110,90,95,105,20,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p._barIndex, 1);
    ASSERT_EQ(p._cramPrice, TPrice(105));
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kUnknown);
    
    bars.resize(2);
    TBar& bar2 = bars.back();
    
    bar2._index = 2;
    bar2._open.set(106);
    bar2._close.set(94);
    
    bar2._high.set(110);
    bar2._low.set(90);
    bar2._range = bar._high - bar._low;
    bar2._dir = tw::common_trade::ePatternDir::kUp;
    
    quote.clearRuntime();
    quote.setAsk(TPrice(108), TSize(20), 0, 1);
    quote.setBid(TPrice(104), TSize(20), 0, 1);
    quote.setBid(TPrice(103), TSize(20), 1, 1);
    quote.setBid(TPrice(102), TSize(20), 2, 1);
    quote.setBid(TPrice(101), TSize(20), 3, 1);
    quote.setBid(TPrice(100), TSize(20), 4, 1);
    
    ASSERT_TRUE(!impl.isSignalTriggered(quote, bars, side, reason));
    ASSERT_EQ(reason, "!_p._cramPrice.isValid()");
    ASSERT_EQ(p._barIndex, 2);
    ASSERT_EQ(p._lastSignalBarIndex, 1);
    ASSERT_TRUE(!p._cramPrice.isValid());
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kUnknown);
    
    
}


TEST(CommonTradeLibTestSuit, cramSignalProcessor_test_sell)
{
    return;
    
    // Test initialization
    //
    TParams params;    
    TImpl impl(params);
    TParams& p = impl.getParams();
    
    ASSERT_TRUE(!impl.isEnabled());    
    p = getParams(TPrice(10), TSize(41), 0.15, TPrice(5));    
    ASSERT_TRUE(impl.isEnabled());
    
    // Test isSignalTriggered()
    //
    tw::channel_or::eOrderSide side;
    std::string reason;    
    TBars bars;
    
    // Set quote
    //
    tw::price::QuoteStore::TQuote quote;
    quote.setInstrument(InstrHelper::get6CU2());
    
    ASSERT_TRUE(!impl.isSignalTriggered(quote, bars, side, reason));
    ASSERT_EQ(reason, "bars.empty()");
    ASSERT_EQ(p._barIndex, 0);
    ASSERT_TRUE(!p._cramPrice.isValid());
    
    bars.resize(1);
    TBar& bar = bars.back();
    
    bar._index = 1;
    bar._open.set(102);
    bar._close.set(100);
    
    bar._high.set(110);
    bar._low.set(90);
    bar._range.set(2);
    bar._dir = tw::common_trade::ePatternDir::kDown;
    
    ASSERT_TRUE(!impl.isSignalTriggered(quote, bars, side, reason));
    ASSERT_EQ(reason, "bar._range < _p._minCramBarTicks -- _p._minCramBarTicks=10,bar=1,,,Unknown,110,90,102,100,2,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p._barIndex, 0);
    ASSERT_TRUE(!p._cramPrice.isValid());
    
    bar._open.set(105);
    bar._close.set(95);
    bar._range.set(20);
    
    ASSERT_TRUE(!impl.isSignalTriggered(quote, bars, side, reason));
    ASSERT_EQ(reason, "!_p._cramPrice.isValid()");
    ASSERT_EQ(p._barIndex, 1);
    ASSERT_TRUE(!p._cramPrice.isValid());
    
    quote.setAsk(TPrice(99), TSize(64), 4, 1);
    quote.setAsk(TPrice(98), TSize(53), 3, 1);
    quote.setAsk(TPrice(97), TSize(42), 2, 1);
    quote.setAsk(TPrice(96), TSize(20), 1, 1);
    quote.setAsk(TPrice(95), TSize(20), 0, 1);
    quote.setBid(TPrice(92), TSize(20), 4, 1);
    
    
    ASSERT_TRUE(!impl.isSignalTriggered(quote, bars, side, reason));
    ASSERT_EQ(reason, "!_p._cramPrice.isValid()");
    ASSERT_EQ(p._barIndex, 1);
    ASSERT_TRUE(!p._cramPrice.isValid());
    
    p._cramPriceDepthPercent = 0.48;
    
    ASSERT_TRUE(!impl.isSignalTriggered(quote, bars, side, reason));
    ASSERT_EQ(reason, "!quote._trade._price.isValid()");
    ASSERT_EQ(p._barIndex, 1);
    ASSERT_EQ(p._cramPrice, TPrice(97));
    
    quote.setTrade(TPrice(92), TSize(1));

    ASSERT_TRUE(!impl.isSignalTriggered(quote, bars, side, reason));
    ASSERT_EQ(reason, "quote._trade._price <= _p._cramPrice - _p._cramPrice=97,quote._trade=         1@92         :: u,u,u,u,bar=1,,,Unknown,110,90,105,95,20,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p._barIndex, 1);
    ASSERT_EQ(p._cramPrice, TPrice(97));
    
    quote.setAsk(TPrice(99), TSize(64), 4, 1);
    quote.setAsk(TPrice(98), TSize(53), 3, 1);
    quote.setAsk(TPrice(97), TSize(20), 2, 1);
    quote.setAsk(TPrice(96), TSize(20), 1, 1);
    quote.setAsk(TPrice(95), TSize(20), 0, 1);
    quote.setBid(TPrice(92), TSize(20), 0, 1);
    
    ASSERT_TRUE(!impl.isSignalTriggered(quote, bars, side, reason));
    ASSERT_EQ(reason, "quote._trade._price <= _p._cramPrice - _p._cramPrice=97,quote._trade=         1@92         :: u,u,u,u,bar=1,,,Unknown,110,90,105,95,20,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p._barIndex, 1);
    ASSERT_EQ(p._cramPrice, TPrice(97));
    
    quote.setAsk(TPrice(99), TSize(64), 4, 1);
    quote.setAsk(TPrice(98), TSize(53), 3, 1);
    quote.setAsk(TPrice(97), TSize(20), 2, 1);
    quote.setAsk(TPrice(96), TSize(20), 1, 1);
    quote.setAsk(TPrice(95), TSize(60), 0, 1);
    quote.setBid(TPrice(92), TSize(20), 0, 1);
    
    ASSERT_TRUE(!impl.isSignalTriggered(quote, bars, side, reason));
    ASSERT_EQ(reason, "quote._book[i]._ask._size > _p._cramBarFirmPriceQty :: cramPrice=95,cramPriceDepthPercent=0.25,quote[0]=1,20,92|95,60,1 -- quote._trade._price <= _p._cramPrice - _p._cramPrice=95,quote._trade=         1@92         :: u,u,u,u,bar=1,,,Unknown,110,90,105,95,20,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p._barIndex, 1);
    ASSERT_EQ(p._cramPrice, TPrice(95));
    
    quote.setAsk(TPrice(100), TSize(64), 4, 1);
    quote.setAsk(TPrice(99), TSize(53), 3, 1);
    quote.setAsk(TPrice(98), TSize(20), 2, 1);
    quote.setAsk(TPrice(97), TSize(42), 1, 1);
    quote.setAsk(TPrice(96), TSize(60), 0, 1);        
    quote.setBid(TPrice(92), TSize(20), 0, 1);
    
    quote.setTrade(TPrice(96), TSize(1));
    
    ASSERT_TRUE(impl.isSignalTriggered(quote, bars, side, reason));
    ASSERT_EQ(reason, "CramSignalProcessor::isSignalTriggered()==true -- _p._cramPrice=95,trigger_bar=1,,,Unknown,110,90,105,95,20,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p._barIndex, 1);
    ASSERT_EQ(p._cramPrice, TPrice(95));
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kBuy);
    
    ASSERT_TRUE(!impl.isSignalTriggered(quote, bars, side, reason));
    ASSERT_EQ(reason, "_p._lastSignalIndex == bar._index -- _p._lastSignalBarIndex=1,bar=1,,,Unknown,110,90,105,95,20,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p._barIndex, 1);
    ASSERT_EQ(p._cramPrice, TPrice(95));
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kUnknown);
    
    bars.resize(2);
    TBar& bar2 = bars.back();
    
    bar2._index = 2;
    bar2._open.set(94);
    bar2._close.set(106);
    
    bar2._high.set(110);
    bar2._low.set(90);
    bar2._range = bar._high - bar._low;
    bar2._dir = tw::common_trade::ePatternDir::kDown;
    
    quote.clearRuntime();
    quote.setAsk(TPrice(100), TSize(20), 4, 1);
    quote.setAsk(TPrice(99), TSize(20), 3, 1);
    quote.setAsk(TPrice(98), TSize(20), 2, 1);
    quote.setAsk(TPrice(97), TSize(20), 1, 1);
    quote.setAsk(TPrice(96), TSize(20), 0, 1);        
    quote.setBid(TPrice(92), TSize(20), 0, 1);
    
    ASSERT_TRUE(!impl.isSignalTriggered(quote, bars, side, reason));
    ASSERT_EQ(reason, "!_p._cramPrice.isValid()");
    ASSERT_EQ(p._barIndex, 2);
    ASSERT_TRUE(!p._cramPrice.isValid());
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kUnknown);
    
}

TEST(CommonTradeLibTestSuit, cramSignalProcessor_test_stop_for_buy)
{
    return;
    
    // Test initialization
    //
    TParams params;    
    TImpl impl(params);
    TParams& p = impl.getParams();
    
    ASSERT_TRUE(!impl.isEnabled());    
    p = getParams(TPrice(10), TSize(41), 0.15, TPrice(5));
    
    std::string reason;
    
    // Set bars
    //
    TBars bars;
    
    // Set quote
    //
    tw::price::QuoteStore::TQuote quote;
    quote.setInstrument(InstrHelper::get6CU2());
    
    bars.resize(1);
    TBar& bar = bars.back();
    
    bar._index = 1;
    bar._open.set(100);
    bar._close.set(102);
    
    bar._high.set(110);
    bar._low.set(90);
    bar._range = bar._high - bar._low;
    bar._dir = tw::common_trade::ePatternDir::kUp;
    
    // Set fill info
    //
    FillInfo info;    
    info._fill._side = tw::channel_or::eOrderSide::kBuy;
    info._fill._price.set(100);    
    
    ASSERT_TRUE(!impl.calcStop(info, bars, reason));
    ASSERT_EQ(reason, "checkRange(bars, info._barIndex2) -- info=InV,0,Unknown,,0,0,,bars.size()=1");
    ASSERT_TRUE(!info._stop.isValid());
    
    info._barIndex2 = 1;
    
    ASSERT_TRUE(impl.calcStop(info, bars, reason));
    ASSERT_EQ(reason, "CramSignalProcessor::calcStop()==true -- stop=95,bar=1,,,Unknown,110,90,100,102,20,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(info._stop, TPrice(95));
    
    ASSERT_TRUE(!impl.calcStop(info, bars, reason));
    ASSERT_EQ(reason, "info._stop.isValid() && info._stop >= stop -- stop=95,info=95,0,Unknown,,0,1,,bar=1,,,Unknown,110,90,100,102,20,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(info._stop, TPrice(95));
    
    info._stop.clear();
    p._initialStopTicks.set(12);
    
    ASSERT_TRUE(impl.calcStop(info, bars, reason));
    ASSERT_EQ(reason, "CramSignalProcessor::calcStop()==true -- stop=90,bar=1,,,Unknown,110,90,100,102,20,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(info._stop, TPrice(90));
    
    info._stop.set(92);
    
    ASSERT_TRUE(!impl.calcStop(info, bars, reason));
    ASSERT_EQ(reason, "info._stop.isValid() && info._stop >= stop -- stop=90,info=92,0,Unknown,,0,1,,bar=1,,,Unknown,110,90,100,102,20,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(info._stop, TPrice(92));
    
}

TEST(CommonTradeLibTestSuit, cramSignalProcessor_test_stop_for_sell)
{
    return;
    
    // Test initialization
    //
    TParams params;    
    TImpl impl(params);
    TParams& p = impl.getParams();
    
    ASSERT_TRUE(!impl.isEnabled());    
    p = getParams(TPrice(10), TSize(41), 0.15, TPrice(5));
    
    std::string reason;
    
    // Set bars
    //
    TBars bars;
    
    // Set quote
    //
    tw::price::QuoteStore::TQuote quote;
    quote.setInstrument(InstrHelper::get6CU2());
    
    bars.resize(1);
    TBar& bar = bars.back();
    
    bar._index = 1;
    bar._open.set(102);
    bar._close.set(100);
    
    bar._high.set(110);
    bar._low.set(90);
    bar._range = bar._high - bar._low;
    bar._dir = tw::common_trade::ePatternDir::kDown;
    
    // Set fill info
    //
    FillInfo info;    
    info._fill._side = tw::channel_or::eOrderSide::kSell;
    info._fill._price.set(100);    
    
    ASSERT_TRUE(!impl.calcStop(info, bars, reason));
    ASSERT_EQ(reason, "checkRange(bars, info._barIndex2) -- info=InV,0,Unknown,,0,0,,bars.size()=1");
    ASSERT_TRUE(!info._stop.isValid());
    
    info._barIndex2 = 1;
    
    ASSERT_TRUE(impl.calcStop(info, bars, reason));
    ASSERT_EQ(reason, "CramSignalProcessor::calcStop()==true -- stop=105,bar=1,,,Unknown,110,90,102,100,20,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(info._stop, TPrice(105));
    
    ASSERT_TRUE(!impl.calcStop(info, bars, reason));
    ASSERT_EQ(reason, "info._stop.isValid() && info._stop <= stop -- stop=105,info=105,0,Unknown,,0,1,,bar=1,,,Unknown,110,90,102,100,20,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(info._stop, TPrice(105));
    
    info._stop.clear();
    p._initialStopTicks.set(12);
    
    ASSERT_TRUE(impl.calcStop(info, bars, reason));
    ASSERT_EQ(reason, "CramSignalProcessor::calcStop()==true -- stop=110,bar=1,,,Unknown,110,90,102,100,20,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(info._stop, TPrice(110));
    
    info._stop.set(108);
    
    ASSERT_TRUE(!impl.calcStop(info, bars, reason));
    ASSERT_EQ(reason, "info._stop.isValid() && info._stop <= stop -- stop=110,info=108,0,Unknown,,0,1,,bar=1,,,Unknown,110,90,102,100,20,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(info._stop, TPrice(108));
    
}


} // common_trade
} // tw

