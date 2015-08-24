#include <tw/common_trade/corr_manager.h>
#include "../unit_test_price_lib/instr_helper.h"
#include "tw/generated/instrument.h"

#include <gtest/gtest.h>

#include <vector>

namespace CorrBarsTest 
{
typedef tw::price::Ticks TPrice;
typedef tw::price::Size TSize;

class TestRouter {
public:
    typedef tw::common_trade::CorrManagerImpl<TestRouter> TImpl;        
    
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

void addTrade(tw::price::QuoteStore::TQuote& quote,
              uint32_t& seqNum,
              TestRouter::TImpl& barsManager,
              const TPrice& price,
              const TSize& size = TSize(1)) {
    quote._seqNum = ++seqNum;
    quote.setTrade(price, size);
    barsManager.onQuote(quote);
}

bool checkBar(TPrice high,
              TPrice low,
              TPrice open,
              TPrice close,
              TSize volume,
              uint32_t duration,
              uint32_t numOfTrades,
              bool formed,
              std::string displayName,
              tw::instr::eExchange exchange,
              TPrice lastValidPrice,
              const TestRouter::TImpl::TBar& bar)
{
    bool status = true;
    EXPECT_EQ(bar._high, high);
    if ( bar._high != high )
        status = false;
                
    EXPECT_EQ(bar._low, low);
    if ( bar._low != low )
        status = false;
    
    EXPECT_EQ(bar._open, open);
    if ( bar._open != open )
        status = false;
    
    EXPECT_EQ(bar._close, close);
    if ( bar._close != close )
        status = false;
    
    EXPECT_EQ(bar._volume, volume);
    if ( bar._volume != volume )
        status = false;
    
    EXPECT_EQ(bar._duration, duration);
    if ( bar._duration != duration )
        status = false;
    
    EXPECT_EQ(bar._numOfTrades, numOfTrades);
    if ( bar._numOfTrades != numOfTrades )
        status = false;
    
    EXPECT_EQ(bar._formed, formed);
    if ( bar._formed != formed )
        status = false;
    
    EXPECT_EQ(bar._displayName, displayName);
    if ( bar._displayName != displayName )
        status = false;
    
    EXPECT_EQ(bar._exchange, exchange);
    if ( bar._exchange != exchange )
        status = false;
    
    EXPECT_EQ(bar._lastValidPrice, lastValidPrice);
    if ( bar._lastValidPrice != lastValidPrice )
        status = false;
    
    return status;
    
}

TEST(CommonTradeLibTestSuit, corr_bars_impl_test_cycle_timeout)
{   
    TestRouter::TImpl barsManager;
    
    clearProcessors();
    
    barsManager.setCycleTime(1000);
    
    TestRouter::_msecFromMidnight = 3719010;
    EXPECT_EQ(barsManager.calcCycleTimeout(), 990);
    
    TestRouter::_msecFromMidnight = 3719210;
    EXPECT_EQ(barsManager.calcCycleTimeout(), 790);
    
    TestRouter::_msecFromMidnight = 3719000;
    EXPECT_EQ(barsManager.calcCycleTimeout(), 1000);
}

TEST(CommonTradeLibTestSuit, corr_bars_impl_test_normal_processing)
{
    TestRouter::TImpl barsManager;
    TestRouter::TImpl::TBars bars;
    TestRouter::TImpl::TBar bar;
    
    clearProcessors();
    
    uint32_t seqNum = 0;
    TestRouter::_msecFromMidnight = 3719010;
    
    // Init quotes
    //
    tw::price::QuoteStore::TQuote quoteNQM2;
    tw::price::QuoteStore::TQuote quote6CU2;
    
    tw::instr::InstrumentPtr instrNQM2 = InstrHelper::getNQM2();
    tw::instr::InstrumentPtr instr6CU2 = InstrHelper::get6CU2();
    
    quoteNQM2.setInstrument(instrNQM2);
    quote6CU2.setInstrument(instr6CU2);
    
    bars = barsManager.getBars(instrNQM2->_keyId);
    ASSERT_EQ(bars.size(), 0UL);
    
    bars = barsManager.getBars(instr6CU2->_keyId);
    ASSERT_EQ(bars.size(), 0UL);
    
    ASSERT_EQ(TestRouter::_timerClientInfos.size(), 0UL);    
    
    // Subscribe to NQM2
    //
    ASSERT_TRUE(barsManager.subscribe(instrNQM2->_keyId));
    
    // Check that we registered for timeout
    //
    ASSERT_EQ(TestRouter::_timerClientInfos.size(), 1UL);
    ASSERT_EQ(TestRouter::_timerClientInfos.back()._msecs, 990);
    
    bars = barsManager.getBars(instrNQM2->_keyId);
    ASSERT_EQ(bars.size(), 1UL);
    
    bar = bars.back();
    ASSERT_TRUE(checkBar(TPrice(),TPrice(),TPrice(),TPrice(),TSize(0),1000,0,false,"NQM2",tw::instr::eExchange::kCME,TPrice(),bar));
    
    ASSERT_TRUE(barsManager.subscribe(instr6CU2->_keyId));    
    
    // Subscribe to 6CU2
    //
    bars = barsManager.getBars(instr6CU2->_keyId);
    ASSERT_EQ(bars.size(), 1UL);
    
    bar = bars.back();
    ASSERT_TRUE(checkBar(TPrice(),TPrice(),TPrice(),TPrice(),TSize(0),1000,0,false,"6CU2",tw::instr::eExchange::kCME,TPrice(),bar));
    
    // Create an empty bar 1
    //
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);    

