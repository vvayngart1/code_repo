#include <vector>

#include <tw/common_strat/defs.h>
#include <tw/generated/common_trade_blocks.h>
#include <tw/common_trade/continuationTradeSignalProcessor.h>
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

typedef ContinuationTradeSignalProcessor TImpl;
typedef ContinuationTradeSignalParamsWire TParams;
typedef ContinuationTradeSignalParamsRuntime TParamsRuntime;

static TParams getParams(TPrice minDispTicks,
                         float bodyToBarRatio,
                         bool checkSwingHighLow,
                         bool reqBarInSwingDir,
                         bool reqBarInTrendDir,
                         float swingBreakTrendRatio,
                         TPrice maxPayupTicks,
                         TPrice maxFlowbackTicks,
                         TPrice maxFlowbackStopTicks,
                         TSize minContinuationQtyFor,
                         float ratioFor) {
    TParams p;
    
    p._ctMinDispTicks = minDispTicks;
    p._ctBodyToBarRatio = bodyToBarRatio;
    p._ctCheckSwingHighLow = checkSwingHighLow;
    p._ctReqBarInSwingDir = reqBarInSwingDir;
    p._ctReqBarInTrendDir = reqBarInTrendDir;
    p._ctSwingBreakTrendRatio = swingBreakTrendRatio;
    p._ctMaxPayupTicks = maxPayupTicks;
    p._ctMaxFlowbackTicks = maxFlowbackTicks;
    p._ctMaxFlowbackStopTicks = maxFlowbackStopTicks;
    p._ctMinContinuationQtyFor = minContinuationQtyFor;
    p._ctRatioFor = ratioFor;
    
    p._ctReqTrendExtreme = true;
    
    return p;
}


