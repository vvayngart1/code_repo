#include <vector>
#include <tw/common_strat/defs.h>
#include <tw/generated/common_trade_blocks.h>
#include <tw/common_trade/transitionSignalProcessor.h>
#include <tw/price/defs.h>
#include "../unit_test_price_lib/instr_helper.h"
#include <gtest/gtest.h>

namespace tw {
namespace common_trade {

// typedef tw::common::THighResTime TTime;
// typedef tw::price::Quote TQuote;
typedef tw::price::Ticks TPrice;
typedef tw::price::Size TSize;
typedef tw::common_trade::TBar TBar;
typedef tw::common_trade::TBars TBars;
typedef tw::common_trade::TBarPatterns TBarPatterns;
typedef TransitionSignalProcessor TImpl;
typedef TransitionSignalParamsWire TParams;
const double EPSILON = 0.0001;

static TParams getParams(TPrice tCramStopEvalTicks,
                         TPrice tCramMaxTicks,
                         TSize tCramFirmPriceQty,
                         TSize tCramVolGood,
                         float tCramRatio,
                         float atr_required,
                         TPrice tMinRangeTicks,
                         TPrice tMaxRangeTicks,
        
                         TPrice tBarLargeDispTicks,
                         TPrice tMaxOutsideTicks,
                         uint32_t tMaxBarsToPersist,
                         float tReadMinRatio,
                         TSize tReadMinTraded,
                         TPrice tReadMaxTicks) {
    TParams p;
    
    p._tCramStopEvalTicks = tCramStopEvalTicks;
    p._tCramMaxTicks = tCramMaxTicks;
    p._tCramFirmPriceQty = tCramFirmPriceQty;
    p._tCramVolGood = tCramVolGood;
    p._tCramRatio = tCramRatio;
    p._atr_required = atr_required;
    p._tMinRangeTicks = tMinRangeTicks;
    p._tMaxRangeTicks = tMaxRangeTicks;
    
    p._tBarLargeDispTicks = tBarLargeDispTicks;
    p._tMaxOutsideTicks = tMaxOutsideTicks;
    p._tMaxBarsToPersist = tMaxBarsToPersist;
    p._tReadMinRatio = tReadMinRatio;
    p._tReadMinTraded = tReadMinTraded;
    p._tReadMaxTicks = tReadMaxTicks;
    
    // guide to non-serializable fields
    // -----------------------------------------------------------------------------
    // <tReadTradedAbove      type="uint32_t"                serializable='false'/>
    // <tReadTradedBelow      type="uint32_t"                serializable='false'/>
    // <tDisp                 type="tw::price::Ticks"        serializable='false'/>
    // <atr                   type="float"                   serializable='false'/>
    // <tBarLocated           type="bool"                    serializable='false'/>
    // <tBar                  type="tw::common_trade::TBar"  serializable='false'/>
    // <tBarIndex             type="uint32_t"                serializable='false'/>
    // <tHigh                 type="tw::price::Ticks"        serializable='false'/>
    // <tLow                  type="tw::price::Ticks"        serializable='false'/>
    // <tCramCompleted        type="bool"                    serializable='false'/>
    // <top                   type="tw::price::PriceLevel"   serializable='false'/>
    
    return p;
}

void updateQuote(const TPrice& bid, const TPrice& ask, const TPrice& trade, tw::price::QuoteStore::TQuote& quote) {
    static uint32_t seqNum = 0;
    
    quote._seqNum = ++seqNum;
    quote.setBid(bid, TSize(1), 0, 1);
    quote.setAsk(ask, TSize(1), 0, 1);
    quote.setTrade(trade, TSize(1));
}

TEST(CommonTradeLibTestSuit, transitionSignalProcessor_test)
{
    // until method is vetted:
    return;
    
    tw::channel_or::eOrderSide side;
    std::string reason;
    TParams params;
    TBars bars;
    TBarPatterns swings;
    TBarPatterns trends;
    TImpl impl(params);
    impl.clear();
    TParams& p = impl.getParams();
    p = getParams(TPrice(0), TPrice(1), TSize(25), TSize(25), 2, 2.5, TPrice(2), TPrice(6), TPrice(3), TPrice(2), 2, 2, TSize(20), TPrice(2));
    
//    p._tCramStopEvalTicks = 0
//    p._tCramMaxTicks = 1
//    p._tCramFirmPriceQty = 25
//    p._tCramVolGood = 25
//    p._tCramRatio = 2
//    p._atr_required = 2.5
//    p._tMinRangeTicks = 2
//    p._tMaxRangeTicks = 6
//    p._tBarLargeDispTicks = 3
//    p._tMaxOutsideTicks = 2
//    p._tMaxBarsToPersist = 2
//    p._tReadMinRatio = 2
//    p._tReadMinTraded = 20
//    p._tReadMaxTicks = 2
    
    // need to define a quote here
    // to prescribe a deep book:
    // quote.setAsk(TPrice(108), TSize(20), 0, 1);
    // quote.setBid(TPrice(104), TSize(20), 0, 1);
    // quote.setBid(TPrice(103), TSize(20), 1, 1);
    // quote.setBid(TPrice(102), TSize(20), 2, 1);
    // quote.setBid(TPrice(101), TSize(20), 3, 1);
    // quote.setBid(TPrice(100), TSize(20), 4, 1);  
    // TQuote quote;
    tw::price::QuoteStore::TQuote quote;
    quote.setBid(TPrice(10157), TSize(10), 0, 1);
    quote.setAsk(TPrice(10159), TSize(20), 0, 1); 
    
    // ==> 0a. need 3 bars to proceed with soldier evaluation
    bars.resize(2);
    swings.resize(2);
    trends.resize(2);
    ASSERT_TRUE(!impl.isSignalTriggered(bars, swings, trends, side, quote, reason));
    ASSERT_EQ(reason, "bars.size() < 4 -- bars.size()=0");
    bars.resize(4);
    
    // ==> 0b. verify whether there have been ANY trades in the current bar    
    TBar& bar = bars[bars.size()-2];
    bar._range.set(5);
    bar._high.clear();
    bar._low.clear();
    bar._close.clear();
    bar._open.clear();
    tw::price::Ticks disp = bar._close - bar._open;
    ASSERT_TRUE(!impl.isSignalTriggered(bars, swings, trends, side, quote, reason));
    ASSERT_EQ(reason, "bar not valid; exiting");
    
    // check bar index (should not be assigned yet)
    ASSERT_EQ(bar._index, 0);
    
// ==> 1a. transition bar flaw
    bar._index = 1029;
    // bar._source = std::string("");
    bar._displayName = std::string("6CU2");
    bar._exchange = tw::instr::eExchange::kCME;
    bar._high = TPrice(9089);
    bar._low = TPrice(9082);
    bar._open = TPrice(9086);
    bar._close = TPrice(9086);
    bar._range.set(6);
    bar._volume = TSize(176);
    bar._numOfTrades = 139;
    bar._dir = tw::common_trade::ePatternDir::kUnknown;
    bar._duration = 60;
    bar._open_timestamp = tw::common::THighResTime::parseSqlTime("2012-01-17 14:41:01.548");
    disp = bar._close - bar._open;
    ASSERT_TRUE(!impl.isSignalTriggered(bars, swings, trends, side, quote, reason));
    ASSERT_EQ(reason, "_p._tBarLocated = false and not a transition, bar=1029,,6CU2,CME,9089,9082,9086,9086,6,176,139,Unknown,60,20120117-14:41:01.548000");
    
// ==> 1b. verify _p._tBar of desired characteristics
    // test tMaxRangeTicks 
    ASSERT_TRUE(!impl.isSignalTriggered(bars, swings, trends, side, quote, reason));
    ASSERT_EQ(reason, "candidate transition bar has invalid range, bar=1029,,6CU2,CME,9089,9082,9086,9086,6,176,139,Unknown,60,20120117-14:41:01.548000");
    
    // test tMinRangeTicks
    bar._high = TPrice(9084);
    bar._low = TPrice(9082);
    bar._range.set(2);
    ASSERT_TRUE(!impl.isSignalTriggered(bars, swings, trends, side, quote, reason));
    ASSERT_EQ(reason, "candidate transition bar has invalid range, bar=1029,,6CU2,CME,9084,9082,9086,9086,6,176,139,Unknown,60,20120117-14:41:01.548000"); 
            
// ==> 1c. verify ATR requirement on potential tBar
    bar._atr = 1.8;
    ASSERT_TRUE(!impl.isSignalTriggered(bars, swings, trends, side, quote, reason));
    ASSERT_EQ(reason, "_p._atr = 1.8 not sufficient for _p._tBar requirement; exiting");
    
    // now satisfy 1a, 1b, 1c and proceed to identify p._tBar:
    bar._high = TPrice(9087);
    bar._low = TPrice(9082);
    bar._open = TPrice(9086);
    bar._close = TPrice(9086);
    bar._range.set(5);
    bar._atr = 3.5;
    ASSERT_EQ(p._tDisp, bar._range);
    ASSERT_TRUE(p._tBarLocated);
    ASSERT_EQ(p._tBarIndex, bar._index);
    ASSERT_EQ(p._tHigh, bar._high);
    ASSERT_EQ(p._tLow, bar._low);
    ASSERT_TRUE(!p._tCramCompleted);
    // can also check top has been cleared
    ASSERT_TRUE(!impl.isSignalTriggered(bars, swings, trends, side, quote, reason));
    ASSERT_EQ(reason, "transition bar located, barLocated=true, barIndex=1029, bar=1029,,6CU2,CME,9087,9082,9086,9086,6,176,139,Unknown,60,20120117-14:41:01.548000"); 
    
// ==> 1d. check if _p._tBar is a "T" bar (or upside-down "T" bar)
    // create and up T bar
    p._tHigh = TPrice(9086);
    p._tBar._close = TPrice(9086);
    bar._close = TPrice(9086);
    
// ==> 2a. consider extending the length of original _p._tBar (once _p._tBar is located)
    bar._index = 1030;
    // higher high
    bar._high = TPrice(9091);
    ASSERT_TRUE(bar._index < p._tBarIndex + p._tMaxBarsToPersist);
    ASSERT_EQ(p._tHigh, bar._high);
    
    // lower low
    bar._low = TPrice(9080);
    ASSERT_TRUE(bar._index < p._tBarIndex + p._tMaxBarsToPersist);
    ASSERT_EQ(p._tLow, bar._low);
    
// ==> 2b. time lapse condition
    bar._index = 1033;
    ASSERT_TRUE(!impl.isSignalTriggered(bars, swings, trends, side, quote, reason));
    ASSERT_EQ(reason, "too much time since tBar, barIndex >= 1029 + 2");
    
// ==> 3b. check if read logic is satisfied
    quote.setTrade(TPrice(9091), TSize(1));
    // now define a series of trades
    // to qualify under the read logic
    quote.setTrade(TPrice(9091), TSize(2));
    quote.setTrade(TPrice(9092), TSize(11));
    quote.setTrade(TPrice(9091), TSize(4));
    quote.setTrade(TPrice(9091), TSize(2));
    quote.setTrade(TPrice(9092), TSize(3));
    quote.setTrade(TPrice(9091), TSize(1));
    
    std::string r_read;
    std::string r_flag;
    // 9091 > 9086 and _p._up_T_active
    ASSERT_TRUE(!impl.isReadCompleted(bars, quote, r_read, r_flag, p._tBar._close, p._tBar._close));
    
    quote.setTrade(TPrice(9081), TSize(2));
    quote.setTrade(TPrice(9082), TSize(11));
    quote.setTrade(TPrice(9081), TSize(4));
    quote.setTrade(TPrice(9081), TSize(2));
    quote.setTrade(TPrice(9082), TSize(3));
    quote.setTrade(TPrice(9081), TSize(1));
    
    // now create a down T bar
    p._tLow = TPrice(9086);
    p._tBar._close = TPrice(9086);
    // 9082 < 9086 and _p._down_T_active
    ASSERT_TRUE(!impl.isReadCompleted(bars, quote, r_read, r_flag, p._tBar._close, p._tBar._close));
    
    // finally a trade which qualifies for neither logic
    quote.setTrade(TPrice(9086), TSize(1));
    // 9086 = 9086
    ASSERT_TRUE(!impl.isReadCompleted(bars, quote, r_read, r_flag, p._tBar._close, p._tBar._close));
    ASSERT_EQ(r_flag, "NO READ");
    
    // as to actual read logic, tReadMaxTicks=2 here
    quote.setTrade(TPrice(9081), TSize(2));
    // 9081 <= 9086 - 2
    ASSERT_TRUE(!impl.isSignalTriggered(bars, swings, trends, side, quote, reason));
    ASSERT_EQ(reason, "read below violation: quote._trade._price=9081 <= 9086 - 2");
    
    // also as to read logic
    quote.setTrade(TPrice(9087), TSize(2));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideSell;
    ASSERT_EQ(p._tReadTradedAbove, TSize(2));
    
    // OR alternatively:
    // quote._trade._aggressorSide = tw::price::Trade::kAggressorSideUnknown;
    // quote._trade._price == quote._book[0]._ask._price
    
    // case we are in: 9087 > 9086, 9090 < 9086 + 2
    // for now, do not test readRatio
    // readRatio = _p._tReadTradedAbove / _p._tReadTradedBelow;
    // readRatio = _p._tReadTradedAbove; // if denominator=0
    
    // _p._tReadMinTraded = 20
    ASSERT_TRUE(!impl.isReadCompleted(bars, quote, r_read, r_flag, p._tBar._close, p._tBar._close));
    ASSERT_EQ(reason, "read above not satisfied: _p._tReadTradedAbove=2, quote._trade._price=9087, quote._trade._size=2 and readRatio=2 and bar=1029,,6CU2,CME,9089,9082,9086,9086,6,176,139,Unknown,60,20120117-14:41:01.548000");
    
    // ++ A. small range case
// -> 4a. next close condition
    side = tw::channel_or::eOrderSide::kUnknown;
    p._tHigh = TPrice(9087);
    p._tLow = TPrice(9084);
    p._tBar._close = TPrice(9085);
    // p._tDisp = 3;
    // recall: p._tBarLargeDispTicks = 3
    bar._index = 1030;
    bar._high = TPrice(9091);
    bar._low = TPrice(9085);
    bar._open = TPrice(9086);
    bar._close = TPrice(9089);
    bar._range.set(5);
    // recall p._tBarIndex = 1029
    // 3 <= 3 (p._tDisp <= p._tBarLargeDispTicks)
    // 9089 > 9087 (bar._close > p._tHigh)
    // go with up trend
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kBuy);
    ASSERT_TRUE(impl.isSignalTriggered(bars, swings, trends, side, quote, reason));
    
// -> 4b. cram condition (buy side)
    p._tHigh = TPrice(9087);
    p._tLow = TPrice(9084);
    p._tBar._close = TPrice(9087);
    side = tw::channel_or::eOrderSide::kUnknown;
    quote.setAsk(TPrice(9090), TSize(10), 0, 1);
    quote.setBid(TPrice(9088), TSize(10), 0, 1);
    