    // Add quote to NQM2
    //
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setBid(TPrice(5), TSize(10), 0, 1);
    quoteNQM2.setAsk(TPrice(20), TSize(20), 0, 1);
    barsManager.onQuote(quoteNQM2);
    
    ASSERT_EQ(TestRouter::_timerClientInfos.size(), 2UL);
    ASSERT_EQ(TestRouter::_timerClientInfos.back()._msecs, 990);
    
    bars = barsManager.getBars(instrNQM2->_keyId);
    ASSERT_EQ(bars.size(), 2UL);
    
    bar = bars.back();
    ASSERT_TRUE(checkBar(TPrice(),TPrice(),TPrice(),TPrice(),TSize(0),1000,0,false,"NQM2",tw::instr::eExchange::kCME,TPrice(),bar));
    
    
    bars = barsManager.getBars(instr6CU2->_keyId);
    ASSERT_EQ(bars.size(), 2UL);
    
    bar = bars.back();
    ASSERT_TRUE(checkBar(TPrice(),TPrice(),TPrice(),TPrice(),TSize(0),1000,0,false,"6CU2",tw::instr::eExchange::kCME,TPrice(),bar));
    
    
    // Add trade to NQM2
    //
    addTrade(quoteNQM2, seqNum, barsManager, TPrice(12), TSize(1));    
    
    bars = barsManager.getBars(instrNQM2->_keyId);
    ASSERT_EQ(bars.size(), 2UL);
    
    bar = bars.back();
    ASSERT_TRUE(checkBar(TPrice(12),TPrice(12),TPrice(12),TPrice(12),TSize(1),1000,1,false,"NQM2",tw::instr::eExchange::kCME,TPrice(12),bar));
    
    // Add another trade to NQM2
    //
    addTrade(quoteNQM2, seqNum, barsManager, TPrice(11), TSize(5));
    
    bars = barsManager.getBars(instrNQM2->_keyId);
    ASSERT_EQ(bars.size(), 2UL);
    
    bar = bars.back();
    ASSERT_TRUE(checkBar(TPrice(12),TPrice(11),TPrice(12),TPrice(11),TSize(6),1000,2,false,"NQM2",tw::instr::eExchange::kCME,TPrice(11),bar));
    
    
    // Add another trade to NQM2
    //
    addTrade(quoteNQM2, seqNum, barsManager, TPrice(13), TSize(10));
    
    bars = barsManager.getBars(instrNQM2->_keyId);
    ASSERT_EQ(bars.size(), 2UL);
    
    bar = bars.back();
    ASSERT_TRUE(checkBar(TPrice(13),TPrice(11),TPrice(12),TPrice(13),TSize(16),1000,3,false,"NQM2",tw::instr::eExchange::kCME,TPrice(13),bar));
    
    
    // Add trade to 6CU2
    //
    addTrade(quote6CU2, seqNum, barsManager, TPrice(10), TSize(1));
    