TEST(CommonTradeLibTestSuit, continuationTradeSignalProcessor_test_buy)
{
    TParams params;
    
    TBars bars;
    TBarPattern pattern;
    TBarPattern trend;
    eTradeBias tradeBias = eTradeBias::kNeutral;
    
    TImpl impl(params);
    TParams& p = impl.getParams();
    TParamsRuntime& p_runtime = impl.getParamsRuntime();
    p = getParams(TPrice(6), 0.75, false, false, false, 0.56, TPrice(3), TPrice(2), TPrice(1), TSize(20), 1.5);
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "bars.size() < 3");
    
    bars.resize(4);    
    bars[2]._range.set(5);
    bars[2]._dir = tw::common_trade::ePatternDir::kUp;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "!bar._range.isValid() || bar._range < _p._ctMinDispTicks -- _p._ctMinDispTicks=6,bar=0,,,Unknown,InV,InV,InV,InV,5,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 0);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());

    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 0);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());    
    
    bars[2]._range.set(10);
    bars[2]._open.set(100);
    bars[2]._close.set(100);
    bars[2]._low.set(100);
    p_runtime._ctLastProcessedBarIndex = 2;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "body=0 -- bar=0,,,Unknown,InV,100,100,100,10,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 0);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());    
    
    bars[2]._high.set(110);
    bars[2]._low.set(100);
    bars[2]._open.set(104);
    bars[2]._close.set(106);
    p_runtime._ctLastProcessedBarIndex = 2;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "bodyToBarRatio < _p._ctBodyToBarRatio -- bodyToBarRatio=0.6,_p._ctBodyToBarRatio=0.75,bar=0,,,Unknown,110,100,104,106,10,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 0);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());

    bars[2]._open.set(101);
    bars[2]._close.set(109);
    bars[2]._atr = 1.5;
    p_runtime._ctLastProcessedBarIndex = 2;
    p._ctAtrRequired = 2.5;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "_p._ctAtrRequired > bar._atr -- _p._ctAtrRequired=2.5,bar=0,,,Unknown,110,100,101,109,10,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,1.5,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 0);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());
    
    
    bars.back()._open.set(bars[2]._close-TPrice(1));
    p_runtime._ctLastProcessedBarIndex = 2;
    bars[2]._atr = 0;
    p._ctAtrRequired = 0;
    
    tradeBias = eTradeBias::kShort;
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "signal NOT triggered because of tradeBias -- tradeBias=Short,_p_runtime._ctSide=Buy");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 0);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());
    
    tradeBias = eTradeBias::kNeutral;
    p_runtime._ctLastProcessedBarIndex = 2;
    
    ASSERT_TRUE(impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "ContinuationTradeSignalProcessor::isSignalTriggered()==true -- _p_runtime._ctSide=Buy,_p_runtime._ctMaxPayupPrice=112,_p_runtime._ctMaxFlowbackPrice=107,_p_runtime._ctStopPrice=106,_p._ctMinDispTicks=6,_p._ctBodyToBarRatio=0.75,_p._ctAtrRequired=0,trigger_bar=0,,,Unknown,110,100,101,109,10,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,,pattern=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Unknown,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,,trend=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Unknown,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(112)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(107));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(106));
    
    impl.clear();
    p_runtime._ctLastSignalBarIndex = 0;
    p_runtime._ctLastProcessedBarIndex = 2;
    
    p._ctCheckSwingHighLow = true;
    p._ctReqBarInSwingDir = true;
    p._ctReqBarInTrendDir = true;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "tw::common_trade::ePatternDir::kUnknown == pattern._dir -- pattern=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Unknown,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 0);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());
    
    pattern._dir = tw::common_trade::ePatternDir::kDown;
    p_runtime._ctLastProcessedBarIndex = 2;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "_p._ctReqBarInSwingDir && tw::common_trade::ePatternDir::kDown != bar._dir -- pattern=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Down,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,,bar=0,,,Unknown,110,100,101,109,10,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 0);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());
    
    pattern._dir = tw::common_trade::ePatternDir::kUp;
    pattern._highBarIndex = 2;
    bars[2]._index = 3;
    p_runtime._ctLastProcessedBarIndex = 2;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "_p._ctCheckSwingHighLow && bar._index != pattern._highBarIndex -- pattern=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Up,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,2,0,0,,bar=3,,,Unknown,110,100,101,109,10,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 0);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());
    
    pattern._highBarIndex = 3;
    p_runtime._ctLastProcessedBarIndex = 2;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "tw::common_trade::ePatternDir::kUnknown == trend._dir -- trend=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Unknown,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 0);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());
    
    // Test ctReqTrendExtreme
    //
    trend._dir = tw::common_trade::ePatternDir::kUp;
    trend._highBarIndex = 2;
    p_runtime._ctLastProcessedBarIndex = 2;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "trend._highBarIndex != bar._index -- trend=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Up,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,2,0,0,,bar=3,,,Unknown,110,100,101,109,10,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 0);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());
    
    impl.clear();
    p_runtime._ctLastSignalBarIndex = 0;
    p_runtime._ctLastProcessedBarIndex = 2;
    trend._highBarIndex = 3;
    
    ASSERT_TRUE(impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "ContinuationTradeSignalProcessor::isSignalTriggered()==true -- _p_runtime._ctSide=Buy,_p_runtime._ctMaxPayupPrice=112,_p_runtime._ctMaxFlowbackPrice=107,_p_runtime._ctStopPrice=106,_p._ctMinDispTicks=6,_p._ctBodyToBarRatio=0.75,_p._ctAtrRequired=0,trigger_bar=3,,,Unknown,110,100,101,109,10,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,,pattern=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Up,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,3,0,0,,trend=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Up,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,3,0,0,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(112)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(107));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(106));
    
    impl.clear();
    p_runtime._ctLastSignalBarIndex = 0;
    p_runtime._ctLastProcessedBarIndex = 2;
    trend._dir = tw::common_trade::ePatternDir::kDown;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "tw::common_trade::ePatternDir::kDown != bar._dir -- trend=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Down,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,3,0,0,,bar=3,,,Unknown,110,100,101,109,10,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 0);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());
    
    trend._dir = tw::common_trade::ePatternDir::kUp;
    
    // Test counter trend
    //
    pattern._high = bars[2]._high;
    pattern._low = bars[2]._low;
    pattern._open = bars[2]._open;
    pattern._close = bars[2]._close;
    
    trend._dir = tw::common_trade::ePatternDir::kDown;
    trend._high.set(200);
    trend._low.set(100);
    trend._range = trend._high - trend._low;
    trend._highBarIndex = 0;
    
    p._ctReqTrendExtreme = false;
    p._ctReqBarInTrendDir = false;
    impl.clear();
    p_runtime._ctLastSignalBarIndex = 0;
    p_runtime._ctLastProcessedBarIndex = 2;
    
    ASSERT_TRUE(impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "ContinuationTradeSignalProcessor::isSignalTriggered()==true -- _p_runtime._ctSide=Buy,_p_runtime._ctMaxPayupPrice=112,_p_runtime._ctMaxFlowbackPrice=107,_p_runtime._ctStopPrice=106,_p._ctMinDispTicks=6,_p._ctBodyToBarRatio=0.75,_p._ctAtrRequired=0,trigger_bar=3,,,Unknown,110,100,101,109,10,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,,pattern=0,,,Unknown,110,100,101,109,InV,0,0,Up,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,3,0,0,,trend=0,,,Unknown,200,100,InV,InV,100,0,0,Down,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(112)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(107));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(106));
    
    p._ctReqBarInTrendDir = true;
    impl.clear();
    p_runtime._ctLastSignalBarIndex = 0;
    p_runtime._ctLastProcessedBarIndex = 2;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "_p._ctReqBarInTrendDir && tw::common_trade::ePatternDir::kDown != bar._dir) && (breakTrendWatermark > pattern._high.toDouble()) -- breakTrendWatermark=156,_p._ctSwingBreakTrendRatio=0.56,pattern=0,,,Unknown,110,100,101,109,InV,0,0,Up,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,3,0,0,,trend=0,,,Unknown,200,100,InV,InV,100,0,0,Down,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,,bar=3,,,Unknown,110,100,101,109,10,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 0);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());
    
    trend._high.set(150);
    trend._low.set(50);
    trend._range = trend._high - trend._low;
    p_runtime._ctLastProcessedBarIndex = 2;
    
    ASSERT_TRUE(impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "ContinuationTradeSignalProcessor::isSignalTriggered()==true -- _p_runtime._ctSide=Buy,_p_runtime._ctMaxPayupPrice=112,_p_runtime._ctMaxFlowbackPrice=107,_p_runtime._ctStopPrice=106,_p._ctMinDispTicks=6,_p._ctBodyToBarRatio=0.75,_p._ctAtrRequired=0,trigger_bar=3,,,Unknown,110,100,101,109,10,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,,pattern=0,,,Unknown,110,100,101,109,InV,0,0,Up,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,3,0,0,,trend=0,,,Unknown,150,50,InV,InV,100,0,0,Down,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(112)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(107));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(106));
    
    impl.clear();
    p_runtime._ctLastSignalBarIndex = 0;
    p_runtime._ctLastProcessedBarIndex = 2;

    trend._dir = tw::common_trade::ePatternDir::kUp;
    trend._high.set(200);
    trend._low.set(100);
    trend._range = trend._high - trend._low;
    
    ASSERT_TRUE(impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "ContinuationTradeSignalProcessor::isSignalTriggered()==true -- _p_runtime._ctSide=Buy,_p_runtime._ctMaxPayupPrice=112,_p_runtime._ctMaxFlowbackPrice=107,_p_runtime._ctStopPrice=106,_p._ctMinDispTicks=6,_p._ctBodyToBarRatio=0.75,_p._ctAtrRequired=0,trigger_bar=3,,,Unknown,110,100,101,109,10,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,,pattern=0,,,Unknown,110,100,101,109,InV,0,0,Up,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,3,0,0,,trend=0,,,Unknown,200,100,InV,InV,100,0,0,Up,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(112)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(107));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(106));
    
    // Test ctStopTicks functionality
    //
    impl.clear();
    p_runtime._ctLastSignalBarIndex = 0;
    p_runtime._ctLastProcessedBarIndex = 2;
    p._ctStopTicks.set(2);
    
    ASSERT_TRUE(impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "ContinuationTradeSignalProcessor::isSignalTriggered()==true -- _p_runtime._ctSide=Buy,_p_runtime._ctMaxPayupPrice=112,_p_runtime._ctMaxFlowbackPrice=107,_p_runtime._ctStopPrice=98,_p._ctMinDispTicks=6,_p._ctBodyToBarRatio=0.75,_p._ctAtrRequired=0,trigger_bar=3,,,Unknown,110,100,101,109,10,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,,pattern=0,,,Unknown,110,100,101,109,InV,0,0,Up,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,3,0,0,,trend=0,,,Unknown,200,100,InV,InV,100,0,0,Up,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(112)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(107));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(98));
    p_runtime._ctStopPrice.set(106);
    
    // Test monitorSignal()
    //
    
    // Set quote
    //
    tw::price::QuoteStore::TQuote quote;
    quote.setInstrument(InstrHelper::get6CU2());
    
    tw::price::Ticks activePrice;
    
    p_runtime._ctLastSignalBarIndex = 5;
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, "Severe error: !checkRange(bars, _p_runtime._ctLastSignalBarIndex): _p_runtime._ctLastSignalBarIndex=5");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 5);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kCancelled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(112)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(107));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(106));
    
    p_runtime._ctLastSignalBarIndex = 2;
    p_runtime._ctSignalState = tw::common_trade::eContinuationTradeSignalState::kSignaled;
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, "_p_runtime._ctLastSignalBarIndex < bars.size()-1: _p_runtime._ctLastSignalBarIndex=2,bars.size()=4");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 2);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kCancelled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(112)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(107));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(106));
    
    p_runtime._ctLastSignalBarIndex = 3;
    p_runtime._ctSignalState = tw::common_trade::eContinuationTradeSignalState::kSignaled;
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, "!activePrice.isValid()");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(112)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(107));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(106));
    
    // Test lower limit signal cancellation
    //
    activePrice.set(110);
    
    quote.setTrade(TPrice(106), TSize(1));
    quote.setBid(TPrice(107), TSize(20), 0);
    quote.setAsk(TPrice(108), TSize(40), 0);
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, " ==> price < _p_runtime._ctMaxFlowbackPrice -- _p_runtime._ctMaxFlowbackPrice=107,side=Buy,quote=book[0]=0,20,107|108,40,0,trade=1,106,u,u,u,u,bar=3,,,Unknown,110,100,101,109,10,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kCancelled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(112)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(107));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(106));
    
    p_runtime._ctSignalState = tw::common_trade::eContinuationTradeSignalState::kSignaled;
    quote.setTrade(TPrice(107), TSize(1));
    p_runtime._ctReason.clear();
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, "");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(112)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(107));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(106));
    
    quote.setBid(TPrice(105), TSize(20), 0);
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, "");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(112)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(107));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(106));
    
    quote.setAsk(TPrice(106), TSize(40), 0);
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, " ==> price < _p_runtime._ctMaxFlowbackPrice -- _p_runtime._ctMaxFlowbackPrice=107,side=Buy,quote=book[0]=0,20,105|106,40,0,trade=1,107,u,u,u,u,bar=3,,,Unknown,110,100,101,109,10,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kCancelled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(112)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(107));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(106));
    
    p_runtime._ctSignalState = tw::common_trade::eContinuationTradeSignalState::kSignaled;
    p_runtime._ctReason.clear();
    
    quote.setTrade(TPrice(113), TSize(1));
    quote.setBid(TPrice(111), TSize(40), 0);
    quote.setAsk(TPrice(113), TSize(20), 0);
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, "");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(112)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(107));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(106));
    
    activePrice.set(113);
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, " ==> price > _p_runtime._ctMaxPayupPrice -- _p_runtime._ctMaxPayupPrice=112,side=Buy,quote=book[0]=0,40,111|113,20,0,trade=1,113,u,u,u,u,bar=3,,,Unknown,110,100,101,109,10,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kCancelled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(112)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(107));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(106));
    
    // Test volumeFor/volumeAgainst
    //
    p_runtime._ctSignalState = tw::common_trade::eContinuationTradeSignalState::kSignaled;
    p_runtime._ctReason.clear();
    
    activePrice.set(110);
    
    // VolumeFor(109 askVol + totalVol 110-112)=6+2+2+2=12
    // VolumeAGainst(109 bidVol + totalVol 108-107)=2+2+2=6
    // Ratio is satisfied: 12/6=2 > 1.5, but minContinuationQtyFor is not:  12 < 20
    //
    bars[3]._trades[TPrice(113)]._volAsk.set(10);
    bars[3]._trades[TPrice(113)]._volBid.set(10);
    
    bars[3]._trades[TPrice(112)]._volAsk.set(1);
    bars[3]._trades[TPrice(112)]._volBid.set(1);
    
    bars[3]._trades[TPrice(111)]._volAsk.set(1);
    bars[3]._trades[TPrice(111)]._volBid.set(1);
    
    bars[3]._trades[TPrice(110)]._volAsk.set(1);
    bars[3]._trades[TPrice(110)]._volBid.set(1);
    
    bars[3]._trades[TPrice(109)]._volAsk.set(6);
    bars[3]._trades[TPrice(109)]._volBid.set(2);
    
    bars[3]._trades[TPrice(108)]._volAsk.set(1);
    bars[3]._trades[TPrice(108)]._volBid.set(1);
    
    bars[3]._trades[TPrice(107)]._volAsk.set(1);
    bars[3]._trades[TPrice(107)]._volBid.set(1);
    
    // Test gapOnOpen as well as volumeFor/volumeAgainst
    //
    p._ctInitiateOnGap = true;
    bars.back()._index = bars.size();
    bars[2]._index = bars.size()-1;
    
    bars.back()._open = bars[2]._close - TPrice(1);
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, "");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(112)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(107));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(106));
    
    bars.back()._open = bars[2]._close + TPrice(1);
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, " ==> gapOnOpen > 0: gapOnOpen=1,bar=3,,,Unknown,110,100,101,109,10,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,,currBar=4,,,Unknown,InV,InV,110,InV,InV,0,0,Unknown,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kValidated);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(112)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(107));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(106));
    
    p._ctInitiateOnGap = false;
    p_runtime._ctSignalState = tw::common_trade::eContinuationTradeSignalState::kSignaled;
    p_runtime._ctReason.clear();
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, "");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(112)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(107));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(106));
    
    // Change volumeFor so that minContinuationQtyFor is satisfied
    //
    bars[3]._trades[TPrice(111)]._volAsk.set(6);
    bars[3]._trades[TPrice(111)]._volBid.set(4);
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, " ==> ratio >= _p._ctRatioFor -- volumeFor=20,volumeAgainst=6,ratio=3.33,_p._ctMinContinuationQtyFor=20,_p._ctRatioFor=1.5");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kValidated);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(112)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(107));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(106));
    
    // Test _p._ctIgnoreCloseVol functionality
    //
    p._ctIgnoreCloseVol = true;
    p_runtime._ctSignalState = tw::common_trade::eContinuationTradeSignalState::kSignaled;
    p_runtime._ctReason.clear();
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, "");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(112)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(107));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(106));
    
    p._ctMinContinuationQtyFor.set(14);
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, " ==> ratio >= _p._ctRatioFor -- volumeFor=14,volumeAgainst=4,ratio=3.5,_p._ctMinContinuationQtyFor=14,_p._ctRatioFor=1.5");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kValidated);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(112)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(107));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(106));
    
    
    
    // Check that signal can be invalidated
    //
    p_runtime._ctReason.clear();    
    activePrice.set(113);
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, " ==> price > _p_runtime._ctMaxPayupPrice -- _p_runtime._ctMaxPayupPrice=112,side=Buy,quote=book[0]=0,40,111|113,20,0,trade=1,113,u,u,u,u,bar=3,,,Unknown,110,100,101,109,10,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kCancelled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(112)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(107));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(106));
    
    // Test resetting state on new bar
    //
    p_runtime._ctReason.clear();
    p_runtime._ctSignalState = tw::common_trade::eContinuationTradeSignalState::kValidated;
    
    bars.resize(5);
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "tw::common_trade::ePatternDir::kUnknown == bar._dir bar=4,,,Unknown,InV,InV,110,InV,InV,0,0,Unknown,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 4);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());
}