    // 9088 >= 9087
    // 9088 <= 9087 + 1
    // if (_p._top._bid._price != quote._book[0]._bid._price)
    std::string r_buy = "";
    ASSERT_TRUE(!impl.isInitialCramCompleted(tw::channel_or::eOrderSide::kBuy, p._tHigh, quote, r_buy));
    ASSERT_TRUE(impl.isSignalTriggered(bars, swings, trends, side, quote, reason));
    
    side = tw::channel_or::eOrderSide::kBuy;
    p._tHigh = TPrice(9088);
    p._tLow = TPrice(9082);
    quote.setInstrument(InstrHelper::get6CU2());
    // check cramStopEvalTicks: 9084 < 9088 + 2
    quote.setAsk(TPrice(9086), TSize(20), 0, 1);
    quote.setBid(TPrice(9084), TSize(10), 0, 1);
    ASSERT_TRUE(!impl.isInitialCramCompleted(side, p._tHigh, quote, reason));
    ASSERT_EQ(reason, "quote._book[0]._bid._price < 9088 + _p._tCramStopEvalTicks");
    // check tCramMaxTicks: 9092 > 9088 + 2
    quote.setAsk(TPrice(9093), TSize(21), 0, 1);
    quote.setBid(TPrice(9092), TSize(11), 0, 1);
    ASSERT_TRUE(!impl.isInitialCramCompleted(side, p._tHigh, quote, reason));
    ASSERT_EQ(reason, "quote._book[0]._bid._price > 9088 + _p._tCramMaxTicks");
    
// -> 4c. cram condition (sell side)
    p._tHigh = TPrice(9087);
    p._tLow = TPrice(9084);
    p._tBar._close = TPrice(9084);
    side = tw::channel_or::eOrderSide::kUnknown;
    quote.setInstrument(InstrHelper::get6CU2());
    quote.setAsk(TPrice(9083), TSize(10), 0, 1);
    quote.setBid(TPrice(9082), TSize(10), 0, 1);
    // (1) check _p._tCramStopEvalTicks: 9083 <= 9084
    // (2) check _p._tCramMaxTicks: 9083 >= 9084 - 1
    // (3) set top object prices if ( _p._top._ask._price != quote._book[0]._ask._price )
    // _p._top._bid._price = quote._book[0]._bid._price;
    //     _p._top._ask._price = quote._book[0]._ask._price;
    // (4a) check if enough size on ask compared to firm requirement
    // recall: p._tCramFirmPriceQty = 25
    quote.setAsk(TPrice(9083), TSize(10), 0, 1);
    ASSERT_TRUE(!impl.isInitialCramCompleted(side, p._tHigh, quote, reason));
    ASSERT_EQ(reason, "tCramFirmPriceQty > quote._book[0]._ask._size=10");
    // (4b) check if ask price is suff. below _p._tLow
    quote.setAsk(TPrice(9083), TSize(30), 0, 1);
    // recall p._tCramStopEvalTicks = 0
    // 9083 < 9084 - 0
    ASSERT_TRUE(p._tCramCompleted);
    ASSERT_EQ(reason, "FIRM SATISFIED && quote._book[0]._ask._price < 9083 - _p._tCramStopEvalTicks");
    
