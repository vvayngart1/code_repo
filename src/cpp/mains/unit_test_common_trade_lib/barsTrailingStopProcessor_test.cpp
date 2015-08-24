#include <vector>

#include <tw/common_strat/defs.h>
#include <tw/generated/common_trade_blocks.h>
#include <tw/common_trade/barsTrailingStopProcessor.h>
#include <tw/common_trade/bars_manager.h>
#include <tw/price/defs.h>

#include "../unit_test_price_lib/instr_helper.h"

#include <gtest/gtest.h>

namespace tw {
namespace common_trade {
    
typedef tw::price::Ticks TPrice;
typedef tw::price::Size TSize;

typedef tw::common_trade::TBar TBar;
typedef tw::common_trade::TBars TBars;

class TestRouter {
public:
    typedef tw::common_trade::BarsManagerImpl<TestRouter> TImpl;        
    
    static void clear() {
        _timerClientInfos.clear();
    }
    
public:    
    static bool registerTimerClient(TImpl* impl, const uint32_t msecs, const bool once, tw::common::TTimerId& id) {
        TimerClientInfo timerClientInfo;
        timerClientInfo._impl = impl;
        timerClientInfo._msecs = msecs;
        timerClientInfo._once = once;
        id = timerClientInfo._timerId = _timerClientInfos.size() + 1;
        _timerClientInfos.push_back(timerClientInfo);
        
        return true;
    }
    
    static uint32_t getMSecFromMidnight() {
        return _msecFromMidnight;
    }
    
    static void setDbSource(const std::string& source) {
    }
    
    static bool outputToDb() {
        return true;
    }
    
    static bool readBars(uint32_t x, uint32_t barLength, uint32_t lookbackMinutes, TBars& bars) {
        return true;
    }
    
    static bool processPatterns() {
        return true;
    }

    static bool isValid() {
        return true;
    }
    
    static bool canPersistToDb() {
        return true;
    }
    
    template <typename TItemType>
    static bool persist(const TItemType& v) {
        return true;
    }

public:        
    struct TimerClientInfo {
        TimerClientInfo() {
            clear();
        }
        
        void clear() {
            _impl = NULL;
            _msecs = 0;
            _once = false;
            _timerId = tw::common::TTimerId();
        }
        
        TImpl* _impl;
        uint32_t _msecs;
        bool _once;
        tw::common::TTimerId _timerId;
    };
    
    typedef std::vector<TimerClientInfo> TTimerClientInfos;
        