TEST(CommonTradeLibTestSuit, continuationTradeSignalProcessor_test_sell)
{
    TParams params;
    
    TBars bars;
    TBarPattern pattern;
    TBarPattern trend;
    eTradeBias tradeBias = eTradeBias::kNeutral;
    
    TImpl impl(params);
    TParams& p = impl.getParams();
    TParamsRuntime& p_runtime = impl.getParamsRuntime();
    p = getParams(TPrice(6), 0.75, false, false, false, 0.56, TPrice(3), TPrice(2), TPrice(1), TSize(20), 1.5);
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "bars.size() < 3");
    
    bars.resize(4);    
    bars[2]._range.set(5);
    bars[2]._dir = tw::common_trade::ePatternDir::kDown;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "!bar._range.isValid() || bar._range < _p._ctMinDispTicks -- _p._ctMinDispTicks=6,bar=0,,,Unknown,InV,InV,InV,InV,5,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 0);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());    
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 0);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());    
    
    bars[2]._range.set(10);
    bars[2]._open.set(100);
    bars[2]._close.set(100);
    bars[2]._high.set(100);
    p_runtime._ctLastProcessedBarIndex = 2;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "body=0 -- bar=0,,,Unknown,100,InV,100,100,10,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 0);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());    
    
    bars[2]._high.set(110);
    bars[2]._low.set(100);
    bars[2]._open.set(108);
    bars[2]._close.set(106);
    p_runtime._ctLastProcessedBarIndex = 2;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "bodyToBarRatio < _p._ctBodyToBarRatio -- bodyToBarRatio=0.4,_p._ctBodyToBarRatio=0.75,bar=0,,,Unknown,110,100,108,106,10,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 0);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());

    bars[2]._open.set(109);
    bars[2]._close.set(101);
    bars[2]._atr = 1.5;
    p_runtime._ctLastProcessedBarIndex = 2;
    p._ctAtrRequired = 2.5;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "_p._ctAtrRequired > bar._atr -- _p._ctAtrRequired=2.5,bar=0,,,Unknown,110,100,109,101,10,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,1.5,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 0);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());
    
    bars.back()._open.set(bars[2]._close+TPrice(1));
    p_runtime._ctLastProcessedBarIndex = 2;
    bars[2]._atr = 0;
    p._ctAtrRequired = 0;
    
    tradeBias = eTradeBias::kLong;
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "signal NOT triggered because of tradeBias -- tradeBias=Long,_p_runtime._ctSide=Sell");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 0);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());
    
    tradeBias = eTradeBias::kNeutral;
    p_runtime._ctLastProcessedBarIndex = 2;
    
    ASSERT_TRUE(impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "ContinuationTradeSignalProcessor::isSignalTriggered()==true -- _p_runtime._ctSide=Sell,_p_runtime._ctMaxPayupPrice=98,_p_runtime._ctMaxFlowbackPrice=103,_p_runtime._ctStopPrice=104,_p._ctMinDispTicks=6,_p._ctBodyToBarRatio=0.75,_p._ctAtrRequired=0,trigger_bar=0,,,Unknown,110,100,109,101,10,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,,pattern=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Unknown,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,,trend=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Unknown,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(98)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(103));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(104));
    
    impl.clear();
    p_runtime._ctLastSignalBarIndex = 0;
    
    p._ctCheckSwingHighLow = true;
    p._ctReqBarInSwingDir = true;
    p._ctReqBarInTrendDir = true;
    p_runtime._ctLastProcessedBarIndex = 2;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "tw::common_trade::ePatternDir::kUnknown == pattern._dir -- pattern=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Unknown,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 0);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());
    
    pattern._dir = tw::common_trade::ePatternDir::kUp;
    p_runtime._ctLastProcessedBarIndex = 2;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "_p._ctReqBarInSwingDir && tw::common_trade::ePatternDir::kUp != bar._dir -- pattern=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Up,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,,bar=0,,,Unknown,110,100,109,101,10,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 0);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());
    
    pattern._dir = tw::common_trade::ePatternDir::kDown;
    pattern._lowBarIndex = 2;
    bars[2]._index = 3;
    p_runtime._ctLastProcessedBarIndex = 2;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "_p._ctCheckSwingHighLow && bar._index != pattern._lowBarIndex -- pattern=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Down,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,2,0,,bar=3,,,Unknown,110,100,109,101,10,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 0);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());
    
    pattern._lowBarIndex = 3;
    p_runtime._ctLastProcessedBarIndex = 2;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "tw::common_trade::ePatternDir::kUnknown == trend._dir -- trend=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Unknown,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 0);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());
    
    // Test ctReqTrendExtreme
    //
    trend._dir = tw::common_trade::ePatternDir::kDown;
    trend._lowBarIndex = 2;
    p_runtime._ctLastProcessedBarIndex = 2;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "trend._lowBarIndex != bar._index -- trend=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Down,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,2,0,,bar=3,,,Unknown,110,100,109,101,10,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 0);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());
    
    impl.clear();
    p_runtime._ctLastSignalBarIndex = 0;
    p_runtime._ctLastProcessedBarIndex = 2;
    trend._lowBarIndex = 3;
    
    ASSERT_TRUE(impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "ContinuationTradeSignalProcessor::isSignalTriggered()==true -- _p_runtime._ctSide=Sell,_p_runtime._ctMaxPayupPrice=98,_p_runtime._ctMaxFlowbackPrice=103,_p_runtime._ctStopPrice=104,_p._ctMinDispTicks=6,_p._ctBodyToBarRatio=0.75,_p._ctAtrRequired=0,trigger_bar=3,,,Unknown,110,100,109,101,10,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,,pattern=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Down,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,3,0,,trend=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Down,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,3,0,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(98)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(103));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(104));
    
    impl.clear();
    p_runtime._ctLastSignalBarIndex = 0;
    p_runtime._ctLastProcessedBarIndex = 2;
    trend._dir = tw::common_trade::ePatternDir::kUp;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "tw::common_trade::ePatternDir::kUp != bar._dir -- trend=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Up,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,3,0,,bar=3,,,Unknown,110,100,109,101,10,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 0);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());
    
    trend._dir = tw::common_trade::ePatternDir::kDown;
    
    // Test counter trend
    //    
    pattern._high = bars[2]._high;
    pattern._low = bars[2]._low;
    pattern._open = bars[2]._open;
    pattern._close = bars[2]._close;
    
    trend._dir = tw::common_trade::ePatternDir::kUp;
    trend._high.set(101);
    trend._low.set(1);
    trend._range = trend._high - trend._low;
    trend._lowBarIndex = 0;
    
    p._ctReqTrendExtreme = false;
    p._ctReqBarInTrendDir = false;
    impl.clear();
    p_runtime._ctLastSignalBarIndex = 0;
    p_runtime._ctLastProcessedBarIndex = 2;
    
    ASSERT_TRUE(impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "ContinuationTradeSignalProcessor::isSignalTriggered()==true -- _p_runtime._ctSide=Sell,_p_runtime._ctMaxPayupPrice=98,_p_runtime._ctMaxFlowbackPrice=103,_p_runtime._ctStopPrice=104,_p._ctMinDispTicks=6,_p._ctBodyToBarRatio=0.75,_p._ctAtrRequired=0,trigger_bar=3,,,Unknown,110,100,109,101,10,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,,pattern=0,,,Unknown,110,100,109,101,InV,0,0,Down,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,3,0,,trend=0,,,Unknown,101,1,InV,InV,100,0,0,Up,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(98)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(103));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(104));
    
    p._ctReqBarInTrendDir = true;
    impl.clear();
    p_runtime._ctLastSignalBarIndex = 0;
    p_runtime._ctLastProcessedBarIndex = 2;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "_p._ctReqBarInTrendDir && tw::common_trade::ePatternDir::kUp != bar._dir) && (breakTrendWatermark < pattern._low.toDouble()) -- breakTrendWatermark=45,_p._ctSwingBreakTrendRatio=0.56,pattern=0,,,Unknown,110,100,109,101,InV,0,0,Down,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,3,0,,trend=0,,,Unknown,101,1,InV,InV,100,0,0,Up,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,,bar=3,,,Unknown,110,100,109,101,10,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 0);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());
    
    trend._high.set(157);
    trend._low.set(57);
    trend._range = trend._high - trend._low;
    p_runtime._ctLastProcessedBarIndex = 2;
    
    ASSERT_TRUE(impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "ContinuationTradeSignalProcessor::isSignalTriggered()==true -- _p_runtime._ctSide=Sell,_p_runtime._ctMaxPayupPrice=98,_p_runtime._ctMaxFlowbackPrice=103,_p_runtime._ctStopPrice=104,_p._ctMinDispTicks=6,_p._ctBodyToBarRatio=0.75,_p._ctAtrRequired=0,trigger_bar=3,,,Unknown,110,100,109,101,10,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,,pattern=0,,,Unknown,110,100,109,101,InV,0,0,Down,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,3,0,,trend=0,,,Unknown,157,57,InV,InV,100,0,0,Up,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(98)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(103));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(104));
    
    impl.clear();
    p_runtime._ctLastSignalBarIndex = 0;
    p_runtime._ctLastProcessedBarIndex = 2;

    trend._dir = tw::common_trade::ePatternDir::kDown;
    trend._high.set(101);
    trend._low.set(1);
    trend._range = trend._high - trend._low;
    
    ASSERT_TRUE(impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "ContinuationTradeSignalProcessor::isSignalTriggered()==true -- _p_runtime._ctSide=Sell,_p_runtime._ctMaxPayupPrice=98,_p_runtime._ctMaxFlowbackPrice=103,_p_runtime._ctStopPrice=104,_p._ctMinDispTicks=6,_p._ctBodyToBarRatio=0.75,_p._ctAtrRequired=0,trigger_bar=3,,,Unknown,110,100,109,101,10,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,,pattern=0,,,Unknown,110,100,109,101,InV,0,0,Down,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,3,0,,trend=0,,,Unknown,101,1,InV,InV,100,0,0,Down,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(98)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(103));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(104));
    
    // Test ctStopTicks functionality
    //
    impl.clear();
    p_runtime._ctLastSignalBarIndex = 0;
    p_runtime._ctLastProcessedBarIndex = 2;
    p._ctStopTicks.set(2);
    
    ASSERT_TRUE(impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "ContinuationTradeSignalProcessor::isSignalTriggered()==true -- _p_runtime._ctSide=Sell,_p_runtime._ctMaxPayupPrice=98,_p_runtime._ctMaxFlowbackPrice=103,_p_runtime._ctStopPrice=112,_p._ctMinDispTicks=6,_p._ctBodyToBarRatio=0.75,_p._ctAtrRequired=0,trigger_bar=3,,,Unknown,110,100,109,101,10,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,,pattern=0,,,Unknown,110,100,109,101,InV,0,0,Down,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,3,0,,trend=0,,,Unknown,101,1,InV,InV,100,0,0,Down,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(98)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(103));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(112));
    p_runtime._ctStopPrice.set(104);
    
    // Test monitorSignal()
    //
    
    // Set quote
    //
    tw::price::QuoteStore::TQuote quote;
    quote.setInstrument(InstrHelper::get6CU2());
    
    tw::price::Ticks activePrice;
    
    p_runtime._ctLastSignalBarIndex = 5;
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, "Severe error: !checkRange(bars, _p_runtime._ctLastSignalBarIndex): _p_runtime._ctLastSignalBarIndex=5");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 5);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kCancelled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(98)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(103));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(104));
    
    p_runtime._ctLastSignalBarIndex = 2;
    p_runtime._ctSignalState = tw::common_trade::eContinuationTradeSignalState::kSignaled;
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, "_p_runtime._ctLastSignalBarIndex < bars.size()-1: _p_runtime._ctLastSignalBarIndex=2,bars.size()=4");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 2);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kCancelled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(98)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(103));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(104));
    
    p_runtime._ctLastSignalBarIndex = 3;
    p_runtime._ctSignalState = tw::common_trade::eContinuationTradeSignalState::kSignaled;
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, "!activePrice.isValid()");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(98)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(103));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(104));
    
    // Test lower limit signal cancellation
    //
    activePrice.set(100);
    
    quote.setTrade(TPrice(104), TSize(1));
    quote.setBid(TPrice(102), TSize(40), 0);
    quote.setAsk(TPrice(103), TSize(20), 0);
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, " ==> price > _p_runtime._ctMaxFlowbackPrice -- _p_runtime._ctMaxFlowbackPrice=103,side=Sell,quote=book[0]=0,40,102|103,20,0,trade=1,104,u,u,u,u,bar=3,,,Unknown,110,100,109,101,10,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kCancelled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(98)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(103));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(104));
    
    p_runtime._ctSignalState = tw::common_trade::eContinuationTradeSignalState::kSignaled;
    quote.setTrade(TPrice(103), TSize(1));
    p_runtime._ctReason.clear();
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, "");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(98)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(103));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(104));
    
    quote.setAsk(TPrice(105), TSize(20), 0);
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, "");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(98)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(103));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(104));
    
    quote.setBid(TPrice(104), TSize(40), 0);
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, " ==> price > _p_runtime._ctMaxFlowbackPrice -- _p_runtime._ctMaxFlowbackPrice=103,side=Sell,quote=book[0]=0,40,104|105,20,0,trade=1,103,u,u,u,u,bar=3,,,Unknown,110,100,109,101,10,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kCancelled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(98)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(103));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(104));
    
    p_runtime._ctSignalState = tw::common_trade::eContinuationTradeSignalState::kSignaled;
    p_runtime._ctReason.clear();
        
    quote.setTrade(TPrice(97), TSize(1));
    quote.setBid(TPrice(97), TSize(40), 0);
    quote.setAsk(TPrice(99), TSize(20), 0);
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, "");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(98)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(103));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(104));
    
    activePrice.set(97);
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, " ==> price < _p_runtime._ctMaxPayupPrice -- _p_runtime._ctMaxPayupPrice=98,side=Sell,quote=book[0]=0,40,97|99,20,0,trade=1,97,u,u,u,u,bar=3,,,Unknown,110,100,109,101,10,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kCancelled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(98)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(103));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(104));
    
    // Test volumeFor/volumeAgainst
    //
    p_runtime._ctSignalState = tw::common_trade::eContinuationTradeSignalState::kSignaled;
    p_runtime._ctReason.clear();
    
    activePrice.set(110);
    
    // VolumeFor(101 bidVol + totalVol 100-98)=6+2+2+2=12
    // VolumeAGainst(101 askVol + totalVol 102-103)=2+2+2=6
    // Ratio is satisfied: 12/6=2 > 1.5, but minContinuationQtyFor is not:  12 < 20
    //
    bars[3]._trades[TPrice(103)]._volAsk.set(1);
    bars[3]._trades[TPrice(103)]._volBid.set(1);
    
    bars[3]._trades[TPrice(102)]._volAsk.set(1);
    bars[3]._trades[TPrice(102)]._volBid.set(1);
    
    bars[3]._trades[TPrice(101)]._volAsk.set(2);
    bars[3]._trades[TPrice(101)]._volBid.set(6);
    
    bars[3]._trades[TPrice(100)]._volAsk.set(1);
    bars[3]._trades[TPrice(100)]._volBid.set(1);
    
    bars[3]._trades[TPrice(99)]._volAsk.set(1);
    bars[3]._trades[TPrice(99)]._volBid.set(1);    
    
    bars[3]._trades[TPrice(98)]._volAsk.set(1);
    bars[3]._trades[TPrice(98)]._volBid.set(1);
    
    bars[3]._trades[TPrice(97)]._volAsk.set(10);
    bars[3]._trades[TPrice(97)]._volBid.set(10);
    
    // Test gapOnOpen as well as volumeFor/volumeAgainst
    //
    p._ctInitiateOnGap = true;
    bars.back()._index = bars.size();
    bars[2]._index = bars.size()-1;
    
    bars.back()._open = bars[2]._close + TPrice(1);
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, "");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(98)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(103));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(104));
    
    bars.back()._open = bars[2]._close - TPrice(1);
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, " ==> gapOnOpen > 0: gapOnOpen=1,bar=3,,,Unknown,110,100,109,101,10,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,,currBar=4,,,Unknown,InV,InV,100,InV,InV,0,0,Unknown,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kValidated);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(98)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(103));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(104));
    
    p._ctInitiateOnGap = false;
    p_runtime._ctSignalState = tw::common_trade::eContinuationTradeSignalState::kSignaled;
    p_runtime._ctReason.clear();
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, "");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(98)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(103));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(104));
    
    // Change volumeFor so that minContinuationQtyFor is satisfied
    //
    bars[3]._trades[TPrice(99)]._volAsk.set(4);
    bars[3]._trades[TPrice(99)]._volBid.set(6);
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, " ==> ratio >= _p._ctRatioFor -- volumeFor=20,volumeAgainst=6,ratio=3.33,_p._ctMinContinuationQtyFor=20,_p._ctRatioFor=1.5");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kValidated);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(98)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(103));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(104));
    
    // Test _p._ctIgnoreCloseVol functionality
    //
    p._ctIgnoreCloseVol = true;
    p_runtime._ctSignalState = tw::common_trade::eContinuationTradeSignalState::kSignaled;
    p_runtime._ctReason.clear();
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, "");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(98)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(103));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(104));
    
    p._ctMinContinuationQtyFor.set(14);
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, " ==> ratio >= _p._ctRatioFor -- volumeFor=14,volumeAgainst=4,ratio=3.5,_p._ctMinContinuationQtyFor=14,_p._ctRatioFor=1.5");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kValidated);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(98)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(103));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(104));
    
    // Check that signal can be invalidated
    //
    p_runtime._ctReason.clear();    
    activePrice.set(97);
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, " ==> price < _p_runtime._ctMaxPayupPrice -- _p_runtime._ctMaxPayupPrice=98,side=Sell,quote=book[0]=0,40,97|99,20,0,trade=1,97,u,u,u,u,bar=3,,,Unknown,110,100,109,101,10,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kCancelled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(98)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(103));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(104));
    
    // Test resetting state on new bar
    //
    p_runtime._ctReason.clear();
    p_runtime._ctSignalState = tw::common_trade::eContinuationTradeSignalState::kValidated;
    
    bars.resize(5);
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "tw::common_trade::ePatternDir::kUnknown == bar._dir bar=4,,,Unknown,InV,InV,100,InV,InV,0,0,Unknown,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 4);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());
}