    bars = barsManager.getBars(instr6CU2->_keyId);
    ASSERT_EQ(bars.size(), 2UL);
    
    bar = bars.back();
    ASSERT_TRUE(checkBar(TPrice(10),TPrice(10),TPrice(10),TPrice(10),TSize(1),1000,1,false,"6CU2",tw::instr::eExchange::kCME,TPrice(10),bar));
    
    
    // Test timeout processing
    //
    TestRouter::_msecFromMidnight = 3719210;    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    ASSERT_EQ(TestRouter::_timerClientInfos.size(), 3UL);
    ASSERT_EQ(TestRouter::_timerClientInfos.back()._msecs, 790);    
    
    bars = barsManager.getBars(instrNQM2->_keyId);
    ASSERT_EQ(bars.size(), 3UL);    
    bar = bars.back();
    ASSERT_TRUE(checkBar(TPrice(),TPrice(),TPrice(),TPrice(),TSize(0),1000,0,false,"NQM2",tw::instr::eExchange::kCME,TPrice(13),bar));
    
    bars = barsManager.getBars(instr6CU2->_keyId);
    ASSERT_EQ(bars.size(), 3UL);    
    bar = bars.back();
    ASSERT_TRUE(checkBar(TPrice(),TPrice(),TPrice(),TPrice(),TSize(0),1000,0,false,"6CU2",tw::instr::eExchange::kCME,TPrice(10),bar));
    
    // Add enough trade/bars to calculate correlation
    //
    
    // Add trades to bar 3
    //
    addTrade(quoteNQM2, seqNum, barsManager, TPrice(6));
    addTrade(quote6CU2, seqNum, barsManager, TPrice(11));
    
    // Add trades to bar 4
    //
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    addTrade(quoteNQM2, seqNum, barsManager, TPrice(5));
    addTrade(quote6CU2, seqNum, barsManager, TPrice(8));
    
    // Add trades to bar 5
    //
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    addTrade(quoteNQM2, seqNum, barsManager, TPrice(12));
    addTrade(quote6CU2, seqNum, barsManager, TPrice(3));
    
    // Add trades to bar 6
    //
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    addTrade(quoteNQM2, seqNum, barsManager, TPrice(13));
    addTrade(quote6CU2, seqNum, barsManager, TPrice(12));
    
    // Add trades to bar 7
    //
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    addTrade(quoteNQM2, seqNum, barsManager, TPrice(12));
    addTrade(quote6CU2, seqNum, barsManager, TPrice(14));
    
    // Add trades to bar 8
    //
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    addTrade(quoteNQM2, seqNum, barsManager, TPrice(10));
    addTrade(quote6CU2, seqNum, barsManager, TPrice(15));
    
    // Add trades to bar 9
    //
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    addTrade(quoteNQM2, seqNum, barsManager, TPrice(15));
    addTrade(quote6CU2, seqNum, barsManager, TPrice(16));
    
    // Add trades to bar 10
    //
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    addTrade(quoteNQM2, seqNum, barsManager, TPrice(16));
    addTrade(quote6CU2, seqNum, barsManager, TPrice(11));
    
    // Add trades to bar 11
    //
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    addTrade(quoteNQM2, seqNum, barsManager, TPrice(15));
    addTrade(quote6CU2, seqNum, barsManager, TPrice(18));
    
    // Add trades to bar 12
    //
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    addTrade(quoteNQM2, seqNum, barsManager, TPrice(17));
    addTrade(quote6CU2, seqNum, barsManager, TPrice(17));
    
    // Add trades to bar 13
    //
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    addTrade(quoteNQM2, seqNum, barsManager, TPrice(16));
    addTrade(quote6CU2, seqNum, barsManager, TPrice(18));
    
    bars = barsManager.getBars(instrNQM2->_keyId);
    ASSERT_EQ(bars.size(), 13UL); 
    
    bars = barsManager.getBars(instr6CU2->_keyId);
    ASSERT_EQ(bars.size(), 13UL);
    
