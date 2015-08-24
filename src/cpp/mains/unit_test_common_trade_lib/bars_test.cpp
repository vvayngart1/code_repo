#include <tw/common_trade/bars_manager.h>
#include "../unit_test_price_lib/instr_helper.h"
#include "tw/generated/instrument.h"
#include "tw/price/quote.h"

#include <gtest/gtest.h>

#include <vector>

const double EPSILON = 0.0001;

typedef tw::price::Ticks TPrice;
typedef tw::price::Size TSize;
typedef tw::common_trade::TBar TBar;
typedef tw::common_trade::TBars TBars;
typedef tw::common_trade::TBarPatterns TBarPatterns;

class TestRouter {
public:
    typedef tw::common_trade::BarsManagerImpl<TestRouter> TImpl;        
    
    static void clear() {
        _timerClientInfos.clear();
        _barPatterns.clear();
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
    // Bar storage related methods
    //
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
    
    static bool persist(const tw::common_trade::BarPattern& v) {
        _barPatterns.push_back(v);
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
    
public:
    typedef std::vector<TimerClientInfo> TTimerClientInfos;
    typedef std::vector<tw::common_trade::BarPattern> TBarPatterns;
        
    static TTimerClientInfos _timerClientInfos;
    static uint32_t _msecFromMidnight;
    static TBarPatterns _barPatterns;
    
};


TestRouter::TTimerClientInfos TestRouter::_timerClientInfos;
uint32_t TestRouter::_msecFromMidnight = 0;
TestRouter::TBarPatterns TestRouter::_barPatterns;

void clearProcessors() {
    // Clear router
    //
    TestRouter::clear();
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
              tw::common_trade::ePatternDir dir,
              const TBar& bar)
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
    
    EXPECT_EQ(bar._dir, dir);
    if ( bar._dir != dir )
        status = false;
    
    return status;
    
}

bool checkBarTrades(TPrice price,              
                    const TSize& volBid,
                    const TSize& volAsk,
                    const TSize& volBidTotal,
                    const TSize& volAskTotal,
                    const TBar& bar)
{
    bool status = true;
    
    std::tr1::unordered_map<tw::price::Ticks::type, tw::common_trade::PriceTradesInfo>::const_iterator iter = bar._trades.find(price.get());
    EXPECT_TRUE(iter != bar._trades.end());
    if ( iter == bar._trades.end() )
        return false;
    
    tw::common_trade::PriceTradesInfo x = iter->second;
    
    EXPECT_EQ(x._price, price);
    if ( x._price != price )
        status = false;
    
    EXPECT_EQ(x._volBid, volBid);
    if ( x._volBid != volBid )
        status = false;
    
    EXPECT_EQ(x._volAsk, volAsk);
    if ( x._volAsk != volAsk )
        status = false;
    
    EXPECT_EQ(bar._volBid, volBidTotal);
    if ( bar._volBid != volBidTotal )
        status = false;
    
    EXPECT_EQ(bar._volAsk, volAskTotal);
    if ( bar._volAsk != volAskTotal )
        status = false;
    
    return status;
    
}

bool checkBarPattern(TPrice high,
                     TPrice low,
                     TPrice open,
                     TPrice close,
                     TSize volume,
                     uint32_t numOfTrades,
                     tw::common_trade::ePatternDir dir,
                     uint32_t firstBarIndex,                    
                     uint32_t lastBarIndex,
                     uint32_t highBarIndex,                    
                     uint32_t lowBarIndex,
                     tw::common_trade::ePatternType type,
                     const tw::common_trade::BarPattern& bar)
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
    
    EXPECT_EQ(bar._numOfTrades, numOfTrades);
    if ( bar._numOfTrades != numOfTrades )
        status = false;
    
    EXPECT_EQ(bar._dir, dir);
    if ( bar._dir != dir )
        status = false;
    
    EXPECT_EQ(bar._firstBarIndex, firstBarIndex);
    if ( bar._firstBarIndex != firstBarIndex )
        status = false;
    
    EXPECT_EQ(bar._lastBarIndex, lastBarIndex);
    if ( bar._lastBarIndex != lastBarIndex )
        status = false;

    EXPECT_EQ(bar._highBarIndex, highBarIndex);
    if ( bar._highBarIndex != highBarIndex )
        status = false;
    
    EXPECT_EQ(bar._lowBarIndex, lowBarIndex);
    if ( bar._lowBarIndex != lowBarIndex )
        status = false;
    
    EXPECT_EQ(bar._type, type);
    if ( bar._type != type )
        status = false;
    
    return status;
    
}

void addTrade(tw::price::QuoteStore::TQuote& quote,
              const TPrice& price,
              const TSize& size,
              tw::price::Trade::eAggressorSide side) {
    ++quote._seqNum;
    quote.setTrade(price, size);
    quote._trade._aggressorSide = side;
}



void formBar(TPrice high,
             TPrice low,
             TPrice open,
             TPrice close,
             tw::price::QuoteStore::TQuote& quote,
             TestRouter::TImpl& barsManager,
             bool callTimeout = false,
             TSize tradeSize = TSize(1)) {
    if ( high.isValid() && low.isValid() && open.isValid() && close.isValid() ) {
        addTrade(quote, open, tradeSize, tw::price::Trade::kAggressorSideBuy);    
        barsManager.onQuote(quote);

        addTrade(quote, high, tradeSize, tw::price::Trade::kAggressorSideBuy);    
        barsManager.onQuote(quote);

        addTrade(quote, low, tradeSize, tw::price::Trade::kAggressorSideBuy);    
        barsManager.onQuote(quote);

        addTrade(quote, close, tradeSize, tw::price::Trade::kAggressorSideBuy);    
        barsManager.onQuote(quote);
    }
    
    if ( callTimeout )
        barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
}

TEST(CommonTradeLibTestSuit, bars_impl_test_cycle_timeout)
{   
    TestRouter::TImpl barsManager;
    
    clearProcessors();
    
    // Test before minute bounder - 61 minutes 59 seconds after midnight
    //
    TestRouter::_msecFromMidnight = 61 * 60 + 59;
    TestRouter::_msecFromMidnight *= 1000;
    EXPECT_EQ(barsManager.calcCycleTimeout(), 1000);
    
    // Test at minute bounder - 62 minutes after midnight
    //
    TestRouter::_msecFromMidnight = 62 * 60;
    TestRouter::_msecFromMidnight *= 1000;
    EXPECT_EQ(barsManager.calcCycleTimeout(), 60000);
    
    // Test after minute bounder - 62 minutes 1 second after midnight
    //
    TestRouter::_msecFromMidnight = 62 * 60 + 1;
    TestRouter::_msecFromMidnight *= 1000;
    EXPECT_EQ(barsManager.calcCycleTimeout(), 59000);
}

