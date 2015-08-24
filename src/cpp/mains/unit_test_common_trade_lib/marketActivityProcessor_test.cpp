#include <vector>

#include <tw/common_strat/defs.h>
#include <tw/generated/common_trade_blocks.h>
#include <tw/common_trade/marketActivityProcessor.h>
#include <tw/price/defs.h>

#include "../unit_test_price_lib/instr_helper.h"

#include <gtest/gtest.h>

namespace tw {
namespace common_trade {
    
typedef tw::price::Ticks TPrice;
typedef tw::price::Size TSize;

typedef tw::common_trade::TBar TBar;

typedef MarketActivityProcessor TImpl;
typedef MarketActivityParamsWire TParams;

struct TestBarsManagerInfo {
    TestBarsManagerInfo() {
        clear();
    }
    
    void clear() {
        _x = -1;
        _count = 0;
        _offsetFromCurrent = 0;
    }
    
    tw::instr::Instrument::TKeyId _x;
    size_t _count;
    size_t _offsetFromCurrent;
};

typedef std::pair<TestBarsManagerInfo, TestBarsManagerInfo> TTestBarsManagerInfo;

class TestBarsManager {
public:
    TestBarsManager() {
        clear();
    }
    
    void clear() {
        _count = 0;
        _avgVol1.set(0);
        _avgVol2.set(0);;
        _info.first.clear();
        _info.second.clear();
    }
    
    tw::price::Size getBackBarsCumVol(const tw::instr::Instrument::TKeyId& x, size_t count, size_t offsetFromCurrent) const {
        if ( 0 == _count ) {
            TestBarsManagerInfo& info = _info.first;
            
            info._x = x;
            info._count = count;
            info._offsetFromCurrent = offsetFromCurrent;
            
            ++_count;
            return _avgVol1;
        }
        
        ++_count;
        TestBarsManagerInfo& info = _info.second;
            
        info._x = x;
        info._count = count;
        info._offsetFromCurrent = offsetFromCurrent;
        
        return _avgVol2;
    }
    
