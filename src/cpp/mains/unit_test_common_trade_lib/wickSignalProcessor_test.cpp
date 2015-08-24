#include <vector>

#include <tw/common_strat/defs.h>
#include <tw/generated/common_trade_blocks.h>
#include <tw/common_trade/wickSignalProcessor.h>
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

typedef WickSignalProcessor TImpl;
typedef WickSignalParamsWire TParams;

static TParams getParams(uint32_t minWickBarTicks,
                         uint32_t minPatternTicks, 
                         float wickToBodyRatio, 
                         float wickBarToSwingRatio, 
                         bool useBodyInWickToBodyRatio,
                         bool wickOutsidePrevHighLow,
                         eWickBarToPatternDir dir) {
    TParams p;
    
    p._minWickBarTicks.set(minWickBarTicks);
    p._minPatternTicks.set(minPatternTicks);
    p._wickToBodyRatio = wickToBodyRatio;
    p._wickBarToSwingRatio = wickBarToSwingRatio;
    p._useBodyInWickToBodyRatio = useBodyInWickToBodyRatio;
    p._wickOutsidePrevHighLow = wickOutsidePrevHighLow;
    p._wickBarToPatternDir = dir;
    
    return p;
}


TEST(CommonTradeLibTestSuit, wickSignalProcessor_test_buy)
{
    tw::channel_or::eOrderSide side;
    std::string reason;
    TParams params;
    
    TBars bars;
    TBarPattern pattern;
    
    TImpl impl(params);
    TParams& p = impl.getParams();
    p = getParams(6, 30, 1.5, 0.5, true, false, eWickBarToPatternDir::kEither);
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(reason, "bars.size() < 3 -- bars.size()=0");
    
    bars.resize(4);
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(reason, "pattern._firstBarIndex == pattern._lastBarIndex -- pattern=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Unknown,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,");
    
    pattern._firstBarIndex = 1;            
    pattern._lastBarIndex = 2;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(reason, "pattern._range < _p._minPatternTicks -- pattern=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Unknown,0,00000000-00:00:00.000000,,Unknown,Unknown,1,2,0,0,0,");
    
    pattern._range.set(30);
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(reason, "bar._range < _p._minWickBarTicks -- bar=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Unknown,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    
    TBar& bar = bars[bars.size()-2];
    
    bar._range.set(17);
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(reason, "wickBarToSwingRatio > _p._wickBarToSwingRatio -- wickBarToSwingRatio=0.57,bar=0,,,Unknown,InV,InV,InV,InV,17,0,0,Unknown,0,00000000-00:00:00.000000,,0,0,0,0,0,,.pattern=0,,,Unknown,InV,InV,InV,InV,30,0,0,Unknown,0,00000000-00:00:00.000000,,Unknown,Unknown,1,2,0,0,0,");
    
    bar._range.set(7);
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(reason, "tw::common_trade::ePatternDir::kUnknown == pattern._dir -- pattern=0,,,Unknown,InV,InV,InV,InV,30,0,0,Unknown,0,00000000-00:00:00.000000,,Unknown,Unknown,1,2,0,0,0,");
    
    pattern._dir = tw::common_trade::ePatternDir::kDown;
    bar._dir = tw::common_trade::ePatternDir::kDown;
    p._wickBarToPatternDir = eWickBarToPatternDir::kOpposite;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(reason, "eWickBarToPatternDir::kOpposite && tw::common_trade::ePatternDir::kDown == bar._dir -- bar=0,,,Unknown,InV,InV,InV,InV,7,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,pattern=0,,,Unknown,InV,InV,InV,InV,30,0,0,Down,0,00000000-00:00:00.000000,,Unknown,Unknown,1,2,0,0,0,");
    
    pattern._dir = tw::common_trade::ePatternDir::kUp;
    bar._dir = tw::common_trade::ePatternDir::kDown;
    p._wickBarToPatternDir = eWickBarToPatternDir::kSame;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(reason, "eWickBarToPatternDir::kSame && tw::common_trade::ePatternDir::kDown == bar._dir -- bar=0,,,Unknown,InV,InV,InV,InV,7,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,pattern=0,,,Unknown,InV,InV,InV,InV,30,0,0,Up,0,00000000-00:00:00.000000,,Unknown,Unknown,1,2,0,0,0,");
    
    pattern._dir = tw::common_trade::ePatternDir::kDown;
    p._wickBarToPatternDir = eWickBarToPatternDir::kEither;
    pattern._lowBarIndex = 1;
    bar._index = 3;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(reason, "pattern._lowBarIndex != bar._index -- bar=3,,,Unknown,InV,InV,InV,InV,7,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,pattern=0,,,Unknown,InV,InV,InV,InV,30,0,0,Down,0,00000000-00:00:00.000000,,Unknown,Unknown,1,2,0,1,0,");
    
    
    pattern._lowBarIndex = 3;
    
    bar._open.set(102);
    bar._close.set(100);
    
    bar._high.set(105);
    bar._low.set(98);
    bar._dir = tw::common_trade::ePatternDir::kDown;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(reason, "wickToBodyRatio < _p._wickToBodyRatio -- wick=2,body=2,wickToBodyRatio=1,bar=3,,,Unknown,105,98,102,100,7,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    
    p._useBodyInWickToBodyRatio = false;
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(reason, "wickToBodyRatio < _p._wickToBodyRatio -- wick=2,body=5,wickToBodyRatio=0.4,bar=3,,,Unknown,105,98,102,100,7,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    
    
    p._useBodyInWickToBodyRatio = true;
    
    bar._open.set(100);
    bar._close.set(102);
    
    bar._high.set(105);
    bar._low.set(98);
    bar._dir = tw::common_trade::ePatternDir::kUp;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(reason, "wickToBodyRatio < _p._wickToBodyRatio -- wick=2,body=2,wickToBodyRatio=1,bar=3,,,Unknown,105,98,100,102,7,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    
    p._useBodyInWickToBodyRatio = false;    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(reason, "wickToBodyRatio < _p._wickToBodyRatio -- wick=2,body=5,wickToBodyRatio=0.4,bar=3,,,Unknown,105,98,100,102,7,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    
        
    p._useBodyInWickToBodyRatio = true;
    
    bar._open.set(104);
    bar._close.set(103);
    
    bar._high.set(105);
    bar._low.set(98);
    bar._dir = tw::common_trade::ePatternDir::kDown;
    
    ASSERT_TRUE(impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(reason, "WickSignalProcessor::isSignalTriggered()==true -- wick=5,body=1,wickBarToSwingRatio=0.23,wickToBodyRatio=5,trigger_bar=3,,,Unknown,105,98,104,103,7,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,,pattern=0,,,Unknown,InV,InV,InV,InV,30,0,0,Down,0,00000000-00:00:00.000000,,Unknown,Unknown,1,2,0,3,0,");
    
    p._useBodyInWickToBodyRatio = false;
    side = tw::channel_or::eOrderSide::kUnknown;
    ASSERT_TRUE(impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(reason, "WickSignalProcessor::isSignalTriggered()==true -- wick=5,body=2,wickBarToSwingRatio=0.23,wickToBodyRatio=2.5,trigger_bar=3,,,Unknown,105,98,104,103,7,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,,pattern=0,,,Unknown,InV,InV,InV,InV,30,0,0,Down,0,00000000-00:00:00.000000,,Unknown,Unknown,1,2,0,3,0,");
        
    bars[0]._close.set(102);
    bars[1]._close.set(104);
    p._wickOutsidePrevHighLow = true;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(reason, "wickHigh > bars[i-1]._close -- wick=5,wickHigh=103,bar=3,,,Unknown,105,98,104,103,7,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,,bars[1]=0,,,Unknown,InV,InV,InV,102,InV,0,0,Unknown,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    
    
    bars[0]._close.set(103);
    side = tw::channel_or::eOrderSide::kUnknown;
    ASSERT_TRUE(impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(reason, "WickSignalProcessor::isSignalTriggered()==true -- wick=5,body=2,wickBarToSwingRatio=0.23,wickToBodyRatio=2.5,trigger_bar=3,,,Unknown,105,98,104,103,7,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,,pattern=0,,,Unknown,InV,InV,InV,InV,30,0,0,Down,0,00000000-00:00:00.000000,,Unknown,Unknown,1,2,0,3,0,");
    
    
    // Test isSignalValid() logic
    //
    TPrice activePrice;
    bool placeOrder = false;
    TBars tempBars;
    
    ASSERT_TRUE(impl.isSignalValid(bars, activePrice, side, placeOrder, reason));
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kBuy);
    ASSERT_TRUE(placeOrder);
    ASSERT_EQ(reason, "!_p._validateSignal");
    
    p._validateSignal = true;
    
    ASSERT_TRUE(impl.isSignalValid(bars, activePrice, side, placeOrder, reason));
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kBuy);
    ASSERT_TRUE(!placeOrder);
    ASSERT_EQ(reason, "!activePrice.isValid()");
    
    activePrice.set(100);
            
    ASSERT_TRUE(!impl.isSignalValid(tempBars, activePrice, side, placeOrder, reason));
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kBuy);
    ASSERT_TRUE(!placeOrder);
    ASSERT_EQ(reason, "bars.size() < 2 -- bars.size()=0");
    
    tempBars.resize(2);
    
    ASSERT_TRUE(impl.isSignalValid(tempBars, activePrice, side, placeOrder, reason));
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kBuy);
    ASSERT_TRUE(!placeOrder);
    ASSERT_EQ(reason, "!currBar._close.isValid()");
    
    tempBars[1]._close.set(100);
    
    ASSERT_TRUE(!impl.isSignalValid(tempBars, activePrice, side, placeOrder, reason));
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kBuy);
    ASSERT_TRUE(!placeOrder);
    ASSERT_EQ(reason, "!signalBar._close.isValid()");
    
    tempBars[0]._close.set(104);
    tempBars[1]._low.set(100);
    
    p._maxAwayFromCloseTicks.set(3);
    ASSERT_TRUE(!impl.isSignalValid(tempBars, activePrice, side, placeOrder, reason));
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kBuy);
    ASSERT_TRUE(!placeOrder);
    ASSERT_EQ(reason, "currBar._low < (signalBar._close-_p._maxAwayFromCloseTicks) -- currBar=0,,,Unknown,InV,100,InV,100,InV,0,0,Unknown,0,00000000-00:00:00.000000,,0,0,0,0,0,,,signalBar=0,,,Unknown,InV,InV,InV,104,InV,0,0,Unknown,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    
    p._maxAwayFromCloseTicks.set(4);
    ASSERT_TRUE(impl.isSignalValid(tempBars, activePrice, side, placeOrder, reason));
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kBuy);
    ASSERT_TRUE(!placeOrder);
    ASSERT_EQ(reason, "");
    
    activePrice.set(105);
    ASSERT_TRUE(impl.isSignalValid(tempBars, activePrice, side, placeOrder, reason));
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kBuy);
    ASSERT_TRUE(placeOrder);
    ASSERT_EQ(reason, "activePrice > signalBar._close -- activePrice=105,signalBar=0,,,Unknown,InV,InV,InV,104,InV,0,0,Unknown,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    
    
}




TEST(CommonTradeLibTestSuit, wickSignalProcessor_test_sell)
{
    tw::channel_or::eOrderSide side;
    std::string reason;
    TParams params;
    
    TBars bars;
    TBarPattern pattern;
    
    TImpl impl(params);
    TParams& p = impl.getParams();
    p = getParams(6, 30, 1.5, 0.5, true, false, eWickBarToPatternDir::kEither);
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(reason, "bars.size() < 3 -- bars.size()=0");
    
    bars.resize(4);
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(reason, "pattern._firstBarIndex == pattern._lastBarIndex -- pattern=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Unknown,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,");
    
    pattern._firstBarIndex = 1;            
    pattern._lastBarIndex = 2;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(reason, "pattern._range < _p._minPatternTicks -- pattern=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Unknown,0,00000000-00:00:00.000000,,Unknown,Unknown,1,2,0,0,0,");
    
    pattern._range.set(30);
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(reason, "bar._range < _p._minWickBarTicks -- bar=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Unknown,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    
    TBar& bar = bars[bars.size()-2];
    
    bar._range.set(17);
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(reason, "wickBarToSwingRatio > _p._wickBarToSwingRatio -- wickBarToSwingRatio=0.57,bar=0,,,Unknown,InV,InV,InV,InV,17,0,0,Unknown,0,00000000-00:00:00.000000,,0,0,0,0,0,,.pattern=0,,,Unknown,InV,InV,InV,InV,30,0,0,Unknown,0,00000000-00:00:00.000000,,Unknown,Unknown,1,2,0,0,0,");
    
    bar._range.set(7);
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(reason, "tw::common_trade::ePatternDir::kUnknown == pattern._dir -- pattern=0,,,Unknown,InV,InV,InV,InV,30,0,0,Unknown,0,00000000-00:00:00.000000,,Unknown,Unknown,1,2,0,0,0,");
    
    pattern._dir = tw::common_trade::ePatternDir::kUp;
    bar._dir = tw::common_trade::ePatternDir::kUp;
    p._wickBarToPatternDir = eWickBarToPatternDir::kOpposite;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(reason, "eWickBarToPatternDir::kOpposite && tw::common_trade::ePatternDir::kUp == bar._dir -- bar=0,,,Unknown,InV,InV,InV,InV,7,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,pattern=0,,,Unknown,InV,InV,InV,InV,30,0,0,Up,0,00000000-00:00:00.000000,,Unknown,Unknown,1,2,0,0,0,");
 
    pattern._dir = tw::common_trade::ePatternDir::kDown;
    bar._dir = tw::common_trade::ePatternDir::kUp;
    p._wickBarToPatternDir = eWickBarToPatternDir::kSame;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(reason, "eWickBarToPatternDir::kSame && tw::common_trade::ePatternDir::kUp == bar._dir -- bar=0,,,Unknown,InV,InV,InV,InV,7,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,pattern=0,,,Unknown,InV,InV,InV,InV,30,0,0,Down,0,00000000-00:00:00.000000,,Unknown,Unknown,1,2,0,0,0,");
    
    pattern._dir = tw::common_trade::ePatternDir::kUp;
    p._wickBarToPatternDir = eWickBarToPatternDir::kEither;
    pattern._highBarIndex = 1;
    bar._index = 3;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(reason, "pattern._highBarIndex != bar._index -- bar=3,,,Unknown,InV,InV,InV,InV,7,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,pattern=0,,,Unknown,InV,InV,InV,InV,30,0,0,Up,0,00000000-00:00:00.000000,,Unknown,Unknown,1,2,1,0,0,");
    
    
    pattern._highBarIndex = 3;
    
    bar._open.set(101);
    bar._close.set(103);
    
    bar._high.set(105);
    bar._low.set(98);
    bar._dir = tw::common_trade::ePatternDir::kUp;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(reason, "wickToBodyRatio < _p._wickToBodyRatio -- wick=2,body=2,wickToBodyRatio=1,bar=3,,,Unknown,105,98,101,103,7,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    
    p._useBodyInWickToBodyRatio = false;
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(reason, "wickToBodyRatio < _p._wickToBodyRatio -- wick=2,body=5,wickToBodyRatio=0.4,bar=3,,,Unknown,105,98,101,103,7,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    
    
    p._useBodyInWickToBodyRatio = true;
    
    bar._open.set(103);
    bar._close.set(101);
    
    bar._high.set(105);
    bar._low.set(98);
    bar._dir = tw::common_trade::ePatternDir::kDown;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(reason, "wickToBodyRatio < _p._wickToBodyRatio -- wick=2,body=2,wickToBodyRatio=1,bar=3,,,Unknown,105,98,103,101,7,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    
    p._useBodyInWickToBodyRatio = false;    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(reason, "wickToBodyRatio < _p._wickToBodyRatio -- wick=2,body=5,wickToBodyRatio=0.4,bar=3,,,Unknown,105,98,103,101,7,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    
        
    p._useBodyInWickToBodyRatio = true;
    
    bar._open.set(99);
    bar._close.set(100);
    
    bar._high.set(105);
    bar._low.set(98);
    bar._dir = tw::common_trade::ePatternDir::kUp;
    
    ASSERT_TRUE(impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(reason, "WickSignalProcessor::isSignalTriggered()==true -- wick=5,body=1,wickBarToSwingRatio=0.23,wickToBodyRatio=5,trigger_bar=3,,,Unknown,105,98,99,100,7,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,,pattern=0,,,Unknown,InV,InV,InV,InV,30,0,0,Up,0,00000000-00:00:00.000000,,Unknown,Unknown,1,2,3,0,0,");
    
    p._useBodyInWickToBodyRatio = false;
    side = tw::channel_or::eOrderSide::kUnknown;
    ASSERT_TRUE(impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(reason, "WickSignalProcessor::isSignalTriggered()==true -- wick=5,body=2,wickBarToSwingRatio=0.23,wickToBodyRatio=2.5,trigger_bar=3,,,Unknown,105,98,99,100,7,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,,pattern=0,,,Unknown,InV,InV,InV,InV,30,0,0,Up,0,00000000-00:00:00.000000,,Unknown,Unknown,1,2,3,0,0,");
        
    bars[0]._close.set(101);
    bars[1]._close.set(99);
    p._wickOutsidePrevHighLow = true;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(reason, "wickLow < bars[i-1]._close -- wick=5,wickLow=100,bar=3,,,Unknown,105,98,99,100,7,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,,bars[1]=0,,,Unknown,InV,InV,InV,101,InV,0,0,Unknown,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    
    
    bars[0]._close.set(100);
    side = tw::channel_or::eOrderSide::kUnknown;
    ASSERT_TRUE(impl.isSignalTriggered(bars, pattern, side, reason));
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(reason, "WickSignalProcessor::isSignalTriggered()==true -- wick=5,body=2,wickBarToSwingRatio=0.23,wickToBodyRatio=2.5,trigger_bar=3,,,Unknown,105,98,99,100,7,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,,pattern=0,,,Unknown,InV,InV,InV,InV,30,0,0,Up,0,00000000-00:00:00.000000,,Unknown,Unknown,1,2,3,0,0,");

    // Test isSignalValid() logic
    //
    TPrice activePrice;
    bool placeOrder = false;
    TBars tempBars;
    
    ASSERT_TRUE(impl.isSignalValid(bars, activePrice, side, placeOrder, reason));
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kSell);
    ASSERT_TRUE(placeOrder);
    ASSERT_EQ(reason, "!_p._validateSignal");
    
    p._validateSignal = true;
    
    ASSERT_TRUE(impl.isSignalValid(bars, activePrice, side, placeOrder, reason));
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kSell);
    ASSERT_TRUE(!placeOrder);
    ASSERT_EQ(reason, "!activePrice.isValid()");
    
    activePrice.set(100);
            
    ASSERT_TRUE(!impl.isSignalValid(tempBars, activePrice, side, placeOrder, reason));
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kSell);
    ASSERT_TRUE(!placeOrder);
    ASSERT_EQ(reason, "bars.size() < 2 -- bars.size()=0");
    
    tempBars.resize(2);
    
    ASSERT_TRUE(impl.isSignalValid(tempBars, activePrice, side, placeOrder, reason));
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kSell);
    ASSERT_TRUE(!placeOrder);
    ASSERT_EQ(reason, "!currBar._close.isValid()");
    
    tempBars[1]._close.set(104);
    
    ASSERT_TRUE(!impl.isSignalValid(tempBars, activePrice, side, placeOrder, reason));
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kSell);
    ASSERT_TRUE(!placeOrder);
    ASSERT_EQ(reason, "!signalBar._close.isValid()");
    
    tempBars[0]._close.set(100);
    tempBars[1]._high.set(104);
    
    p._maxAwayFromCloseTicks.set(3);
    ASSERT_TRUE(!impl.isSignalValid(tempBars, activePrice, side, placeOrder, reason));
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kSell);
    ASSERT_TRUE(!placeOrder);
    ASSERT_EQ(reason, "currBar._high > (signalBar._close+_p._maxAwayFromCloseTicks) -- currBar=0,,,Unknown,104,InV,InV,104,InV,0,0,Unknown,0,00000000-00:00:00.000000,,0,0,0,0,0,,,signalBar=0,,,Unknown,InV,InV,InV,100,InV,0,0,Unknown,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    
    p._maxAwayFromCloseTicks.set(4);
    ASSERT_TRUE(impl.isSignalValid(tempBars, activePrice, side, placeOrder, reason));
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kSell);
    ASSERT_TRUE(!placeOrder);
    ASSERT_EQ(reason, "");
    
    activePrice.set(99);
    ASSERT_TRUE(impl.isSignalValid(tempBars, activePrice, side, placeOrder, reason));
    ASSERT_EQ(side, tw::channel_or::eOrderSide::kSell);
    ASSERT_TRUE(placeOrder);
    ASSERT_EQ(reason, "activePrice < signalBar._close -- activePrice=99,signalBar=0,,,Unknown,InV,InV,InV,100,InV,0,0,Unknown,0,00000000-00:00:00.000000,,0,0,0,0,0,,");

    
}

} // common_trade
} // tw