    // (4c) increment the top object sizes
    quote.setTrade(TPrice(9082), TSize(3));
    // here, 9082 < 9083, so update _p._top._bid._size
    ASSERT_EQ(TSize(3), p._top._bid._size);
    quote.setTrade(TPrice(9083), TSize(7));
    // here 9083 = 9083
    ASSERT_EQ(TSize(7), p._top._ask._size);
    quote.setTrade(TPrice(9086), TSize(1));
    ASSERT_TRUE(!impl.isInitialCramCompleted(side, p._tHigh, quote, reason));
    
    // now test trade of price which is too low
    quote.setTrade(TPrice(9081), TSize(1));
    ASSERT_TRUE(!impl.isInitialCramCompleted(side, p._tHigh, quote, reason));
    ASSERT_EQ(reason, "tCramFirmPriceQty > quote._book[0]._bid._size=1 AND quote._trade._price=9081 < _p._top._bid._price=9082");
    
    // (5a) recall: p._tCramVolGood = 25
    // easy way to ensure !p._tCramCompleted is through size not sufficient for firming
    quote.setAsk(TPrice(9083), TSize(20), 0, 1);
    ASSERT_TRUE(!p._tCramCompleted);
    
    ASSERT_TRUE(!impl.isInitialCramCompleted(side, p._tHigh, quote, reason));
    ASSERT_TRUE(p._top._ask._size < p._tCramVolGood);
    ASSERT_EQ(reason, "tCramFirmPriceQty > quote._book[0]._bid._size=7 AND _p._top._ask._size=20 < _p._tCramVolGood=25");
    
