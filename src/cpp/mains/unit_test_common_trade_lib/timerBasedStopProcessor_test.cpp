#include <vector>

#include <tw/common_strat/defs.h>
#include <tw/generated/common_trade_blocks.h>
#include <tw/common_trade/timerBasedStopProcessor.h>
#include <tw/price/defs.h>

#include "../unit_test_price_lib/instr_helper.h"

#include <gtest/gtest.h>

namespace tw {
namespace common_trade {
    
const double EPSILON = 0.0001;

typedef tw::price::Ticks TPrice;
typedef tw::price::Size TSize;

class TestClient : public ITimerBasedStopProcessorClient {
public:
    virtual void processCycleTimer(bool isLBound) {
        _calls.push_back(isLBound);
    }
    
    std::vector<bool> _calls;
};

class TestRouter : public tw::common::Singleton<TestRouter> {
public:
    typedef tw::common_trade::TimerBasedStopProcessor<TestRouter> TImpl;
    
public:
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

TestRouter::TTimerClientInfos TestRouter::_timerClientInfos;
uint32_t TestRouter::_msecFromMidnight = 0;

void clearProcessors() {
    // Clear router
    //
    TestRouter::clear();
}

TimerStopLossParamsWire getParams1() {
    TimerStopLossParamsWire p;
    
    p._cycleLength = 60;
    p._cycleLBound = 15;
    p._cycleUBound = 45;
    
    return p;
}


TEST(CommonTradeLibTestSuit, timerBasedStopProcessor_test_calcStopSlide)
{
    TimerStopLossParamsWire params = getParams1();
    
    clearProcessors();
    
    TestClient client;    
    TestRouter::TImpl impl(params, client);
    TimerStopLossParamsWire& p = impl.getParams();
    
    p._cycleLength = 0;
    ASSERT_TRUE(!impl.isEnabled());
    
    p._cycleLength = 60000;
    ASSERT_TRUE(impl.isEnabled());

    p._stopLossCycleLBound.set(1);
    p._stopLossCycleUBound.set(2);
    
    // Test anchor mode
    //
    p._stopLossCycleMode = eStopLossCycleMode::kAnchor;    
    
    ASSERT_EQ(impl.calcStopSlide(0, true).get(), 1);
    ASSERT_EQ(impl.calcStopSlide(1, true).get(), 1);
    
    ASSERT_EQ(impl.calcStopSlide(0, false).get(), 2);
    ASSERT_EQ(impl.calcStopSlide(1, false).get(), 2);
    
    
    ASSERT_EQ(impl.calcStopSlide(2, true).get(), 1);
    ASSERT_EQ(impl.calcStopSlide(3, true).get(), 1);
    
    ASSERT_EQ(impl.calcStopSlide(2, false).get(), 2);
    ASSERT_EQ(impl.calcStopSlide(3, false).get(), 2);
    
    // Test float mode
    //
    p._stopLossCycleMode = eStopLossCycleMode::kFloat;    
    
    ASSERT_EQ(impl.calcStopSlide(0, true).get(), 2);
    ASSERT_EQ(impl.calcStopSlide(1, true).get(), 1);
    
    ASSERT_EQ(impl.calcStopSlide(0, false).get(), 2);
    ASSERT_EQ(impl.calcStopSlide(1, false).get(), 1);
    
    
    ASSERT_EQ(impl.calcStopSlide(2, true).get(), 2);
    ASSERT_EQ(impl.calcStopSlide(3, true).get(), 1);
    
    ASSERT_EQ(impl.calcStopSlide(2, false).get(), 2);
    ASSERT_EQ(impl.calcStopSlide(3, false).get(), 1);
    
}

TEST(CommonTradeLibTestSuit, timerBasedStopProcessor_test_calcCycleTimeouts)
{
    TimerStopLossParamsWire params = getParams1();
    clearProcessors();
    
    TestClient client;    
    TestRouter::TImpl impl(params, client);
    
    TestRouter::_msecFromMidnight = 0;
    ASSERT_EQ(impl.calcCycleTimeouts().first, 15000UL);
    ASSERT_EQ(impl.calcCycleTimeouts().second, 45000UL);
    
    TestRouter::_msecFromMidnight = 7000;
    ASSERT_EQ(impl.calcCycleTimeouts().first, 8000UL);
    ASSERT_EQ(impl.calcCycleTimeouts().second, 38000UL);
    
    TestRouter::_msecFromMidnight = 15000;
    ASSERT_EQ(impl.calcCycleTimeouts().first, 60000UL);
    ASSERT_EQ(impl.calcCycleTimeouts().second, 30000UL);
    
    TestRouter::_msecFromMidnight = 28000;
    ASSERT_EQ(impl.calcCycleTimeouts().first, 47000UL);
    ASSERT_EQ(impl.calcCycleTimeouts().second, 17000UL);
    
    TestRouter::_msecFromMidnight = 45000;
    ASSERT_EQ(impl.calcCycleTimeouts().first, 30000UL);
    ASSERT_EQ(impl.calcCycleTimeouts().second, 60000UL);
    
    TestRouter::_msecFromMidnight = 59000;
    ASSERT_EQ(impl.calcCycleTimeouts().first, 16000UL);
    ASSERT_EQ(impl.calcCycleTimeouts().second, 46000UL);
    
    
    
    TestRouter::_msecFromMidnight = 753*60 + 0;
    TestRouter::_msecFromMidnight *= 1000;
    ASSERT_EQ(impl.calcCycleTimeouts().first, 15000UL);
    ASSERT_EQ(impl.calcCycleTimeouts().second, 45000UL);
    
    TestRouter::_msecFromMidnight = 753*60 + 7;
    TestRouter::_msecFromMidnight *= 1000;
    ASSERT_EQ(impl.calcCycleTimeouts().first, 8000UL);
    ASSERT_EQ(impl.calcCycleTimeouts().second, 38000UL);
    
    TestRouter::_msecFromMidnight = 753*60 + 15;
    TestRouter::_msecFromMidnight *= 1000;
    ASSERT_EQ(impl.calcCycleTimeouts().first, 60000UL);
    ASSERT_EQ(impl.calcCycleTimeouts().second, 30000UL);
    
    TestRouter::_msecFromMidnight = 753*60 + 28;
    TestRouter::_msecFromMidnight *= 1000;
    ASSERT_EQ(impl.calcCycleTimeouts().first, 47000UL);
    ASSERT_EQ(impl.calcCycleTimeouts().second, 17000UL);
    
    TestRouter::_msecFromMidnight = 753*60 + 45;
    TestRouter::_msecFromMidnight *= 1000;
    ASSERT_EQ(impl.calcCycleTimeouts().first, 30000UL);
    ASSERT_EQ(impl.calcCycleTimeouts().second, 60000UL);
    
    TestRouter::_msecFromMidnight = 753*60 + 59;
    TestRouter::_msecFromMidnight *= 1000;
    ASSERT_EQ(impl.calcCycleTimeouts().first, 16000UL);
    ASSERT_EQ(impl.calcCycleTimeouts().second, 46000UL);
    
    
}


TEST(CommonTradeLibTestSuit, timerBasedStopProcessor_test_timeoutLogic)
{
    TimerStopLossParamsWire params = getParams1();
    clearProcessors();
    
    TestClient client;    
    
    // Test initial timeout registration
    //
    TestRouter::_msecFromMidnight = 7000;
    TestRouter::TImpl impl(params, client);
    TimerStopLossParamsWire& p = impl.getParams();
    
    ASSERT_EQ(TestRouter::_timerClientInfos.size(), 2UL);
    
    ASSERT_EQ(TestRouter::_timerClientInfos[0]._msecs, 8000UL);
    ASSERT_TRUE(TestRouter::_timerClientInfos[0]._once);
    ASSERT_EQ(TestRouter::_timerClientInfos[0]._timerId, 1UL);
    
    ASSERT_EQ(TestRouter::_timerClientInfos[1]._msecs, 38000UL);
    ASSERT_TRUE(TestRouter::_timerClientInfos[1]._once);
    ASSERT_EQ(TestRouter::_timerClientInfos[1]._timerId, 2UL);
    
    ASSERT_EQ(p._cycleLBoundTimerId, 1UL);
    ASSERT_EQ(p._cycleUBoundTimerId, 2UL);
    
    ASSERT_EQ(client._calls.size(), 0UL);
    
    // Test onTimeout() for LBound
    //
    TestRouter::_msecFromMidnight = 753*60 + 28;
    TestRouter::_msecFromMidnight *= 1000;
    ASSERT_TRUE(!impl.onTimeout(p._cycleLBoundTimerId));
    
    ASSERT_EQ(TestRouter::_timerClientInfos.size(), 3UL);
    
    ASSERT_EQ(TestRouter::_timerClientInfos[2]._msecs, 47000UL);
    ASSERT_TRUE(TestRouter::_timerClientInfos[2]._once);
    ASSERT_EQ(TestRouter::_timerClientInfos[2]._timerId, 3UL);
    
    ASSERT_EQ(p._cycleLBoundTimerId, 3UL);
    ASSERT_EQ(p._cycleUBoundTimerId, 2UL);
    
    ASSERT_EQ(client._calls.size(), 1UL);
    ASSERT_TRUE(client._calls[0]);
    
    
    // Test onTimeout() for LBound
    //
    TestRouter::_msecFromMidnight = 753*60 + 59;
    TestRouter::_msecFromMidnight *= 1000;
    ASSERT_TRUE(!impl.onTimeout(p._cycleUBoundTimerId));
    
    ASSERT_EQ(TestRouter::_timerClientInfos.size(), 4UL);
    
    ASSERT_EQ(TestRouter::_timerClientInfos[3]._msecs, 46000UL);
    ASSERT_TRUE(TestRouter::_timerClientInfos[3]._once);
    ASSERT_EQ(TestRouter::_timerClientInfos[3]._timerId, 4UL);
    
    ASSERT_EQ(p._cycleLBoundTimerId, 3UL);
    ASSERT_EQ(p._cycleUBoundTimerId, 4UL);
    
    ASSERT_EQ(client._calls.size(), 2UL);
    ASSERT_TRUE(client._calls[0]);
    ASSERT_TRUE(!client._calls[1]);                
}


TEST(CanaryTestSuit, canary_impl_test_take_profit_seq_calcs)
{
    TimerStopLossParamsWire params = getParams1();
    clearProcessors();
    
    TestClient client;    
    TestRouter::TImpl impl(params, client);    
    TimerStopLossParamsWire& p = impl.getParams();
    
    tw::channel_or::eOrderSide side;
    tw::price::Ticks price;
    FillInfo info;
    std::string reason;
    
    tw::price::Ticks stopLossPayupTicks;
    tw::price::QuoteStore::TQuote quote;
    uint32_t seqNum = 0;
    
    // Set quote
    //
    quote.setInstrument(InstrHelper::get6CU2());
    quote._seqNum = ++seqNum;
    quote.setBid(TPrice(110), TSize(20), 0, 1);
    quote.setAsk(TPrice(111), TSize(30), 0, 1);
    
    // Test buy side take profit
    //
    info._fill._side = tw::channel_or::eOrderSide::kBuy;    
    
    // isLBound = true
    //
    p._takeProfitCycleLBound.set(2);
    p._takeProfitCycleUBound.set(0);
    stopLossPayupTicks.set(10);    
    
    info._fill._price.set(112);
    ASSERT_TRUE(!impl.isTakeProfitTriggered(info, quote, stopLossPayupTicks, true, price, reason));
    
    info._fill._price.set(110);
    ASSERT_TRUE(!impl.isTakeProfitTriggered(info, quote, stopLossPayupTicks, true, price, reason));
    
    info._fill._price.set(109);
    ASSERT_TRUE(!impl.isTakeProfitTriggered(info, quote, stopLossPayupTicks, true, price, reason));
    
    info._fill._price.set(108);
    ASSERT_TRUE(impl.isTakeProfitTriggered(info, quote, stopLossPayupTicks, true, price, reason));    
    ASSERT_EQ(price, tw::price::Ticks(100));
    
    stopLossPayupTicks.set(5);
    info._fill._price.set(106);
    ASSERT_TRUE(impl.isTakeProfitTriggered(info, quote, stopLossPayupTicks, true, price, reason));    
    ASSERT_EQ(price, tw::price::Ticks(105));
    
    p._takeProfitCycleLBound.set(0);
    info._fill._price.set(106);
    ASSERT_TRUE(!impl.isTakeProfitTriggered(info, quote, stopLossPayupTicks, true, price, reason));
    
    // isLBound = false
    //
    p._takeProfitCycleLBound.set(0);
    p._takeProfitCycleUBound.set(2);
    stopLossPayupTicks.set(10);
    
    info._fill._price.set(112);
    ASSERT_TRUE(!impl.isTakeProfitTriggered(info, quote, stopLossPayupTicks, false, price, reason));
    
    info._fill._price.set(110);
    ASSERT_TRUE(!impl.isTakeProfitTriggered(info, quote, stopLossPayupTicks, false, price, reason));
    
    info._fill._price.set(109);
    ASSERT_TRUE(!impl.isTakeProfitTriggered(info, quote, stopLossPayupTicks, false, price, reason));
    
    info._fill._price.set(108);
    ASSERT_TRUE(impl.isTakeProfitTriggered(info, quote, stopLossPayupTicks, false, price, reason));    
    ASSERT_EQ(price, tw::price::Ticks(100));
    
    stopLossPayupTicks.set(5);
    info._fill._price.set(106);
    ASSERT_TRUE(impl.isTakeProfitTriggered(info, quote, stopLossPayupTicks, false, price, reason));    
    ASSERT_EQ(price, tw::price::Ticks(105));
    
    p._takeProfitCycleUBound.set(0);
    info._fill._price.set(106);
    ASSERT_TRUE(!impl.isTakeProfitTriggered(info, quote, stopLossPayupTicks, false, price, reason));
    
        
    // Test sell side take profit
    //
    info._fill._side = tw::channel_or::eOrderSide::kSell;
    
    // isLBound = true
    //
    p._takeProfitCycleLBound.set(2);
    p._takeProfitCycleUBound.set(0);
    stopLossPayupTicks.set(10);    
    
    info._fill._price.set(109);
    ASSERT_TRUE(!impl.isTakeProfitTriggered(info, quote, stopLossPayupTicks, true, price, reason));
    
    info._fill._price.set(111);
    ASSERT_TRUE(!impl.isTakeProfitTriggered(info, quote, stopLossPayupTicks, true, price, reason));
    
    info._fill._price.set(112);
    ASSERT_TRUE(!impl.isTakeProfitTriggered(info, quote, stopLossPayupTicks, true, price, reason));
    
    info._fill._price.set(113);
    ASSERT_TRUE(impl.isTakeProfitTriggered(info, quote, stopLossPayupTicks, true, price, reason));    
    ASSERT_EQ(price, tw::price::Ticks(121));
    
    stopLossPayupTicks.set(5);
    info._fill._price.set(115);
    ASSERT_TRUE(impl.isTakeProfitTriggered(info, quote, stopLossPayupTicks, true, price, reason));    
    ASSERT_EQ(price, tw::price::Ticks(116));
    
    p._takeProfitCycleLBound.set(0);
    info._fill._price.set(115);
    ASSERT_TRUE(!impl.isTakeProfitTriggered(info, quote, stopLossPayupTicks, true, price, reason));
    
    // isLBound = false
    //
    p._takeProfitCycleLBound.set(0);
    p._takeProfitCycleUBound.set(2);
    stopLossPayupTicks.set(10);
    
    
    info._fill._price.set(109);
    ASSERT_TRUE(!impl.isTakeProfitTriggered(info, quote, stopLossPayupTicks, false, price, reason));
    
    info._fill._price.set(111);
    ASSERT_TRUE(!impl.isTakeProfitTriggered(info, quote, stopLossPayupTicks, false, price, reason));
    
    info._fill._price.set(112);
    ASSERT_TRUE(!impl.isTakeProfitTriggered(info, quote, stopLossPayupTicks, false, price, reason));
    
    info._fill._price.set(113);
    ASSERT_TRUE(impl.isTakeProfitTriggered(info, quote, stopLossPayupTicks, false, price, reason));    
    ASSERT_EQ(price, tw::price::Ticks(121));
    
    stopLossPayupTicks.set(5);
    info._fill._price.set(115);
    ASSERT_TRUE(impl.isTakeProfitTriggered(info, quote, stopLossPayupTicks, false, price, reason));    
    ASSERT_EQ(price, tw::price::Ticks(116));
    
    p._takeProfitCycleUBound.set(0);
    info._fill._price.set(115);
    ASSERT_TRUE(!impl.isTakeProfitTriggered(info, quote, stopLossPayupTicks, false, price, reason));

   
}


} // common_trade
} // tw
