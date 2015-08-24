#include <tw/common_trade/mkt_vels_manager.h>
#include "../unit_test_price_lib/instr_helper.h"
#include "tw/generated/instrument.h"
#include "tw/price/quote.h"

#include <gtest/gtest.h>

#include <vector>

typedef tw::price::Ticks TPrice;
typedef tw::price::Size TSize;

static const double PRICE_EPSILON = 0.005;

class TestRouter2 {
public:
    typedef tw::common_trade::MktVelsManagerImpl<TestRouter2> TImpl;        
    
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


TestRouter2::TTimerClientInfos TestRouter2::_timerClientInfos;
uint32_t TestRouter2::_msecFromMidnight = 0;

static void clearProcessors() {
    // Clear router
    //
    TestRouter2::clear();
}

TEST(CommonTradeLibTestSuit, mkt_vels_impl_test_cycle_timeout)
{   
    TestRouter2::TImpl impl;
    
    clearProcessors();
    
    // Test before minute bounder - 61 min 59 sec and 790 msec after midnight
    //
    TestRouter2::_msecFromMidnight = (61 * 60 + 59)*1000 + 790;
    EXPECT_EQ(impl.calcCycleTimeout(), 110);
    
    // Test at minute bounder - 62 min after midnight
    //
    TestRouter2::_msecFromMidnight = (62 * 60)*1000;
    EXPECT_EQ(impl.calcCycleTimeout(), 100);
    
    // Test after minute bounder - 62 min 1 msec after midnight
    //
    TestRouter2::_msecFromMidnight = 62 * 60 * 1000 + 1;
    EXPECT_EQ(impl.calcCycleTimeout(), 199);
}


TEST(CommonTradeLibTestSuit, mkt_vels_impl_test_tick_velocity)
{
    TestRouter2::TImpl impl;
    TestRouter2::TImpl::TWbos wbos;
    TestRouter2::TImpl::TResult r;
    
    clearProcessors();
    
    TestRouter2::_msecFromMidnight = (62 * 60 + 1)*1000 + 10;
    
    // Init quotes
    //
    tw::price::QuoteStore::TQuote quoteNQM2;
    tw::price::QuoteStore::TQuote quote6CU2;
    
    tw::instr::InstrumentPtr instrNQM2 = InstrHelper::getNQM2();
    tw::instr::InstrumentPtr instr6CU2 = InstrHelper::get6CU2();
    
    quoteNQM2.setInstrument(instrNQM2);
    quote6CU2.setInstrument(instr6CU2);
    
    wbos = impl.getWBos(instrNQM2->_keyId);
    ASSERT_EQ(wbos.size(), 0UL);
    
    wbos = impl.getWBos(instr6CU2->_keyId);
    ASSERT_EQ(wbos.size(), 0UL);
    
    ASSERT_EQ(TestRouter2::_timerClientInfos.size(), 0UL);    
    
    // Subscribe to NQM2
    //
    ASSERT_TRUE(impl.subscribe(instrNQM2->_keyId));
    
    // Check that we registered for timeout
    //
    ASSERT_EQ(TestRouter2::_timerClientInfos.size(), 1UL);
    ASSERT_EQ(TestRouter2::_timerClientInfos.back()._msecs, 100);
    
    // Test max queue size
    //
    
    // Set max queue size to 2 entries
    //
    impl.setMaxQueueSize(2);
    
    quoteNQM2.setBid(TPrice(100), TSize(10), 0, 1);
    quoteNQM2.setAsk(TPrice(101), TSize(10), 0, 1);
    impl.onQuote(quoteNQM2);    
    impl.onTimeout(TestRouter2::_timerClientInfos.back()._timerId);
    
    wbos = impl.getWBos(instrNQM2->_keyId);
    ASSERT_EQ(wbos.size(), 1UL);
    ASSERT_NEAR(wbos.front(), 100.5, PRICE_EPSILON);
    
    
    // Test boundary case of 1 entry
    //
    r = impl.getVelDelta(instrNQM2->_keyId, 100, 0);
    ASSERT_TRUE(!r.first);
    ASSERT_NEAR(r.second, 0.0, PRICE_EPSILON);
    
    quoteNQM2.setBid(TPrice(101), TSize(10), 0, 1);
    quoteNQM2.setAsk(TPrice(102), TSize(10), 0, 1);
    impl.onQuote(quoteNQM2);    
    impl.onTimeout(TestRouter2::_timerClientInfos.back()._timerId);
    
    wbos = impl.getWBos(instrNQM2->_keyId);
    ASSERT_EQ(wbos.size(), 2UL);
    ASSERT_NEAR(wbos.front(), 101.5, PRICE_EPSILON);
    
    
    quoteNQM2.setBid(TPrice(102), TSize(10), 0, 1);
    quoteNQM2.setAsk(TPrice(103), TSize(10), 0, 1);
    impl.onQuote(quoteNQM2);    
    impl.onTimeout(TestRouter2::_timerClientInfos.back()._timerId);
    
    wbos = impl.getWBos(instrNQM2->_keyId);
    ASSERT_EQ(wbos.size(), 2UL);
    ASSERT_NEAR(wbos.back(), 101.5, PRICE_EPSILON);
    ASSERT_NEAR(wbos.front(), 102.5, PRICE_EPSILON);
    
    
    // Set max queue size to 12 entries
    //
    impl.setMaxQueueSize(12);
    
    quoteNQM2.setBid(TPrice(100), TSize(10), 0, 1);
    quoteNQM2.setAsk(TPrice(101), TSize(10), 0, 1);
    impl.onQuote(quoteNQM2);    
    impl.onTimeout(TestRouter2::_timerClientInfos.back()._timerId);
    
    wbos = impl.getWBos(instrNQM2->_keyId);
    ASSERT_EQ(wbos.size(), 3UL);
    ASSERT_NEAR(wbos.front(), 100.5, PRICE_EPSILON);
    
    quoteNQM2.setBid(TPrice(99), TSize(10), 0, 1);
    quoteNQM2.setAsk(TPrice(100), TSize(10), 0, 1);
    impl.onQuote(quoteNQM2);    
    impl.onTimeout(TestRouter2::_timerClientInfos.back()._timerId);
    
    wbos = impl.getWBos(instrNQM2->_keyId);
    ASSERT_EQ(wbos.size(), 4UL);
    ASSERT_NEAR(wbos.front(), 99.5, PRICE_EPSILON);
    
    quoteNQM2.setBid(TPrice(98), TSize(10), 0, 1);
    quoteNQM2.setAsk(TPrice(99), TSize(10), 0, 1);
    impl.onQuote(quoteNQM2);    
    impl.onTimeout(TestRouter2::_timerClientInfos.back()._timerId);
    
    wbos = impl.getWBos(instrNQM2->_keyId);
    ASSERT_EQ(wbos.size(), 5UL);
    ASSERT_NEAR(wbos.front(), 98.5, PRICE_EPSILON);
    
    quoteNQM2.setBid(TPrice(100), TSize(10), 0, 1);
    quoteNQM2.setAsk(TPrice(101), TSize(10), 0, 1);
    impl.onQuote(quoteNQM2);    
    impl.onTimeout(TestRouter2::_timerClientInfos.back()._timerId);
    
    wbos = impl.getWBos(instrNQM2->_keyId);
    ASSERT_EQ(wbos.size(), 6UL);
    ASSERT_NEAR(wbos.front(), 100.5, PRICE_EPSILON);
    
    quoteNQM2.setBid(TPrice(102), TSize(10), 0, 1);
    quoteNQM2.setAsk(TPrice(103), TSize(10), 0, 1);
    impl.onQuote(quoteNQM2);    
    impl.onTimeout(TestRouter2::_timerClientInfos.back()._timerId);
    
    wbos = impl.getWBos(instrNQM2->_keyId);
    ASSERT_EQ(wbos.size(), 7UL);
    ASSERT_NEAR(wbos.front(), 102.5, PRICE_EPSILON);
    
    
    quoteNQM2.setBid(TPrice(103), TSize(10), 0, 1);
    quoteNQM2.setAsk(TPrice(104), TSize(10), 0, 1);
    impl.onQuote(quoteNQM2);    
    impl.onTimeout(TestRouter2::_timerClientInfos.back()._timerId);
    
    wbos = impl.getWBos(instrNQM2->_keyId);
    ASSERT_EQ(wbos.size(), 8UL);
    ASSERT_NEAR(wbos.front(), 103.5, PRICE_EPSILON);
    
    quoteNQM2.setBid(TPrice(104), TSize(10), 0, 1);
    quoteNQM2.setAsk(TPrice(105), TSize(10), 0, 1);
    impl.onQuote(quoteNQM2);    
    impl.onTimeout(TestRouter2::_timerClientInfos.back()._timerId);
    
    wbos = impl.getWBos(instrNQM2->_keyId);
    ASSERT_EQ(wbos.size(), 9UL);
    ASSERT_NEAR(wbos.front(), 104.5, PRICE_EPSILON);
    
    
    quoteNQM2.setBid(TPrice(104), TSize(10), 0, 1);
    quoteNQM2.setAsk(TPrice(105), TSize(10), 0, 1);
    impl.onQuote(quoteNQM2);    
    impl.onTimeout(TestRouter2::_timerClientInfos.back()._timerId);
    
    wbos = impl.getWBos(instrNQM2->_keyId);
    ASSERT_EQ(wbos.size(), 10UL);
    ASSERT_NEAR(wbos.front(), 104.5, PRICE_EPSILON);
    
    
    quoteNQM2.setBid(TPrice(103), TSize(10), 0, 1);
    quoteNQM2.setAsk(TPrice(104), TSize(10), 0, 1);
    impl.onQuote(quoteNQM2);    
    impl.onTimeout(TestRouter2::_timerClientInfos.back()._timerId);
    
    wbos = impl.getWBos(instrNQM2->_keyId);
    ASSERT_EQ(wbos.size(), 11UL);
    ASSERT_NEAR(wbos.front(), 103.5, PRICE_EPSILON);
    
    quoteNQM2.setBid(TPrice(106), TSize(10), 0, 1);
    quoteNQM2.setAsk(TPrice(107), TSize(10), 0, 1);
    impl.onQuote(quoteNQM2);    
    impl.onTimeout(TestRouter2::_timerClientInfos.back()._timerId);
    
    wbos = impl.getWBos(instrNQM2->_keyId);
    ASSERT_EQ(wbos.size(), 12UL);
    ASSERT_NEAR(wbos.front(), 106.5, PRICE_EPSILON);
    
    // Test getVelDelta()
    //
    r = impl.getVelDelta(instrNQM2->_keyId, 100, 0);
    ASSERT_TRUE(r.first);
    ASSERT_NEAR(r.second, 3.0, PRICE_EPSILON);
    
    
    r = impl.getVelDelta(instrNQM2->_keyId, 500, 2);
    ASSERT_TRUE(r.first);
    ASSERT_NEAR(r.second, 6.0, PRICE_EPSILON);
    
    
    r = impl.getVelDelta(instrNQM2->_keyId, 300, 7);
    ASSERT_TRUE(r.first);
    ASSERT_NEAR(r.second, -4.0, PRICE_EPSILON);
    
    r = impl.getVelDelta(instrNQM2->_keyId, 500, 7);
    ASSERT_TRUE(r.first);
    ASSERT_NEAR(r.second, -3.0, PRICE_EPSILON);
    
    
    r = impl.getVelDelta(instrNQM2->_keyId, 100, 11);
    ASSERT_TRUE(!r.first);
    ASSERT_NEAR(r.second, 0.0, PRICE_EPSILON);
    
    r = impl.getVelDelta(instrNQM2->_keyId, 100, 12);
    ASSERT_TRUE(!r.first);
    ASSERT_NEAR(r.second, 0.0, PRICE_EPSILON);
    
    
    r = impl.getVelDelta(instrNQM2->_keyId, 2000, 0);
    ASSERT_TRUE(r.first);
    ASSERT_NEAR(r.second, 5.0, PRICE_EPSILON);
    
    
    // Test getAvgVel()
    //
    ASSERT_NEAR(impl.getAvgVel(instrNQM2->_keyId, 800, 100, 1), 0.571429, PRICE_EPSILON);    
    ASSERT_NEAR(impl.getAvgVel(instrNQM2->_keyId, 900, 100, 1), 0.375, PRICE_EPSILON);
    ASSERT_NEAR(impl.getAvgVel(instrNQM2->_keyId, 20000, 200, 1), 0.4, PRICE_EPSILON);
    
    // Add another subscription
    //
    
    // Subscribe to NQM2
    //
    ASSERT_TRUE(impl.subscribe(instr6CU2->_keyId));
    
    quoteNQM2.setBid(TPrice(102), TSize(10), 0, 1);
    quoteNQM2.setAsk(TPrice(103), TSize(10), 0, 1);
    impl.onQuote(quoteNQM2);
    
    quote6CU2.setBid(TPrice(202), TSize(10), 0, 1);
    quote6CU2.setAsk(TPrice(203), TSize(10), 0, 1);
    impl.onQuote(quote6CU2);
    
    impl.onTimeout(TestRouter2::_timerClientInfos.back()._timerId);
    
    wbos = impl.getWBos(instrNQM2->_keyId);
    ASSERT_EQ(wbos.size(), 12UL);
    ASSERT_NEAR(wbos.front(), 102.5, PRICE_EPSILON);
    
    wbos = impl.getWBos(instr6CU2->_keyId);
    ASSERT_EQ(wbos.size(), 1UL);
    ASSERT_NEAR(wbos.front(), 202.5, PRICE_EPSILON);
    
}

