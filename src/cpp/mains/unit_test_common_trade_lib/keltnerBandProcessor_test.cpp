#include <vector>

#include <tw/common_strat/defs.h>
#include <tw/generated/common_trade_blocks.h>
#include <tw/common_trade/keltnerBandProcessor.h>
#include <tw/price/defs.h>

#include "../unit_test_price_lib/instr_helper.h"

#include <gtest/gtest.h>

namespace tw {
namespace common_trade {
    
const double EPSILON = 0.001;

typedef tw::price::Ticks TPrice;
typedef tw::price::Size TSize;

typedef KeltnerBandProcessor TImpl;
typedef KeltnerBandProcessorParamsWire TParams;


static bool isEqual(double a, double b, double epsilon) {
    return std::abs(a - b) < epsilon;
}

static bool checkKeltnerBandInfo(const KeltnerBandInfo& info,
                                 float middleValue,
                                 float upperValue,
                                 float lowerValue,
                                 float atr,
                                 uint32_t barIndex) {
    bool status = true;
    EXPECT_NEAR(info._middleValue, middleValue, EPSILON);
    if ( !isEqual(info._middleValue, middleValue, EPSILON) )
        status = false;
    
    EXPECT_NEAR(info._upperValue, upperValue, EPSILON);
    if ( !isEqual(info._upperValue, upperValue, EPSILON) )
        status = false;
    
    EXPECT_NEAR(info._lowerValue, lowerValue, EPSILON);
    if ( !isEqual(info._lowerValue, lowerValue, EPSILON) )
        status = false;
    
    EXPECT_NEAR(info._atr, atr, EPSILON);
    if ( !isEqual(info._atr, atr, EPSILON) )
        status = false;
    
    EXPECT_EQ(info._barIndex, barIndex);
    if ( info._barIndex != barIndex )
        status = false;
    
    return status;
}
    
static bool checkKeltnerBandStateInfo(const KeltnerBandStateInfo& info,
                                      eTradeBias bias,
                                      TPrice price) {
    bool status = true;

    EXPECT_EQ(info._bias, bias);
    if ( info._bias != bias )
        status = false;
    
    EXPECT_EQ(info._extremeBarHighLowPrice, price);
    if ( info._extremeBarHighLowPrice != price )
        status = false;
    
    return status;
}

TBars getBarsImpl(uint32_t numberOfBars) {
   TBars bars;
   TBar bar;
   
   uint32_t count = 0;
   
   bar._low = bar._open = TPrice(90);
   bar._high = bar._close = TPrice(100);
   bar._range = bar._high-bar._low;
   bar._dir = ePatternDir::kUp;
   bar._numOfTrades = 10;
   bar._volume = TSize(20);
   bar._atr = 20;
   bar._index = ++count;
   bars.push_back(bar);
   
   if ( numberOfBars == 1 )
       return bars;
   
   bar._high = bar._open = TPrice(101);
   bar._low = bar._close = TPrice(92);
   bar._range = bar._high-bar._low;
   bar._dir = ePatternDir::kDown;
   bar._numOfTrades = 10;
   bar._volume = TSize(20);
   bar._atr = 20;
   bar._index = ++count;
   bars.push_back(bar);
   
   if ( numberOfBars == 2 )
       return bars;
   
   bar._low = bar._open = TPrice(95);
   bar._high = bar._close = TPrice(105);
   bar._range = bar._high-bar._low;
   bar._dir = ePatternDir::kUp;
   bar._numOfTrades = 10;
   bar._volume = TSize(20);
   bar._atr = 20;
   bar._index = ++count;
   bars.push_back(bar);
   
   if ( numberOfBars == 3 )
       return bars;
   
   bar._low = bar._open = TPrice(105);
   bar._high = TPrice(120);
   bar._close = TPrice(112);
   bar._range = bar._high-bar._low;
   bar._dir = ePatternDir::kUp;
   bar._numOfTrades = 10;
   bar._volume = TSize(20);
   bar._atr = 20;
   bar._index = ++count;
   bars.push_back(bar);
   
   if ( numberOfBars == 4 )
       return bars;
   
   bar._high = bar._open = TPrice(112);
   bar._low = bar._close = TPrice(85);
   bar._low = TPrice(85);
   bar._close = TPrice(111);
   bar._range = bar._high-bar._low;
   bar._dir = ePatternDir::kDown;
   bar._numOfTrades = 10;
   bar._volume = TSize(20);
   bar._atr = 20;
   bar._index = ++count;
   bars.push_back(bar);
   
   if ( numberOfBars == 5 )
       return bars;
   
   bar._low = bar._open = TPrice(120);
   bar._high = TPrice(138);
   bar._close = TPrice(135);
   bar._range = bar._high-bar._low;
   bar._dir = ePatternDir::kUp;
   bar._numOfTrades = 10;
   bar._volume = TSize(20);
   bar._atr = 20;
   bar._index = ++count;
   bars.push_back(bar);
   
   if ( numberOfBars == 6 )
       return bars;
   
   bar._low = bar._open = TPrice(136);
   bar._high = TPrice(145);
   bar._close = TPrice(137);
   bar._range = bar._high-bar._low;
   bar._dir = ePatternDir::kUp;
   bar._numOfTrades = 10;
   bar._volume = TSize(20);
   bar._atr = 20;
   bar._index = ++count;
   bars.push_back(bar);
   
   if ( numberOfBars == 7 )
       return bars;
   
   bar._high = bar._open = TPrice(135);
   bar._low = bar._close = TPrice(134);
   bar._range = bar._high-bar._low;
   bar._dir = ePatternDir::kDown;
   bar._numOfTrades = 10;
   bar._volume = TSize(20);
   bar._atr = 20;
   bar._index = ++count;
   bars.push_back(bar);
   
   if ( numberOfBars == 8 )
       return bars;
   
   bar._low = bar._open = TPrice(136);
   bar._high = TPrice(142);
   bar._close = TPrice(137);
   bar._range = bar._high-bar._low;
   bar._dir = ePatternDir::kUp;
   bar._numOfTrades = 10;
   bar._volume = TSize(20);
   bar._atr = 20;
   bar._index = ++count;
   bars.push_back(bar);
   
   if ( numberOfBars == 9 )
       return bars;
   
   bar._low = bar._open = TPrice(138);
   bar._high = bar._close = TPrice(146);
   bar._range = bar._high-bar._low;
   bar._dir = ePatternDir::kUp;
   bar._numOfTrades = 10;
   bar._volume = TSize(20);
   bar._atr = 20;
   bar._index = ++count;
   bars.push_back(bar);
   
   if ( numberOfBars == 10 )
       return bars;
   
   bar._high = bar._open = TPrice(133);
   bar._low = TPrice(127);
   bar._close = TPrice(129);
   bar._range = bar._high-bar._low;
   bar._dir = ePatternDir::kDown;
   bar._numOfTrades = 10;
   bar._volume = TSize(20);
   bar._atr = 20;
   bar._index = ++count;
   bars.push_back(bar);
   
   if ( numberOfBars == 11 )
       return bars;
   
   bar._high = bar._open = TPrice(130);
   bar._low = TPrice(125);
   bar._close = TPrice(128);
   bar._range = bar._high-bar._low;
   bar._dir = ePatternDir::kDown;
   bar._numOfTrades = 10;
   bar._volume = TSize(20);
   bar._atr = 20;
   bar._index = ++count;
   bars.push_back(bar);
   
   if ( numberOfBars == 12 )
       return bars;
   
   bar._low = bar._open = TPrice(132);
   bar._high = TPrice(141);
   bar._close = TPrice(135);
   bar._range = bar._high-bar._low;
   bar._dir = ePatternDir::kUp;
   bar._numOfTrades = 10;
   bar._volume = TSize(20);
   bar._atr = 20;
   bar._index = ++count;
   bars.push_back(bar);
   
   if ( numberOfBars == 13 )
       return bars;
   
   bar._high = bar._open = TPrice(131);
   bar._low = bar._close = TPrice(129);
   bar._range = bar._high-bar._low;
   bar._dir = ePatternDir::kDown;
   bar._numOfTrades = 10;
   bar._volume = TSize(20);
   bar._atr = 20;
   bar._index = ++count;
   bars.push_back(bar);
   
   if ( numberOfBars == 14 )
       return bars;
   
   bar._high = bar._open = TPrice(128);
   bar._low = TPrice(122);
   bar._close = TPrice(124);
   bar._range = bar._high-bar._low;
   bar._dir = ePatternDir::kDown;
   bar._numOfTrades = 10;
   bar._volume = TSize(20);
   bar._atr = 20;
   bar._index = ++count;
   bars.push_back(bar);
   
   if ( numberOfBars == 15 )
       return bars;
   
   bar._low = bar._open = TPrice(129);
   bar._high = TPrice(135);
   bar._close = TPrice(132);
   bar._range = bar._high-bar._low;
   bar._dir = ePatternDir::kUp;
   bar._numOfTrades = 10;
   bar._volume = TSize(20);
   bar._atr = 20;
   bar._index = ++count;
   bars.push_back(bar);
   
   if ( numberOfBars == 16 )
       return bars;
   
   bar._high = bar._open = TPrice(126);
   bar._low = TPrice(118);
   bar._close = TPrice(124);
   bar._range = bar._high-bar._low;
   bar._dir = ePatternDir::kDown;
   bar._numOfTrades = 10;
   bar._volume = TSize(20);
   bar._atr = 20;
   bar._index = ++count;
   bars.push_back(bar);
   
   if ( numberOfBars == 17 )
       return bars;
   
   bar._low = bar._open = TPrice(125);
   bar._high = TPrice(143);
   bar._close = TPrice(136);
   bar._range = bar._high-bar._low;
   bar._dir = ePatternDir::kUp;
   bar._numOfTrades = 10;
   bar._volume = TSize(20);
   bar._atr = 20;
   bar._index = ++count;
   bars.push_back(bar);
   
   return bars;
}


TBars getBars(uint32_t numberOfBars) {
   TBars bars = getBarsImpl(numberOfBars);
   bars.push_back(TBar());
   
   return bars;
}

TEST(CommonTradeLibTestSuit, keltnerBandProcessor_test_isStateChanged)
{
    TParams params;
    TImpl impl(params);
    
    TParams& p = impl.getParams();
    ASSERT_TRUE(!impl.isEnabled());
    
    p._kbMANumOfPeriods = 3;
    p._kbAtrNumOfPeriods = 3;
    p._kbAtrMult = 0.8;    
    ASSERT_TRUE(impl.isEnabled());        
    
    ASSERT_TRUE(!impl.isStateChanged(getBars(1)));
    ASSERT_EQ(p._kbReason, "KeltnerBandProcessor: size < _p._kbMANumOfPeriods");
    ASSERT_EQ(p._kbProcessdBarIndex, 1UL);
    ASSERT_EQ(p._kbBias, eTradeBias::kUnknown);
    ASSERT_TRUE(p._kbStateInfos.empty());
    ASSERT_TRUE(p._kbInfos.empty());
    
    ASSERT_TRUE(!impl.isStateChanged(getBars(3)));
    ASSERT_EQ(p._kbReason, "KeltnerBandProcessor: NO STATE CHANGE - bar is inside keltner band ==> bias=Unknown,info=99,107.533,90.4667,10.6667,3,bar=3,,,Unknown,105,95,95,105,10,20,10,Up,0,00000000-00:00:00.000000,,0,0,0,20,0,,");
    ASSERT_EQ(p._kbProcessdBarIndex, 3UL);
    ASSERT_EQ(p._kbBias, eTradeBias::kUnknown);
    ASSERT_EQ(p._kbBarPositionToKeltnerBand, eBarPositionToKeltnerBand::kMiddle);
    ASSERT_TRUE(p._kbStateInfos.empty());
    ASSERT_EQ(p._kbInfos.size(), 1UL);
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[0], 99, 107.533, 90.4667, 10.66667, 3));
    