TEST(CommonTradeLibTestSuit, bars_impl_test_normal_processing)
{
    TestRouter::TImpl barsManager(60, -1, 4);
    TBars bars;
    TBar bar;
    
    clearProcessors();
    
    uint32_t seqNum = 0;
    TestRouter::_msecFromMidnight = 62 * 60 + 1;
    TestRouter::_msecFromMidnight *= 1000;
    
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
    ASSERT_EQ(TestRouter::_timerClientInfos.back()._msecs, 59000);
    
    bars = barsManager.getBars(instrNQM2->_keyId);
    ASSERT_EQ(bars.size(), 1UL);
    
    bar = bars.back();
    ASSERT_TRUE(checkBar(TPrice(),TPrice(),TPrice(),TPrice(),TSize(0),60,0,false,"NQM2",tw::instr::eExchange::kCME,tw::common_trade::ePatternDir::kUnknown,bar));
    
    ASSERT_TRUE(barsManager.subscribe(instr6CU2->_keyId));    
    
    // Subscribe to 6CU2
    //
    bars = barsManager.getBars(instr6CU2->_keyId);
    ASSERT_EQ(bars.size(), 1UL);
    
    bar = bars.back();
    ASSERT_TRUE(checkBar(TPrice(),TPrice(),TPrice(),TPrice(),TSize(0),60,0,false,"6CU2",tw::instr::eExchange::kCME,tw::common_trade::ePatternDir::kUnknown,bar));    
    
    // Add quote to NQM2
    //
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setBid(TPrice(100), TSize(10), 0, 1);
    quoteNQM2.setAsk(TPrice(101), TSize(20), 0, 1);
    barsManager.onQuote(quoteNQM2);
    
    ASSERT_EQ(TestRouter::_timerClientInfos.size(), 1UL);
    ASSERT_EQ(TestRouter::_timerClientInfos.back()._msecs, 59000);
    
    bars = barsManager.getBars(instrNQM2->_keyId);
    ASSERT_EQ(bars.size(), 1UL);
    
    bar = bars.back();
    ASSERT_TRUE(checkBar(TPrice(),TPrice(),TPrice(),TPrice(),TSize(0),60,0,false,"NQM2",tw::instr::eExchange::kCME,tw::common_trade::ePatternDir::kUnknown,bar));
    ASSERT_EQ(bar._range, TPrice());
    
    
    bars = barsManager.getBars(instr6CU2->_keyId);
    ASSERT_EQ(bars.size(), 1UL);
    
    bar = bars.back();
    ASSERT_TRUE(checkBar(TPrice(),TPrice(),TPrice(),TPrice(),TSize(0),60,0,false,"6CU2",tw::instr::eExchange::kCME,tw::common_trade::ePatternDir::kUnknown,bar));
    ASSERT_EQ(bar._range, TPrice());
    
    ASSERT_EQ(bar._atrNumOfPeriods, 0UL);
    ASSERT_NEAR(bar._atr, 0.0, EPSILON);
    
    // Add trade to NQM2
    //
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(101), TSize(1));
    quoteNQM2._trade._aggressorSide = tw::price::Trade::kAggressorSideBuy;
    barsManager.onQuote(quoteNQM2);
    
    ASSERT_EQ(TestRouter::_timerClientInfos.size(), 1UL);
    ASSERT_EQ(TestRouter::_timerClientInfos.back()._msecs, 59000);
    
    bars = barsManager.getBars(instrNQM2->_keyId);
    ASSERT_EQ(bars.size(), 1UL);
    
    bar = bars.back();
    ASSERT_TRUE(checkBar(TPrice(101),TPrice(101),TPrice(101),TPrice(101),TSize(1),60,1,false,"NQM2",tw::instr::eExchange::kCME,tw::common_trade::ePatternDir::kUnknown,bar));
    ASSERT_EQ(bar._range, TPrice(0));
    ASSERT_TRUE(checkBarTrades(TPrice(101), TSize(0), TSize(1), TSize(0), TSize(1), bar));
    ASSERT_EQ(bar._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bar._atr, 0.0, EPSILON);
    
    bars = barsManager.getBars(instr6CU2->_keyId);
    ASSERT_EQ(bars.size(), 1UL);
    
    bar = bars.back();
    ASSERT_TRUE(checkBar(TPrice(),TPrice(),TPrice(),TPrice(),TSize(0),60,0,false,"6CU2",tw::instr::eExchange::kCME,tw::common_trade::ePatternDir::kUnknown,bar));
    
    
    // Add another trade to NQM2
    //
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(102), TSize(3));
    quoteNQM2._trade._aggressorSide = tw::price::Trade::kAggressorSideSell;
    barsManager.onQuote(quoteNQM2);
    
    ASSERT_EQ(TestRouter::_timerClientInfos.size(), 1UL);
    ASSERT_EQ(TestRouter::_timerClientInfos.back()._msecs, 59000);
    
    bars = barsManager.getBars(instrNQM2->_keyId);
    ASSERT_EQ(bars.size(), 1UL);
    
    bar = bars.back();
    ASSERT_TRUE(checkBar(TPrice(102),TPrice(101),TPrice(101),TPrice(102),TSize(4),60,2,false,"NQM2",tw::instr::eExchange::kCME,tw::common_trade::ePatternDir::kUp,bar));
    ASSERT_EQ(bar._range, TPrice(1));
    ASSERT_TRUE(checkBarTrades(TPrice(101), TSize(0), TSize(1), TSize(3), TSize(1), bar));
    ASSERT_TRUE(checkBarTrades(TPrice(102), TSize(3), TSize(0), TSize(3), TSize(1), bar));
    ASSERT_EQ(bar._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bar._atr, 1.0, EPSILON);
    
    bars = barsManager.getBars(instr6CU2->_keyId);
    ASSERT_EQ(bars.size(), 1UL);
    
    bar = bars.back();
    ASSERT_TRUE(checkBar(TPrice(),TPrice(),TPrice(),TPrice(),TSize(0),60,0,false,"6CU2",tw::instr::eExchange::kCME,tw::common_trade::ePatternDir::kUnknown,bar));
    
    
    
    // Add another trade to NQM2
    //
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(100), TSize(4));
    quoteNQM2._trade._aggressorSide = tw::price::Trade::kAggressorSideSell;
    barsManager.onQuote(quoteNQM2);
    
    ASSERT_EQ(TestRouter::_timerClientInfos.size(), 1UL);
    ASSERT_EQ(TestRouter::_timerClientInfos.back()._msecs, 59000);
    
    bars = barsManager.getBars(instrNQM2->_keyId);
    ASSERT_EQ(bars.size(), 1UL);
    
    bar = bars.back();
    ASSERT_TRUE(checkBar(TPrice(102),TPrice(100),TPrice(101),TPrice(100),TSize(8),60,3,false,"NQM2",tw::instr::eExchange::kCME,tw::common_trade::ePatternDir::kDown,bar));
    ASSERT_EQ(bar._range, TPrice(2));
    ASSERT_TRUE(checkBarTrades(TPrice(101), TSize(0), TSize(1), TSize(7), TSize(1), bar));
    ASSERT_TRUE(checkBarTrades(TPrice(102), TSize(3), TSize(0), TSize(7), TSize(1), bar));
    ASSERT_TRUE(checkBarTrades(TPrice(100), TSize(4), TSize(0), TSize(7), TSize(1), bar));
    ASSERT_EQ(bar._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bar._atr, 2.0, EPSILON);
    
    bars = barsManager.getBars(instr6CU2->_keyId);
    ASSERT_EQ(bars.size(), 1UL);
    
    bar = bars.back();
    ASSERT_TRUE(checkBar(TPrice(),TPrice(),TPrice(),TPrice(),TSize(0),60,0,false,"6CU2",tw::instr::eExchange::kCME,tw::common_trade::ePatternDir::kUnknown,bar));
    ASSERT_EQ(bar._atrNumOfPeriods, 0UL);
    ASSERT_NEAR(bar._atr, 0.0, EPSILON);
    
    
    // Add another trade to NQM2
    //
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(101), TSize(8));
    quoteNQM2._trade._aggressorSide = tw::price::Trade::kAggressorSideSell;
    barsManager.onQuote(quoteNQM2);
    
    ASSERT_EQ(TestRouter::_timerClientInfos.size(), 1UL);
    ASSERT_EQ(TestRouter::_timerClientInfos.back()._msecs, 59000);
    
    bars = barsManager.getBars(instrNQM2->_keyId);
    ASSERT_EQ(bars.size(), 1UL);
    
    bar = bars.back();
    ASSERT_TRUE(checkBar(TPrice(102),TPrice(100),TPrice(101),TPrice(101),TSize(16),60,4,false,"NQM2",tw::instr::eExchange::kCME,tw::common_trade::ePatternDir::kUnknown,bar));
    ASSERT_TRUE(checkBarTrades(TPrice(101), TSize(8), TSize(1), TSize(15), TSize(1), bar));
    ASSERT_TRUE(checkBarTrades(TPrice(102), TSize(3), TSize(0), TSize(15), TSize(1), bar));
    ASSERT_TRUE(checkBarTrades(TPrice(100), TSize(4), TSize(0), TSize(15), TSize(1), bar));
    ASSERT_EQ(bar._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bar._atr, 2.0, EPSILON);
    
    bars = barsManager.getBars(instr6CU2->_keyId);
    ASSERT_EQ(bars.size(), 1UL);
    
    bar = bars.back();
    ASSERT_TRUE(checkBar(TPrice(),TPrice(),TPrice(),TPrice(),TSize(0),60,0,false,"6CU2",tw::instr::eExchange::kCME,tw::common_trade::ePatternDir::kUnknown,bar));
    ASSERT_EQ(bar._atrNumOfPeriods, 0UL);
    ASSERT_NEAR(bar._atr, 0.0, EPSILON);
    
    
    
    // Add trade to 6CU2
    //
    quote6CU2._seqNum = ++seqNum;
    quote6CU2.setTrade(TPrice(201), TSize(10));
    quote6CU2._trade._aggressorSide = tw::price::Trade::kAggressorSideSell;
    barsManager.onQuote(quote6CU2);
    
    
    ASSERT_EQ(TestRouter::_timerClientInfos.size(), 1UL);
    ASSERT_EQ(TestRouter::_timerClientInfos.back()._msecs, 59000);
    
    bars = barsManager.getBars(instrNQM2->_keyId);
    ASSERT_EQ(bars.size(), 1UL);
    
    bar = bars.back();
    ASSERT_TRUE(checkBar(TPrice(102),TPrice(100),TPrice(101),TPrice(101),TSize(16),60,4,false,"NQM2",tw::instr::eExchange::kCME,tw::common_trade::ePatternDir::kUnknown,bar));
    ASSERT_TRUE(checkBarTrades(TPrice(101), TSize(8), TSize(1), TSize(15), TSize(1), bar));
    ASSERT_TRUE(checkBarTrades(TPrice(102), TSize(3), TSize(0), TSize(15), TSize(1), bar));
    ASSERT_TRUE(checkBarTrades(TPrice(100), TSize(4), TSize(0), TSize(15), TSize(1), bar));
    ASSERT_EQ(bar._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bar._atr, 2.0, EPSILON);
    
    bars = barsManager.getBars(instr6CU2->_keyId);
    ASSERT_EQ(bars.size(), 1UL);
    
    bar = bars.back();
    ASSERT_TRUE(checkBar(TPrice(201),TPrice(201),TPrice(201),TPrice(201),TSize(10),60,1,false,"6CU2",tw::instr::eExchange::kCME,tw::common_trade::ePatternDir::kUnknown,bar));
    ASSERT_TRUE(checkBarTrades(TPrice(201), TSize(10), TSize(0), TSize(10), TSize(0), bar));
    ASSERT_EQ(bar._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bar._atr, 0.0, EPSILON);
    
    
    // Test timeout processing
    //
    TestRouter::_msecFromMidnight = 63 * 60;
    TestRouter::_msecFromMidnight *= 1000;  
    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    ASSERT_EQ(TestRouter::_timerClientInfos.size(), 2UL);
    ASSERT_EQ(TestRouter::_timerClientInfos.back()._msecs, 60000);
    
    
    bars = barsManager.getBars(instrNQM2->_keyId);
    ASSERT_EQ(bars.size(), 2UL);
    
    bar = bars.front();
    ASSERT_TRUE(checkBar(TPrice(102),TPrice(100),TPrice(101),TPrice(101),TSize(16),60,4,true,"NQM2",tw::instr::eExchange::kCME,tw::common_trade::ePatternDir::kUnknown,bar));
    ASSERT_EQ(bar._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bar._atr, 2.0, EPSILON);
    
    bar = bars.back();
    ASSERT_TRUE(checkBar(TPrice(),TPrice(),TPrice(),TPrice(),TSize(),60,0,false,"NQM2",tw::instr::eExchange::kCME,tw::common_trade::ePatternDir::kUnknown,bar));
    ASSERT_EQ(bar._atrNumOfPeriods, 0UL);
    ASSERT_NEAR(bar._atr, 0.0, EPSILON);
    
    bars = barsManager.getBars(instr6CU2->_keyId);
    ASSERT_EQ(bars.size(), 2UL);
    
    bar = bars.front();
    ASSERT_TRUE(checkBar(TPrice(201),TPrice(201),TPrice(201),TPrice(201),TSize(10),60,1,true,"6CU2",tw::instr::eExchange::kCME,tw::common_trade::ePatternDir::kUnknown,bar));
    ASSERT_EQ(bar._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bar._atr, 0.0, EPSILON);
    
    bar = bars.back();
    ASSERT_TRUE(checkBar(TPrice(),TPrice(),TPrice(),TPrice(),TSize(),60,0,false,"6CU2",tw::instr::eExchange::kCME,tw::common_trade::ePatternDir::kUnknown,bar));
    ASSERT_EQ(bar._atrNumOfPeriods, 0UL);
    ASSERT_NEAR(bar._atr, 0.0, EPSILON);
    
    
    
    // Test one more timeout cycle
    //
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(105), TSize(20));
    barsManager.onQuote(quoteNQM2);
    
    quote6CU2._seqNum = ++seqNum;
    quote6CU2.setTrade(TPrice(208), TSize(30));
    barsManager.onQuote(quote6CU2);
    
    bars = barsManager.getBars(instrNQM2->_keyId);
    ASSERT_EQ(bars.size(), 2UL);
    
    bar = bars.front();
    ASSERT_TRUE(checkBar(TPrice(102),TPrice(100),TPrice(101),TPrice(101),TSize(16),60,4,true,"NQM2",tw::instr::eExchange::kCME,tw::common_trade::ePatternDir::kUnknown,bar));
    ASSERT_EQ(bar._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bar._atr, 2.0, EPSILON);
    
    bar = bars.back();
    ASSERT_TRUE(checkBar(TPrice(105),TPrice(105),TPrice(105),TPrice(105),TSize(20),60,1,false,"NQM2",tw::instr::eExchange::kCME,tw::common_trade::ePatternDir::kUnknown,bar));
    ASSERT_EQ(bar._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bar._atr, 3.0, EPSILON);
    
    bars = barsManager.getBars(instr6CU2->_keyId);
    ASSERT_EQ(bars.size(), 2UL);
    
    bar = bars.front();
    ASSERT_TRUE(checkBar(TPrice(201),TPrice(201),TPrice(201),TPrice(201),TSize(10),60,1,true,"6CU2",tw::instr::eExchange::kCME,tw::common_trade::ePatternDir::kUnknown,bar));
    ASSERT_EQ(bar._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bar._atr, 0.0, EPSILON);
            
    bar = bars.back();
    ASSERT_TRUE(checkBar(TPrice(208),TPrice(208),TPrice(208),TPrice(208),TSize(30),60,1,false,"6CU2",tw::instr::eExchange::kCME,tw::common_trade::ePatternDir::kUnknown,bar));
    ASSERT_EQ(bar._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bar._atr, 3.5, EPSILON);
    
    
    
    // Test timeout processing
    //
    TestRouter::_msecFromMidnight = 64 * 60 + 1;
    TestRouter::_msecFromMidnight *= 1000;
    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    ASSERT_EQ(TestRouter::_timerClientInfos.size(), 3UL);
    ASSERT_EQ(TestRouter::_timerClientInfos.back()._msecs, 59000);
    
    
    bars = barsManager.getBars(instrNQM2->_keyId);
    ASSERT_EQ(bars.size(), 3UL);
    
    bar = bars[0];
    ASSERT_TRUE(checkBar(TPrice(102),TPrice(100),TPrice(101),TPrice(101),TSize(16),60,4,true,"NQM2",tw::instr::eExchange::kCME,tw::common_trade::ePatternDir::kUnknown,bar));
    
    bar = bars[1];
    ASSERT_TRUE(checkBar(TPrice(105),TPrice(105),TPrice(105),TPrice(105),TSize(20),60,1,true,"NQM2",tw::instr::eExchange::kCME,tw::common_trade::ePatternDir::kUnknown,bar));
    
    bar = bars[2];
    ASSERT_TRUE(checkBar(TPrice(),TPrice(),TPrice(),TPrice(),TSize(),60,0,false,"NQM2",tw::instr::eExchange::kCME,tw::common_trade::ePatternDir::kUnknown,bar));
    
    bars = barsManager.getBars(instr6CU2->_keyId);
    ASSERT_EQ(bars.size(), 3UL);
    
    bar = bars[0];
    ASSERT_TRUE(checkBar(TPrice(201),TPrice(201),TPrice(201),TPrice(201),TSize(10),60,1,true,"6CU2",tw::instr::eExchange::kCME,tw::common_trade::ePatternDir::kUnknown,bar));
            
    bar = bars[1];
    ASSERT_TRUE(checkBar(TPrice(208),TPrice(208),TPrice(208),TPrice(208),TSize(30),60,1,true,"6CU2",tw::instr::eExchange::kCME,tw::common_trade::ePatternDir::kUnknown,bar));
    
    bar = bars[2];
    ASSERT_TRUE(checkBar(TPrice(),TPrice(),TPrice(),TPrice(),TSize(),60,0,false,"6CU2",tw::instr::eExchange::kCME,tw::common_trade::ePatternDir::kUnknown,bar));
    
    
    // Test cumVol
    //
    ASSERT_EQ(barsManager.getBackBarsCumVol(instrNQM2->_keyId, 1, 0), TSize());
    ASSERT_EQ(barsManager.getBackBarsCumVol(instrNQM2->_keyId, 2, 0), TSize(20));
    ASSERT_EQ(barsManager.getBackBarsCumVol(instrNQM2->_keyId, 3, 0), TSize(36));
    ASSERT_EQ(barsManager.getBackBarsCumVol(instrNQM2->_keyId, 30, 0), TSize(36));
    
    
    ASSERT_EQ(barsManager.getBackBarsCumVol(instrNQM2->_keyId, 1, 1), TSize(20));
    ASSERT_EQ(barsManager.getBackBarsCumVol(instrNQM2->_keyId, 2, 1), TSize(36));
    ASSERT_EQ(barsManager.getBackBarsCumVol(instrNQM2->_keyId, 2, 2), TSize(16));
    ASSERT_EQ(barsManager.getBackBarsCumVol(instrNQM2->_keyId, 2, 3), TSize());
    ASSERT_EQ(barsManager.getBackBarsCumVol(instrNQM2->_keyId, 2, 30), TSize());
    ASSERT_EQ(barsManager.getBackBarsCumVol(instrNQM2->_keyId, 3, 1), TSize(36));
    ASSERT_EQ(barsManager.getBackBarsCumVol(instrNQM2->_keyId, 3, 2), TSize(16));
    ASSERT_EQ(barsManager.getBackBarsCumVol(instrNQM2->_keyId, 3, 3), TSize());
    ASSERT_EQ(barsManager.getBackBarsCumVol(instrNQM2->_keyId, 3, 30), TSize());
    ASSERT_EQ(barsManager.getBackBarsCumVol(instrNQM2->_keyId, 30, 0), TSize(36));
    ASSERT_EQ(barsManager.getBackBarsCumVol(instrNQM2->_keyId, 30, 30), TSize());
    
    ASSERT_EQ(barsManager.getBackBarsCumVol(instr6CU2->_keyId, 1, 0), TSize());
    ASSERT_EQ(barsManager.getBackBarsCumVol(instr6CU2->_keyId, 2, 0), TSize(30));
    ASSERT_EQ(barsManager.getBackBarsCumVol(instr6CU2->_keyId, 3, 0), TSize(40));
    ASSERT_EQ(barsManager.getBackBarsCumVol(instr6CU2->_keyId, 30, 0), TSize(40));
    
    
    // Test highest high for bars
    //
    ASSERT_EQ(barsManager.getBackBarsHigh(instrNQM2->_keyId, 1), TPrice());
    ASSERT_EQ(barsManager.getBackBarsHigh(instrNQM2->_keyId, 2), TPrice(105));
    ASSERT_EQ(barsManager.getBackBarsHigh(instrNQM2->_keyId, 3), TPrice(105));
    ASSERT_EQ(barsManager.getBackBarsHigh(instrNQM2->_keyId, 30), TPrice(105));
    
    ASSERT_EQ(barsManager.getBackBarsHigh(instr6CU2->_keyId, 1), TPrice());
    ASSERT_EQ(barsManager.getBackBarsHigh(instr6CU2->_keyId, 2), TPrice(208));
    ASSERT_EQ(barsManager.getBackBarsHigh(instr6CU2->_keyId, 3), TPrice(208));
    ASSERT_EQ(barsManager.getBackBarsHigh(instr6CU2->_keyId, 30), TPrice(208));
    
    
    // Test lowest low for bars
    //
    ASSERT_EQ(barsManager.getBackBarsLow(instrNQM2->_keyId, 1), TPrice());
    ASSERT_EQ(barsManager.getBackBarsLow(instrNQM2->_keyId, 2), TPrice(105));
    ASSERT_EQ(barsManager.getBackBarsLow(instrNQM2->_keyId, 3), TPrice(100));
    ASSERT_EQ(barsManager.getBackBarsLow(instrNQM2->_keyId, 30), TPrice(100));
    
    ASSERT_EQ(barsManager.getBackBarsLow(instr6CU2->_keyId, 1), TPrice());
    ASSERT_EQ(barsManager.getBackBarsLow(instr6CU2->_keyId, 2), TPrice(208));
    ASSERT_EQ(barsManager.getBackBarsLow(instr6CU2->_keyId, 3), TPrice(201));
    ASSERT_EQ(barsManager.getBackBarsLow(instr6CU2->_keyId, 30), TPrice(201));
    
    
    // Test high-low range average for bars
    //
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(107), TSize(20));
    barsManager.onQuote(quoteNQM2);
    
    quoteNQM2._seqNum = ++seqNum;
    quoteNQM2.setTrade(TPrice(102), TSize(20));
    barsManager.onQuote(quoteNQM2);
    
    quote6CU2._seqNum = ++seqNum;
    quote6CU2.setTrade(TPrice(218), TSize(30));
    barsManager.onQuote(quote6CU2);
    
    quote6CU2._seqNum = ++seqNum;
    quote6CU2.setTrade(TPrice(204), TSize(30));
    barsManager.onQuote(quote6CU2);    
    
    
    ASSERT_EQ(barsManager.getBackBarsHighLowRangeAvg(instrNQM2->_keyId, 1, 0), TPrice(5));
    ASSERT_EQ(barsManager.getBackBarsHighLowRangeAvg(instrNQM2->_keyId, 2, 0), TPrice(2));
    ASSERT_EQ(barsManager.getBackBarsHighLowRangeAvg(instrNQM2->_keyId, 3, 0), TPrice(2));
    ASSERT_EQ(barsManager.getBackBarsHighLowRangeAvg(instrNQM2->_keyId, 30, 0), TPrice(2));
    
    ASSERT_EQ(barsManager.getBackBarsHighLowRangeAvg(instrNQM2->_keyId, 1, 1), TPrice(0));
    ASSERT_EQ(barsManager.getBackBarsHighLowRangeAvg(instrNQM2->_keyId, 1, 2), TPrice(2));
    ASSERT_EQ(barsManager.getBackBarsHighLowRangeAvg(instrNQM2->_keyId, 2, 1), TPrice(1));
    ASSERT_EQ(barsManager.getBackBarsHighLowRangeAvg(instrNQM2->_keyId, 2, 2), TPrice(2));
    ASSERT_EQ(barsManager.getBackBarsHighLowRangeAvg(instrNQM2->_keyId, 2, 3), TPrice(0));
    ASSERT_EQ(barsManager.getBackBarsHighLowRangeAvg(instrNQM2->_keyId, 30, 1), TPrice(1));
    ASSERT_EQ(barsManager.getBackBarsHighLowRangeAvg(instrNQM2->_keyId, 30, 2), TPrice(2));
    ASSERT_EQ(barsManager.getBackBarsHighLowRangeAvg(instrNQM2->_keyId, 30, 30), TPrice(0));    
    
    
    
    ASSERT_EQ(barsManager.getBackBarsHighLowRangeAvg(instr6CU2->_keyId, 1, 0), TPrice(14));
    ASSERT_EQ(barsManager.getBackBarsHighLowRangeAvg(instr6CU2->_keyId, 2, 0), TPrice(7));
    ASSERT_EQ(barsManager.getBackBarsHighLowRangeAvg(instr6CU2->_keyId, 3, 0), TPrice(5));
    ASSERT_EQ(barsManager.getBackBarsHighLowRangeAvg(instr6CU2->_keyId, 30, 0), TPrice(5));
    
}