    // (5b) consider cramRatio, first have to ensure tCramVolGood satisfied
    // recall: p._tReadMinRatio = 2
    quote.setTrade(TPrice(9083), TSize(100));
    ASSERT_TRUE(impl.isInitialCramCompleted(side, p._tHigh, quote, reason));
    ASSERT_TRUE(p._tCramCompleted);
    ASSERT_EQ(reason, " tCramFirmPriceQty > quote._book[0]._bid._size=10 AND cramSlideRatio >= _p._tCramRatio");
    
    // LARGE DISP CASE
    // recall p._tBarLargeDispTicks = 3
    p._tHigh = TPrice(9088);
    p._tLow = TPrice(9084);
    p._tBar._close = TPrice(9085);
    
// ==> 4d. here a quote is too far outside the tBar range
    // recall p._tMaxOutsideTicks = 2
    quote.setBid(TPrice(9078), TSize(10), 0, 1);
    quote.setAsk(TPrice(9079), TSize(10), 0, 1);
    // 9079 < 9084 - 2
    ASSERT_TRUE(!impl.isSignalTriggered(bars, swings, trends, side, quote, reason));
    ASSERT_EQ(reason, "large _p._tDisp case: ask = 9079 < 9084 - 2");
    
// ==> 4e. here wait for a clue bar to close inside the tBar range
    // recall tBar:
    // p._tHigh = TPrice(9087);
    // p._tLow = TPrice(9084);
    // p._tBar._close = TPrice(9085);
    
    // recall bar:
    // bar._high = TPrice(9087);
    // bar._low = TPrice(9082);
    // bar._open = TPrice(9085);
    // bar._close = TPrice(9086);
    // bar._range.set(5);
    // bar._atr = 3.5;
    