    // Test error checking
    //
    ASSERT_NEAR(barsManager.calcCorrelation(instrNQM2->_keyId, instr6CU2->_keyId, 15, 0, 0), 0.0, 0.00000001);
    ASSERT_NEAR(barsManager.calcCorrelation(instrNQM2->_keyId, instr6CU2->_keyId, 12, 12, 0), 0.0, 0.00000001);
    ASSERT_NEAR(barsManager.calcCorrelation(instrNQM2->_keyId, instr6CU2->_keyId, 10, 0, 11), 0.0, 0.00000001);
    ASSERT_NEAR(barsManager.calcCorrelation(instrNQM2->_keyId, instr6CU2->_keyId, 11, 0, 10), 0.0, 0.00000001);
    ASSERT_NEAR(barsManager.calcCorrelation(instrNQM2->_keyId, instr6CU2->_keyId, 10, 4, 0), 0.0, 0.00000001);
    ASSERT_NEAR(barsManager.calcCorrelation(instrNQM2->_keyId, instr6CU2->_keyId, 13, 0, 0), 0.0, 0.00000001);
    ASSERT_NEAR(barsManager.calcCorrelation(instrNQM2->_keyId, instr6CU2->_keyId, 5, 6, 4), 0.0, 0.00000001);
    ASSERT_NEAR(barsManager.calcCorrelation(instrNQM2->_keyId, instr6CU2->_keyId, 5, 9, 0), 0.0, 0.00000001);
    
    // Test correlations
    //
    ASSERT_NEAR(barsManager.calcCorrelation(instrNQM2->_keyId, instr6CU2->_keyId, 12, 0, 0), 0.507005448, 0.00000001);
    ASSERT_NEAR(barsManager.calcCorrelation(instrNQM2->_keyId, instr6CU2->_keyId, 12, 0, 1), 0.804705617, 0.00000001);
    ASSERT_NEAR(barsManager.calcCorrelation(instrNQM2->_keyId, instr6CU2->_keyId, 12, 0, 2), 0.710563815, 0.00000001);
    ASSERT_NEAR(barsManager.calcCorrelation(instr6CU2->_keyId, instrNQM2->_keyId, 12, 0, 1), 0.433359746, 0.00000001);
    ASSERT_NEAR(barsManager.calcCorrelation(instr6CU2->_keyId, instrNQM2->_keyId, 12, 0, 2), 0.437227188, 0.00000001);
    
    
    // Add empty bar 14
    //
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);    
    
    bars = barsManager.getBars(instrNQM2->_keyId);
    ASSERT_EQ(bars.size(), 14UL); 
    
    bars = barsManager.getBars(instr6CU2->_keyId);
    ASSERT_EQ(bars.size(), 14UL);
    
    // Test correlations
    //
    ASSERT_NEAR(barsManager.calcCorrelation(instrNQM2->_keyId, instr6CU2->_keyId, 12, 1, 0), 0.507005448, 0.00000001);
    ASSERT_NEAR(barsManager.calcCorrelation(instrNQM2->_keyId, instr6CU2->_keyId, 12, 1, 1), 0.804705617, 0.00000001);
    ASSERT_NEAR(barsManager.calcCorrelation(instrNQM2->_keyId, instr6CU2->_keyId, 12, 1, 2), 0.710563815, 0.00000001);
    ASSERT_NEAR(barsManager.calcCorrelation(instr6CU2->_keyId, instrNQM2->_keyId, 12, 1, 1), 0.433359746, 0.00000001);
    ASSERT_NEAR(barsManager.calcCorrelation(instr6CU2->_keyId, instrNQM2->_keyId, 12, 1, 2), 0.437227188, 0.00000001);
    
    ASSERT_NEAR(barsManager.calcCorrelation(instrNQM2->_keyId, instr6CU2->_keyId, 13, 0, 0), 0.546052163, 0.00000001);
    ASSERT_NEAR(barsManager.calcCorrelation(instrNQM2->_keyId, instr6CU2->_keyId, 13, 0, 0, true), 0.0, 0.00000001);
    ASSERT_NEAR(barsManager.calcCorrelation(instrNQM2->_keyId, instr6CU2->_keyId, 5, 6, 2), 0.97622104, 0.00000001);
    ASSERT_NEAR(barsManager.calcCorrelation(instrNQM2->_keyId, instr6CU2->_keyId, 5, 6, 3), 1.0, 0.00000001);
    ASSERT_NEAR(barsManager.calcCorrelation(instrNQM2->_keyId, instr6CU2->_keyId, 5, 8, 0), -0.056655007, 0.00000001);
    
}

}