    ASSERT_TRUE(impl.isStateChanged(getBars(4)));
    ASSERT_EQ(p._kbReason, "KeltnerBandProcessor: STATE CHANGE ==> stateInfo=Long,120,bias=Long,info=103,112.867,93.1333,12.3333,4,bar=4,,,Unknown,120,105,105,112,15,20,10,Up,0,00000000-00:00:00.000000,,0,0,0,20,0,,");
    ASSERT_EQ(p._kbProcessdBarIndex, 4UL);
    ASSERT_EQ(p._kbBias, eTradeBias::kLong);
    ASSERT_EQ(p._kbBarPositionToKeltnerBand, eBarPositionToKeltnerBand::kHigh);
    ASSERT_EQ(p._kbStateInfos.size(), 1UL);
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[0], eTradeBias::kLong, TPrice(120)));
    ASSERT_EQ(p._kbInfos.size(), 2UL);
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[0], 99, 107.533, 90.4667, 10.66667, 3));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[1], 103, 112.867, 93.1333, 12.3333, 4));
    
    ASSERT_TRUE(!impl.isStateChanged(getBars(4)));
    ASSERT_EQ(p._kbReason, "");
    ASSERT_EQ(p._kbProcessdBarIndex, 4UL);
    ASSERT_EQ(p._kbBias, eTradeBias::kLong);
    ASSERT_EQ(p._kbBarPositionToKeltnerBand, eBarPositionToKeltnerBand::kHigh);
    ASSERT_EQ(p._kbStateInfos.size(), 1UL);
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[0], eTradeBias::kLong, TPrice(120)));
    ASSERT_EQ(p._kbInfos.size(), 2UL);
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[0], 99, 107.533, 90.4667, 10.66667, 3));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[1], 103, 112.867, 93.1333, 12.3333, 4));
    
    ASSERT_TRUE(impl.isStateChanged(getBars(5)));
    ASSERT_EQ(p._kbReason, "KeltnerBandProcessor: STATE CHANGE ==> stateInfo=Short,85,bias=Short,info=109.333,124,94.6667,18.3333,5,bar=5,,,Unknown,112,85,112,111,27,20,10,Down,0,00000000-00:00:00.000000,,0,0,0,20,0,,");
    ASSERT_EQ(p._kbProcessdBarIndex, 5UL);
    ASSERT_EQ(p._kbBias, eTradeBias::kShort);
    ASSERT_EQ(p._kbBarPositionToKeltnerBand, eBarPositionToKeltnerBand::kLow);
    ASSERT_EQ(p._kbStateInfos.size(), 2UL);
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[0], eTradeBias::kLong, TPrice(120)));
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[1], eTradeBias::kShort, TPrice(85)));
    ASSERT_EQ(p._kbInfos.size(), 3UL);
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[0], 99, 107.533, 90.4667, 10.66667, 3));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[1], 103, 112.867, 93.1333, 12.3333, 4));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[2], 109.333, 124, 94.6667, 18.3333, 5));
    
    ASSERT_TRUE(impl.isStateChanged(getBars(6)));
    ASSERT_EQ(p._kbReason, "KeltnerBandProcessor: STATE CHANGE to kNoTrades ==> stateInfo=Long,138,bias=NoTrades,info=119.333,137.733,100.933,23,6,bar=6,,,Unknown,138,120,120,135,18,20,10,Up,0,00000000-00:00:00.000000,,0,0,0,20,0,,");
    ASSERT_EQ(p._kbProcessdBarIndex, 6UL);
    ASSERT_EQ(p._kbBias, eTradeBias::kNoTrades);
    ASSERT_EQ(p._kbBarPositionToKeltnerBand, eBarPositionToKeltnerBand::kHigh);
    ASSERT_EQ(p._kbStateInfos.size(), 3UL);
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[0], eTradeBias::kLong, TPrice(120)));
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[1], eTradeBias::kShort, TPrice(85)));
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[2], eTradeBias::kLong, TPrice(138)));
    ASSERT_EQ(p._kbInfos.size(), 4UL);
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[0], 99, 107.533, 90.4667, 10.66667, 3));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[1], 103, 112.867, 93.1333, 12.3333, 4));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[2], 109.333, 124, 94.6667, 18.3333, 5));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[3], 119.333, 137.733, 100.933, 23, 6));
        
    ASSERT_TRUE(!impl.isStateChanged(getBars(7)));
    ASSERT_EQ(p._kbReason, "KeltnerBandProcessor: NO STATE CHANGE (checkIsSameBreak()==true) from kNoTrades ==> bias=NoTrades,info=127.667,144.733,110.6,21.3333,7,stateInfo=Long,145,bar=7,,,Unknown,145,136,136,137,9,20,10,Up,0,00000000-00:00:00.000000,,0,0,0,20,0,,");
    ASSERT_EQ(p._kbProcessdBarIndex, 7UL);
    ASSERT_EQ(p._kbBias, eTradeBias::kNoTrades);
    ASSERT_EQ(p._kbBarPositionToKeltnerBand, eBarPositionToKeltnerBand::kHigh);
    ASSERT_EQ(p._kbStateInfos.size(), 3UL);
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[0], eTradeBias::kLong, TPrice(120)));
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[1], eTradeBias::kShort, TPrice(85)));
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[2], eTradeBias::kLong, TPrice(145)));
    ASSERT_EQ(p._kbInfos.size(), 5UL);
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[0], 99, 107.533, 90.4667, 10.66667, 3));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[1], 103, 112.867, 93.1333, 12.3333, 4));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[2], 109.333, 124, 94.6667, 18.3333, 5));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[3], 119.333, 137.733, 100.933, 23, 6));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[4], 127.667, 144.733, 110.6, 21.3333, 7));
    
    ASSERT_TRUE(!impl.isStateChanged(getBars(8)));
    ASSERT_EQ(p._kbReason, "KeltnerBandProcessor: NO STATE CHANGE - bar is inside keltner band ==> bias=NoTrades,info=135.333,146,124.667,13.3333,8,bar=8,,,Unknown,135,134,135,134,1,20,10,Down,0,00000000-00:00:00.000000,,0,0,0,20,0,,");
    ASSERT_EQ(p._kbProcessdBarIndex, 8UL);
    ASSERT_EQ(p._kbBias, eTradeBias::kNoTrades);
    ASSERT_EQ(p._kbBarPositionToKeltnerBand, eBarPositionToKeltnerBand::kMiddle);
    ASSERT_EQ(p._kbStateInfos.size(), 3UL);
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[0], eTradeBias::kLong, TPrice(120)));
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[1], eTradeBias::kShort, TPrice(85)));
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[2], eTradeBias::kLong, TPrice(145)));
    ASSERT_EQ(p._kbInfos.size(), 6UL);
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[0], 99, 107.533, 90.4667, 10.66667, 3));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[1], 103, 112.867, 93.1333, 12.3333, 4));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[2], 109.333, 124, 94.6667, 18.3333, 5));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[3], 119.333, 137.733, 100.933, 23, 6));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[4], 127.667, 144.733, 110.6, 21.3333, 7));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[5], 135.333, 146, 124.667, 13.3333, 8));
    
    ASSERT_TRUE(!impl.isStateChanged(getBars(9)));
    ASSERT_EQ(p._kbReason, "KeltnerBandProcessor: NO STATE CHANGE from kNoTrades ==> bias=NoTrades,info=136,141.6,130.4,7,9,stateInfoOld=Long,145,stateInfoNew=Long,142,bar=9,,,Unknown,142,136,136,137,6,20,10,Up,0,00000000-00:00:00.000000,,0,0,0,20,0,,");
    ASSERT_EQ(p._kbProcessdBarIndex, 9UL);
    ASSERT_EQ(p._kbBias, eTradeBias::kNoTrades);
    ASSERT_EQ(p._kbBarPositionToKeltnerBand, eBarPositionToKeltnerBand::kMiddle);
    ASSERT_EQ(p._kbStateInfos.size(), 3UL);
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[0], eTradeBias::kLong, TPrice(120)));
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[1], eTradeBias::kShort, TPrice(85)));
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[2], eTradeBias::kLong, TPrice(145)));
    ASSERT_EQ(p._kbInfos.size(), 7UL);
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[0], 99, 107.533, 90.4667, 10.66667, 3));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[1], 103, 112.867, 93.1333, 12.3333, 4));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[2], 109.333, 124, 94.6667, 18.3333, 5));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[3], 119.333, 137.733, 100.933, 23, 6));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[4], 127.667, 144.733, 110.6, 21.3333, 7));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[5], 135.333, 146, 124.667, 13.3333, 8));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[6], 136, 141.6, 130.4, 7, 9));
    
    ASSERT_TRUE(impl.isStateChanged(getBars(10)));
    ASSERT_EQ(p._kbReason, "KeltnerBandProcessor: STATE CHANGE from kNoTrades ==> bias=Long,info=139,144.333,133.667,6.66667,10,stateInfoOld=Long,145,stateInfoNew=Long,146,bar=10,,,Unknown,146,138,138,146,8,20,10,Up,0,00000000-00:00:00.000000,,0,0,0,20,0,,");
    ASSERT_EQ(p._kbProcessdBarIndex, 10UL);
    ASSERT_EQ(p._kbBias, eTradeBias::kLong);
    ASSERT_EQ(p._kbBarPositionToKeltnerBand, eBarPositionToKeltnerBand::kHigh);
    ASSERT_EQ(p._kbStateInfos.size(), 1UL);
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[0], eTradeBias::kLong, TPrice(146)));
    ASSERT_EQ(p._kbInfos.size(), 8UL);
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[0], 99, 107.533, 90.4667, 10.66667, 3));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[1], 103, 112.867, 93.1333, 12.3333, 4));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[2], 109.333, 124, 94.6667, 18.3333, 5));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[3], 119.333, 137.733, 100.933, 23, 6));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[4], 127.667, 144.733, 110.6, 21.3333, 7));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[5], 135.333, 146, 124.667, 13.3333, 8));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[6], 136, 141.6, 130.4, 7, 9));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[7], 139, 144.333, 133.667, 6.66667, 10));
    
    ASSERT_TRUE(impl.isStateChanged(getBars(11)));
    ASSERT_EQ(p._kbReason, "KeltnerBandProcessor: STATE CHANGE ==> stateInfo=Short,127,bias=Short,info=137.333,146.933,127.733,12,11,bar=11,,,Unknown,133,127,133,129,6,20,10,Down,0,00000000-00:00:00.000000,,0,0,0,20,0,,");
    ASSERT_EQ(p._kbProcessdBarIndex, 11UL);
    ASSERT_EQ(p._kbBias, eTradeBias::kShort);
    ASSERT_EQ(p._kbBarPositionToKeltnerBand, eBarPositionToKeltnerBand::kLow);
    ASSERT_EQ(p._kbStateInfos.size(), 2UL);
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[0], eTradeBias::kLong, TPrice(146)));
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[1], eTradeBias::kShort, TPrice(127)));
    ASSERT_EQ(p._kbInfos.size(), 9UL);
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[0], 99, 107.533, 90.4667, 10.66667, 3));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[1], 103, 112.867, 93.1333, 12.3333, 4));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[2], 109.333, 124, 94.6667, 18.3333, 5));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[3], 119.333, 137.733, 100.933, 23, 6));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[4], 127.667, 144.733, 110.6, 21.3333, 7));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[5], 135.333, 146, 124.667, 13.3333, 8));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[6], 136, 141.6, 130.4, 7, 9));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[7], 139, 144.333, 133.667, 6.66667, 10));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[8], 137.333, 146.933, 127.733, 12, 11));

    ASSERT_TRUE(!impl.isStateChanged(getBars(12)));
    ASSERT_EQ(p._kbReason, "KeltnerBandProcessor: NO STATE CHANGE (checkIsSameBreak()==true) ==> bias=Short,info=134.333,143.133,125.533,11,12,stateInfo=Short,125,bar=12,,,Unknown,130,125,130,128,5,20,10,Down,0,00000000-00:00:00.000000,,0,0,0,20,0,,");
    ASSERT_EQ(p._kbProcessdBarIndex, 12UL);
    ASSERT_EQ(p._kbBias, eTradeBias::kShort);
    ASSERT_EQ(p._kbBarPositionToKeltnerBand, eBarPositionToKeltnerBand::kLow);
    ASSERT_EQ(p._kbStateInfos.size(), 2UL);
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[0], eTradeBias::kLong, TPrice(146)));
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[1], eTradeBias::kShort, TPrice(125)));
    ASSERT_EQ(p._kbInfos.size(), 10UL);
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[0], 99, 107.533, 90.4667, 10.66667, 3));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[1], 103, 112.867, 93.1333, 12.3333, 4));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[2], 109.333, 124, 94.6667, 18.3333, 5));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[3], 119.333, 137.733, 100.933, 23, 6));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[4], 127.667, 144.733, 110.6, 21.3333, 7));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[5], 135.333, 146, 124.667, 13.3333, 8));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[6], 136, 141.6, 130.4, 7, 9));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[7], 139, 144.333, 133.667, 6.66667, 10));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[9], 134.333, 143.133, 125.533, 11, 12));
        
    ASSERT_TRUE(impl.isStateChanged(getBars(13)));
    ASSERT_EQ(p._kbReason, "KeltnerBandProcessor: STATE CHANGE to kNoTrades ==> stateInfo=Long,141,bias=NoTrades,info=130.667,140.533,120.8,12.3333,13,bar=13,,,Unknown,141,132,132,135,9,20,10,Up,0,00000000-00:00:00.000000,,0,0,0,20,0,,");
    ASSERT_EQ(p._kbProcessdBarIndex, 13UL);
    ASSERT_EQ(p._kbBias, eTradeBias::kNoTrades);
    ASSERT_EQ(p._kbBarPositionToKeltnerBand, eBarPositionToKeltnerBand::kHigh);
    ASSERT_EQ(p._kbStateInfos.size(), 3UL);
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[0], eTradeBias::kLong, TPrice(146)));
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[1], eTradeBias::kShort, TPrice(125)));
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[2], eTradeBias::kLong, TPrice(141)));
    ASSERT_EQ(p._kbInfos.size(), 11UL);
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[0], 99, 107.533, 90.4667, 10.66667, 3));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[1], 103, 112.867, 93.1333, 12.3333, 4));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[2], 109.333, 124, 94.6667, 18.3333, 5));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[3], 119.333, 137.733, 100.933, 23, 6));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[4], 127.667, 144.733, 110.6, 21.3333, 7));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[5], 135.333, 146, 124.667, 13.3333, 8));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[6], 136, 141.6, 130.4, 7, 9));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[7], 139, 144.333, 133.667, 6.66667, 10));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[9], 134.333, 143.133, 125.533, 11, 12));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[10], 130.667, 140.533, 120.8, 12.3333, 13));
    
    ASSERT_TRUE(!impl.isStateChanged(getBars(14)));
    ASSERT_EQ(p._kbReason, "KeltnerBandProcessor: NO STATE CHANGE - bar is inside keltner band ==> bias=NoTrades,info=130.667,137.067,124.267,8,14,bar=14,,,Unknown,131,129,131,129,2,20,10,Down,0,00000000-00:00:00.000000,,0,0,0,20,0,,");
    ASSERT_EQ(p._kbProcessdBarIndex, 14UL);
    ASSERT_EQ(p._kbBias, eTradeBias::kNoTrades);
    ASSERT_EQ(p._kbBarPositionToKeltnerBand, eBarPositionToKeltnerBand::kMiddle);
    ASSERT_EQ(p._kbStateInfos.size(), 3UL);
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[0], eTradeBias::kLong, TPrice(146)));
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[1], eTradeBias::kShort, TPrice(125)));
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[2], eTradeBias::kLong, TPrice(141)));
    ASSERT_EQ(p._kbInfos.size(), 12UL);
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[0], 99, 107.533, 90.4667, 10.66667, 3));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[1], 103, 112.867, 93.1333, 12.3333, 4));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[2], 109.333, 124, 94.6667, 18.3333, 5));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[3], 119.333, 137.733, 100.933, 23, 6));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[4], 127.667, 144.733, 110.6, 21.3333, 7));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[5], 135.333, 146, 124.667, 13.3333, 8));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[6], 136, 141.6, 130.4, 7, 9));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[7], 139, 144.333, 133.667, 6.66667, 10));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[9], 134.333, 143.133, 125.533, 11, 12));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[10], 130.667, 140.533, 120.8, 12.3333, 13));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[11], 130.667, 137.067, 124.267, 8, 14));
    
    ASSERT_TRUE(impl.isStateChanged(getBars(15)));
    ASSERT_EQ(p._kbReason, "KeltnerBandProcessor: STATE CHANGE from kNoTrades ==> bias=Short,info=129.333,136.267,122.4,8.66667,15,stateInfoOld=Short,125,stateInfoNew=Short,122,bar=15,,,Unknown,128,122,128,124,6,20,10,Down,0,00000000-00:00:00.000000,,0,0,0,20,0,,");
    ASSERT_EQ(p._kbProcessdBarIndex, 15UL);
    ASSERT_EQ(p._kbBias, eTradeBias::kShort);
    ASSERT_EQ(p._kbBarPositionToKeltnerBand, eBarPositionToKeltnerBand::kLow);
    ASSERT_EQ(p._kbStateInfos.size(), 1UL);
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[0], eTradeBias::kShort, TPrice(122)));
    ASSERT_EQ(p._kbInfos.size(), 13UL);
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[0], 99, 107.533, 90.4667, 10.66667, 3));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[1], 103, 112.867, 93.1333, 12.3333, 4));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[2], 109.333, 124, 94.6667, 18.3333, 5));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[3], 119.333, 137.733, 100.933, 23, 6));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[4], 127.667, 144.733, 110.6, 21.3333, 7));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[5], 135.333, 146, 124.667, 13.3333, 8));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[6], 136, 141.6, 130.4, 7, 9));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[7], 139, 144.333, 133.667, 6.66667, 10));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[9], 134.333, 143.133, 125.533, 11, 12));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[10], 130.667, 140.533, 120.8, 12.3333, 13));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[11], 130.667, 137.067, 124.267, 8, 14));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[12], 129.333, 136.267, 122.4, 8.667, 15));
    
    ASSERT_TRUE(impl.isStateChanged(getBars(16)));
    ASSERT_EQ(p._kbReason, "KeltnerBandProcessor: STATE CHANGE ==> stateInfo=Long,135,bias=Long,info=128.333,134.733,121.933,8,16,bar=16,,,Unknown,135,129,129,132,6,20,10,Up,0,00000000-00:00:00.000000,,0,0,0,20,0,,");
    ASSERT_EQ(p._kbProcessdBarIndex, 16UL);
    ASSERT_EQ(p._kbBias, eTradeBias::kLong);
    ASSERT_EQ(p._kbBarPositionToKeltnerBand, eBarPositionToKeltnerBand::kHigh);
    ASSERT_EQ(p._kbStateInfos.size(), 2UL);
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[0], eTradeBias::kShort, TPrice(122)));
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[1], eTradeBias::kLong, TPrice(135)));
    ASSERT_EQ(p._kbInfos.size(), 14UL);
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[0], 99, 107.533, 90.4667, 10.66667, 3));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[1], 103, 112.867, 93.1333, 12.3333, 4));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[2], 109.333, 124, 94.6667, 18.3333, 5));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[3], 119.333, 137.733, 100.933, 23, 6));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[4], 127.667, 144.733, 110.6, 21.3333, 7));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[5], 135.333, 146, 124.667, 13.3333, 8));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[6], 136, 141.6, 130.4, 7, 9));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[7], 139, 144.333, 133.667, 6.66667, 10));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[9], 134.333, 143.133, 125.533, 11, 12));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[10], 130.667, 140.533, 120.8, 12.3333, 13));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[11], 130.667, 137.067, 124.267, 8, 14));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[12], 129.333, 136.267, 122.4, 8.667, 15));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[13], 128.333, 134.733, 121.933, 8, 16));
    
    ASSERT_TRUE(impl.isStateChanged(getBars(17)));
    ASSERT_EQ(p._kbReason, "KeltnerBandProcessor: STATE CHANGE to kNoTrades ==> stateInfo=Short,118,bias=NoTrades,info=126.667,135.2,118.133,10.6667,17,bar=17,,,Unknown,126,118,126,124,8,20,10,Down,0,00000000-00:00:00.000000,,0,0,0,20,0,,");
    ASSERT_EQ(p._kbProcessdBarIndex, 17UL);
    ASSERT_EQ(p._kbBias, eTradeBias::kNoTrades);
    ASSERT_EQ(p._kbBarPositionToKeltnerBand, eBarPositionToKeltnerBand::kLow);
    ASSERT_EQ(p._kbStateInfos.size(), 3UL);
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[0], eTradeBias::kShort, TPrice(122)));
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[1], eTradeBias::kLong, TPrice(135)));
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[2], eTradeBias::kShort, TPrice(118)));
    ASSERT_EQ(p._kbInfos.size(), 15UL);
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[0], 99, 107.533, 90.4667, 10.66667, 3));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[1], 103, 112.867, 93.1333, 12.3333, 4));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[2], 109.333, 124, 94.6667, 18.3333, 5));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[3], 119.333, 137.733, 100.933, 23, 6));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[4], 127.667, 144.733, 110.6, 21.3333, 7));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[5], 135.333, 146, 124.667, 13.3333, 8));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[6], 136, 141.6, 130.4, 7, 9));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[7], 139, 144.333, 133.667, 6.66667, 10));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[9], 134.333, 143.133, 125.533, 11, 12));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[10], 130.667, 140.533, 120.8, 12.3333, 13));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[11], 130.667, 137.067, 124.267, 8, 14));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[12], 129.333, 136.267, 122.4, 8.667, 15));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[13], 128.333, 134.733, 121.933, 8, 16));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[14], 126.667, 135.2, 118.133, 10.667, 17));
    
    ASSERT_TRUE(impl.isStateChanged(getBars(18)));
    ASSERT_EQ(p._kbReason, "KeltnerBandProcessor: STATE CHANGE from kNoTrades ==> bias=Long,info=130.667,142.4,118.933,14.6667,18,stateInfoOld=Long,135,stateInfoNew=Long,143,bar=18,,,Unknown,143,125,125,136,18,20,10,Up,0,00000000-00:00:00.000000,,0,0,0,20,0,,");
    ASSERT_EQ(p._kbProcessdBarIndex, 18UL);
    ASSERT_EQ(p._kbBias, eTradeBias::kLong);
    ASSERT_EQ(p._kbBarPositionToKeltnerBand, eBarPositionToKeltnerBand::kHigh);
    ASSERT_EQ(p._kbStateInfos.size(), 1UL);
    ASSERT_TRUE(checkKeltnerBandStateInfo(p._kbStateInfos[0], eTradeBias::kLong, TPrice(143)));
    ASSERT_EQ(p._kbInfos.size(), 16UL);
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[0], 99, 107.533, 90.4667, 10.66667, 3));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[1], 103, 112.867, 93.1333, 12.3333, 4));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[2], 109.333, 124, 94.6667, 18.3333, 5));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[3], 119.333, 137.733, 100.933, 23, 6));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[4], 127.667, 144.733, 110.6, 21.3333, 7));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[5], 135.333, 146, 124.667, 13.3333, 8));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[6], 136, 141.6, 130.4, 7, 9));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[7], 139, 144.333, 133.667, 6.66667, 10));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[9], 134.333, 143.133, 125.533, 11, 12));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[10], 130.667, 140.533, 120.8, 12.3333, 13));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[11], 130.667, 137.067, 124.267, 8, 14));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[12], 129.333, 136.267, 122.4, 8.667, 15));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[13], 128.333, 134.733, 121.933, 8, 16));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[14], 126.667, 135.2, 118.133, 10.667, 17));
    ASSERT_TRUE(checkKeltnerBandInfo(p._kbInfos[15], 130.667, 142.4, 118.933, 14.6667, 18));
    

}


} // common_trade
} // tw