    mutable uint32_t _count;
    tw::price::Size _avgVol1;
    tw::price::Size _avgVol2;
    mutable TTestBarsManagerInfo _info;
};

static TParams getParams(TPrice minDispTicks,
                         TSize volToEnter,
                         float atrRequired,
                         float volMultToExit,
                         uint32_t volMultToExitInterval) {
    TParams p;
    
    p._maMinDispTicks = minDispTicks;
    p._maVolToEnter = volToEnter;
    p._maAtrRequired = atrRequired;
    p._maVolMultToExit = volMultToExit;
    p._maVolMultToExitInterval = volMultToExitInterval;
    
    return p;
}

TEST(CommonTradeLibTestSuit, MarketActivityProcessor_test)
{
    TParams params;
    
    TImpl impl(params);
    TParams& p = impl.getParams();
    p = getParams(TPrice(10), TSize(50), 4.5, 0.3, 3);
    
    TBars bars;
    TestBarsManager barsManager;

    // Test initialization
    //
    impl.processSignal(bars, barsManager);
    ASSERT_TRUE(!impl.isEnabled());
    ASSERT_TRUE(!impl.isSignalOn());
    ASSERT_EQ(p._maReason, "");

    p._maEnabled = true;
    impl.processSignal(bars, barsManager);
    ASSERT_TRUE(!p._maIsOn);
    ASSERT_TRUE(impl.isEnabled());
    ASSERT_EQ(p._maReason, "bars.size() < 3");
    
    // Test turning signal on
    //
    bars.resize(4);
    bars[2]._instrument = InstrHelper::get6CU2();
    bars[2]._range.set(9);
    bars[2]._atr = 4.2;
    bars[2]._volume.set(48);
    
    impl.processSignal(bars, barsManager);
    ASSERT_TRUE(impl.isEnabled());
    ASSERT_TRUE(!impl.isSignalOn());
    ASSERT_EQ(p._maLastProcessedBarIndex, 3);
    ASSERT_EQ(p._maLastSignalOnBarIndex, 0);
    ASSERT_EQ(p._maReason, "_p._maIsOn==false(not changed) -- _p._maMinDispTicks=10,_p._maAtrRequired=4.5,_p._maVolToEnter=50,_p._maMinVolToEnter=0,bar=0,,,Unknown,InV,InV,InV,InV,9,48,0,Unknown,0,00000000-00:00:00.000000,,0,0,0,4.2,0,,");
        
    bars[2]._range.set(11);
    p._maLastProcessedBarIndex = 0;
    impl.processSignal(bars, barsManager);
    ASSERT_TRUE(impl.isEnabled());
    ASSERT_TRUE(impl.isSignalOn());
    ASSERT_EQ(p._maLastSignalOnBarIndex, 2);
    ASSERT_EQ(p._maReason, "_p._maIsOn==true(bar._range >= _p._maMinDispTicks && (!_p._maMinVolToEnter.isValid() || bar._volume >= _p._maMinVolToEnter)) -- _p._ctMinDispTicks=10,_p._maMinVolToEnter=0,bar=0,,,Unknown,InV,InV,InV,InV,11,48,0,Unknown,0,00000000-00:00:00.000000,,0,0,0,4.2,0,,");
    
    p._maLastProcessedBarIndex = 0;
    p._maLastSignalOnBarIndex = 0;
    p._maIsOn = false;
    p._maMinVolToEnter.set(49);
    impl.processSignal(bars, barsManager);
    ASSERT_TRUE(impl.isEnabled());
    ASSERT_TRUE(!impl.isSignalOn());
    ASSERT_EQ(p._maLastProcessedBarIndex, 3);
    ASSERT_EQ(p._maLastSignalOnBarIndex, 0);
    ASSERT_EQ(p._maReason, "_p._maIsOn==false(not changed) -- _p._maMinDispTicks=10,_p._maAtrRequired=4.5,_p._maVolToEnter=50,_p._maMinVolToEnter=49,bar=0,,,Unknown,InV,InV,InV,InV,11,48,0,Unknown,0,00000000-00:00:00.000000,,0,0,0,4.2,0,,");
    
    p._maLastProcessedBarIndex = 0;
    p._maLastSignalOnBarIndex = 0;
    p._maIsOn = false;
    p._maMinVolToEnter.set(47);
    impl.processSignal(bars, barsManager);
    ASSERT_TRUE(impl.isEnabled());
    ASSERT_TRUE(impl.isSignalOn());
    ASSERT_EQ(p._maLastSignalOnBarIndex, 2);
    ASSERT_EQ(p._maReason, "_p._maIsOn==true(bar._range >= _p._maMinDispTicks && (!_p._maMinVolToEnter.isValid() || bar._volume >= _p._maMinVolToEnter)) -- _p._ctMinDispTicks=10,_p._maMinVolToEnter=47,bar=0,,,Unknown,InV,InV,InV,InV,11,48,0,Unknown,0,00000000-00:00:00.000000,,0,0,0,4.2,0,,");
    
    
    bars[2]._range.set(9);
    bars[2]._atr = 4.6;
    p._maLastProcessedBarIndex = 0;
    p._maLastSignalOnBarIndex = 0;
    p._maIsOn = false;
    impl.processSignal(bars, barsManager);
    ASSERT_TRUE(impl.isEnabled());
    ASSERT_TRUE(impl.isSignalOn());
    ASSERT_EQ(p._maLastSignalOnBarIndex, 2);
    ASSERT_EQ(p._maReason, "_p._maIsOn==true(bar._atr >= _p._maAtrRequired) -- _p._maAtrRequired=4.5,bar=0,,,Unknown,InV,InV,InV,InV,9,48,0,Unknown,0,00000000-00:00:00.000000,,0,0,0,4.6,0,,");
    
    bars[2]._atr = 4.2;
    bars[2]._volume.set(52);
    p._maLastProcessedBarIndex = 0;
    p._maIsOn = false;
    impl.processSignal(bars, barsManager);
    ASSERT_TRUE(impl.isEnabled());
    ASSERT_TRUE(impl.isSignalOn());
    ASSERT_EQ(p._maLastSignalOnBarIndex, 2);
    ASSERT_EQ(p._maReason, "_p._maIsOn==true(bar._volume >= _p._maVolToEnter) -- _p._maVolToEnter=50,bar=0,,,Unknown,InV,InV,InV,InV,9,52,0,Unknown,0,00000000-00:00:00.000000,,0,0,0,4.2,0,,");
    
    impl.processSignal(bars, barsManager);
    ASSERT_TRUE(impl.isEnabled());
    ASSERT_TRUE(impl.isSignalOn());
    ASSERT_EQ(p._maReason, "");
    
    // Test turning signal off
    //
    p._maLastProcessedBarIndex = 0;
    
    impl.processSignal(bars, barsManager);
    ASSERT_TRUE(impl.isEnabled());
    ASSERT_TRUE(impl.isSignalOn());
    ASSERT_EQ(p._maLastSignalOnBarIndex, 2);
    ASSERT_EQ(barsManager._count, 0);
    ASSERT_EQ(p._maReason, "");
    
    barsManager.clear();
    p._maLastSignalOnBarIndex = 4;
    bars.resize(9);
    bars[7]._instrument = InstrHelper::get6CU2();
    
    barsManager._avgVol1.set(38*3);
    barsManager._avgVol2.set(100*1);

    impl.processSignal(bars, barsManager);
    ASSERT_TRUE(impl.isEnabled());
    ASSERT_TRUE(impl.isSignalOn());
    ASSERT_EQ(p._maLastSignalOnBarIndex, 4);
    ASSERT_EQ(p._maReason, "");
    
    ASSERT_EQ(barsManager._count, 2);
    ASSERT_EQ(barsManager._info.first._x, 15);
    ASSERT_EQ(barsManager._info.first._count, 3);
    ASSERT_EQ(barsManager._info.first._offsetFromCurrent, 1);
    ASSERT_EQ(barsManager._info.second._x, 15);
    ASSERT_EQ(barsManager._info.second._count, 1);
    ASSERT_EQ(barsManager._info.second._offsetFromCurrent, 4);
    
    
    barsManager.clear();
    
    bars.resize(10);
    bars[8]._instrument = InstrHelper::get6CU2();
    
    barsManager._avgVol1.set(28*3);
    barsManager._avgVol2.set(100*2);
    
    impl.processSignal(bars, barsManager);
    ASSERT_TRUE(impl.isEnabled());
    ASSERT_TRUE(!impl.isSignalOn());
    ASSERT_EQ(p._maLastSignalOnBarIndex, 4);
    
    ASSERT_EQ(barsManager._count, 2);
    ASSERT_EQ(barsManager._info.first._x, 15);
    ASSERT_EQ(barsManager._info.first._count, 3);
    ASSERT_EQ(barsManager._info.first._offsetFromCurrent, 1);
    ASSERT_EQ(barsManager._info.second._x, 15);
    ASSERT_EQ(barsManager._info.second._count, 2);
    ASSERT_EQ(barsManager._info.second._offsetFromCurrent, 4);
    
    ASSERT_EQ(p._maReason, "_p._maIsOn==false(ratio < _p._maVolMultToExit) -- _p._maVolMultToExit=0.3,_p._maLastSignalOnBarIndex=4,_p._maVolMultToExitInterval=3,onIntervalCount=2,exitIntervalVolume=28,onIntervalVolume=100,ratio=0.28,bar=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Unknown,0,00000000-00:00:00.000000,,0,0,0,0,0,,");
    
    barsManager.clear();
    
    bars.resize(11);
    bars[9]._instrument = InstrHelper::get6CU2();
    
    barsManager._avgVol1.set(28*3);
    barsManager._avgVol2.set(100*3);
    
    p._maIsOn = true;
    impl.processSignal(bars, barsManager);
    ASSERT_TRUE(impl.isEnabled());
    ASSERT_TRUE(!impl.isSignalOn());
    ASSERT_EQ(p._maLastSignalOnBarIndex, 4);
    
    ASSERT_EQ(barsManager._count, 2);
    ASSERT_EQ(barsManager._info.first._x, 15);
    ASSERT_EQ(barsManager._info.first._count, 3);
    ASSERT_EQ(barsManager._info.first._offsetFromCurrent, 1);
    ASSERT_EQ(barsManager._info.second._x, 15);
    ASSERT_EQ(barsManager._info.second._count, 3);
    ASSERT_EQ(barsManager._info.second._offsetFromCurrent, 4);
    
    ASSERT_EQ(p._maReason, "_p._maIsOn==false(ratio < _p._maVolMultToExit) -- _p._maVolMultToExit=0.3,_p._maLastSignalOnBarIndex=4,_p._maVolMultToExitInterval=3,onIntervalCount=3,exitIntervalVolume=28,onIntervalVolume=100,ratio=0.28,bar=0,,,Unknown,InV,InV,InV,InV,InV,0,0,Unknown,0,00000000-00:00:00.000000,,0,0,0,0,0,,");

}

} // common_trade
} // tw