TEST(CommonTradeLibTestSuit, bars_impl_test_pattern_processing)
{
    TestRouter::TImpl barsManager(60, -1, 4);
    
    clearProcessors();    
    
    // Init quotes
    //
    tw::price::QuoteStore::TQuote quoteNQM2;        
    tw::instr::InstrumentPtr instrNQM2 = InstrHelper::getNQM2();    
    
    quoteNQM2.setInstrument(instrNQM2);    
    
    // Subscribe to NQM2
    //
    ASSERT_TRUE(barsManager.subscribe(instrNQM2->_keyId));
    const TBars& bars = barsManager.getBars(instrNQM2->_keyId);
    
    // Bar 1 - transitional bar
    //
    ASSERT_EQ(TestRouter::_barPatterns.size(), 0UL);
    formBar(TPrice(103), TPrice(98), TPrice(101), TPrice(101), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    const TBarPatterns& patterns = barsManager.getBarPatternsSimple().getBarPatterns(instrNQM2->_keyId);
    const TBarPatterns& patternsSwingLevel1 = barsManager.getBarPatternsSwingLevel1().getBarPatterns(instrNQM2->_keyId);
    const TBarPatterns& patternsSwingLevel2 = barsManager.getBarPatternsSwingLevel2().getBarPatterns(instrNQM2->_keyId);
        
    ASSERT_EQ(bars.size(), 2UL);
    ASSERT_EQ(patterns.size(), 1UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 0UL);
    ASSERT_EQ(patternsSwingLevel2.size(), 0UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_EQ(patterns[0]._parentType, tw::common_trade::ePatternsType::kSimple);
    
    // Bar 2 - swing bar up
    //
    formBar(TPrice(104), TPrice(100), TPrice(101), TPrice(103), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
        
    ASSERT_EQ(bars.size(), 3UL);
    ASSERT_EQ(patterns.size(), 2UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 1UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_EQ(patterns[0]._range, TPrice(5));
    ASSERT_TRUE(checkBarPattern(TPrice(104), TPrice(100), TPrice(101), TPrice(103), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 2, 2, 2, 2, tw::common_trade::ePatternType::kSwing, patterns[1]));
    ASSERT_EQ(patterns[1]._range, TPrice(4));
    
    ASSERT_TRUE(checkBarPattern(TPrice(104), TPrice(98), TPrice(101), TPrice(103), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    ASSERT_EQ(patternsSwingLevel1[0]._range, TPrice(6));
    ASSERT_EQ(patternsSwingLevel1[0]._parentType, tw::common_trade::ePatternsType::kSwingLevel1);
    
    ASSERT_TRUE(checkBarPattern(TPrice(104), TPrice(98), TPrice(101), TPrice(103), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    ASSERT_EQ(patternsSwingLevel2[0]._range, TPrice(6));
    ASSERT_EQ(patternsSwingLevel2[0]._parentType, tw::common_trade::ePatternsType::kSwingLevel2);
    
    ASSERT_EQ(TestRouter::_barPatterns.size(), 4UL);
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, TestRouter::_barPatterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(104), TPrice(100), TPrice(101), TPrice(103), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 2, 2, 2, 2, tw::common_trade::ePatternType::kSwing, TestRouter::_barPatterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(104), TPrice(98), TPrice(101), TPrice(103), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kSwing, TestRouter::_barPatterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(104), TPrice(98), TPrice(101), TPrice(103), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kTrend, TestRouter::_barPatterns[3]));
    TestRouter::_barPatterns.clear();
    
    // Bar 3 - swing bar up
    //
    formBar(TPrice(105), TPrice(101), TPrice(102), TPrice(104), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    ASSERT_EQ(bars.size(), 4UL);
    ASSERT_EQ(patterns.size(), 2UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 1UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(100), TPrice(101), TPrice(104), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 2, 3, 3, 2, tw::common_trade::ePatternType::kSwing, patterns[1]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(98), TPrice(101), TPrice(104), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 1, 3, 3, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(98), TPrice(101), TPrice(104), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 1, 3, 3, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    
    ASSERT_EQ(TestRouter::_barPatterns.size(), 3UL);
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(100), TPrice(101), TPrice(104), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 2, 3, 3, 2, tw::common_trade::ePatternType::kSwing, TestRouter::_barPatterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(98), TPrice(101), TPrice(104), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 1, 3, 3, 1, tw::common_trade::ePatternType::kSwing, TestRouter::_barPatterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(98), TPrice(101), TPrice(104), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 1, 3, 3, 1, tw::common_trade::ePatternType::kTrend, TestRouter::_barPatterns[2]));
    TestRouter::_barPatterns.clear();
    
    // Bar 4 - swing bar up
    //    
    formBar(TPrice(106), TPrice(102), TPrice(103), TPrice(105), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    ASSERT_EQ(bars.size(), 5UL);
    ASSERT_EQ(patterns.size(), 2UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 1UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(100), TPrice(101), TPrice(105), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 2, 4, 4, 2, tw::common_trade::ePatternType::kSwing, patterns[1]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(98), TPrice(101), TPrice(105), TSize(16), 16, tw::common_trade::ePatternDir::kUp, 1, 4, 4, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(98), TPrice(101), TPrice(105), TSize(16), 16, tw::common_trade::ePatternDir::kUp, 1, 4, 4, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    
    ASSERT_EQ(TestRouter::_barPatterns.size(), 3UL);
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(100), TPrice(101), TPrice(105), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 2, 4, 4, 2, tw::common_trade::ePatternType::kSwing, TestRouter::_barPatterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(98), TPrice(101), TPrice(105), TSize(16), 16, tw::common_trade::ePatternDir::kUp, 1, 4, 4, 1, tw::common_trade::ePatternType::kSwing, TestRouter::_barPatterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(98), TPrice(101), TPrice(105), TSize(16), 16, tw::common_trade::ePatternDir::kUp, 1, 4, 4, 1, tw::common_trade::ePatternType::kTrend, TestRouter::_barPatterns[2]));
    TestRouter::_barPatterns.clear();
    
    // Bar 5 - hitch bar up
    //
    formBar(TPrice(103), TPrice(98), TPrice(100), TPrice(102), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    ASSERT_EQ(bars.size(), 6UL);
    ASSERT_EQ(patterns.size(), 3UL);
    
    ASSERT_EQ(patternsSwingLevel1.size(), 1UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(100), TPrice(101), TPrice(105), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 2, 4, 4, 2, tw::common_trade::ePatternType::kSwing, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(100), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[2]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(98), TPrice(101), TPrice(102), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 1, 5, 4, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(98), TPrice(101), TPrice(102), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 1, 5, 4, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    
    ASSERT_EQ(TestRouter::_barPatterns.size(), 3UL);
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(100), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, TestRouter::_barPatterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(98), TPrice(101), TPrice(102), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 1, 5, 4, 1, tw::common_trade::ePatternType::kSwing, TestRouter::_barPatterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(98), TPrice(101), TPrice(102), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 1, 5, 4, 1, tw::common_trade::ePatternType::kTrend, TestRouter::_barPatterns[2]));
    TestRouter::_barPatterns.clear();
    
    // Bar 6 - swing bar up
    //
    formBar(TPrice(105), TPrice(97), TPrice(103), TPrice(104), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    ASSERT_EQ(bars.size(), 7UL);
    ASSERT_EQ(patterns.size(), 4UL);
    
    ASSERT_EQ(patternsSwingLevel1.size(), 1UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(100), TPrice(101), TPrice(105), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 2, 4, 4, 2, tw::common_trade::ePatternType::kSwing, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(100), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(97), TPrice(103), TPrice(104), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 6, 6, 6, 6, tw::common_trade::ePatternType::kHitch, patterns[3]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(98), TPrice(101), TPrice(102), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 1, 5, 4, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(98), TPrice(101), TPrice(102), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 1, 5, 4, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    
    // Bar 7 - transition bar
    //
    formBar(TPrice(103), TPrice(98), TPrice(101), TPrice(101), quoteNQM2, barsManager);     
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    ASSERT_EQ(bars.size(), 8UL);
    ASSERT_EQ(patterns.size(), 5UL);
    
    ASSERT_EQ(patternsSwingLevel1.size(), 1UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(100), TPrice(101), TPrice(105), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 2, 4, 4, 2, tw::common_trade::ePatternType::kSwing, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(100), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(97), TPrice(103), TPrice(104), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 6, 6, 6, 6, tw::common_trade::ePatternType::kHitch, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 7, 7, 7, 7, tw::common_trade::ePatternType::kTransitional, patterns[4]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(98), TPrice(101), TPrice(102), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 1, 5, 4, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));    
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(98), TPrice(101), TPrice(101), TSize(24), 24, tw::common_trade::ePatternDir::kUp, 1, 7, 4, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    
    // Bar 8 - swing bar down
    //
    formBar(TPrice(112), TPrice(108), TPrice(110), TPrice(109), quoteNQM2, barsManager);
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    ASSERT_EQ(bars.size(), 9UL);
    ASSERT_EQ(patterns.size(), 6UL);
    
    ASSERT_EQ(patternsSwingLevel1.size(), 1UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(100), TPrice(101), TPrice(105), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 2, 4, 4, 2, tw::common_trade::ePatternType::kSwing, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(100), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(97), TPrice(103), TPrice(104), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 6, 6, 6, 6, tw::common_trade::ePatternType::kHitch, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 7, 7, 7, 7, tw::common_trade::ePatternType::kTransitional, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(108), TPrice(110), TPrice(109), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 8, 8, 8, 8, tw::common_trade::ePatternType::kHitch, patterns[5]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(98), TPrice(101), TPrice(102), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 1, 5, 4, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    ASSERT_EQ(patternsSwingLevel1[0]._range, TPrice(8));
        
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(98), TPrice(101), TPrice(109), TSize(28), 28, tw::common_trade::ePatternDir::kUp, 1, 8, 8, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    ASSERT_EQ(patternsSwingLevel2[0]._range, TPrice(14));
    
    
    // Bar 9 - swing bar down
    //
    formBar(TPrice(106), TPrice(100), TPrice(105), TPrice(101), quoteNQM2, barsManager);
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    ASSERT_EQ(bars.size(), 10UL);
    ASSERT_EQ(patterns.size(), 7UL);
    
    ASSERT_EQ(patternsSwingLevel1.size(), 2UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(100), TPrice(101), TPrice(105), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 2, 4, 4, 2, tw::common_trade::ePatternType::kSwing, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(100), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(97), TPrice(103), TPrice(104), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 6, 6, 6, 6, tw::common_trade::ePatternType::kHitch, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 7, 7, 7, 7, tw::common_trade::ePatternType::kTransitional, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(108), TPrice(110), TPrice(109), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 8, 8, 8, 8, tw::common_trade::ePatternType::kHitch, patterns[5]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(100), TPrice(105), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 9, 9, 9, 9, tw::common_trade::ePatternType::kSwing, patterns[6]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(98), TPrice(101), TPrice(102), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 1, 5, 4, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));    
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(100), TPrice(110), TPrice(101), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 8, 9, 8, 9, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));    
    
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(98), TPrice(101), TPrice(101), TSize(32), 32, tw::common_trade::ePatternDir::kUp, 1, 9, 8, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    
    // Bar 10 - another swing bar down
    //
    formBar(TPrice(105), TPrice(99), TPrice(104), TPrice(100), quoteNQM2, barsManager);
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    ASSERT_EQ(bars.size(), 11UL);
    ASSERT_EQ(patterns.size(), 7UL);
    
    ASSERT_EQ(patternsSwingLevel1.size(), 2UL);
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(100), TPrice(101), TPrice(105), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 2, 4, 4, 2, tw::common_trade::ePatternType::kSwing, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(100), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(97), TPrice(103), TPrice(104), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 6, 6, 6, 6, tw::common_trade::ePatternType::kHitch, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 7, 7, 7, 7, tw::common_trade::ePatternType::kTransitional, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(108), TPrice(110), TPrice(109), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 8, 8, 8, 8, tw::common_trade::ePatternType::kHitch, patterns[5]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(99), TPrice(105), TPrice(100), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 9, 10, 9, 10, tw::common_trade::ePatternType::kSwing, patterns[6]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(98), TPrice(101), TPrice(102), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 1, 5, 4, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));    
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(99), TPrice(110), TPrice(100), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 8, 10, 8, 10, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));    
    
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(98), TPrice(101), TPrice(100), TSize(36), 36, tw::common_trade::ePatternDir::kUp, 1, 10, 8, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    
    // Bar 11 - swing bar up
    //
    formBar(TPrice(107), TPrice(100), TPrice(101), TPrice(106), quoteNQM2, barsManager);
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    ASSERT_EQ(bars.size(), 12UL);
    ASSERT_EQ(patterns.size(), 8UL);
    
    ASSERT_EQ(patternsSwingLevel1.size(), 3UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(100), TPrice(101), TPrice(105), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 2, 4, 4, 2, tw::common_trade::ePatternType::kSwing, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(100), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(97), TPrice(103), TPrice(104), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 6, 6, 6, 6, tw::common_trade::ePatternType::kHitch, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 7, 7, 7, 7, tw::common_trade::ePatternType::kTransitional, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(108), TPrice(110), TPrice(109), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 8, 8, 8, 8, tw::common_trade::ePatternType::kHitch, patterns[5]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(99), TPrice(105), TPrice(100), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 9, 10, 9, 10, tw::common_trade::ePatternType::kSwing, patterns[6]));
    ASSERT_TRUE(checkBarPattern(TPrice(107), TPrice(100), TPrice(101), TPrice(106), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 11, 11, 11, 11, tw::common_trade::ePatternType::kSwing, patterns[7]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(98), TPrice(101), TPrice(102), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 1, 5, 4, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));    
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(99), TPrice(110), TPrice(100), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 8, 10, 8, 10, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(107), TPrice(99), TPrice(104), TPrice(106), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 10, 11, 11, 10, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[2]));    
    
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(98), TPrice(101), TPrice(106), TSize(40), 40, tw::common_trade::ePatternDir::kUp, 1, 11, 8, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    
    // Bar 12 - swing bar up
    //
    formBar(TPrice(109), TPrice(103), TPrice(104), TPrice(108), quoteNQM2, barsManager);
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);

    ASSERT_EQ(bars.size(), 13UL);
    ASSERT_EQ(patterns.size(), 8UL);
    
    ASSERT_EQ(patternsSwingLevel1.size(), 3UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(100), TPrice(101), TPrice(105), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 2, 4, 4, 2, tw::common_trade::ePatternType::kSwing, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(100), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(97), TPrice(103), TPrice(104), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 6, 6, 6, 6, tw::common_trade::ePatternType::kHitch, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 7, 7, 7, 7, tw::common_trade::ePatternType::kTransitional, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(108), TPrice(110), TPrice(109), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 8, 8, 8, 8, tw::common_trade::ePatternType::kHitch, patterns[5]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(99), TPrice(105), TPrice(100), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 9, 10, 9, 10, tw::common_trade::ePatternType::kSwing, patterns[6]));
    ASSERT_TRUE(checkBarPattern(TPrice(109), TPrice(100), TPrice(101), TPrice(108), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 11, 12, 12, 11, tw::common_trade::ePatternType::kSwing, patterns[7]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(98), TPrice(101), TPrice(102), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 1, 5, 4, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));    
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(99), TPrice(110), TPrice(100), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 8, 10, 8, 10, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(109), TPrice(99), TPrice(104), TPrice(108), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 10, 12, 12, 10, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[2]));    
    
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(98), TPrice(101), TPrice(108), TSize(44), 44, tw::common_trade::ePatternDir::kUp, 1, 12, 8, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));

    
    // Bar 13 - hitch bar up
    //
    formBar(TPrice(106), TPrice(102), TPrice(103), TPrice(105), quoteNQM2, barsManager);
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);

    ASSERT_EQ(bars.size(), 14UL);
    ASSERT_EQ(patterns.size(), 9UL);
    
    ASSERT_EQ(patternsSwingLevel1.size(), 3UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(100), TPrice(101), TPrice(105), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 2, 4, 4, 2, tw::common_trade::ePatternType::kSwing, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(100), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(97), TPrice(103), TPrice(104), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 6, 6, 6, 6, tw::common_trade::ePatternType::kHitch, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 7, 7, 7, 7, tw::common_trade::ePatternType::kTransitional, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(108), TPrice(110), TPrice(109), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 8, 8, 8, 8, tw::common_trade::ePatternType::kHitch, patterns[5]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(99), TPrice(105), TPrice(100), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 9, 10, 9, 10, tw::common_trade::ePatternType::kSwing, patterns[6]));
    ASSERT_TRUE(checkBarPattern(TPrice(109), TPrice(100), TPrice(101), TPrice(108), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 11, 12, 12, 11, tw::common_trade::ePatternType::kSwing, patterns[7]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(102), TPrice(103), TPrice(105), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 13, 13, 13, 13, tw::common_trade::ePatternType::kHitch, patterns[8]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(98), TPrice(101), TPrice(102), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 1, 5, 4, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));    
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(99), TPrice(110), TPrice(100), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 8, 10, 8, 10, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(109), TPrice(99), TPrice(104), TPrice(105), TSize(16), 16, tw::common_trade::ePatternDir::kUp, 10, 13, 12, 10, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[2]));    
    
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(98), TPrice(101), TPrice(105), TSize(48), 48, tw::common_trade::ePatternDir::kUp, 1, 13, 8, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    
    
    // Bar 14 - transitional bar
    //
    formBar(TPrice(107), TPrice(101), TPrice(104), TPrice(104), quoteNQM2, barsManager);
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);

    ASSERT_EQ(bars.size(), 15UL);
    ASSERT_EQ(patterns.size(), 10UL);
    
    ASSERT_EQ(patternsSwingLevel1.size(), 3UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(100), TPrice(101), TPrice(105), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 2, 4, 4, 2, tw::common_trade::ePatternType::kSwing, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(100), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(97), TPrice(103), TPrice(104), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 6, 6, 6, 6, tw::common_trade::ePatternType::kHitch, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 7, 7, 7, 7, tw::common_trade::ePatternType::kTransitional, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(108), TPrice(110), TPrice(109), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 8, 8, 8, 8, tw::common_trade::ePatternType::kHitch, patterns[5]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(99), TPrice(105), TPrice(100), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 9, 10, 9, 10, tw::common_trade::ePatternType::kSwing, patterns[6]));
    ASSERT_TRUE(checkBarPattern(TPrice(109), TPrice(100), TPrice(101), TPrice(108), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 11, 12, 12, 11, tw::common_trade::ePatternType::kSwing, patterns[7]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(102), TPrice(103), TPrice(105), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 13, 13, 13, 13, tw::common_trade::ePatternType::kHitch, patterns[8]));
    ASSERT_TRUE(checkBarPattern(TPrice(107), TPrice(101), TPrice(104), TPrice(104), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 14, 14, 14, 14, tw::common_trade::ePatternType::kTransitional, patterns[9]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(98), TPrice(101), TPrice(102), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 1, 5, 4, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));    
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(99), TPrice(110), TPrice(100), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 8, 10, 8, 10, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(109), TPrice(99), TPrice(104), TPrice(104), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 10, 14, 12, 10, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[2]));    
    
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(98), TPrice(101), TPrice(104), TSize(52), 52, tw::common_trade::ePatternDir::kUp, 1, 14, 8, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));

        
    // Bar 15 - transitional bar
    //
    formBar(TPrice(103), TPrice(100), TPrice(102), TPrice(102), quoteNQM2, barsManager);
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);

    ASSERT_EQ(bars.size(), 16UL);
    ASSERT_EQ(patterns.size(), 11UL);
    
    ASSERT_EQ(patternsSwingLevel1.size(), 3UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(100), TPrice(101), TPrice(105), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 2, 4, 4, 2, tw::common_trade::ePatternType::kSwing, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(100), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(97), TPrice(103), TPrice(104), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 6, 6, 6, 6, tw::common_trade::ePatternType::kHitch, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 7, 7, 7, 7, tw::common_trade::ePatternType::kTransitional, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(108), TPrice(110), TPrice(109), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 8, 8, 8, 8, tw::common_trade::ePatternType::kHitch, patterns[5]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(99), TPrice(105), TPrice(100), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 9, 10, 9, 10, tw::common_trade::ePatternType::kSwing, patterns[6]));
    ASSERT_TRUE(checkBarPattern(TPrice(109), TPrice(100), TPrice(101), TPrice(108), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 11, 12, 12, 11, tw::common_trade::ePatternType::kSwing, patterns[7]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(102), TPrice(103), TPrice(105), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 13, 13, 13, 13, tw::common_trade::ePatternType::kHitch, patterns[8]));
    ASSERT_TRUE(checkBarPattern(TPrice(107), TPrice(101), TPrice(104), TPrice(104), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 14, 14, 14, 14, tw::common_trade::ePatternType::kTransitional, patterns[9]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(100), TPrice(102), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 15, 15, 15, 15, tw::common_trade::ePatternType::kTransitional, patterns[10]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(98), TPrice(101), TPrice(102), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 1, 5, 4, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));    
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(99), TPrice(110), TPrice(100), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 8, 10, 8, 10, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(109), TPrice(99), TPrice(104), TPrice(102), TSize(24), 24, tw::common_trade::ePatternDir::kUp, 10, 15, 12, 10, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[2]));    
    
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(98), TPrice(101), TPrice(102), TSize(56), 56, tw::common_trade::ePatternDir::kUp, 1, 15, 8, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));

        
    // Bar 16 - swing bar down
    //
    formBar(TPrice(102), TPrice(99), TPrice(101), TPrice(100), quoteNQM2, barsManager);
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);

    ASSERT_EQ(bars.size(), 17UL);
    ASSERT_EQ(patterns.size(), 12UL);
    
    ASSERT_EQ(patternsSwingLevel1.size(), 4UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(100), TPrice(101), TPrice(105), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 2, 4, 4, 2, tw::common_trade::ePatternType::kSwing, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(100), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(97), TPrice(103), TPrice(104), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 6, 6, 6, 6, tw::common_trade::ePatternType::kHitch, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 7, 7, 7, 7, tw::common_trade::ePatternType::kTransitional, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(108), TPrice(110), TPrice(109), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 8, 8, 8, 8, tw::common_trade::ePatternType::kHitch, patterns[5]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(99), TPrice(105), TPrice(100), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 9, 10, 9, 10, tw::common_trade::ePatternType::kSwing, patterns[6]));
    ASSERT_TRUE(checkBarPattern(TPrice(109), TPrice(100), TPrice(101), TPrice(108), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 11, 12, 12, 11, tw::common_trade::ePatternType::kSwing, patterns[7]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(102), TPrice(103), TPrice(105), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 13, 13, 13, 13, tw::common_trade::ePatternType::kHitch, patterns[8]));
    ASSERT_TRUE(checkBarPattern(TPrice(107), TPrice(101), TPrice(104), TPrice(104), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 14, 14, 14, 14, tw::common_trade::ePatternType::kTransitional, patterns[9]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(100), TPrice(102), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 15, 15, 15, 15, tw::common_trade::ePatternType::kTransitional, patterns[10]));
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(99), TPrice(101), TPrice(100), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 16, 16, 16, 16, tw::common_trade::ePatternType::kSwing, patterns[11]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(98), TPrice(101), TPrice(102), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 1, 5, 4, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));    
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(99), TPrice(110), TPrice(100), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 8, 10, 8, 10, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(109), TPrice(99), TPrice(104), TPrice(102), TSize(24), 24, tw::common_trade::ePatternDir::kUp, 10, 15, 12, 10, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[2]));    
    ASSERT_TRUE(checkBarPattern(TPrice(109), TPrice(99), TPrice(104), TPrice(100), TSize(20), 20, tw::common_trade::ePatternDir::kDown, 12, 16, 12, 16, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[3]));

    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(98), TPrice(101), TPrice(100), TSize(60), 60, tw::common_trade::ePatternDir::kUp, 1, 16, 8, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));

        
    // Bar 17 - another swing bar down
    //
    formBar(TPrice(101), TPrice(97), TPrice(100), TPrice(99), quoteNQM2, barsManager);
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);

    ASSERT_EQ(bars.size(), 18UL);
    ASSERT_EQ(patterns.size(), 12UL);
    
    ASSERT_EQ(patternsSwingLevel1.size(), 4UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 2UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(100), TPrice(101), TPrice(105), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 2, 4, 4, 2, tw::common_trade::ePatternType::kSwing, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(100), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(97), TPrice(103), TPrice(104), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 6, 6, 6, 6, tw::common_trade::ePatternType::kHitch, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 7, 7, 7, 7, tw::common_trade::ePatternType::kTransitional, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(108), TPrice(110), TPrice(109), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 8, 8, 8, 8, tw::common_trade::ePatternType::kHitch, patterns[5]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(99), TPrice(105), TPrice(100), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 9, 10, 9, 10, tw::common_trade::ePatternType::kSwing, patterns[6]));
    ASSERT_TRUE(checkBarPattern(TPrice(109), TPrice(100), TPrice(101), TPrice(108), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 11, 12, 12, 11, tw::common_trade::ePatternType::kSwing, patterns[7]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(102), TPrice(103), TPrice(105), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 13, 13, 13, 13, tw::common_trade::ePatternType::kHitch, patterns[8]));
    ASSERT_TRUE(checkBarPattern(TPrice(107), TPrice(101), TPrice(104), TPrice(104), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 14, 14, 14, 14, tw::common_trade::ePatternType::kTransitional, patterns[9]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(100), TPrice(102), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 15, 15, 15, 15, tw::common_trade::ePatternType::kTransitional, patterns[10]));
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(97), TPrice(101), TPrice(99), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 16, 17, 16, 17, tw::common_trade::ePatternType::kSwing, patterns[11]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(98), TPrice(101), TPrice(102), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 1, 5, 4, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));    
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(99), TPrice(110), TPrice(100), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 8, 10, 8, 10, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(109), TPrice(99), TPrice(104), TPrice(102), TSize(24), 24, tw::common_trade::ePatternDir::kUp, 10, 15, 12, 10, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[2]));    
    ASSERT_TRUE(checkBarPattern(TPrice(109), TPrice(97), TPrice(104), TPrice(99), TSize(24), 24, tw::common_trade::ePatternDir::kDown, 12, 17, 12, 17, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[3]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(98), TPrice(101), TPrice(100), TSize(60), 60, tw::common_trade::ePatternDir::kUp, 1, 16, 8, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(97), TPrice(110), TPrice(99), TSize(40), 40, tw::common_trade::ePatternDir::kDown, 8, 17, 8, 17, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[1]));

        
    // Bar 18 - hitch bar down
    //
    formBar(TPrice(102), TPrice(99), TPrice(101), TPrice(100), quoteNQM2, barsManager);
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);

    ASSERT_EQ(bars.size(), 19UL);
    ASSERT_EQ(patterns.size(), 13UL);
    
    ASSERT_EQ(patternsSwingLevel1.size(), 4UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 2UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(100), TPrice(101), TPrice(105), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 2, 4, 4, 2, tw::common_trade::ePatternType::kSwing, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(100), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(97), TPrice(103), TPrice(104), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 6, 6, 6, 6, tw::common_trade::ePatternType::kHitch, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 7, 7, 7, 7, tw::common_trade::ePatternType::kTransitional, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(108), TPrice(110), TPrice(109), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 8, 8, 8, 8, tw::common_trade::ePatternType::kHitch, patterns[5]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(99), TPrice(105), TPrice(100), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 9, 10, 9, 10, tw::common_trade::ePatternType::kSwing, patterns[6]));
    ASSERT_TRUE(checkBarPattern(TPrice(109), TPrice(100), TPrice(101), TPrice(108), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 11, 12, 12, 11, tw::common_trade::ePatternType::kSwing, patterns[7]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(102), TPrice(103), TPrice(105), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 13, 13, 13, 13, tw::common_trade::ePatternType::kHitch, patterns[8]));
    ASSERT_TRUE(checkBarPattern(TPrice(107), TPrice(101), TPrice(104), TPrice(104), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 14, 14, 14, 14, tw::common_trade::ePatternType::kTransitional, patterns[9]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(100), TPrice(102), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 15, 15, 15, 15, tw::common_trade::ePatternType::kTransitional, patterns[10]));
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(97), TPrice(101), TPrice(99), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 16, 17, 16, 17, tw::common_trade::ePatternType::kSwing, patterns[11]));
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(99), TPrice(101), TPrice(100), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 18, 18, 18, 18, tw::common_trade::ePatternType::kHitch, patterns[12]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(98), TPrice(101), TPrice(102), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 1, 5, 4, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));    
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(99), TPrice(110), TPrice(100), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 8, 10, 8, 10, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(109), TPrice(99), TPrice(104), TPrice(102), TSize(24), 24, tw::common_trade::ePatternDir::kUp, 10, 15, 12, 10, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[2]));    
    ASSERT_TRUE(checkBarPattern(TPrice(109), TPrice(97), TPrice(104), TPrice(100), TSize(28), 28, tw::common_trade::ePatternDir::kDown, 12, 18, 12, 17, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[3]));

    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(98), TPrice(101), TPrice(100), TSize(60), 60, tw::common_trade::ePatternDir::kUp, 1, 16, 8, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(97), TPrice(110), TPrice(100), TSize(44), 44, tw::common_trade::ePatternDir::kDown, 8, 18, 8, 17, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[1]));
    
        
    // Bar 19 - transition bar
    //
    formBar(TPrice(104), TPrice(98), TPrice(102), TPrice(102), quoteNQM2, barsManager);
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);

    ASSERT_EQ(bars.size(), 20UL);
    ASSERT_EQ(patterns.size(), 14UL);
    
    ASSERT_EQ(patternsSwingLevel1.size(), 4UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 2UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(100), TPrice(101), TPrice(105), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 2, 4, 4, 2, tw::common_trade::ePatternType::kSwing, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(100), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(97), TPrice(103), TPrice(104), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 6, 6, 6, 6, tw::common_trade::ePatternType::kHitch, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 7, 7, 7, 7, tw::common_trade::ePatternType::kTransitional, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(108), TPrice(110), TPrice(109), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 8, 8, 8, 8, tw::common_trade::ePatternType::kHitch, patterns[5]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(99), TPrice(105), TPrice(100), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 9, 10, 9, 10, tw::common_trade::ePatternType::kSwing, patterns[6]));
    ASSERT_TRUE(checkBarPattern(TPrice(109), TPrice(100), TPrice(101), TPrice(108), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 11, 12, 12, 11, tw::common_trade::ePatternType::kSwing, patterns[7]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(102), TPrice(103), TPrice(105), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 13, 13, 13, 13, tw::common_trade::ePatternType::kHitch, patterns[8]));
    ASSERT_TRUE(checkBarPattern(TPrice(107), TPrice(101), TPrice(104), TPrice(104), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 14, 14, 14, 14, tw::common_trade::ePatternType::kTransitional, patterns[9]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(100), TPrice(102), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 15, 15, 15, 15, tw::common_trade::ePatternType::kTransitional, patterns[10]));
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(97), TPrice(101), TPrice(99), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 16, 17, 16, 17, tw::common_trade::ePatternType::kSwing, patterns[11]));
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(99), TPrice(101), TPrice(100), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 18, 18, 18, 18, tw::common_trade::ePatternType::kHitch, patterns[12]));
    ASSERT_TRUE(checkBarPattern(TPrice(104), TPrice(98), TPrice(102), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 19, 19, 19, 19, tw::common_trade::ePatternType::kTransitional, patterns[13]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(98), TPrice(101), TPrice(102), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 1, 5, 4, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));    
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(99), TPrice(110), TPrice(100), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 8, 10, 8, 10, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(109), TPrice(99), TPrice(104), TPrice(102), TSize(24), 24, tw::common_trade::ePatternDir::kUp, 10, 15, 12, 10, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[2]));    
    ASSERT_TRUE(checkBarPattern(TPrice(109), TPrice(97), TPrice(104), TPrice(102), TSize(32), 32, tw::common_trade::ePatternDir::kDown, 12, 19, 12, 17, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[3]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(98), TPrice(101), TPrice(100), TSize(60), 60, tw::common_trade::ePatternDir::kUp, 1, 16, 8, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(97), TPrice(110), TPrice(102), TSize(48), 48, tw::common_trade::ePatternDir::kDown, 8, 19, 8, 17, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[1]));

       
    // Bar 20 - transition bar
    //
    formBar(TPrice(105), TPrice(102), TPrice(103), TPrice(103), quoteNQM2, barsManager);
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);

    ASSERT_EQ(bars.size(), 21UL);
    ASSERT_EQ(patterns.size(), 15UL);
    
    ASSERT_EQ(patternsSwingLevel1.size(), 4UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 2UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(100), TPrice(101), TPrice(105), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 2, 4, 4, 2, tw::common_trade::ePatternType::kSwing, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(100), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(97), TPrice(103), TPrice(104), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 6, 6, 6, 6, tw::common_trade::ePatternType::kHitch, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 7, 7, 7, 7, tw::common_trade::ePatternType::kTransitional, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(108), TPrice(110), TPrice(109), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 8, 8, 8, 8, tw::common_trade::ePatternType::kHitch, patterns[5]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(99), TPrice(105), TPrice(100), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 9, 10, 9, 10, tw::common_trade::ePatternType::kSwing, patterns[6]));
    ASSERT_TRUE(checkBarPattern(TPrice(109), TPrice(100), TPrice(101), TPrice(108), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 11, 12, 12, 11, tw::common_trade::ePatternType::kSwing, patterns[7]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(102), TPrice(103), TPrice(105), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 13, 13, 13, 13, tw::common_trade::ePatternType::kHitch, patterns[8]));
    ASSERT_TRUE(checkBarPattern(TPrice(107), TPrice(101), TPrice(104), TPrice(104), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 14, 14, 14, 14, tw::common_trade::ePatternType::kTransitional, patterns[9]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(100), TPrice(102), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 15, 15, 15, 15, tw::common_trade::ePatternType::kTransitional, patterns[10]));
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(97), TPrice(101), TPrice(99), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 16, 17, 16, 17, tw::common_trade::ePatternType::kSwing, patterns[11]));
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(99), TPrice(101), TPrice(100), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 18, 18, 18, 18, tw::common_trade::ePatternType::kHitch, patterns[12]));
    ASSERT_TRUE(checkBarPattern(TPrice(104), TPrice(98), TPrice(102), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 19, 19, 19, 19, tw::common_trade::ePatternType::kTransitional, patterns[13]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(102), TPrice(103), TPrice(103), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 20, 20, 20, 20, tw::common_trade::ePatternType::kTransitional, patterns[14]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(98), TPrice(101), TPrice(102), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 1, 5, 4, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));    
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(99), TPrice(110), TPrice(100), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 8, 10, 8, 10, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(109), TPrice(99), TPrice(104), TPrice(102), TSize(24), 24, tw::common_trade::ePatternDir::kUp, 10, 15, 12, 10, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[2]));    
    ASSERT_TRUE(checkBarPattern(TPrice(109), TPrice(97), TPrice(104), TPrice(103), TSize(36), 36, tw::common_trade::ePatternDir::kDown, 12, 20, 12, 17, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[3]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(98), TPrice(101), TPrice(100), TSize(60), 60, tw::common_trade::ePatternDir::kUp, 1, 16, 8, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(97), TPrice(110), TPrice(103), TSize(52), 52, tw::common_trade::ePatternDir::kDown, 8, 20, 8, 17, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[1]));

    
    // Bar 21 - swing bar up
    //
    formBar(TPrice(107), TPrice(103), TPrice(105), TPrice(106), quoteNQM2, barsManager);
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);

    ASSERT_EQ(bars.size(), 22UL);
    
    ASSERT_EQ(bars[0]._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bars[0]._atr, 5.0, EPSILON);
    
    ASSERT_EQ(bars[1]._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bars[1]._atr, 4.5, EPSILON);
    
    ASSERT_EQ(bars[2]._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bars[2]._atr, 4.3333, EPSILON);
    
    ASSERT_EQ(bars[3]._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bars[3]._atr, 4.25, EPSILON);
    
    ASSERT_EQ(bars[4]._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bars[4]._atr, 4.75, EPSILON);
    
    ASSERT_EQ(bars[5]._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bars[5]._atr, 5.75, EPSILON);
    
    ASSERT_EQ(bars[6]._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bars[6]._atr, 6.25, EPSILON);
    
    ASSERT_EQ(bars[7]._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bars[7]._atr, 8.0, EPSILON);
    
    ASSERT_EQ(bars[8]._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bars[8]._atr, 8.5, EPSILON);
    
    ASSERT_EQ(bars[9]._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bars[9]._atr, 8.0, EPSILON);
    
    ASSERT_EQ(bars[10]._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bars[10]._atr, 8.25, EPSILON);
    
    ASSERT_EQ(bars[11]._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bars[11]._atr, 7.0, EPSILON);
    
    ASSERT_EQ(bars[12]._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bars[12]._atr, 6.25, EPSILON);
    
    ASSERT_EQ(bars[13]._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bars[13]._atr, 6.25, EPSILON);
    
    ASSERT_EQ(bars[14]._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bars[14]._atr, 5.5, EPSILON);
    
    ASSERT_EQ(bars[15]._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bars[15]._atr, 4.75, EPSILON);
    
    ASSERT_EQ(bars[16]._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bars[16]._atr, 4.25, EPSILON);
    
    ASSERT_EQ(bars[17]._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bars[17]._atr, 3.5, EPSILON);
    
    ASSERT_EQ(bars[18]._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bars[18]._atr, 4.0, EPSILON);
    
    ASSERT_EQ(bars[19]._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bars[19]._atr, 4.0, EPSILON);
    
    ASSERT_EQ(bars[20]._atrNumOfPeriods, 4UL);
    ASSERT_NEAR(bars[20]._atr, 4.0, EPSILON);
    
    
    
    ASSERT_EQ(patterns.size(), 16UL);
    
    ASSERT_EQ(patternsSwingLevel1.size(), 5UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 2UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(100), TPrice(101), TPrice(105), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 2, 4, 4, 2, tw::common_trade::ePatternType::kSwing, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(100), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(97), TPrice(103), TPrice(104), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 6, 6, 6, 6, tw::common_trade::ePatternType::kHitch, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(98), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 7, 7, 7, 7, tw::common_trade::ePatternType::kTransitional, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(108), TPrice(110), TPrice(109), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 8, 8, 8, 8, tw::common_trade::ePatternType::kHitch, patterns[5]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(99), TPrice(105), TPrice(100), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 9, 10, 9, 10, tw::common_trade::ePatternType::kSwing, patterns[6]));
    ASSERT_TRUE(checkBarPattern(TPrice(109), TPrice(100), TPrice(101), TPrice(108), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 11, 12, 12, 11, tw::common_trade::ePatternType::kSwing, patterns[7]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(102), TPrice(103), TPrice(105), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 13, 13, 13, 13, tw::common_trade::ePatternType::kHitch, patterns[8]));
    ASSERT_TRUE(checkBarPattern(TPrice(107), TPrice(101), TPrice(104), TPrice(104), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 14, 14, 14, 14, tw::common_trade::ePatternType::kTransitional, patterns[9]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(100), TPrice(102), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 15, 15, 15, 15, tw::common_trade::ePatternType::kTransitional, patterns[10]));
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(97), TPrice(101), TPrice(99), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 16, 17, 16, 17, tw::common_trade::ePatternType::kSwing, patterns[11]));
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(99), TPrice(101), TPrice(100), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 18, 18, 18, 18, tw::common_trade::ePatternType::kHitch, patterns[12]));
    ASSERT_TRUE(checkBarPattern(TPrice(104), TPrice(98), TPrice(102), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 19, 19, 19, 19, tw::common_trade::ePatternType::kTransitional, patterns[13]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(102), TPrice(103), TPrice(103), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 20, 20, 20, 20, tw::common_trade::ePatternType::kTransitional, patterns[14]));
    ASSERT_TRUE(checkBarPattern(TPrice(107), TPrice(103), TPrice(105), TPrice(106), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 21, 21, 21, 21, tw::common_trade::ePatternType::kSwing, patterns[15]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(98), TPrice(101), TPrice(102), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 1, 5, 4, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));    
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(99), TPrice(110), TPrice(100), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 8, 10, 8, 10, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(109), TPrice(99), TPrice(104), TPrice(102), TSize(24), 24, tw::common_trade::ePatternDir::kUp, 10, 15, 12, 10, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[2]));    
    ASSERT_TRUE(checkBarPattern(TPrice(109), TPrice(97), TPrice(104), TPrice(103), TSize(36), 36, tw::common_trade::ePatternDir::kDown, 12, 20, 12, 17, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(107), TPrice(97), TPrice(100), TPrice(106), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 17, 21, 21, 17, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[4]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(98), TPrice(101), TPrice(100), TSize(60), 60, tw::common_trade::ePatternDir::kUp, 1, 16, 8, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(97), TPrice(110), TPrice(106), TSize(56), 56, tw::common_trade::ePatternDir::kDown, 8, 21, 8, 17, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[1]));
    
    // Check trend's swings
    //
    std::vector<uint32_t> swingIndexes;
    
    swingIndexes = patternsSwingLevel2[0]._trendSwingIndexes;
    ASSERT_EQ(swingIndexes.size(), 2UL);
    ASSERT_EQ(swingIndexes[0], 1UL);
    ASSERT_EQ(swingIndexes[1], 3UL);
    
    swingIndexes = patternsSwingLevel2[0]._counterTrendSwingIndexes;
    ASSERT_EQ(swingIndexes.size(), 2UL);
    ASSERT_EQ(swingIndexes[0], 2UL);
    ASSERT_EQ(swingIndexes[1], 4UL);
    
    swingIndexes = patternsSwingLevel2[1]._trendSwingIndexes;
    ASSERT_EQ(swingIndexes.size(), 2UL);
    ASSERT_EQ(swingIndexes[0], 2UL);
    ASSERT_EQ(swingIndexes[1], 4UL);
    
    swingIndexes = patternsSwingLevel2[1]._counterTrendSwingIndexes;
    ASSERT_EQ(swingIndexes.size(), 2UL);
    ASSERT_EQ(swingIndexes[0], 3UL);
    ASSERT_EQ(swingIndexes[1], 5UL);

}