TEST(CommonTradeLibTestSuit, continuationTradeSignalProcessor_test_flowbackEnabled_buy)
{
    TParams params;
    
    TBars bars;
    TBarPattern pattern;
    TBarPattern trend;
    tw::price::Ticks activePrice;
    eTradeBias tradeBias = eTradeBias::kNeutral;
    
    TImpl impl(params);
    TParams& p = impl.getParams();
    TParamsRuntime& p_runtime = impl.getParamsRuntime();
    p = getParams(TPrice(6), 0.75, false, false, false, 0.56, TPrice(3), TPrice(2), TPrice(1), TSize(10), 1.5);
    
    // Set ctFlowbackEnabled=true
    //
    p._ctFlowbackEnabled = true;
    
    bars.resize(4);    
    bars[2]._dir = tw::common_trade::ePatternDir::kUp;
    bars[2]._high.set(110);
    bars[2]._low.set(100);
    bars[2]._open.set(101);
    bars[2]._close.set(109);
    bars[2]._range.set(bars[2]._high-bars[2]._low);
    bars[2]._atr = 0;
    
    ASSERT_TRUE(impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "ContinuationTradeSignalProcessor::isSignalTriggered()==true -- _p_runtime._ctSide=Buy,_p_runtime._ctMaxPayupPrice=112,_p_runtime._ctMaxFlowbackPrice=107,_p_runtime._ctStopPrice=106,_p._ctMinDispTicks=6,_p._ctBodyToBarRatio=0.75,_p._ctAtrRequired=0,trigger_bar=0,,,Unknown,110,100,101,109,10,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,,pattern=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Unknown,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,,trend=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Unknown,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(112)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(107));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(106));
    ASSERT_TRUE(!p_runtime._ctFlowbackOccurred);
    
    // Test monitorSignal()
    //
    
    // Set quote
    //
    tw::price::QuoteStore::TQuote quote;
    quote.setInstrument(InstrHelper::get6CU2());
    
    // Test circuit breakers
    //
    
    // Test atr limit
    //
    bars[2]._atr = 6.7;
    
    quote.setTrade(TPrice(101), TSize(1));
    quote.setBid(TPrice(101), TSize(20), 0);
    quote.setAsk(TPrice(102), TSize(40), 0);
    activePrice.set(101);
    
    p_runtime._ctFlowbackOccurred = true;
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, " ==> price < ATR -- ATR=102,side=Buy,quote=book[0]=0,20,101|102,40,0,trade=1,101,u,u,u,u,bar=0,,,Unknown,110,100,101,109,10,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,6.7,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kCancelled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(112)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(107));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(106));
    ASSERT_TRUE(!p_runtime._ctFlowbackOccurred);
    
    // Test bar._low limit
    //
    bars[2]._atr = 16.7;
    
    quote.setTrade(TPrice(99), TSize(1));
    quote.setBid(TPrice(99), TSize(20), 0);
    quote.setAsk(TPrice(100), TSize(40), 0);
    activePrice.set(99);
    
    p_runtime._ctFlowbackOccurred = true;
    p_runtime._ctLastSignalBarIndex = 3;
    p_runtime._ctSignalState = tw::common_trade::eContinuationTradeSignalState::kSignaled;
    p_runtime._ctReason.clear();
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, " ==> price < bar._low -- bar._low=100,side=Buy,quote=book[0]=0,20,99|100,40,0,trade=1,99,u,u,u,u,bar=0,,,Unknown,110,100,101,109,10,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,16.7,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kCancelled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(112)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(107));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(106));
    ASSERT_TRUE(!p_runtime._ctFlowbackOccurred);
    
    p_runtime._ctLastSignalBarIndex = 3;
    p_runtime._ctSignalState = tw::common_trade::eContinuationTradeSignalState::kSignaled;
    p_runtime._ctReason.clear();
    
    // Test volRatio logic
    //
    quote.setTrade(TPrice(107), TSize(1));
    quote.setBid(TPrice(106), TSize(20), 0);
    quote.setAsk(TPrice(107), TSize(40), 0);
    activePrice.set(107);
    
    bars[3]._low.set(100);
    
    bars[3]._trades[TPrice(107)]._volAsk.set(4);
    bars[3]._trades[TPrice(107)]._volBid.set(8);
    
    bars[3]._trades[TPrice(106)]._volAsk.set(8);
    bars[3]._trades[TPrice(106)]._volBid.set(4);
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, "");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(112)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(107));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(106));
    ASSERT_TRUE(p_runtime._ctFlowbackOccurred);
        
    bars[3]._trades[TPrice(107)]._volAsk.set(18);
    bars[3]._trades[TPrice(107)]._volBid.set(6);
    
    bars[3]._trades[TPrice(105)]._volAsk.set(10);
    bars[3]._trades[TPrice(105)]._volBid.set(8);
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, " ==> ratio >= _p._ctRatioFor -- volumeFor=18,volumeAgainst=12,ratio=1.5,_p._ctMinContinuationQtyFor=10,_p._ctRatioFor=1.5");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kValidated);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(112)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(107));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(106));
    ASSERT_TRUE(!p_runtime._ctFlowbackOccurred);

}