    // :: 9086 >= 9084 && 9086 <= 9087
    // go with up trend 
    // :: 9086 > 9085
    ASSERT_TRUE(impl.isSignalTriggered(bars, swings, trends, side, quote, reason));
    // final check: do double taps
    // if ( (_p._tBarIndexPrev == _p._tBarIndex) && (_p._tSidePrev == side) )
    //    side_notation = "trying to trade same side again off same _p._tBar";
}

TEST(CommonTradeLibTestSuit, transitionSignalProcessor_test_readLogic1)
{
    // until method is vetted
    return;
    
    tw::channel_or::eOrderSide side;
    std::string reason;
    TParams params;
    TBars bars;
    TBarPatterns swings;
    TBarPatterns trends;
    TImpl impl(params);
    impl.clear();
    TParams& p = impl.getParams();
    p = getParams(TPrice(0), TPrice(1), TSize(25), TSize(25), 2, 2.5, TPrice(2), TPrice(6), TPrice(3), TPrice(2), 2, 2, TSize(20), TPrice(2));
//    p._tCramStopEvalTicks = 0
//    p._tCramMaxTicks = 1
//    p._tCramFirmPriceQty = 25
//    p._tCramVolGood = 25
//    p._tCramRatio = 2
//    p._atr_required = 2.5
//    p._tMinRangeTicks = 2
//    p._tMaxRangeTicks = 6
//    p._tBarLargeDispTicks = 3
//    p._tMaxOutsideTicks = 2
//    p._tMaxBarsToPersist = 2
//    p._tReadMinRatio = 2
//    p._tReadMinTraded = 20
//    p._tReadMaxTicks = 2
    
    tw::price::QuoteStore::TQuote quote;
    quote.setBid(TPrice(9084), TSize(10), 0, 1);
    quote.setAsk(TPrice(9085), TSize(20), 0, 1); 
    bars.resize(4);
    swings.resize(4);
    trends.resize(4);
    TBar& bar = bars[bars.size()-2];
    
// -> 1c. transition bar OK
    bar._index = 1029;
    // bar._source = std::string("");
    bar._displayName = std::string("6CU2");
    bar._exchange = tw::instr::eExchange::kCME;
    bar._high = TPrice(9087);
    bar._low = TPrice(9082);
    bar._open = TPrice(9086);
    bar._close = TPrice(9086);
    bar._range.set(5);
    bar._volume = TSize(176);
    bar._numOfTrades = 139;
    bar._dir = tw::common_trade::ePatternDir::kUnknown;
    bar._duration = 60;
    bar._open_timestamp = tw::common::THighResTime::parseSqlTime("2012-01-17 14:41:00.321");
    bar._atr = 3.5;
    tw::price::Ticks disp = bar._close - bar._open;
    ASSERT_TRUE(!impl.isSignalTriggered(bars, swings, trends, side, quote, reason));
    ASSERT_EQ(p._tDisp, bar._range);
    ASSERT_TRUE(p._tBarLocated);
    ASSERT_EQ(p._tBarIndex, bar._index);
    ASSERT_EQ(p._tHigh, bar._high);
    ASSERT_EQ(p._tLow, bar._low);
    ASSERT_TRUE(!p._tCramCompleted);
    // can also check top has been cleared
    ASSERT_TRUE(!impl.isSignalTriggered(bars, swings, trends, side, quote, reason));
    ASSERT_EQ(reason, "transition bar located, barLocated=true, barIndex=1029, bar=1029,,6CU2,CME,9087,9082,9086,9086,6,176,139,Unknown,60,20120117-14:41:00.321000"); 

// -> 3. check if read logic is satisfied
    quote.setTrade(TPrice(9083), TSize(1));
    // cause p._tSellAllowed to be set to false
    std::string r_read;
    std::string r_flag;
    ASSERT_TRUE(!impl.isReadCompleted(bars, quote, r_read, r_flag, p._tBar._close, p._tBar._close));
    ASSERT_EQ(r_read, "read below violation: quote._trade._price=9083 <= 9086 - 2");
    ASSERT_TRUE(!p._tSellAllowed);
    
    // now reset to have a clean slate:
    impl.clear();
    bar._index = 1030;
    // bar._source = std::string("");
    bar._displayName = std::string("6CU2");
    bar._exchange = tw::instr::eExchange::kCME;
    bar._high = TPrice(9087);
    bar._low = TPrice(9082);
    bar._open = TPrice(9086);
    bar._close = TPrice(9086);
    bar._range.set(5);
    bar._volume = TSize(176);
    bar._numOfTrades = 139;
    bar._dir = tw::common_trade::ePatternDir::kUnknown;
    bar._duration = 60;
    bar._open_timestamp = tw::common::THighResTime::parseSqlTime("2012-01-17 14:42:00.642");
    bar._atr = 3.5;
    disp = bar._close - bar._open;
    ASSERT_TRUE(!impl.isSignalTriggered(bars, swings, trends, side, quote, reason));
    ASSERT_EQ(reason, "transition bar located, barLocated=true, barIndex=1029, bar=1030,,6CU2,CME,9087,9082,9086,9086,6,176,139,Unknown,60,20120117-14:42:00.642000"); 
    ASSERT_EQ(p._tHigh, TPrice(9087));
    ASSERT_EQ(p._tLow, TPrice(9082));
    ASSERT_EQ(p._tBar._close, TPrice(9086));
    
    ASSERT_EQ(p._tReadTradedBelow, TSize(0));
    ASSERT_EQ(p._tReadTradedAbove, TSize(0));
    quote.setTrade(TPrice(9085), TSize(1));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideBuy;
    // 9085 < 9086
    ASSERT_TRUE(!impl.isReadCompleted(bars, quote, r_read, r_flag, p._tBar._close, p._tBar._close));
    ASSERT_EQ(r_read, "read below not satisfied: increment: no add below, _p._tReadTradedBelow=0, quote._trade._price=9085, quote._book[0]._bid._price=9084, quote._trade._size=1, quote._trade._aggressorSide=1 and readRatio=0 and bar=1030,,6CU2,CME,9087,9082,9086,9086,6,176,139,Unknown,60,20120117-14:42:00.642000");
    
    quote.setTrade(TPrice(9085), TSize(1));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideBuy;
    
    // process another quote
    quote.setTrade(TPrice(9084), TSize(2));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideSell;
    ASSERT_TRUE(!impl.isReadCompleted(bars, quote, r_read, r_flag, p._tBar._close, p._tBar._close));
    ASSERT_EQ(p._tReadTradedBelow, 2);
    
    quote.setTrade(TPrice(9084), TSize(3));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideSell;
    ASSERT_TRUE(!impl.isReadCompleted(bars, quote, r_read, r_flag, p._tBar._close, p._tBar._close));
    ASSERT_EQ(p._tReadTradedBelow, 5);
    
    quote.setTrade(TPrice(9084), TSize(3));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideSell;
    ASSERT_TRUE(!impl.isReadCompleted(bars, quote, r_read, r_flag, p._tBar._close, p._tBar._close));
    ASSERT_EQ(p._tReadTradedBelow, 8);
    ASSERT_EQ(r_read, "read below not satisfied: increment: added below 2, _p._tReadTradedBelow=7, quote._trade._price=9084, quote._book[0]._bid._price=9084, quote._trade._size=1, quote._trade._aggressorSide=2 and readRatio=8 and bar=1030,,6CU2,CME,9087,9082,9086,9086,6,176,139,Unknown,60,20120117-14:42:00.642000");
    
    // now for some above, to affect readRatio
    quote.setTrade(TPrice(9085), TSize(2));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideBuy;
    ASSERT_TRUE(!impl.isReadCompleted(bars, quote, r_read, r_flag, p._tBar._close, p._tBar._close));
    ASSERT_EQ(p._tReadTradedAbove, TSize(2));
    ASSERT_EQ(r_read, "read below not satisfied: increment: added below 2, _p._tReadTradedBelow=7, quote._trade._price=9084, quote._book[0]._bid._price=9084, quote._trade._size=1, quote._trade._aggressorSide=2 and readRatio=4 and bar=1030,,6CU2,CME,9087,9082,9086,9086,6,176,139,Unknown,60,20120117-14:42:00.642000");
    // recall settings for read logic:
    // p._tReadMinRatio = 2
    // p._tReadMinTraded = 20
    quote.setTrade(TPrice(9084), TSize(14));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideSell;
    ASSERT_TRUE(impl.isReadCompleted(bars, quote, r_read, r_flag, p._tBar._close, p._tBar._close));
    ASSERT_EQ(p._tReadTradedBelow, 21);
    ASSERT_EQ(r_read, "read below SATISFIED: increment: added below 14, _p._tReadTradedBelow=21, quote._trade._price=9084, quote._book[0]._bid._price=9084, quote._trade._size=14, quote._trade._aggressorSide=2 and readRatio=11 and bar=1030,,6CU2,CME,9087,9082,9086,9086,6,176,139,Unknown,60,20120117-14:42:00.642000");
}

TEST(CommonTradeLibTestSuit, transitionSignalProcessor_test_clueBar)
{
    // until method is vetted
    return;
    
    tw::channel_or::eOrderSide side;
    std::string reason;
    TParams params;
    TBars bars;
    TBarPatterns swings;
    TBarPatterns trends;
    TImpl impl(params);
    impl.clear();
    TParams& p = impl.getParams();
    p = getParams(TPrice(0), TPrice(1), TSize(25), TSize(25), 2, 2.5, TPrice(2), TPrice(6), TPrice(3), TPrice(2), 2, 2, TSize(20), TPrice(2));
//    p._tCramStopEvalTicks = 0
//    p._tCramMaxTicks = 1
//    p._tCramFirmPriceQty = 25
//    p._tCramVolGood = 25
//    p._tCramRatio = 2
//    p._atr_required = 2.5
//    p._tMinRangeTicks = 2
//    p._tMaxRangeTicks = 6
//    p._tBarLargeDispTicks = 3
//    p._tMaxOutsideTicks = 2
//    p._tMaxBarsToPersist = 2
//    p._tReadMinRatio = 2
//    p._tReadMinTraded = 20
//    p._tReadMaxTicks = 2
    
    // set quote in the tBar range
    tw::price::QuoteStore::TQuote quote;
    quote.setBid(TPrice(9084), TSize(10), 0, 1);
    quote.setAsk(TPrice(9085), TSize(20), 0, 1); 
    bars.resize(4);
    swings.resize(4);
    trends.resize(4);
    TBar& bar = bars[bars.size()-2];
    
    bar._index = 1029;
    // bar._source = std::string("");
    bar._displayName = std::string("6CU2");
    bar._exchange = tw::instr::eExchange::kCME;
    bar._high = TPrice(9087);
    bar._low = TPrice(9082);
    bar._open = TPrice(9086);
    bar._close = TPrice(9086);
    bar._range.set(5);
    bar._volume = TSize(176);
    bar._numOfTrades = 139;
    bar._dir = tw::common_trade::ePatternDir::kUnknown;
    bar._duration = 60;
    bar._open_timestamp = tw::common::THighResTime::parseSqlTime("2012-01-17 14:41:00.321");
    bar._atr = 3.5;
    // disp = bar._close - bar._open;
    ASSERT_TRUE(!impl.isSignalTriggered(bars, swings, trends, side, quote, reason));
    ASSERT_EQ(p._tDisp, bar._range);
    ASSERT_TRUE(p._tBarLocated);
    ASSERT_EQ(p._tBarIndex, bar._index);
    ASSERT_EQ(p._tHigh, bar._high);
    ASSERT_EQ(p._tLow, bar._low);
    ASSERT_TRUE(!p._tCramCompleted);
    
    // read logic has to fail to proceed with clueBar logic
    // easy way is to have trade occur at tBar._close
    quote.setTrade(TPrice(9083), TSize(1));
    bar._index = 1030;
    bar._high = TPrice(9086);
    bar._low = TPrice(9082);
    bar._open = TPrice(9086);
    bar._close = TPrice(9085);
    bar._range.set(4);
    
// -> 4e. here wait for a clue bar to close inside the tBar range
    // 9085 >= 9082 && 9085 <= 9087 ... go with down trend: 9085 < 9086
    ASSERT_TRUE(impl.isSignalTriggered(bars, swings, trends, side, quote, reason));
}

TEST(CommonTradeLibTestSuit, transitionSignalProcessor_test_readLogic2)
{
    // until method is vetted
    return;
    
    tw::channel_or::eOrderSide side;
    std::string reason;
    TParams params;
    TBars bars;
    TBarPatterns swings;
    TBarPatterns trends;
    TImpl impl(params);
    impl.clear();
    TParams& p = impl.getParams();
    p = getParams(TPrice(0), TPrice(1), TSize(25), TSize(25), 2, 2.5, TPrice(2), TPrice(6), TPrice(3), TPrice(2), 2, 2, TSize(20), TPrice(2));
//    p._tCramStopEvalTicks = 0
//    p._tCramMaxTicks = 1
//    p._tCramFirmPriceQty = 25
//    p._tCramVolGood = 25
//    p._tCramRatio = 2
//    p._atr_required = 2.5
//    p._tMinRangeTicks = 2
//    p._tMaxRangeTicks = 6
//    p._tBarLargeDispTicks = 3
//    p._tMaxOutsideTicks = 2
//    p._tMaxBarsToPersist = 2
//    p._tReadMinRatio = 2
//    p._tReadMinTraded = 20
//    p._tReadMaxTicks = 2
    
    // set quote
    tw::price::QuoteStore::TQuote quote;
    quote.setBid(TPrice(9085), TSize(10), 0, 1);
    quote.setAsk(TPrice(9086), TSize(20), 0, 1); 
    bars.resize(4);
    swings.resize(4);
    trends.resize(4);
    TBar& bar = bars[bars.size()-2];
    
    bar._index = 1029;
    // bar._source = std::string("");
    bar._displayName = std::string("6CU2");
    bar._exchange = tw::instr::eExchange::kCME;
    bar._high = TPrice(9087);
    bar._low = TPrice(9082);
    bar._open = TPrice(9086);
    bar._close = TPrice(9086);
    bar._range.set(5);
    bar._volume = TSize(176);
    bar._numOfTrades = 139;
    bar._dir = tw::common_trade::ePatternDir::kUnknown;
    bar._duration = 60;
    bar._open_timestamp = tw::common::THighResTime::parseSqlTime("2012-01-17 14:41:00.321");
    bar._atr = 3.5;
    // tw::price::Ticks disp = bar._close - bar._open;
    ASSERT_TRUE(!impl.isSignalTriggered(bars, swings, trends, side, quote, reason));
    ASSERT_EQ(p._tDisp, bar._range);
    ASSERT_TRUE(p._tBarLocated);
    ASSERT_EQ(p._tBarIndex, bar._index);
    ASSERT_EQ(p._tHigh, bar._high);
    ASSERT_EQ(p._tLow, bar._low);
    ASSERT_TRUE(!p._tCramCompleted);
    
    // recall: 
    // p._tReadMinTraded = 20
    // p._tReadMinRatio = 2
    std::string r_read;
    std::string r_flag;
    quote.setTrade(TPrice(9085), TSize(12));
    // if Unknown, ensure the trade prices occurs on the bid of the quote
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideSell;
    ASSERT_TRUE(!impl.isReadCompleted(bars, quote, r_read, r_flag, p._tBar._close, p._tBar._close));
    ASSERT_EQ(p._tReadTradedBelow, TSize(12));
    
    quote.setTrade(TPrice(9087), TSize(6));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideBuy;
    ASSERT_TRUE(!impl.isReadCompleted(bars, quote, r_read, r_flag, p._tBar._close, p._tBar._close));
    ASSERT_EQ(p._tReadTradedAbove, TSize(6));
    // ratio is computed with numerator on the side recently traded
    ASSERT_NEAR(p._tReadRatio, 0.5, EPSILON);
    
    quote.setTrade(TPrice(9085), TSize(12));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideSell;
    ASSERT_TRUE(impl.isReadCompleted(bars, quote, r_read, r_flag, p._tBar._close, p._tBar._close));
    ASSERT_EQ(p._tReadTradedBelow, TSize(24));
    // ratio is computed with numerator on the side recently traded
    ASSERT_NEAR(p._tReadRatio, 4.0, EPSILON);
    ASSERT_EQ(r_read, "read below SATISFIED: increment: added below, _p._tReadTradedBelow=24, quote._trade._price=9085, quote._book[0]._bid._price=9085, quote._trade._size=12, quote._trade._aggressorSide=2 and bar=1029,,6CU2,CME,9087,9082,9086,9086,6,176,139,Unknown,60,20120117-14:41:00.321000");
}

TEST(CommonTradeLibTestSuit, transitionSignalProcessor_test_readLogic3)
{
    // until method is vetted
    return;
    
    tw::channel_or::eOrderSide side;
    std::string reason;
    TParams params;
    TBars bars;
    TBarPatterns swings;
    TBarPatterns trends;
    
    TImpl impl(params);
    impl.clear();
    TParams& p = impl.getParams();
    p = getParams(TPrice(0), TPrice(1), TSize(25), TSize(25), 2, 2.5, TPrice(2), TPrice(6), TPrice(3), TPrice(2), 2, 2, TSize(20), TPrice(2));
    
    // set quote
    tw::price::QuoteStore::TQuote quote;
    quote.setBid(TPrice(9085), TSize(10), 0, 1);
    quote.setAsk(TPrice(9086), TSize(20), 0, 1); 
    bars.resize(4);
    swings.resize(4);
    trends.resize(4);
    TBar& bar = bars[bars.size()-2];
    
    bar._index = 1029;
    // bar._source = std::string("");
    bar._displayName = std::string("6CU2");
    bar._exchange = tw::instr::eExchange::kCME;
    bar._high = TPrice(9087);
    bar._low = TPrice(9082);
    bar._open = TPrice(9086);
    bar._close = TPrice(9086);
    bar._range.set(5);
    bar._volume = TSize(176);
    bar._numOfTrades = 139;
    bar._dir = tw::common_trade::ePatternDir::kUnknown;
    bar._duration = 60;
    bar._open_timestamp = tw::common::THighResTime::parseSqlTime("2012-01-17 14:41:00.321");
    bar._atr = 3.5;
    // tw::price::Ticks disp = bar._close - bar._open;
    ASSERT_TRUE(!impl.isSignalTriggered(bars, swings, trends, side, quote, reason));
    ASSERT_EQ(p._tDisp, bar._range);
    ASSERT_TRUE(p._tBarLocated);
    ASSERT_EQ(p._tBarIndex, bar._index);
    ASSERT_EQ(p._tHigh, bar._high);
    ASSERT_EQ(p._tLow, bar._low);
    ASSERT_TRUE(!p._tCramCompleted);
    
    // recall: 
    // p._tReadMinTraded = 20
    // p._tReadMinRatio = 2
    std::string r_read;
    std::string r_flag;
    quote.setTrade(TPrice(9085), TSize(12));
    // if Unknown, ensure the trade prices occurs on the bid of the quote
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideSell;
    ASSERT_TRUE(!impl.isReadCompleted(bars, quote, r_read, r_flag, p._tBar._close, p._tBar._close));
    ASSERT_EQ(p._tReadTradedBelow, TSize(12));
}

TEST(CommonTradeLibTestSuit, transitionSignalProcessor_test_readLogic4)
{
    // until method is vetted
    return;
    
    tw::channel_or::eOrderSide side;
    std::string reason;
    TParams params;
    TBars bars;
    TBarPatterns swings;
    TBarPatterns trends;
    TImpl impl(params);
    impl.clear();
    TParams& p = impl.getParams();
    p = getParams(TPrice(0), TPrice(1), TSize(25), TSize(25), 2, 2.5, TPrice(2), TPrice(6), TPrice(3), TPrice(2), 2, 2, TSize(20), TPrice(2));
    
    // set quote here above tBar
    tw::price::QuoteStore::TQuote quote;
    quote.setBid(TPrice(9085), TSize(10), 0, 1);
    quote.setAsk(TPrice(9086), TSize(20), 0, 1); 
    bars.resize(4);
    swings.resize(4);
    trends.resize(4);
    TBar& bar = bars[bars.size()-2];
    
    bar._index = 1029;
    // bar._source = std::string("");
    bar._displayName = std::string("6CU2");
    bar._exchange = tw::instr::eExchange::kCME;
    bar._high = TPrice(9084);
    bar._low = TPrice(9081);
    bar._open = TPrice(9082);
    bar._close = TPrice(9082);
    bar._range.set(5);
    bar._volume = TSize(176);
    bar._numOfTrades = 139;
    bar._dir = tw::common_trade::ePatternDir::kUnknown;
    bar._duration = 60;
    bar._open_timestamp = tw::common::THighResTime::parseSqlTime("2012-01-17 14:41:00.321");
    bar._atr = 3.5;
    // tw::price::Ticks disp = bar._close - bar._open;
    ASSERT_TRUE(!impl.isSignalTriggered(bars, swings, trends, side, quote, reason));
    ASSERT_EQ(p._tDisp, bar._range);
    ASSERT_TRUE(p._tBarLocated);
    ASSERT_EQ(p._tBarIndex, bar._index);
    ASSERT_EQ(p._tHigh, bar._high);
    ASSERT_EQ(p._tLow, bar._low);
    ASSERT_TRUE(!p._tCramCompleted);
    
    // recall: 
    // p._tReadMinTraded = 20
    // p._tReadMinRatio = 2
    std::string r_read;
    std::string r_flag;
    quote.setTrade(TPrice(9085), TSize(12));
    // if Unknown, ensure the trade prices occurs on the bid of the quote
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideSell;
    ASSERT_TRUE(!impl.isReadCompleted(bars, quote, r_read, r_flag, p._tBar._close, p._tBar._close));
    ASSERT_EQ(p._tReadTradedBelow, TSize(12));
}


} // common_trade
} // tw