    static TTimerClientInfos _timerClientInfos;
    static uint32_t _msecFromMidnight;
};

typedef BarsTrailingStopProcessor TImpl;

TBars getBarsForBuy(size_t count) {
    TestRouter::TImpl barsManager;
    
    tw::instr::InstrumentPtr instrNQM2 = InstrHelper::getNQM2();
    
    uint32_t seqNum = 0;
    tw::price::QuoteStore::TQuote quoteNQM2;
    quoteNQM2.setInstrument(instrNQM2);
    
    barsManager.subscribe(instrNQM2->_keyId);
    
    // Set bar 1: H=102 L=101 O=101 C=102
    //
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(101), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(102), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    if ( 1 == count )
        return barsManager.getBars(instrNQM2->_keyId);
    
    // Set bar 2: H=108 L=104 O=105 C=106
    //
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(105), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(104), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(108), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(106), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    if ( 2 == count )
        return barsManager.getBars(instrNQM2->_keyId);
    
    // Set bar 3: H=107 L=105 O=106 C=106
    //
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(106), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(105), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(107), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(106), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    if ( 3 == count )
        return barsManager.getBars(instrNQM2->_keyId);
    
    // Set bar 4: H=109 L=104 O=105 C=107
    //
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(105), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(104), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(109), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(107), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    if ( 4 == count )
        return barsManager.getBars(instrNQM2->_keyId);
    
    // Set bar 5: H=114 L=107 O=109 C=112
    //
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(109), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(107), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(114), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(112), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    if ( 5 == count )
        return barsManager.getBars(instrNQM2->_keyId);
    
    // Set bar 6: H=112 L=108 O=110 C=111
    //
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(110), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(108), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(112), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(111), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    return barsManager.getBars(instrNQM2->_keyId);
    
}

TBars getBarsForSell(size_t count) {
    TestRouter::TImpl barsManager;
    
    tw::instr::InstrumentPtr instrNQM2 = InstrHelper::getNQM2();
    
    uint32_t seqNum = 0;
    tw::price::QuoteStore::TQuote quoteNQM2;
    quoteNQM2.setInstrument(instrNQM2);
    
    barsManager.subscribe(instrNQM2->_keyId);
    
    // Set bar 1: H=101 L=100 O=101 C=100
    //
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(101), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(100), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    if ( 1 == count )
        return barsManager.getBars(instrNQM2->_keyId);
    
    // Set bar 2: H=98 L=94 O=97 C=96
    //
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(97), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(98), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(94), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(96), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    if ( 2 == count )
        return barsManager.getBars(instrNQM2->_keyId);
    
    // Set bar 3: H=97 L=95 O=96 C=96
    //
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(96), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(97), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(95), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(96), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    if ( 3 == count )
        return barsManager.getBars(instrNQM2->_keyId);
    
    // Set bar 4: H=98 L=93 O=97 C=95
    //
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(97), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(98), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(93), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(95), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    if ( 4 == count )
        return barsManager.getBars(instrNQM2->_keyId);
    
    // Set bar 5: H=95 L=88 O=93 C=90
    //
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(93), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(95), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(88), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(90), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    if ( 5 == count )
        return barsManager.getBars(instrNQM2->_keyId);
    
    // Set bar 6: H=94 L=90 O=92 C=91
    //
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(92), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(94), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(90), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(91), TSize(1));
    barsManager.onQuote(quoteNQM2);
    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    return barsManager.getBars(instrNQM2->_keyId);
}

TEST(CommonTradeLibTestSuit, barsTrailingStopProcessor_test_isStopSlideRecalculated_buy)
{
    BarsTrailingStopLossParamsWire params;
    std::string reason;
    TBars bars;
    
    TImpl impl(params);
    BarsTrailingStopLossParamsWire& p = impl.getParams();
    
    p._barsCountToStart = -1;
    
    ASSERT_TRUE(!impl.isEnabled());
    
    p._barsCountToStart = 1;
    p._barTrailingStopOffsetTicks.set(1);    
    
    ASSERT_TRUE(impl.isEnabled());
    
    FillInfo info;    
    info._fill._side = tw::channel_or::eOrderSide::kBuy;
    info._fill._price.set(100);
    info._barIndex2 = 1;
    
    // Bug testing
    //
    p._barsCountToStart = 0;
    bars = getBarsForSell(1);
    ASSERT_TRUE(!impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks());
    
    bars = getBarsForBuy(1);
    bars[0]._close.set(99);
    ASSERT_TRUE(!impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks());
    
    bars[0]._close.set(102);
    ASSERT_TRUE(impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(100));
    
    info._stop.clear();
    p._slideBehindBetterBar = true;
    bars = getBarsForSell(1);
    ASSERT_TRUE(!impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks());
    
    bars = getBarsForBuy(1);
    bars[0]._close.set(99);
    ASSERT_TRUE(!impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks());
    
    bars[0]._close.set(102);
    ASSERT_TRUE(impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(100));
    
    // Normal previous testing
    //
    p._slideBehindBetterBar = false;
    p._barsCountToStart = 1;
    info._stop.set(95);
    bars = getBarsForBuy(1);
    ASSERT_TRUE(!impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(95));
    
    bars = getBarsForBuy(2);
    ASSERT_TRUE(impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(103));
    
    p._slideBehindBetterBar = true;
    
    bars = getBarsForBuy(3);
    ASSERT_TRUE(impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(105));
    info._stop.set(104);
            
    p._slideBehindBetterBar = false;
    
    bars = getBarsForBuy(3);
    ASSERT_TRUE(impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(105));
    
    // Test new eTransitionBarSlideMode
    //
    info._stop.set(102);
    
    p._transitionBarSlideMode = eTransitionBarSlideMode::kOff;
    ASSERT_TRUE(!impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(102));
    
    p._transitionBarSlideMode = eTransitionBarSlideMode::kBar;
    ASSERT_TRUE(impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(104));
    
    p._transitionBarSlideMode = eTransitionBarSlideMode::kClose;
    ASSERT_TRUE(impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(105));
    
    bars = getBarsForBuy(4);
    ASSERT_TRUE(!impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(105));
    
    bars = getBarsForBuy(5);
    ASSERT_TRUE(impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(106));
    
    bars = getBarsForBuy(6);
    ASSERT_TRUE(!impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(106));
    
}

TEST(CommonTradeLibTestSuit, barsTrailingStopProcessor_test_isStopSlideRecalculated_first_bar_buy)
{
    BarsTrailingStopLossParamsWire params;
    std::string reason;
    TBars bars;
    
    TImpl impl(params);
    BarsTrailingStopLossParamsWire& p = impl.getParams();
    
    p._barsCountToStart = -1;
    
    ASSERT_TRUE(!impl.isEnabled());
    
    p._barsCountToStart = 0;
    p._barTrailingStopOffsetTicks.set(1);    
    
    ASSERT_TRUE(impl.isEnabled());
    
    FillInfo info;    
    info._fill._side = tw::channel_or::eOrderSide::kBuy;
    info._fill._price.set(100);
    info._barIndex2 = 1;
    info._stop.set(95);
    
    p._slideBehindBetterBar = true;
    
    bars = getBarsForBuy(1);
    ASSERT_TRUE(impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(100));    
    
    p._slideBehindBetterBar = false;
    info._stop.set(95);
    
    bars = getBarsForBuy(1);
    ASSERT_TRUE(impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(100));
    
    bars = getBarsForBuy(2);
    ASSERT_TRUE(impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(103));
    
    p._slideBehindBetterBar = true;
    
    bars = getBarsForBuy(3);
    ASSERT_TRUE(impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(105));
    info._stop.set(104);
            
    p._slideBehindBetterBar = false;
    
    bars = getBarsForBuy(3);
    ASSERT_TRUE(impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(105));        
    
    bars = getBarsForBuy(4);
    ASSERT_TRUE(!impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(105));
    
    bars = getBarsForBuy(5);
    ASSERT_TRUE(impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(106));
    
    bars = getBarsForBuy(6);
    ASSERT_TRUE(!impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(106));
    
}

TEST(CommonTradeLibTestSuit, barsTrailingStopProcessor_test_isStopSlideRecalculated_sell)
{
    BarsTrailingStopLossParamsWire params;
    std::string reason;
    TBars bars;
    
    TImpl impl(params);
    BarsTrailingStopLossParamsWire& p = impl.getParams();
    
    p._barsCountToStart = -1;
    
    ASSERT_TRUE(!impl.isEnabled());
    
    p._barsCountToStart = 1;
    p._barTrailingStopOffsetTicks.set(1);
    
    ASSERT_TRUE(impl.isEnabled());
    
    FillInfo info;    
    info._fill._side = tw::channel_or::eOrderSide::kSell;
    info._fill._price.set(100);
    info._barIndex2 = 1;
    
    // Bug testing
    //
    p._barsCountToStart = 0;
    bars = getBarsForBuy(1);
    ASSERT_TRUE(!impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks());
    
    bars = getBarsForSell(1);
    bars[0]._close.set(101);
    ASSERT_TRUE(!impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks());
    
    bars[0]._close.set(98);
    ASSERT_TRUE(impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(102));
    
    info._stop.clear();
    p._slideBehindBetterBar = true;
    bars = getBarsForBuy(1);
    ASSERT_TRUE(!impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks());
    
    bars = getBarsForSell(1);
    bars[0]._close.set(101);
    ASSERT_TRUE(!impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks());
    
    bars[0]._close.set(98);
    ASSERT_TRUE(impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(102));
    
    
    // Normal previous testing
    //
    p._slideBehindBetterBar = false;
    p._barsCountToStart = 1;
    info._stop.set(105);
    
    bars = getBarsForSell(1);
    ASSERT_TRUE(!impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(105));
    
    bars = getBarsForSell(2);
    ASSERT_TRUE(impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(99));
    
    p._slideBehindBetterBar = true;
    
    bars = getBarsForSell(3);
    ASSERT_TRUE(impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(97));
    info._stop.set(98);
    
    p._slideBehindBetterBar = false;
    
    bars = getBarsForSell(3);
    ASSERT_TRUE(impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(97));
    
    // Test new eTransitionBarSlideMode
    //
    info._stop.set(100);
    
    p._transitionBarSlideMode = eTransitionBarSlideMode::kOff;
    ASSERT_TRUE(!impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(100));
    
    p._transitionBarSlideMode = eTransitionBarSlideMode::kBar;
    ASSERT_TRUE(impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(98));
    
    p._transitionBarSlideMode = eTransitionBarSlideMode::kClose;
    ASSERT_TRUE(impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(97));
                
    bars = getBarsForSell(4);
    ASSERT_TRUE(!impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(97));
    
    bars = getBarsForSell(5);
    ASSERT_TRUE(impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(96));
    
    bars = getBarsForSell(6);
    ASSERT_TRUE(!impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(96));
    
}

TEST(CommonTradeLibTestSuit, barsTrailingStopProcessor_test_isStopSlideRecalculated_first_bar_sell)
{
    BarsTrailingStopLossParamsWire params;
    std::string reason;
    TBars bars;
    
    TImpl impl(params);
    BarsTrailingStopLossParamsWire& p = impl.getParams();
    
    p._barsCountToStart = -1;
    
    ASSERT_TRUE(!impl.isEnabled());
    
    p._barsCountToStart = 0;
    p._barTrailingStopOffsetTicks.set(1);
    
    ASSERT_TRUE(impl.isEnabled());
    
    FillInfo info;    
    info._fill._side = tw::channel_or::eOrderSide::kSell;
    info._fill._price.set(100);
    info._barIndex2 = 1;
    info._stop.set(105);
    
    p._slideBehindBetterBar = true;
    
    bars = getBarsForSell(1);
    ASSERT_TRUE(impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(102));
    
    p._slideBehindBetterBar = false;
    info._stop.set(105);
    
    bars = getBarsForSell(1);
    ASSERT_TRUE(impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(102));
    
    bars = getBarsForSell(2);
    ASSERT_TRUE(impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(99));
    
    p._slideBehindBetterBar = true;
    
    bars = getBarsForSell(3);
    ASSERT_TRUE(impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(97));
    info._stop.set(98);
    
    p._slideBehindBetterBar = false;
    
    bars = getBarsForSell(3);
    ASSERT_TRUE(impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(97));
            
    bars = getBarsForSell(4);
    ASSERT_TRUE(!impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(97));
    
    bars = getBarsForSell(5);
    ASSERT_TRUE(impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(96));
    
    bars = getBarsForSell(6);
    ASSERT_TRUE(!impl.isStopSlideRecalculated(info, bars, reason));
    ASSERT_EQ(info._stop, tw::price::Ticks(96));
    
}


TEST(CommonTradeLibTestSuit, barsTrailingStopProcessor_test_isNumOfTicksInFavor)
{
    BarsTrailingStopLossParamsWire params;
    std::string reason;
    FillInfo info;
    
    TImpl impl(params);
    BarsTrailingStopLossParamsWire& p = impl.getParams();
    
    // Set quote
    //
    tw::price::QuoteStore::TQuote quote;
    quote.setInstrument(InstrHelper::get6CU2());


    ASSERT_TRUE(impl.isNumOfTicksInFavor(info, quote));

    p._barsCountToStart = 1;
    ASSERT_TRUE(impl.isNumOfTicksInFavor(info, quote));

    p._barTrailingCntToStartTicks.set(-1);
    ASSERT_TRUE(impl.isNumOfTicksInFavor(info, quote));
    
    p._barTrailingCntToStartTicks.set(0);
    ASSERT_TRUE(!impl.isNumOfTicksInFavor(info, quote));
    
    p._barTrailingCntToStartTicks.set(5);
    ASSERT_TRUE(!impl.isNumOfTicksInFavor(info, quote));
    
    quote.setTrade(TPrice(101), TSize(1));
    quote.setBid(TPrice(101), TSize(20), 0);
    quote.setAsk(TPrice(102), TSize(40), 0);
    
    ASSERT_TRUE(!impl.isNumOfTicksInFavor(info, quote));
    
    info._fill._side = tw::channel_or::eOrderSide::kBuy;
    info._fill._price.set(100);
    
    ASSERT_TRUE(!impl.isNumOfTicksInFavor(info, quote));
    
    quote.setBid(TPrice(105), TSize(20), 0);
    quote.setAsk(TPrice(106), TSize(40), 0);
    
    ASSERT_TRUE(impl.isNumOfTicksInFavor(info, quote));
    
    info._fill._side = tw::channel_or::eOrderSide::kSell;
    
    ASSERT_TRUE(!impl.isNumOfTicksInFavor(info, quote));
    
    quote.setBid(TPrice(94), TSize(20), 0);
    quote.setAsk(TPrice(95), TSize(40), 0);
    
    ASSERT_TRUE(impl.isNumOfTicksInFavor(info, quote));
    
}


} // common_trade
} // tw