TEST(CommonTradeLibTestSuit, continuationTradeSignalProcessor_test_flowbackEnabled_sell)
{
    TParams params;
    
    TBars bars;
    TBarPattern pattern;
    TBarPattern trend;
    tw::price::Ticks activePrice;
    eTradeBias tradeBias = eTradeBias::kNeutral;
    
    TImpl impl(params);
    TParams& p = impl.getParams();
    TParamsRuntime& p_runtime = impl.getParamsRuntime();
    p = getParams(TPrice(6), 0.75, false, false, false, 0.56, TPrice(3), TPrice(2), TPrice(1), TSize(10), 1.5);
    
    // Set ctFlowbackEnabled=true
    //
    p._ctFlowbackEnabled = true;
    
    bars.resize(4);    
    bars[2]._dir = tw::common_trade::ePatternDir::kDown;
    bars[2]._high.set(110);
    bars[2]._low.set(100);
    bars[2]._open.set(109);
    bars[2]._close.set(101);
    bars[2]._range.set(bars[2]._high-bars[2]._low);
    bars[2]._atr = 0;
    
    ASSERT_TRUE(impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "ContinuationTradeSignalProcessor::isSignalTriggered()==true -- _p_runtime._ctSide=Sell,_p_runtime._ctMaxPayupPrice=98,_p_runtime._ctMaxFlowbackPrice=103,_p_runtime._ctStopPrice=104,_p._ctMinDispTicks=6,_p._ctBodyToBarRatio=0.75,_p._ctAtrRequired=0,trigger_bar=0,,,Unknown,110,100,109,101,10,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,,pattern=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Unknown,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,,trend=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Unknown,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(98)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(103));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(104));
    ASSERT_TRUE(!p_runtime._ctFlowbackOccurred);
    
    // Test monitorSignal()
    //
    
    // Set quote
    //
    tw::price::QuoteStore::TQuote quote;
    quote.setInstrument(InstrHelper::get6CU2());
    
    // Test circuit breakers
    //
    
    // Test atr limit
    //
    bars[2]._atr = 6.7;
    
    quote.setTrade(TPrice(109), TSize(1));
    quote.setBid(TPrice(108), TSize(20), 0);
    quote.setAsk(TPrice(109), TSize(40), 0);
    activePrice.set(109);
    
    p_runtime._ctFlowbackOccurred = true;
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, " ==> price > ATR -- ATR=108,side=Sell,quote=book[0]=0,20,108|109,40,0,trade=1,109,u,u,u,u,bar=0,,,Unknown,110,100,109,101,10,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,6.7,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kCancelled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(98)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(103));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(104));
    ASSERT_TRUE(!p_runtime._ctFlowbackOccurred);
    
    // Test bar._low limit
    //
    bars[2]._atr = 16.7;
    
    quote.setTrade(TPrice(111), TSize(1));
    quote.setBid(TPrice(110), TSize(20), 0);
    quote.setAsk(TPrice(111), TSize(40), 0);
    activePrice.set(111);
    
    p_runtime._ctLastSignalBarIndex = 3;
    p_runtime._ctSignalState = tw::common_trade::eContinuationTradeSignalState::kSignaled;
    p_runtime._ctReason.clear();
    p_runtime._ctFlowbackOccurred = true;
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, " ==> price > bar._high -- bar._high=110,side=Sell,quote=book[0]=0,20,110|111,40,0,trade=1,111,u,u,u,u,bar=0,,,Unknown,110,100,109,101,10,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,16.7,0,,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kCancelled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(98)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(103));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(104));
    ASSERT_TRUE(!p_runtime._ctFlowbackOccurred);
    
    p_runtime._ctLastSignalBarIndex = 3;
    p_runtime._ctSignalState = tw::common_trade::eContinuationTradeSignalState::kSignaled;
    p_runtime._ctReason.clear();
    
    // Test volRatio logic
    //
    quote.setTrade(TPrice(103), TSize(1));
    quote.setBid(TPrice(103), TSize(20), 0);
    quote.setAsk(TPrice(104), TSize(40), 0);
    activePrice.set(107);
    
    bars[3]._high.set(110);
    
    bars[3]._trades[TPrice(103)]._volAsk.set(8);
    bars[3]._trades[TPrice(103)]._volBid.set(4);
    
    bars[3]._trades[TPrice(104)]._volAsk.set(4);
    bars[3]._trades[TPrice(104)]._volBid.set(8);
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, "");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(98)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(103));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(104));
    ASSERT_TRUE(p_runtime._ctFlowbackOccurred);
        
    bars[3]._trades[TPrice(103)]._volAsk.set(6);
    bars[3]._trades[TPrice(103)]._volBid.set(18);
    
    bars[3]._trades[TPrice(105)]._volAsk.set(8);
    bars[3]._trades[TPrice(105)]._volBid.set(10);
    
    impl.monitorSignal(quote, bars, activePrice);
    ASSERT_EQ(p_runtime._ctReason, " ==> ratio >= _p._ctRatioFor -- volumeFor=18,volumeAgainst=12,ratio=1.5,_p._ctMinContinuationQtyFor=10,_p._ctRatioFor=1.5");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kValidated);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(98)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(103));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(104));
    ASSERT_TRUE(!p_runtime._ctFlowbackOccurred);
    

}