TEST(CommonTradeLibTestSuit, bars_impl_test_pattern_processing2)
{
    TestRouter::TImpl barsManager;
    
    clearProcessors();    
    
    // Init quotes
    //
    tw::price::QuoteStore::TQuote quoteNQM2;        
    tw::instr::InstrumentPtr instrNQM2 = InstrHelper::getNQM2();    
    
    quoteNQM2.setInstrument(instrNQM2);    
    
    // Subscribe to NQM2
    //
    ASSERT_TRUE(barsManager.subscribe(instrNQM2->_keyId));
    const TBars& bars = barsManager.getBars(instrNQM2->_keyId);
    
    // Bar 1 - swing bar up
    //
    formBar(TPrice(105), TPrice(101), TPrice(102), TPrice(104), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    const TBarPatterns& patterns = barsManager.getBarPatternsSimple().getBarPatterns(instrNQM2->_keyId);
    const TBarPatterns& patternsSwingLevel1 = barsManager.getBarPatternsSwingLevel1().getBarPatterns(instrNQM2->_keyId);
    const TBarPatterns& patternsSwingLevel2 = barsManager.getBarPatternsSwingLevel2().getBarPatterns(instrNQM2->_keyId);
        
    ASSERT_EQ(bars.size(), 2UL);
    ASSERT_EQ(patterns.size(), 1UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 1UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(101), TPrice(102), TPrice(104), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 1, 1, 1, 1, tw::common_trade::ePatternType::kSwing, patterns[0]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(101), TPrice(102), TPrice(104), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 1, 1, 1, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(101), TPrice(102), TPrice(104), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 1, 1, 1, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    
    // Bar 2 - swing bar up
    //
    formBar(TPrice(106), TPrice(102), TPrice(103), TPrice(105), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
        
    ASSERT_EQ(bars.size(), 3UL);
    ASSERT_EQ(patterns.size(), 1UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 1UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(101), TPrice(102), TPrice(105), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kSwing, patterns[0]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(101), TPrice(102), TPrice(105), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(101), TPrice(102), TPrice(105), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    
    // Bar 3 - hitch bar up
    //
    formBar(TPrice(105), TPrice(98), TPrice(100), TPrice(103), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
        
    ASSERT_EQ(bars.size(), 4UL);
    ASSERT_EQ(patterns.size(), 2UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 1UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(101), TPrice(102), TPrice(105), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kSwing, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(98), TPrice(100), TPrice(103), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 3, 3, 3, 3, tw::common_trade::ePatternType::kHitch, patterns[1]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(101), TPrice(102), TPrice(105), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(101), TPrice(102), TPrice(105), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
 
    // Bar 4 - transitional bar
    //
    formBar(TPrice(104), TPrice(99), TPrice(101), TPrice(101), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
        
    ASSERT_EQ(bars.size(), 5UL);
    ASSERT_EQ(patterns.size(), 3UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 1UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(101), TPrice(102), TPrice(105), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kSwing, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(98), TPrice(100), TPrice(103), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 3, 3, 3, 3, tw::common_trade::ePatternType::kHitch, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(104), TPrice(99), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 4, 4, 4, 4, tw::common_trade::ePatternType::kTransitional, patterns[2]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(101), TPrice(102), TPrice(105), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(101), TPrice(102), TPrice(105), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));

    // Bar 5 - swing bar down
    //
    formBar(TPrice(103), TPrice(100), TPrice(102), TPrice(101), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
        
    ASSERT_EQ(bars.size(), 6UL);
    ASSERT_EQ(patterns.size(), 4UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 2UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 2UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(101), TPrice(102), TPrice(105), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kSwing, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(98), TPrice(100), TPrice(103), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 3, 3, 3, 3, tw::common_trade::ePatternType::kHitch, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(104), TPrice(99), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 4, 4, 4, 4, tw::common_trade::ePatternType::kTransitional, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(100), TPrice(102), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 5, 5, 5, 5, tw::common_trade::ePatternType::kSwing, patterns[3]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(101), TPrice(102), TPrice(105), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(104), TPrice(99), TPrice(101), TPrice(101), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 4, 5, 4, 4, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(101), TPrice(102), TPrice(105), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(100), TPrice(103), TPrice(101), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 2, 5, 2, 5, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[1]));

    // Bar 6 - swing bar down
    //
    formBar(TPrice(102), TPrice(99), TPrice(101), TPrice(100), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
        
    ASSERT_EQ(bars.size(), 7UL);
    ASSERT_EQ(patterns.size(), 4UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 2UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 2UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(101), TPrice(102), TPrice(105), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kSwing, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(98), TPrice(100), TPrice(103), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 3, 3, 3, 3, tw::common_trade::ePatternType::kHitch, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(104), TPrice(99), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 4, 4, 4, 4, tw::common_trade::ePatternType::kTransitional, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(99), TPrice(102), TPrice(100), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 5, 6, 5, 6, tw::common_trade::ePatternType::kSwing, patterns[3]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(101), TPrice(102), TPrice(105), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(104), TPrice(99), TPrice(101), TPrice(100), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 4, 6, 4, 4, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(101), TPrice(102), TPrice(105), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(99), TPrice(103), TPrice(100), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 2, 6, 2, 6, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[1]));

    // Bar 7 - hitch bar down
    //
    formBar(TPrice(106), TPrice(100), TPrice(105), TPrice(101), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
        
    ASSERT_EQ(bars.size(), 8UL);
    ASSERT_EQ(patterns.size(), 5UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 2UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 2UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(101), TPrice(102), TPrice(105), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kSwing, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(98), TPrice(100), TPrice(103), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 3, 3, 3, 3, tw::common_trade::ePatternType::kHitch, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(104), TPrice(99), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 4, 4, 4, 4, tw::common_trade::ePatternType::kTransitional, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(99), TPrice(102), TPrice(100), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 5, 6, 5, 6, tw::common_trade::ePatternType::kSwing, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(100), TPrice(105), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 7, 7, 7, 7, tw::common_trade::ePatternType::kHitch, patterns[4]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(101), TPrice(102), TPrice(105), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(104), TPrice(99), TPrice(101), TPrice(100), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 4, 6, 4, 4, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(101), TPrice(102), TPrice(105), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(99), TPrice(103), TPrice(101), TSize(16), 16, tw::common_trade::ePatternDir::kDown, 2, 7, 2, 6, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[1]));

    // Bar 8 - transitional bar
    //
    formBar(TPrice(105), TPrice(101), TPrice(103), TPrice(103), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
        
    ASSERT_EQ(bars.size(), 9UL);
    ASSERT_EQ(patterns.size(), 6UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 2UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 2UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(101), TPrice(102), TPrice(105), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kSwing, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(98), TPrice(100), TPrice(103), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 3, 3, 3, 3, tw::common_trade::ePatternType::kHitch, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(104), TPrice(99), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 4, 4, 4, 4, tw::common_trade::ePatternType::kTransitional, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(99), TPrice(102), TPrice(100), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 5, 6, 5, 6, tw::common_trade::ePatternType::kSwing, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(100), TPrice(105), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 7, 7, 7, 7, tw::common_trade::ePatternType::kHitch, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(101), TPrice(103), TPrice(103), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 8, 8, 8, 8, tw::common_trade::ePatternType::kTransitional, patterns[5]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(101), TPrice(102), TPrice(105), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(104), TPrice(99), TPrice(101), TPrice(100), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 4, 6, 4, 4, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(101), TPrice(102), TPrice(105), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(99), TPrice(103), TPrice(103), TSize(20), 20, tw::common_trade::ePatternDir::kDown, 2, 8, 2, 6, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[1]));

    // Bar 9 - swing bar up
    //
    formBar(TPrice(104), TPrice(101), TPrice(102), TPrice(103), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
        
    ASSERT_EQ(bars.size(), 10UL);
    ASSERT_EQ(patterns.size(), 7UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 3UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 2UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(101), TPrice(102), TPrice(105), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kSwing, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(98), TPrice(100), TPrice(103), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 3, 3, 3, 3, tw::common_trade::ePatternType::kHitch, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(104), TPrice(99), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 4, 4, 4, 4, tw::common_trade::ePatternType::kTransitional, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(99), TPrice(102), TPrice(100), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 5, 6, 5, 6, tw::common_trade::ePatternType::kSwing, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(100), TPrice(105), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 7, 7, 7, 7, tw::common_trade::ePatternType::kHitch, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(101), TPrice(103), TPrice(103), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 8, 8, 8, 8, tw::common_trade::ePatternType::kTransitional, patterns[5]));
    ASSERT_TRUE(checkBarPattern(TPrice(104), TPrice(101), TPrice(102), TPrice(103), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 9, 9, 9, 9, tw::common_trade::ePatternType::kSwing, patterns[6]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(101), TPrice(102), TPrice(105), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(104), TPrice(99), TPrice(101), TPrice(100), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 4, 6, 4, 4, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(105), TPrice(101), TPrice(103), TPrice(103), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 8, 9, 8, 8, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[2]));

    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(101), TPrice(102), TPrice(105), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(106), TPrice(99), TPrice(103), TPrice(103), TSize(24), 24, tw::common_trade::ePatternDir::kDown, 2, 9, 2, 6, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[1]));
    
    // Check trend's swings
    //
    std::vector<uint32_t> swingIndexes;
    
    swingIndexes = patternsSwingLevel2[0]._trendSwingIndexes;
    ASSERT_EQ(swingIndexes.size(), 1UL);
    ASSERT_EQ(swingIndexes[0], 1UL);
    
    swingIndexes = patternsSwingLevel2[0]._counterTrendSwingIndexes;
    ASSERT_EQ(swingIndexes.size(), 0UL);
    
    swingIndexes = patternsSwingLevel2[1]._trendSwingIndexes;
    ASSERT_EQ(swingIndexes.size(), 1UL);
    ASSERT_EQ(swingIndexes[0], 2UL);
    
    swingIndexes = patternsSwingLevel2[1]._counterTrendSwingIndexes;
    ASSERT_EQ(swingIndexes.size(), 1UL);
    ASSERT_EQ(swingIndexes[0], 3UL);
}

TEST(CommonTradeLibTestSuit, bars_impl_test_pattern_processing3)
{
    TestRouter::TImpl barsManager;
    
    clearProcessors();    
    
    // Init quotes
    //
    tw::price::QuoteStore::TQuote quoteNQM2;        
    tw::instr::InstrumentPtr instrNQM2 = InstrHelper::getNQM2();    
    
    quoteNQM2.setInstrument(instrNQM2);    
    
    // Subscribe to NQM2
    //
    ASSERT_TRUE(barsManager.subscribe(instrNQM2->_keyId));
    const TBars& bars = barsManager.getBars(instrNQM2->_keyId);
    
    // Bar 1 - swing bar down
    //
    formBar(TPrice(102), TPrice(98), TPrice(101), TPrice(99), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    const TBarPatterns& patterns = barsManager.getBarPatternsSimple().getBarPatterns(instrNQM2->_keyId);
    const TBarPatterns& patternsSwingLevel1 = barsManager.getBarPatternsSwingLevel1().getBarPatterns(instrNQM2->_keyId);
    
    const TBarPatterns& patternsSwingLevel2 = barsManager.getBarPatternsSwingLevel2().getBarPatterns(instrNQM2->_keyId);
        
    ASSERT_EQ(bars.size(), 2UL);
    ASSERT_EQ(patterns.size(), 1UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 1UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(98), TPrice(101), TPrice(99), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 1, 1, 1, 1, tw::common_trade::ePatternType::kSwing, patterns[0]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(98), TPrice(101), TPrice(99), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 1, 1, 1, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(98), TPrice(101), TPrice(99), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    
    // Bar 2 - swing bar down
    //
    formBar(TPrice(101), TPrice(97), TPrice(100), TPrice(98), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
        
    ASSERT_EQ(bars.size(), 3UL);
    ASSERT_EQ(patterns.size(), 1UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 1UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(97), TPrice(101), TPrice(98), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 1, 2, 1, 2, tw::common_trade::ePatternType::kSwing, patterns[0]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(97), TPrice(101), TPrice(98), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 1, 2, 1, 2, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(97), TPrice(101), TPrice(98), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 1, 2, 1, 2, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));

    // Bar 3 - hitch bar down
    //
    formBar(TPrice(103), TPrice(99), TPrice(100), TPrice(101), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
        
    ASSERT_EQ(bars.size(), 4UL);
    ASSERT_EQ(patterns.size(), 2UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 1UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(97), TPrice(101), TPrice(98), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 1, 2, 1, 2, tw::common_trade::ePatternType::kSwing, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(99), TPrice(100), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 3, 3, 3, 3, tw::common_trade::ePatternType::kHitch, patterns[1]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(97), TPrice(101), TPrice(98), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 1, 2, 1, 2, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(97), TPrice(101), TPrice(98), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 1, 2, 1, 2, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));

    
    // Bar 4 - transitional bar
    //
    formBar(TPrice(102), TPrice(100), TPrice(101), TPrice(101), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
        
    ASSERT_EQ(bars.size(), 5UL);
    ASSERT_EQ(patterns.size(), 3UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 1UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(97), TPrice(101), TPrice(98), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 1, 2, 1, 2, tw::common_trade::ePatternType::kSwing, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(99), TPrice(100), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 3, 3, 3, 3, tw::common_trade::ePatternType::kHitch, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(100), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 4, 4, 4, 4, tw::common_trade::ePatternType::kTransitional, patterns[2]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(97), TPrice(101), TPrice(98), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 1, 2, 1, 2, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(97), TPrice(101), TPrice(101), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 1, 4, 1, 2, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));

    
    // Bar 5 - hitch bar up
    //
    formBar(TPrice(96), TPrice(92), TPrice(93), TPrice(95), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
        
    ASSERT_EQ(bars.size(), 6UL);
    ASSERT_EQ(patterns.size(), 4UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 1UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(97), TPrice(101), TPrice(98), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 1, 2, 1, 2, tw::common_trade::ePatternType::kSwing, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(99), TPrice(100), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 3, 3, 3, 3, tw::common_trade::ePatternType::kHitch, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(100), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 4, 4, 4, 4, tw::common_trade::ePatternType::kTransitional, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(96), TPrice(92), TPrice(93), TPrice(95), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[3]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(97), TPrice(101), TPrice(98), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 1, 2, 1, 2, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(92), TPrice(101), TPrice(95), TSize(16), 16, tw::common_trade::ePatternDir::kDown, 1, 5, 1, 5, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    
    // Bar 6 - swing bar up
    //
    formBar(TPrice(103), TPrice(93), TPrice(94), TPrice(102), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
        
    ASSERT_EQ(bars.size(), 7UL);
    ASSERT_EQ(patterns.size(), 5UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 2UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 2UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(97), TPrice(101), TPrice(98), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 1, 2, 1, 2, tw::common_trade::ePatternType::kSwing, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(99), TPrice(100), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 3, 3, 3, 3, tw::common_trade::ePatternType::kHitch, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(100), TPrice(101), TPrice(101), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 4, 4, 4, 4, tw::common_trade::ePatternType::kTransitional, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(96), TPrice(92), TPrice(93), TPrice(95), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(93), TPrice(94), TPrice(102), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 6, 6, 6, 6, tw::common_trade::ePatternType::kSwing, patterns[4]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(97), TPrice(101), TPrice(98), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 1, 2, 1, 2, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(92), TPrice(93), TPrice(102), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 5, 6, 6, 5, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(102), TPrice(92), TPrice(101), TPrice(95), TSize(16), 16, tw::common_trade::ePatternDir::kDown, 1, 5, 1, 5, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(103), TPrice(92), TPrice(93), TPrice(102), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 5, 6, 6, 5, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[1]));
    
    // Check trend's swings
    //
    std::vector<uint32_t> swingIndexes;
    
    swingIndexes = patternsSwingLevel2[0]._trendSwingIndexes;
    ASSERT_EQ(swingIndexes.size(), 1UL);
    ASSERT_EQ(swingIndexes[0], 1UL);
    
    swingIndexes = patternsSwingLevel2[0]._counterTrendSwingIndexes;
    ASSERT_EQ(swingIndexes.size(), 0UL);
    
    swingIndexes = patternsSwingLevel2[1]._trendSwingIndexes;
    ASSERT_EQ(swingIndexes.size(), 1UL);
    ASSERT_EQ(swingIndexes[0], 2UL);
    
    swingIndexes = patternsSwingLevel2[1]._counterTrendSwingIndexes;
    ASSERT_EQ(swingIndexes.size(), 0UL);
    
}


TEST(CommonTradeLibTestSuit, bars_impl_test_pattern_processing4)
{
    TestRouter::TImpl barsManager;
    
    clearProcessors();    
    
    // Init quotes
    //
    tw::price::QuoteStore::TQuote quoteNQM2;        
    tw::instr::InstrumentPtr instrNQM2 = InstrHelper::getNQM2();    
    
    quoteNQM2.setInstrument(instrNQM2);    
    
    // Subscribe to NQM2
    //
    ASSERT_TRUE(barsManager.subscribe(instrNQM2->_keyId));
    const TBars& bars = barsManager.getBars(instrNQM2->_keyId);
    
    // Bar 1 - swing bar up
    //
    formBar(TPrice(99), TPrice(96), TPrice(97), TPrice(98), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    const TBarPatterns& patterns = barsManager.getBarPatternsSimple().getBarPatterns(instrNQM2->_keyId);
    const TBarPatterns& patternsSwingLevel1 = barsManager.getBarPatternsSwingLevel1().getBarPatterns(instrNQM2->_keyId);
    
    const TBarPatterns& patternsSwingLevel2 = barsManager.getBarPatternsSwingLevel2().getBarPatterns(instrNQM2->_keyId);
        
    ASSERT_EQ(bars.size(), 2UL);
    ASSERT_EQ(patterns.size(), 1UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 1UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(99), TPrice(96), TPrice(97), TPrice(98), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 1, 1, 1, 1, tw::common_trade::ePatternType::kSwing, patterns[0]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(99), TPrice(96), TPrice(97), TPrice(98), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 1, 1, 1, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(99), TPrice(96), TPrice(97), TPrice(98), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 1, 1, 1, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    
    // Bar 2 - swing bar up
    //
    formBar(TPrice(100), TPrice(97), TPrice(98), TPrice(99), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
        
    ASSERT_EQ(bars.size(), 3UL);
    ASSERT_EQ(patterns.size(), 1UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 1UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(100), TPrice(96), TPrice(97), TPrice(99), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kSwing, patterns[0]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(100), TPrice(96), TPrice(97), TPrice(99), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(100), TPrice(96), TPrice(97), TPrice(99), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    

    // Bar 3 - swing bar up
    //
    formBar(TPrice(101), TPrice(98), TPrice(99), TPrice(100), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
        
    ASSERT_EQ(bars.size(), 4UL);
    ASSERT_EQ(patterns.size(), 1UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 1UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(101), TPrice(96), TPrice(97), TPrice(100), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 1, 3, 3, 1, tw::common_trade::ePatternType::kSwing, patterns[0]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(101), TPrice(96), TPrice(97), TPrice(100), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 1, 3, 3, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(101), TPrice(96), TPrice(97), TPrice(100), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 1, 3, 3, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));

    
    // Bar 4 - hitch bar down
    //
    formBar(TPrice(100), TPrice(97), TPrice(99), TPrice(98), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
        
    ASSERT_EQ(bars.size(), 5UL);
    ASSERT_EQ(patterns.size(), 2UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 1UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(101), TPrice(96), TPrice(97), TPrice(100), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 1, 3, 3, 1, tw::common_trade::ePatternType::kSwing, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(100), TPrice(97), TPrice(99), TPrice(98), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 4, 4, 4, 4, tw::common_trade::ePatternType::kHitch, patterns[1]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(101), TPrice(96), TPrice(97), TPrice(98), TSize(16), 16, tw::common_trade::ePatternDir::kUp, 1, 4, 3, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(101), TPrice(96), TPrice(97), TPrice(98), TSize(16), 16, tw::common_trade::ePatternDir::kUp, 1, 4, 3, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));

    // Bar 5 - swing bar down
    //
    formBar(TPrice(99), TPrice(96), TPrice(98), TPrice(97), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
        
    ASSERT_EQ(bars.size(), 6UL);
    ASSERT_EQ(patterns.size(), 3UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 2UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(101), TPrice(96), TPrice(97), TPrice(100), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 1, 3, 3, 1, tw::common_trade::ePatternType::kSwing, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(100), TPrice(97), TPrice(99), TPrice(98), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 4, 4, 4, 4, tw::common_trade::ePatternType::kHitch, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(99), TPrice(96), TPrice(98), TPrice(97), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 5, 5, 5, 5, tw::common_trade::ePatternType::kSwing, patterns[2]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(101), TPrice(96), TPrice(97), TPrice(98), TSize(16), 16, tw::common_trade::ePatternDir::kUp, 1, 4, 3, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(101), TPrice(96), TPrice(99), TPrice(97), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 3, 5, 3, 5, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(101), TPrice(96), TPrice(97), TPrice(97), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 1, 5, 3, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));

    // Bar 6 - swing bar down
    //
    formBar(TPrice(98), TPrice(95), TPrice(97), TPrice(96), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
        
    ASSERT_EQ(bars.size(), 7UL);
    ASSERT_EQ(patterns.size(), 3UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 2UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 2UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(101), TPrice(96), TPrice(97), TPrice(100), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 1, 3, 3, 1, tw::common_trade::ePatternType::kSwing, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(100), TPrice(97), TPrice(99), TPrice(98), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 4, 4, 4, 4, tw::common_trade::ePatternType::kHitch, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(99), TPrice(95), TPrice(98), TPrice(96), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 5, 6, 5, 6, tw::common_trade::ePatternType::kSwing, patterns[2]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(101), TPrice(96), TPrice(97), TPrice(98), TSize(16), 16, tw::common_trade::ePatternDir::kUp, 1, 4, 3, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(101), TPrice(95), TPrice(99), TPrice(96), TSize(16), 16, tw::common_trade::ePatternDir::kDown, 3, 6, 3, 6, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(101), TPrice(96), TPrice(97), TPrice(97), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 1, 5, 3, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(101), TPrice(95), TPrice(99), TPrice(96), TSize(16), 16, tw::common_trade::ePatternDir::kDown, 3, 6, 3, 6, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[1]));

    // Bar 7 - hitch bar up
    //
    formBar(TPrice(99), TPrice(96), TPrice(97), TPrice(98), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
        
    ASSERT_EQ(bars.size(), 8UL);
    ASSERT_EQ(patterns.size(), 4UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 2UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 2UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(101), TPrice(96), TPrice(97), TPrice(100), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 1, 3, 3, 1, tw::common_trade::ePatternType::kSwing, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(100), TPrice(97), TPrice(99), TPrice(98), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 4, 4, 4, 4, tw::common_trade::ePatternType::kHitch, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(99), TPrice(95), TPrice(98), TPrice(96), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 5, 6, 5, 6, tw::common_trade::ePatternType::kSwing, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(99), TPrice(96), TPrice(97), TPrice(98), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 7, 7, 7, 7, tw::common_trade::ePatternType::kHitch, patterns[3]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(101), TPrice(96), TPrice(97), TPrice(98), TSize(16), 16, tw::common_trade::ePatternDir::kUp, 1, 4, 3, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(101), TPrice(95), TPrice(99), TPrice(98), TSize(20), 20, tw::common_trade::ePatternDir::kDown, 3, 7, 3, 6, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(101), TPrice(96), TPrice(97), TPrice(97), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 1, 5, 3, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(101), TPrice(95), TPrice(99), TPrice(98), TSize(20), 20, tw::common_trade::ePatternDir::kDown, 3, 7, 3, 6, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[1]));

    // Bar 8 - swing bar up
    //
    formBar(TPrice(100), TPrice(97), TPrice(98), TPrice(99), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
        
    ASSERT_EQ(bars.size(), 9UL);
    ASSERT_EQ(patterns.size(), 5UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 3UL);
    
    ASSERT_EQ(patternsSwingLevel2.size(), 2UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(101), TPrice(96), TPrice(97), TPrice(100), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 1, 3, 3, 1, tw::common_trade::ePatternType::kSwing, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(100), TPrice(97), TPrice(99), TPrice(98), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 4, 4, 4, 4, tw::common_trade::ePatternType::kHitch, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(99), TPrice(95), TPrice(98), TPrice(96), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 5, 6, 5, 6, tw::common_trade::ePatternType::kSwing, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(99), TPrice(96), TPrice(97), TPrice(98), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 7, 7, 7, 7, tw::common_trade::ePatternType::kHitch, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(100), TPrice(97), TPrice(98), TPrice(99), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 8, 8, 8, 8, tw::common_trade::ePatternType::kSwing, patterns[4]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(101), TPrice(96), TPrice(97), TPrice(98), TSize(16), 16, tw::common_trade::ePatternDir::kUp, 1, 4, 3, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(101), TPrice(95), TPrice(99), TPrice(98), TSize(20), 20, tw::common_trade::ePatternDir::kDown, 3, 7, 3, 6, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(100), TPrice(95), TPrice(97), TPrice(99), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 6, 8, 8, 6, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[2]));

    ASSERT_TRUE(checkBarPattern(TPrice(101), TPrice(96), TPrice(97), TPrice(97), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 1, 5, 3, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(101), TPrice(95), TPrice(99), TPrice(99), TSize(24), 24, tw::common_trade::ePatternDir::kDown, 3, 8, 3, 6, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[1]));
    
    // Check trend's swings
    //
    std::vector<uint32_t> swingIndexes;
    
    swingIndexes = patternsSwingLevel2[0]._trendSwingIndexes;
    ASSERT_EQ(swingIndexes.size(), 1UL);
    ASSERT_EQ(swingIndexes[0], 1UL);
    
    swingIndexes = patternsSwingLevel2[0]._counterTrendSwingIndexes;
    ASSERT_EQ(swingIndexes.size(), 1UL);
    ASSERT_EQ(swingIndexes[0], 2UL);
    
    swingIndexes = patternsSwingLevel2[1]._trendSwingIndexes;
    ASSERT_EQ(swingIndexes.size(), 1UL);
    ASSERT_EQ(swingIndexes[0], 2UL);
    
    swingIndexes = patternsSwingLevel2[1]._counterTrendSwingIndexes;
    ASSERT_EQ(swingIndexes.size(), 1UL);
    ASSERT_EQ(swingIndexes[0], 3UL);
    
}


TEST(CommonTradeLibTestSuit, bars_impl_test_pattern_processing_hitch_swing_up)
{
    TestRouter::TImpl barsManager(60, 1);
    
    clearProcessors();    
    
    // Init quotes
    //
    tw::price::QuoteStore::TQuote quoteNQM2;        
    tw::instr::InstrumentPtr instrNQM2 = InstrHelper::getNQM2();    
    
    quoteNQM2.setInstrument(instrNQM2);    
    
    // Subscribe to NQM2
    //
    ASSERT_TRUE(barsManager.subscribe(instrNQM2->_keyId));
    const TBars& bars = barsManager.getBars(instrNQM2->_keyId);
    
    formBar(TPrice(110), TPrice(100), TPrice(100), TPrice(110), quoteNQM2, barsManager, true);    
    formBar(TPrice(115), TPrice(105), TPrice(105), TPrice(115), quoteNQM2, barsManager, true);    
    formBar(TPrice(110), TPrice(108), TPrice(109), TPrice(109), quoteNQM2, barsManager, true);    
    formBar(TPrice(112), TPrice(108), TPrice(112), TPrice(108), quoteNQM2, barsManager, true);    
    formBar(TPrice(120), TPrice(109), TPrice(109), TPrice(120), quoteNQM2, barsManager, true);    
    formBar(TPrice(125), TPrice(115), TPrice(115), TPrice(125), quoteNQM2, barsManager, true);    
    formBar(TPrice(124), TPrice(110), TPrice(120), TPrice(124), quoteNQM2, barsManager, true);    
    formBar(TPrice(130), TPrice(120), TPrice(120), TPrice(130), quoteNQM2, barsManager, true);    
    formBar(TPrice(135), TPrice(125), TPrice(125), TPrice(135), quoteNQM2, barsManager, true);
    
    const TBarPatterns& patterns = barsManager.getBarPatternsSimple().getBarPatterns(instrNQM2->_keyId);
    const TBarPatterns& patternsSwingLevel1 = barsManager.getBarPatternsSwingLevel1().getBarPatterns(instrNQM2->_keyId);
    const TBarPatterns& patternsSwingLevel2 = barsManager.getBarPatternsSwingLevel2().getBarPatterns(instrNQM2->_keyId);
        
    ASSERT_EQ(bars.size(), 10UL);
    ASSERT_EQ(patterns.size(), 6UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 3UL); 
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL); 
    
    
    ASSERT_TRUE(checkBarPattern(TPrice(115), TPrice(100), TPrice(100), TPrice(115), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 1, 2, 2, 1, tw::common_trade::ePatternType::kSwing, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(110), TPrice(108), TPrice(109), TPrice(109), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 3, 3, 3, 3, tw::common_trade::ePatternType::kTransitional, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(112), TPrice(108), TPrice(112), TPrice(108), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 4, 4, 4, 4, tw::common_trade::ePatternType::kHitch, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(125), TPrice(109), TPrice(109), TPrice(125), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 5, 6, 6, 5, tw::common_trade::ePatternType::kSwing, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(124), TPrice(110), TPrice(120), TPrice(124), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 7, 7, 7, 7, tw::common_trade::ePatternType::kHitch, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(135), TPrice(120), TPrice(120), TPrice(135), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 8, 9, 9, 8, tw::common_trade::ePatternType::kSwing, patterns[5]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(115), TPrice(100), TPrice(100), TPrice(109), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 1, 3, 2, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(125), TPrice(108), TPrice(109), TPrice(124), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 3, 7, 6, 3, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(135), TPrice(110), TPrice(120), TPrice(135), TSize(12), 12, tw::common_trade::ePatternDir::kUp, 7, 9, 9, 7, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[2]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(135), TPrice(100), TPrice(100), TPrice(135), TSize(36), 36, tw::common_trade::ePatternDir::kUp, 1, 9, 9, 1, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    
    
}

TEST(CommonTradeLibTestSuit, bars_impl_test_pattern_processing_hitch_swing_down)
{
    TestRouter::TImpl barsManager(60, 1);
    
    clearProcessors();    
    
    // Init quotes
    //
    tw::price::QuoteStore::TQuote quoteNQM2;        
    tw::instr::InstrumentPtr instrNQM2 = InstrHelper::getNQM2();    
    
    quoteNQM2.setInstrument(instrNQM2);    
    
    // Subscribe to NQM2
    //
    ASSERT_TRUE(barsManager.subscribe(instrNQM2->_keyId));
    const TBars& bars = barsManager.getBars(instrNQM2->_keyId);
    
    // Bar 1
    //
    formBar(TPrice(170), TPrice(160), TPrice(170), TPrice(160), quoteNQM2, barsManager, true);
    formBar(TPrice(165), TPrice(155), TPrice(165), TPrice(155), quoteNQM2, barsManager, true);    
    formBar(TPrice(162), TPrice(160), TPrice(161), TPrice(161), quoteNQM2, barsManager, true);    
    formBar(TPrice(162), TPrice(158), TPrice(158), TPrice(162), quoteNQM2, barsManager, true);    
    formBar(TPrice(161), TPrice(150), TPrice(161), TPrice(150), quoteNQM2, barsManager, true);    
    formBar(TPrice(155), TPrice(145), TPrice(155), TPrice(145), quoteNQM2, barsManager, true);    
    formBar(TPrice(160), TPrice(146), TPrice(150), TPrice(146), quoteNQM2, barsManager, true);    
    formBar(TPrice(150), TPrice(140), TPrice(150), TPrice(140), quoteNQM2, barsManager, true);    
    formBar(TPrice(145), TPrice(135), TPrice(145), TPrice(135), quoteNQM2, barsManager, true);    
    
    const TBarPatterns& patterns = barsManager.getBarPatternsSimple().getBarPatterns(instrNQM2->_keyId);
    const TBarPatterns& patternsSwingLevel1 = barsManager.getBarPatternsSwingLevel1().getBarPatterns(instrNQM2->_keyId);
    const TBarPatterns& patternsSwingLevel2 = barsManager.getBarPatternsSwingLevel2().getBarPatterns(instrNQM2->_keyId);
        
    ASSERT_EQ(bars.size(), 10UL);
    ASSERT_EQ(patterns.size(), 6UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 3UL); 
    ASSERT_EQ(patternsSwingLevel2.size(), 1UL); 
    
    
    ASSERT_TRUE(checkBarPattern(TPrice(170), TPrice(155), TPrice(170), TPrice(155), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 1, 2, 1, 2, tw::common_trade::ePatternType::kSwing, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(162), TPrice(160), TPrice(161), TPrice(161), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 3, 3, 3, 3, tw::common_trade::ePatternType::kTransitional, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(162), TPrice(158), TPrice(158), TPrice(162), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 4, 4, 4, 4, tw::common_trade::ePatternType::kHitch, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(161), TPrice(145), TPrice(161), TPrice(145), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 5, 6, 5, 6, tw::common_trade::ePatternType::kSwing, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(160), TPrice(146), TPrice(150), TPrice(146), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 7, 7, 7, 7, tw::common_trade::ePatternType::kHitch, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(150), TPrice(135), TPrice(150), TPrice(135), TSize(8), 8, tw::common_trade::ePatternDir::kDown, 8, 9, 8, 9, tw::common_trade::ePatternType::kSwing, patterns[5]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(170), TPrice(155), TPrice(170), TPrice(161), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 1, 3, 1, 2, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(162), TPrice(145), TPrice(161), TPrice(146), TSize(20), 20, tw::common_trade::ePatternDir::kDown, 3, 7, 3, 6, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(160), TPrice(135), TPrice(150), TPrice(135), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 7, 9, 7, 9, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[2]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(170), TPrice(135), TPrice(170), TPrice(135), TSize(36), 36, tw::common_trade::ePatternDir::kDown, 1, 9, 1, 9, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[0]));
    
    
}

TEST(CommonTradeLibTestSuit, bars_impl_test_pattern_processing_bug_20130516_a)
{
    TestRouter::TImpl barsManager;
    
    clearProcessors();    
    
    // Init quotes
    //
    tw::price::QuoteStore::TQuote quoteNQM2;        
    tw::instr::InstrumentPtr instrNQM2 = InstrHelper::getNQM2();    
    
    quoteNQM2.setInstrument(instrNQM2);    
    
    // Subscribe to NQM2
    //
    ASSERT_TRUE(barsManager.subscribe(instrNQM2->_keyId));
    const TBars& bars = barsManager.getBars(instrNQM2->_keyId);
    
    // Bar 1
    //
    formBar(TPrice(9367), TPrice(9365), TPrice(9366), TPrice(9366), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    const TBarPatterns& patterns = barsManager.getBarPatternsSimple().getBarPatterns(instrNQM2->_keyId);
    const TBarPatterns& patternsSwingLevel1 = barsManager.getBarPatternsSwingLevel1().getBarPatterns(instrNQM2->_keyId);
        
    ASSERT_EQ(bars.size(), 2UL);
    ASSERT_EQ(patterns.size(), 1UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 0UL); 
    
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9366), TPrice(9366), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
        
    // Bar 2
    //
    formBar(TPrice(9368), TPrice(9367), TPrice(9367), TPrice(9367), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
        
    ASSERT_EQ(bars.size(), 3UL);
    ASSERT_EQ(patterns.size(), 2UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 0UL);
        
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9366), TPrice(9366), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9367), TPrice(9367), TPrice(9367), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 2, 2, 2, 2, tw::common_trade::ePatternType::kTransitional, patterns[1]));
    
    // Bar 3
    //
    formBar(TPrice(9367), TPrice(9365), TPrice(9367), TPrice(9365), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
        
    ASSERT_EQ(bars.size(), 4UL);
    ASSERT_EQ(patterns.size(), 3UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 1UL);
        
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9366), TPrice(9366), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9367), TPrice(9367), TPrice(9367), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 2, 2, 2, 2, tw::common_trade::ePatternType::kTransitional, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9367), TPrice(9365), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 3, 3, 3, 3, tw::common_trade::ePatternType::kSwing, patterns[2]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9365), TPrice(9366), TPrice(9365), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 1, 3, 2, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    
    // Bar 4
    //
    formBar(TPrice(9371), TPrice(9364), TPrice(9365), TPrice(9371), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
        
    ASSERT_EQ(bars.size(), 5UL);
    ASSERT_EQ(patterns.size(), 4UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 2UL);
        
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9366), TPrice(9366), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9367), TPrice(9367), TPrice(9367), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 2, 2, 2, 2, tw::common_trade::ePatternType::kTransitional, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9367), TPrice(9365), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 3, 3, 3, 3, tw::common_trade::ePatternType::kSwing, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9364), TPrice(9365), TPrice(9371), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 4, 4, 4, 4, tw::common_trade::ePatternType::kSwing, patterns[3]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9365), TPrice(9366), TPrice(9365), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 1, 3, 2, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9364), TPrice(9366), TPrice(9371), TSize(16), 16, tw::common_trade::ePatternDir::kUp, 1, 4, 4, 4, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    
    // Bar 5
    //
    formBar(TPrice(9371), TPrice(9365), TPrice(9370), TPrice(9366), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
        
    ASSERT_EQ(bars.size(), 6UL);
    ASSERT_EQ(patterns.size(), 5UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 2UL);
        
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9366), TPrice(9366), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9367), TPrice(9367), TPrice(9367), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 2, 2, 2, 2, tw::common_trade::ePatternType::kTransitional, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9367), TPrice(9365), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 3, 3, 3, 3, tw::common_trade::ePatternType::kSwing, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9364), TPrice(9365), TPrice(9371), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 4, 4, 4, 4, tw::common_trade::ePatternType::kSwing, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9365), TPrice(9370), TPrice(9366), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[4]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9365), TPrice(9366), TPrice(9365), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 1, 3, 2, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9364), TPrice(9366), TPrice(9366), TSize(20), 20, tw::common_trade::ePatternDir::kUp, 1, 5, 4, 4, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    
    
    // Bar 6
    //
    formBar(TPrice(9369), TPrice(9366), TPrice(9366), TPrice(9368), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
        
    ASSERT_EQ(bars.size(), 7UL);
    ASSERT_EQ(patterns.size(), 6UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 2UL);
        
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9366), TPrice(9366), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9367), TPrice(9367), TPrice(9367), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 2, 2, 2, 2, tw::common_trade::ePatternType::kTransitional, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9367), TPrice(9365), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 3, 3, 3, 3, tw::common_trade::ePatternType::kSwing, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9364), TPrice(9365), TPrice(9371), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 4, 4, 4, 4, tw::common_trade::ePatternType::kSwing, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9365), TPrice(9370), TPrice(9366), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(9369), TPrice(9366), TPrice(9366), TPrice(9368), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 6, 6, 6, 6, tw::common_trade::ePatternType::kHitch, patterns[5]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9365), TPrice(9366), TPrice(9365), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 1, 3, 2, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9364), TPrice(9366), TPrice(9368), TSize(24), 24, tw::common_trade::ePatternDir::kUp, 1, 6, 4, 4, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
        
    // Bar 7
    //
    formBar(TPrice(9369), TPrice(9367), TPrice(9367), TPrice(9367), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    ASSERT_EQ(bars.size(), 8UL);
    ASSERT_EQ(patterns.size(), 7UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 2UL);
        
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9366), TPrice(9366), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9367), TPrice(9367), TPrice(9367), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 2, 2, 2, 2, tw::common_trade::ePatternType::kTransitional, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9367), TPrice(9365), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 3, 3, 3, 3, tw::common_trade::ePatternType::kSwing, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9364), TPrice(9365), TPrice(9371), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 4, 4, 4, 4, tw::common_trade::ePatternType::kSwing, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9365), TPrice(9370), TPrice(9366), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(9369), TPrice(9366), TPrice(9366), TPrice(9368), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 6, 6, 6, 6, tw::common_trade::ePatternType::kHitch, patterns[5]));
    ASSERT_TRUE(checkBarPattern(TPrice(9369), TPrice(9367), TPrice(9367), TPrice(9367), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 7, 7, 7, 7, tw::common_trade::ePatternType::kTransitional, patterns[6]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9365), TPrice(9366), TPrice(9365), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 1, 3, 2, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9364), TPrice(9366), TPrice(9367), TSize(28), 28, tw::common_trade::ePatternDir::kUp, 1, 7, 4, 4, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    
    // Bar 8
    //
    formBar(TPrice(9368), TPrice(9367), TPrice(9368), TPrice(9368), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    ASSERT_EQ(bars.size(), 9UL);
    ASSERT_EQ(patterns.size(), 8UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 2UL);
        
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9366), TPrice(9366), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9367), TPrice(9367), TPrice(9367), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 2, 2, 2, 2, tw::common_trade::ePatternType::kTransitional, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9367), TPrice(9365), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 3, 3, 3, 3, tw::common_trade::ePatternType::kSwing, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9364), TPrice(9365), TPrice(9371), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 4, 4, 4, 4, tw::common_trade::ePatternType::kSwing, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9365), TPrice(9370), TPrice(9366), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(9369), TPrice(9366), TPrice(9366), TPrice(9368), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 6, 6, 6, 6, tw::common_trade::ePatternType::kHitch, patterns[5]));
    ASSERT_TRUE(checkBarPattern(TPrice(9369), TPrice(9367), TPrice(9367), TPrice(9367), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 7, 7, 7, 7, tw::common_trade::ePatternType::kTransitional, patterns[6]));
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9367), TPrice(9368), TPrice(9368), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 8, 8, 8, 8, tw::common_trade::ePatternType::kTransitional, patterns[7]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9365), TPrice(9366), TPrice(9365), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 1, 3, 2, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9364), TPrice(9366), TPrice(9368), TSize(32), 32, tw::common_trade::ePatternDir::kUp, 1, 8, 4, 4, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));

    // Bar 9
    //
    formBar(TPrice(9370), TPrice(9368), TPrice(9368), TPrice(9370), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    ASSERT_EQ(bars.size(), 10UL);
    ASSERT_EQ(patterns.size(), 9UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 2UL);
        
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9366), TPrice(9366), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9367), TPrice(9367), TPrice(9367), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 2, 2, 2, 2, tw::common_trade::ePatternType::kTransitional, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9367), TPrice(9365), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 3, 3, 3, 3, tw::common_trade::ePatternType::kSwing, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9364), TPrice(9365), TPrice(9371), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 4, 4, 4, 4, tw::common_trade::ePatternType::kSwing, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9365), TPrice(9370), TPrice(9366), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(9369), TPrice(9366), TPrice(9366), TPrice(9368), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 6, 6, 6, 6, tw::common_trade::ePatternType::kHitch, patterns[5]));
    ASSERT_TRUE(checkBarPattern(TPrice(9369), TPrice(9367), TPrice(9367), TPrice(9367), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 7, 7, 7, 7, tw::common_trade::ePatternType::kTransitional, patterns[6]));
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9367), TPrice(9368), TPrice(9368), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 8, 8, 8, 8, tw::common_trade::ePatternType::kTransitional, patterns[7]));
    ASSERT_TRUE(checkBarPattern(TPrice(9370), TPrice(9368), TPrice(9368), TPrice(9370), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 9, 9, 9, 9, tw::common_trade::ePatternType::kHitch, patterns[8]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9365), TPrice(9366), TPrice(9365), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 1, 3, 2, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9364), TPrice(9366), TPrice(9370), TSize(36), 36, tw::common_trade::ePatternDir::kUp, 1, 9, 4, 4, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));

    // Bar 10
    //
    formBar(TPrice(9374), TPrice(9369), TPrice(9369), TPrice(9373), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    ASSERT_EQ(bars.size(), 11UL);
    ASSERT_EQ(patterns.size(), 10UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 2UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9366), TPrice(9366), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9367), TPrice(9367), TPrice(9367), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 2, 2, 2, 2, tw::common_trade::ePatternType::kTransitional, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9367), TPrice(9365), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 3, 3, 3, 3, tw::common_trade::ePatternType::kSwing, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9364), TPrice(9365), TPrice(9371), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 4, 4, 4, 4, tw::common_trade::ePatternType::kSwing, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9365), TPrice(9370), TPrice(9366), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(9369), TPrice(9366), TPrice(9366), TPrice(9368), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 6, 6, 6, 6, tw::common_trade::ePatternType::kHitch, patterns[5]));
    ASSERT_TRUE(checkBarPattern(TPrice(9369), TPrice(9367), TPrice(9367), TPrice(9367), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 7, 7, 7, 7, tw::common_trade::ePatternType::kTransitional, patterns[6]));
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9367), TPrice(9368), TPrice(9368), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 8, 8, 8, 8, tw::common_trade::ePatternType::kTransitional, patterns[7]));
    ASSERT_TRUE(checkBarPattern(TPrice(9370), TPrice(9368), TPrice(9368), TPrice(9370), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 9, 9, 9, 9, tw::common_trade::ePatternType::kHitch, patterns[8]));
    ASSERT_TRUE(checkBarPattern(TPrice(9374), TPrice(9369), TPrice(9369), TPrice(9373), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 10, 10, 10, 10, tw::common_trade::ePatternType::kSwing, patterns[9]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9365), TPrice(9366), TPrice(9365), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 1, 3, 2, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(9374), TPrice(9364), TPrice(9366), TPrice(9373), TSize(40), 40, tw::common_trade::ePatternDir::kUp, 1, 10, 10, 4, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));

    
    // Bar 11
    //
    formBar(TPrice(9384), TPrice(9372), TPrice(9373), TPrice(9377), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    ASSERT_EQ(bars.size(), 12UL);
    ASSERT_EQ(patterns.size(), 10UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 2UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9366), TPrice(9366), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9367), TPrice(9367), TPrice(9367), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 2, 2, 2, 2, tw::common_trade::ePatternType::kTransitional, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9367), TPrice(9365), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 3, 3, 3, 3, tw::common_trade::ePatternType::kSwing, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9364), TPrice(9365), TPrice(9371), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 4, 4, 4, 4, tw::common_trade::ePatternType::kSwing, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9365), TPrice(9370), TPrice(9366), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(9369), TPrice(9366), TPrice(9366), TPrice(9368), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 6, 6, 6, 6, tw::common_trade::ePatternType::kHitch, patterns[5]));
    ASSERT_TRUE(checkBarPattern(TPrice(9369), TPrice(9367), TPrice(9367), TPrice(9367), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 7, 7, 7, 7, tw::common_trade::ePatternType::kTransitional, patterns[6]));
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9367), TPrice(9368), TPrice(9368), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 8, 8, 8, 8, tw::common_trade::ePatternType::kTransitional, patterns[7]));
    ASSERT_TRUE(checkBarPattern(TPrice(9370), TPrice(9368), TPrice(9368), TPrice(9370), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 9, 9, 9, 9, tw::common_trade::ePatternType::kHitch, patterns[8]));
    ASSERT_TRUE(checkBarPattern(TPrice(9384), TPrice(9369), TPrice(9369), TPrice(9377), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 10, 11, 11, 10, tw::common_trade::ePatternType::kSwing, patterns[9]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9365), TPrice(9366), TPrice(9365), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 1, 3, 2, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(9384), TPrice(9364), TPrice(9366), TPrice(9377), TSize(44), 44, tw::common_trade::ePatternDir::kUp, 1, 11, 11, 4, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    
    // Bar 12
    //
    formBar(TPrice(9380), TPrice(9376), TPrice(9377), TPrice(9379), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    ASSERT_EQ(bars.size(), 13UL);
    ASSERT_EQ(patterns.size(), 11UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 2UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9366), TPrice(9366), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9367), TPrice(9367), TPrice(9367), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 2, 2, 2, 2, tw::common_trade::ePatternType::kTransitional, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9367), TPrice(9365), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 3, 3, 3, 3, tw::common_trade::ePatternType::kSwing, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9364), TPrice(9365), TPrice(9371), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 4, 4, 4, 4, tw::common_trade::ePatternType::kSwing, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9365), TPrice(9370), TPrice(9366), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(9369), TPrice(9366), TPrice(9366), TPrice(9368), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 6, 6, 6, 6, tw::common_trade::ePatternType::kHitch, patterns[5]));
    ASSERT_TRUE(checkBarPattern(TPrice(9369), TPrice(9367), TPrice(9367), TPrice(9367), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 7, 7, 7, 7, tw::common_trade::ePatternType::kTransitional, patterns[6]));
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9367), TPrice(9368), TPrice(9368), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 8, 8, 8, 8, tw::common_trade::ePatternType::kTransitional, patterns[7]));
    ASSERT_TRUE(checkBarPattern(TPrice(9370), TPrice(9368), TPrice(9368), TPrice(9370), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 9, 9, 9, 9, tw::common_trade::ePatternType::kHitch, patterns[8]));
    ASSERT_TRUE(checkBarPattern(TPrice(9384), TPrice(9369), TPrice(9369), TPrice(9377), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 10, 11, 11, 10, tw::common_trade::ePatternType::kSwing, patterns[9]));
    ASSERT_TRUE(checkBarPattern(TPrice(9380), TPrice(9376), TPrice(9377), TPrice(9379), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 12, 12, 12, 12, tw::common_trade::ePatternType::kHitch, patterns[10]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9365), TPrice(9366), TPrice(9365), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 1, 3, 2, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(9384), TPrice(9364), TPrice(9366), TPrice(9379), TSize(48), 48, tw::common_trade::ePatternDir::kUp, 1, 12, 11, 4, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
 
    
    // Bar 13
    //
    formBar(TPrice(9381), TPrice(9380), TPrice(9380), TPrice(9380), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    ASSERT_EQ(bars.size(), 14UL);
    ASSERT_EQ(patterns.size(), 12UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 2UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9366), TPrice(9366), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9367), TPrice(9367), TPrice(9367), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 2, 2, 2, 2, tw::common_trade::ePatternType::kTransitional, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9367), TPrice(9365), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 3, 3, 3, 3, tw::common_trade::ePatternType::kSwing, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9364), TPrice(9365), TPrice(9371), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 4, 4, 4, 4, tw::common_trade::ePatternType::kSwing, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9365), TPrice(9370), TPrice(9366), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(9369), TPrice(9366), TPrice(9366), TPrice(9368), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 6, 6, 6, 6, tw::common_trade::ePatternType::kHitch, patterns[5]));
    ASSERT_TRUE(checkBarPattern(TPrice(9369), TPrice(9367), TPrice(9367), TPrice(9367), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 7, 7, 7, 7, tw::common_trade::ePatternType::kTransitional, patterns[6]));
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9367), TPrice(9368), TPrice(9368), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 8, 8, 8, 8, tw::common_trade::ePatternType::kTransitional, patterns[7]));
    ASSERT_TRUE(checkBarPattern(TPrice(9370), TPrice(9368), TPrice(9368), TPrice(9370), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 9, 9, 9, 9, tw::common_trade::ePatternType::kHitch, patterns[8]));
    ASSERT_TRUE(checkBarPattern(TPrice(9384), TPrice(9369), TPrice(9369), TPrice(9377), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 10, 11, 11, 10, tw::common_trade::ePatternType::kSwing, patterns[9]));
    ASSERT_TRUE(checkBarPattern(TPrice(9380), TPrice(9376), TPrice(9377), TPrice(9379), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 12, 12, 12, 12, tw::common_trade::ePatternType::kHitch, patterns[10]));
    ASSERT_TRUE(checkBarPattern(TPrice(9381), TPrice(9380), TPrice(9380), TPrice(9380), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 13, 13, 13, 13, tw::common_trade::ePatternType::kTransitional, patterns[11]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9365), TPrice(9366), TPrice(9365), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 1, 3, 2, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(9384), TPrice(9364), TPrice(9366), TPrice(9380), TSize(52), 52, tw::common_trade::ePatternDir::kUp, 1, 13, 11, 4, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
 
    
    // Bar 14
    // 
    formBar(TPrice(9382), TPrice(9380), TPrice(9380), TPrice(9381), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
   
    ASSERT_EQ(bars.size(), 15UL);
    ASSERT_EQ(patterns.size(), 13UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 2UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9366), TPrice(9366), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9367), TPrice(9367), TPrice(9367), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 2, 2, 2, 2, tw::common_trade::ePatternType::kTransitional, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9367), TPrice(9365), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 3, 3, 3, 3, tw::common_trade::ePatternType::kSwing, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9364), TPrice(9365), TPrice(9371), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 4, 4, 4, 4, tw::common_trade::ePatternType::kSwing, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9365), TPrice(9370), TPrice(9366), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(9369), TPrice(9366), TPrice(9366), TPrice(9368), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 6, 6, 6, 6, tw::common_trade::ePatternType::kHitch, patterns[5]));
    ASSERT_TRUE(checkBarPattern(TPrice(9369), TPrice(9367), TPrice(9367), TPrice(9367), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 7, 7, 7, 7, tw::common_trade::ePatternType::kTransitional, patterns[6]));
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9367), TPrice(9368), TPrice(9368), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 8, 8, 8, 8, tw::common_trade::ePatternType::kTransitional, patterns[7]));
    ASSERT_TRUE(checkBarPattern(TPrice(9370), TPrice(9368), TPrice(9368), TPrice(9370), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 9, 9, 9, 9, tw::common_trade::ePatternType::kHitch, patterns[8]));
    ASSERT_TRUE(checkBarPattern(TPrice(9384), TPrice(9369), TPrice(9369), TPrice(9377), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 10, 11, 11, 10, tw::common_trade::ePatternType::kSwing, patterns[9]));
    ASSERT_TRUE(checkBarPattern(TPrice(9380), TPrice(9376), TPrice(9377), TPrice(9379), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 12, 12, 12, 12, tw::common_trade::ePatternType::kHitch, patterns[10]));
    ASSERT_TRUE(checkBarPattern(TPrice(9381), TPrice(9380), TPrice(9380), TPrice(9380), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 13, 13, 13, 13, tw::common_trade::ePatternType::kTransitional, patterns[11]));
    ASSERT_TRUE(checkBarPattern(TPrice(9382), TPrice(9380), TPrice(9380), TPrice(9381), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 14, 14, 14, 14, tw::common_trade::ePatternType::kHitch, patterns[12]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9365), TPrice(9366), TPrice(9365), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 1, 3, 2, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(9384), TPrice(9364), TPrice(9366), TPrice(9381), TSize(56), 56, tw::common_trade::ePatternDir::kUp, 1, 14, 11, 4, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));

    
    // Bar 15
    //
    formBar(TPrice(9384), TPrice(9380), TPrice(9381), TPrice(9383), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    ASSERT_EQ(bars.size(), 16UL);
    ASSERT_EQ(patterns.size(), 14UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 2UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9366), TPrice(9366), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9367), TPrice(9367), TPrice(9367), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 2, 2, 2, 2, tw::common_trade::ePatternType::kTransitional, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9367), TPrice(9365), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 3, 3, 3, 3, tw::common_trade::ePatternType::kSwing, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9364), TPrice(9365), TPrice(9371), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 4, 4, 4, 4, tw::common_trade::ePatternType::kSwing, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9365), TPrice(9370), TPrice(9366), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(9369), TPrice(9366), TPrice(9366), TPrice(9368), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 6, 6, 6, 6, tw::common_trade::ePatternType::kHitch, patterns[5]));
    ASSERT_TRUE(checkBarPattern(TPrice(9369), TPrice(9367), TPrice(9367), TPrice(9367), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 7, 7, 7, 7, tw::common_trade::ePatternType::kTransitional, patterns[6]));
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9367), TPrice(9368), TPrice(9368), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 8, 8, 8, 8, tw::common_trade::ePatternType::kTransitional, patterns[7]));
    ASSERT_TRUE(checkBarPattern(TPrice(9370), TPrice(9368), TPrice(9368), TPrice(9370), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 9, 9, 9, 9, tw::common_trade::ePatternType::kHitch, patterns[8]));
    ASSERT_TRUE(checkBarPattern(TPrice(9384), TPrice(9369), TPrice(9369), TPrice(9377), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 10, 11, 11, 10, tw::common_trade::ePatternType::kSwing, patterns[9]));
    ASSERT_TRUE(checkBarPattern(TPrice(9380), TPrice(9376), TPrice(9377), TPrice(9379), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 12, 12, 12, 12, tw::common_trade::ePatternType::kHitch, patterns[10]));
    ASSERT_TRUE(checkBarPattern(TPrice(9381), TPrice(9380), TPrice(9380), TPrice(9380), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 13, 13, 13, 13, tw::common_trade::ePatternType::kTransitional, patterns[11]));
    ASSERT_TRUE(checkBarPattern(TPrice(9382), TPrice(9380), TPrice(9380), TPrice(9381), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 14, 14, 14, 14, tw::common_trade::ePatternType::kHitch, patterns[12]));
    ASSERT_TRUE(checkBarPattern(TPrice(9384), TPrice(9380), TPrice(9381), TPrice(9383), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 15, 15, 15, 15, tw::common_trade::ePatternType::kHitch, patterns[13]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9365), TPrice(9366), TPrice(9365), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 1, 3, 2, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(9384), TPrice(9364), TPrice(9366), TPrice(9383), TSize(60), 60, tw::common_trade::ePatternDir::kUp, 1, 15, 11, 4, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));

    
    // Bar 16
    //
    formBar(TPrice(9387), TPrice(9383), TPrice(9384), TPrice(9385), quoteNQM2, barsManager);    
    barsManager.onTimeout(TestRouter::_timerClientInfos.back()._timerId);
    
    ASSERT_EQ(bars.size(), 17UL);
    ASSERT_EQ(patterns.size(), 15UL);
    ASSERT_EQ(patternsSwingLevel1.size(), 2UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9366), TPrice(9366), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 1, 1, 1, 1, tw::common_trade::ePatternType::kTransitional, patterns[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9367), TPrice(9367), TPrice(9367), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 2, 2, 2, 2, tw::common_trade::ePatternType::kTransitional, patterns[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(9367), TPrice(9365), TPrice(9367), TPrice(9365), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 3, 3, 3, 3, tw::common_trade::ePatternType::kSwing, patterns[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9364), TPrice(9365), TPrice(9371), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 4, 4, 4, 4, tw::common_trade::ePatternType::kSwing, patterns[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(9371), TPrice(9365), TPrice(9370), TPrice(9366), TSize(4), 4, tw::common_trade::ePatternDir::kDown, 5, 5, 5, 5, tw::common_trade::ePatternType::kHitch, patterns[4]));
    ASSERT_TRUE(checkBarPattern(TPrice(9369), TPrice(9366), TPrice(9366), TPrice(9368), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 6, 6, 6, 6, tw::common_trade::ePatternType::kHitch, patterns[5]));
    ASSERT_TRUE(checkBarPattern(TPrice(9369), TPrice(9367), TPrice(9367), TPrice(9367), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 7, 7, 7, 7, tw::common_trade::ePatternType::kTransitional, patterns[6]));
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9367), TPrice(9368), TPrice(9368), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 8, 8, 8, 8, tw::common_trade::ePatternType::kTransitional, patterns[7]));
    ASSERT_TRUE(checkBarPattern(TPrice(9370), TPrice(9368), TPrice(9368), TPrice(9370), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 9, 9, 9, 9, tw::common_trade::ePatternType::kHitch, patterns[8]));
    ASSERT_TRUE(checkBarPattern(TPrice(9384), TPrice(9369), TPrice(9369), TPrice(9377), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 10, 11, 11, 10, tw::common_trade::ePatternType::kSwing, patterns[9]));
    ASSERT_TRUE(checkBarPattern(TPrice(9380), TPrice(9376), TPrice(9377), TPrice(9379), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 12, 12, 12, 12, tw::common_trade::ePatternType::kHitch, patterns[10]));
    ASSERT_TRUE(checkBarPattern(TPrice(9381), TPrice(9380), TPrice(9380), TPrice(9380), TSize(4), 4, tw::common_trade::ePatternDir::kUnknown, 13, 13, 13, 13, tw::common_trade::ePatternType::kTransitional, patterns[11]));
    ASSERT_TRUE(checkBarPattern(TPrice(9382), TPrice(9380), TPrice(9380), TPrice(9381), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 14, 14, 14, 14, tw::common_trade::ePatternType::kHitch, patterns[12]));
    ASSERT_TRUE(checkBarPattern(TPrice(9384), TPrice(9380), TPrice(9381), TPrice(9383), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 15, 15, 15, 15, tw::common_trade::ePatternType::kHitch, patterns[13]));    
    ASSERT_TRUE(checkBarPattern(TPrice(9387), TPrice(9383), TPrice(9384), TPrice(9385), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 16, 16, 16, 16, tw::common_trade::ePatternType::kSwing, patterns[14]));
    
    ASSERT_TRUE(checkBarPattern(TPrice(9368), TPrice(9365), TPrice(9366), TPrice(9365), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 1, 3, 2, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(9387), TPrice(9364), TPrice(9366), TPrice(9385), TSize(64), 64, tw::common_trade::ePatternDir::kUp, 1, 16, 16, 4, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    
}

TEST(CommonTradeLibTestSuit, bars_impl_test_pattern_processing_hitch_swing_bug_20130710_a)
{
    TestRouter::TImpl barsManager(60, 0);
    
    clearProcessors();    
    
    // Init quotes
    //
    tw::price::QuoteStore::TQuote quoteNQM2;        
    tw::instr::InstrumentPtr instrNQM2 = InstrHelper::getNQM2();    
    
    quoteNQM2.setInstrument(instrNQM2);    
    
    // Subscribe to NQM2
    //
    ASSERT_TRUE(barsManager.subscribe(instrNQM2->_keyId));
    const TBars& bars = barsManager.getBars(instrNQM2->_keyId);
    
    formBar(TPrice(10629), TPrice(10620), TPrice(10620), TPrice(10625), quoteNQM2, barsManager, true); // 1
    formBar(TPrice(10632), TPrice(10617), TPrice(10626), TPrice(10617), quoteNQM2, barsManager, true); // 2   
    formBar(TPrice(10619), TPrice(10607), TPrice(10617), TPrice(10609), quoteNQM2, barsManager, true); // 3   
    formBar(TPrice(10629), TPrice(10610), TPrice(10610), TPrice(10624), quoteNQM2, barsManager, true); // 4   
    formBar(TPrice(10640), TPrice(10620), TPrice(10624), TPrice(10638), quoteNQM2, barsManager, true); // 5   
    formBar(TPrice(10642), TPrice(10630), TPrice(10638), TPrice(10634), quoteNQM2, barsManager, true); // 6   
    formBar(TPrice(10642), TPrice(10633), TPrice(10635), TPrice(10637), quoteNQM2, barsManager, true); // 7   
    formBar(TPrice(10641), TPrice(10628), TPrice(10637), TPrice(10641), quoteNQM2, barsManager, true); // 8   
    formBar(TPrice(10641), TPrice(10632), TPrice(10640), TPrice(10633), quoteNQM2, barsManager, true); // 9   
    formBar(TPrice(10644), TPrice(10631), TPrice(10633), TPrice(10643), quoteNQM2, barsManager, true); // 10   
    formBar(TPrice(10649), TPrice(10640), TPrice(10643), TPrice(10643), quoteNQM2, barsManager, true); // 11   
    formBar(TPrice(10647), TPrice(10633), TPrice(10643), TPrice(10639), quoteNQM2, barsManager, true); // 12   
    formBar(TPrice(10644), TPrice(10629), TPrice(10640), TPrice(10644), quoteNQM2, barsManager, true); // 13   
    formBar(TPrice(10651), TPrice(10639), TPrice(10645), TPrice(10651), quoteNQM2, barsManager, true); // 14
    
    const TBarPatterns& patternsSwingLevel1 = barsManager.getBarPatternsSwingLevel1().getBarPatterns(instrNQM2->_keyId);
        
    ASSERT_EQ(bars.size(), 15UL);
    
    ASSERT_EQ(patternsSwingLevel1.size(), 5UL);
    
    ASSERT_TRUE(checkBarPattern(TPrice(10629), TPrice(10620), TPrice(10620), TPrice(10625), TSize(4), 4, tw::common_trade::ePatternDir::kUp, 1, 1, 1, 1, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[0]));
    ASSERT_TRUE(checkBarPattern(TPrice(10632), TPrice(10607), TPrice(10620), TPrice(10609), TSize(12), 12, tw::common_trade::ePatternDir::kDown, 1, 3, 2, 3, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[1]));
    ASSERT_TRUE(checkBarPattern(TPrice(10642), TPrice(10607), TPrice(10617), TPrice(10634), TSize(16), 16, tw::common_trade::ePatternDir::kUp, 3, 6, 6, 3, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[2]));
    ASSERT_TRUE(checkBarPattern(TPrice(10642), TPrice(10630), TPrice(10638), TPrice(10637), TSize(8), 8, tw::common_trade::ePatternDir::kUp, 6, 7, 6, 6, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[3]));
    ASSERT_TRUE(checkBarPattern(TPrice(10651), TPrice(10628), TPrice(10637), TPrice(10651), TSize(28), 28, tw::common_trade::ePatternDir::kUp, 8, 14, 14, 8, tw::common_trade::ePatternType::kSwing, patternsSwingLevel1[4]));
    
 
    
}

TEST(CommonTradeLibTestSuit, bars_impl_test_pattern_processing_swing_InV_price_bug_20130717_a)
{
    TestRouter::TImpl barsManager(60, 0);
    
    clearProcessors();    
    
    // Init quotes
    //
    tw::price::QuoteStore::TQuote quoteNQM2;        
    tw::instr::InstrumentPtr instrNQM2 = InstrHelper::getNQM2();    
    
    quoteNQM2.setInstrument(instrNQM2);    
    
    // Subscribe to NQM2
    //
    ASSERT_TRUE(barsManager.subscribe(instrNQM2->_keyId));
    
    formBar(TPrice(10065), TPrice(10064), TPrice(10065), TPrice(10064), quoteNQM2, barsManager, true);
    formBar(TPrice(10066), TPrice(10065), TPrice(10065), TPrice(10066), quoteNQM2, barsManager, true);
    formBar(TPrice(10065), TPrice(10065), TPrice(10065), TPrice(10065), quoteNQM2, barsManager, true);
    formBar(TPrice(10065), TPrice(10064), TPrice(10065), TPrice(10064), quoteNQM2, barsManager, true);
    formBar(TPrice(10064), TPrice(10064), TPrice(10064), TPrice(10064), quoteNQM2, barsManager, true);
    formBar(TPrice(10064), TPrice(10063), TPrice(10064), TPrice(10063), quoteNQM2, barsManager, true);
    
    formBar(TPrice(10063), TPrice(10061), TPrice(10063), TPrice(10061), quoteNQM2, barsManager, true);
    formBar(TPrice(10063), TPrice(10061), TPrice(10062), TPrice(10062), quoteNQM2, barsManager, true);
    formBar(TPrice(10062), TPrice(10061), TPrice(10061), TPrice(10062), quoteNQM2, barsManager, true);
    formBar(TPrice(10063), TPrice(10063), TPrice(10063), TPrice(10063), quoteNQM2, barsManager, true);
    formBar(TPrice(10063), TPrice(10063), TPrice(10063), TPrice(10063), quoteNQM2, barsManager, true);
    formBar(TPrice(10062), TPrice(10062), TPrice(10062), TPrice(10062), quoteNQM2, barsManager, true);
    formBar(TPrice(), TPrice(), TPrice(), TPrice(), quoteNQM2, barsManager, true);
    formBar(TPrice(10064), TPrice(10062), TPrice(10062), TPrice(10064), quoteNQM2, barsManager, true);
    formBar(TPrice(10064), TPrice(10064), TPrice(10064), TPrice(10064), quoteNQM2, barsManager, true);
    formBar(TPrice(10065), TPrice(10064), TPrice(10064), TPrice(10065), quoteNQM2, barsManager, true);
    formBar(TPrice(10066), TPrice(10065), TPrice(10065), TPrice(10066), quoteNQM2, barsManager, true);
    formBar(TPrice(10069), TPrice(10066), TPrice(10066), TPrice(10067), quoteNQM2, barsManager, true);
    formBar(TPrice(10069), TPrice(10068), TPrice(10068), TPrice(10068), quoteNQM2, barsManager, true);
    formBar(TPrice(10067), TPrice(10067), TPrice(10067), TPrice(10067), quoteNQM2, barsManager, true);
    
    const TBarPatterns& patternsSwingLevel2 = barsManager.getBarPatternsSwingLevel2().getBarPatterns(instrNQM2->_keyId);
    ASSERT_EQ(patternsSwingLevel2.size(), 4UL);
    ASSERT_TRUE(checkBarPattern(TPrice(10069), TPrice(10061), TPrice(10063), TPrice(10067), TSize(52), 52, tw::common_trade::ePatternDir::kUp, 7, 20, 18, 7, tw::common_trade::ePatternType::kTrend, patternsSwingLevel2[3]));
    
    
}


TEST(CommonTradeLibTestSuit, bars_impl_test_vol_per_price)
{
    TestRouter::TImpl barsManager(60, 0);
    
    clearProcessors();    
    
    // Init quotes
    //
    tw::price::QuoteStore::TQuote quoteNQM2;        
    tw::instr::InstrumentPtr instrNQM2 = InstrHelper::getNQM2();    
    
    quoteNQM2.setInstrument(instrNQM2);    
    
    // Subscribe to NQM2
    //
    ASSERT_TRUE(barsManager.subscribe(instrNQM2->_keyId));
    
    formBar(TPrice(103), TPrice(101), TPrice(102), TPrice(103), quoteNQM2, barsManager, true, TSize(3));
    ASSERT_EQ(barsManager.getVolByPrice(instrNQM2->_keyId, TPrice(100)), TSize(0));
    ASSERT_EQ(barsManager.getVolByPrice(instrNQM2->_keyId, TPrice(101)), TSize(3));
    ASSERT_EQ(barsManager.getVolByPrice(instrNQM2->_keyId, TPrice(102)), TSize(3));
    ASSERT_EQ(barsManager.getVolByPrice(instrNQM2->_keyId, TPrice(103)), TSize(6));
    ASSERT_EQ(barsManager.getVolByPrice(instrNQM2->_keyId, TPrice(105)), TSize(0));
    
    ASSERT_NEAR(barsManager.getAvgVolByPrice(instrNQM2->_keyId), 4.0, EPSILON);
    
    formBar(TPrice(105), TPrice(102), TPrice(103), TPrice(104), quoteNQM2, barsManager, true);
    
    ASSERT_EQ(barsManager.getVolByPrice(instrNQM2->_keyId, TPrice(100)), TSize(0));
    ASSERT_EQ(barsManager.getVolByPrice(instrNQM2->_keyId, TPrice(101)), TSize(3));
    ASSERT_EQ(barsManager.getVolByPrice(instrNQM2->_keyId, TPrice(102)), TSize(4));
    ASSERT_EQ(barsManager.getVolByPrice(instrNQM2->_keyId, TPrice(103)), TSize(7));
    ASSERT_EQ(barsManager.getVolByPrice(instrNQM2->_keyId, TPrice(105)), TSize(1));
    ASSERT_EQ(barsManager.getVolByPrice(instrNQM2->_keyId, TPrice(105)), TSize(1));
    
    ASSERT_NEAR(barsManager.getAvgVolByPrice(instrNQM2->_keyId), 3.2, EPSILON);
    
    
    
}