TEST(CommonTradeLibTestSuit, continuationTradeSignalProcessor_test_reqBarFlatClose_buy)
{
    TParams params;
    
    TBars bars;
    TBarPattern pattern;
    TBarPattern trend;
    tw::price::Ticks activePrice;
    eTradeBias tradeBias = eTradeBias::kNeutral;
    
    TImpl impl(params);
    TParams& p = impl.getParams();
    TParamsRuntime& p_runtime = impl.getParamsRuntime();
    p = getParams(TPrice(6), 0.75, false, false, false, 0.56, TPrice(3), TPrice(2), TPrice(1), TSize(10), 1.5);
    
    // Set ctReqBarFlatClose=true
    //
    p._ctReqBarFlatClose = true;
    
    bars.resize(4);    
    bars[2]._dir = tw::common_trade::ePatternDir::kUp;
    bars[2]._high.set(110);
    bars[2]._low.set(100);
    bars[2]._open.set(101);
    bars[2]._close.set(109);
    bars[2]._range.set(bars[2]._high-bars[2]._low);
    bars[2]._atr = 0;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "signal NOT triggered because bar doesn't have flat close");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 0);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());
    
    bars[2]._close.set(bars[2]._high);
    p_runtime._ctLastProcessedBarIndex = 2;
    
    ASSERT_TRUE(impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "ContinuationTradeSignalProcessor::isSignalTriggered()==true -- _p_runtime._ctSide=Buy,_p_runtime._ctMaxPayupPrice=113,_p_runtime._ctMaxFlowbackPrice=108,_p_runtime._ctStopPrice=107,_p._ctMinDispTicks=6,_p._ctBodyToBarRatio=0.75,_p._ctAtrRequired=0,trigger_bar=0,,,Unknown,110,100,101,110,10,0,0,Up,0,00000000-00:00:00.000000,,0,0,0,0,0,,,pattern=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Unknown,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,,trend=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Unknown,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(113)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(108));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(107));

}

TEST(CommonTradeLibTestSuit, continuationTradeSignalProcessor_test_reqBarFlatClose_sell)
{
    TParams params;
    
    TBars bars;
    TBarPattern pattern;
    TBarPattern trend;
    tw::price::Ticks activePrice;
    eTradeBias tradeBias = eTradeBias::kNeutral;
    
    TImpl impl(params);
    TParams& p = impl.getParams();
    TParamsRuntime& p_runtime = impl.getParamsRuntime();
    p = getParams(TPrice(6), 0.75, false, false, false, 0.56, TPrice(3), TPrice(2), TPrice(1), TSize(10), 1.5);
    
    // Set ctReqBarFlatClose=true
    //
    p._ctReqBarFlatClose = true;
    
    bars.resize(4);    
    bars[2]._dir = tw::common_trade::ePatternDir::kDown;
    bars[2]._high.set(110);
    bars[2]._low.set(100);
    bars[2]._open.set(109);
    bars[2]._close.set(101);
    bars[2]._range.set(bars[2]._high-bars[2]._low);
    bars[2]._atr = 0;
    
    ASSERT_TRUE(!impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "signal NOT triggered because bar doesn't have flat close");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kUnknown);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 0);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kUnknown);
    ASSERT_TRUE(!p_runtime._ctMaxPayupPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctMaxFlowbackPrice.isValid());
    ASSERT_TRUE(!p_runtime._ctStopPrice.isValid());
    
    bars[2]._close.set(bars[2]._low);
    p_runtime._ctLastProcessedBarIndex = 2;
    
    ASSERT_TRUE(impl.isSignalTriggered(bars, pattern, trend, tradeBias));
    ASSERT_EQ(p_runtime._ctReason, "ContinuationTradeSignalProcessor::isSignalTriggered()==true -- _p_runtime._ctSide=Sell,_p_runtime._ctMaxPayupPrice=97,_p_runtime._ctMaxFlowbackPrice=102,_p_runtime._ctStopPrice=103,_p._ctMinDispTicks=6,_p._ctBodyToBarRatio=0.75,_p._ctAtrRequired=0,trigger_bar=0,,,Unknown,110,100,109,100,10,0,0,Down,0,00000000-00:00:00.000000,,0,0,0,0,0,,,pattern=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Unknown,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,,trend=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Unknown,0,00000000-00:00:00.000000,,Unknown,Unknown,0,0,0,0,0,");
    ASSERT_EQ(p_runtime._ctSide, tw::channel_or::eOrderSide::kSell);
    ASSERT_EQ(p_runtime._ctLastSignalBarIndex, 3);
    ASSERT_EQ(p_runtime._ctLastProcessedBarIndex, 3);
    ASSERT_EQ(p_runtime._ctSignalState, tw::common_trade::eContinuationTradeSignalState::kSignaled);
    ASSERT_EQ(p_runtime._ctMaxPayupPrice, TPrice(97)); 
    ASSERT_EQ(p_runtime._ctMaxFlowbackPrice, TPrice(102));
    ASSERT_EQ(p_runtime._ctStopPrice, TPrice(103));
    ASSERT_TRUE(!p_runtime._ctFlowbackOccurred);

}

} // common_trade
} // tw

