#include <tw/common_trade/priceVolTracker.h>
#include "../unit_test_price_lib/instr_helper.h"
#include "tw/generated/instrument.h"
#include "tw/price/quote.h"
#include "tw/price/quote_store.h"

#include <gtest/gtest.h>

#include <vector>

typedef tw::price::Ticks TPrice;
typedef tw::price::Size TSize;
typedef tw::common_trade::TPriceSize TPriceSize;
typedef tw::common_trade::TPriceVolTracker TPriceVolTracker;
typedef TPriceVolTracker::TPriceVolInfo TPriceVolInfo;

static const double PRICE_EPSILON = 0.005;

class TestHighResolutionTimer {
public:
    static TestHighResolutionTimer now() {
        TestHighResolutionTimer x;
        x.setToNow();
        return x;
    }
    
public:
    TestHighResolutionTimer() {        
        _time = 0;
    }
    
    void setToNow() {
        _time = ++_now;
    }
    
    int32_t operator-(const TestHighResolutionTimer& rhs) {
        return _time - rhs._time;
    }
    
    bool operator>(const TestHighResolutionTimer& x) const {
        return _time > x._time;
    }
    
    int32_t _time;
    static int32_t _now;
};

int32_t TestHighResolutionTimer::_now = 0;

typedef tw::common_trade::PriceVolTracker<TestHighResolutionTimer> TPriceVolTracker2;
typedef TPriceVolTracker2::TPriceVolInfo TPriceVolInfo2;
typedef TPriceVolTracker2::TTimeVolInfo TTimeVolInfo2;
typedef TPriceVolTracker2::TBuySellVolInfo TBuySellVolInfo2;

template <typename TPriceVolInfo>
bool checkLevel(const TPriceSize& bid, const TPriceSize& ask, const TPriceVolInfo& info) {
    bool status = true;        
    
    EXPECT_EQ(info._bid.first, bid.first);
    if ( info._bid.first != bid.first )
        status = false;
    
    EXPECT_EQ(info._bid.second, bid.second);
    if ( info._bid.second != bid.second )
        status = false;
    
    EXPECT_EQ(info._ask.first, ask.first);
    if ( info._ask.first != ask.first )
        status = false;
    
    EXPECT_EQ(info._ask.second, ask.second);
    if ( info._ask.second != ask.second )
        status = false;
    
    return status;
}

bool checkBuySellVolInfo(const tw::price::Size& buyVol, const tw::price::Size& sellVol, const TBuySellVolInfo2& info) {
    bool status = true;        
    
    EXPECT_EQ(info.first, buyVol);
    if ( info.first != buyVol )
        status = false;
    
    EXPECT_EQ(info.second, sellVol);
    if ( info.second != sellVol )
        status = false;
    
    return status;
}


TEST(CommonTradeLibTestSuit, priceVolTracker_test_normal_processing)
{   
    TPriceVolTracker impl;
    const TPriceVolTracker::TInfos& infos = impl.getInfos();
    
    ASSERT_EQ(infos.size(), 0UL);
    
    // Init quotes
    //
    tw::price::QuoteStore::TQuote quote;        
    tw::instr::InstrumentPtr instrNQM2 = InstrHelper::getNQM2();
    quote.setInstrument(instrNQM2);
    
    quote.setBid(TPrice(100), TSize(10), 0, 1);
    quote.setAsk(TPrice(101), TSize(10), 0, 1);    
    impl.onQuote(quote);    
    quote.clearFlag();
    ASSERT_EQ(infos.size(), 1UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(0)), TPriceSize(TPrice(101), TSize(0)), infos[0]));
    
    quote.setTrade(TPrice(101), TSize(3));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideBuy;
    impl.onQuote(quote);
    ASSERT_EQ(infos.size(), 1UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(0)), TPriceSize(TPrice(101), TSize(3)), infos[0]));
    
    quote.setTrade(TPrice(100), TSize(8));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideSell;
    impl.onQuote(quote);
    ASSERT_EQ(infos.size(), 1UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(8)), TPriceSize(TPrice(101), TSize(3)), infos[0]));
    
    quote.setTrade(TPrice(101), TSize(6));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideBuy;
    impl.onQuote(quote);
    ASSERT_EQ(infos.size(), 1UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(8)), TPriceSize(TPrice(101), TSize(9)), infos[0]));    
    
    quote.setTrade(TPrice(100), TSize(2));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideSell;
    impl.onQuote(quote);
    ASSERT_EQ(infos.size(), 1UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[0]));
    
    quote.setTrade(TPrice(99), TSize(2));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideSell;
    impl.onQuote(quote);
    ASSERT_EQ(infos.size(), 2UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[1]));
    
    quote.setTrade(TPrice(98), TSize(5));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideSell;
    impl.onQuote(quote);
    ASSERT_EQ(infos.size(), 3UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(5)), TPriceSize(TPrice(), TSize(0)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[2]));
    
    quote.clearFlag();
    quote.setTrade(TPrice(98), TSize(4));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideSell;
    quote.setBid(TPrice(98), TSize(10), 0, 1);
    quote.setAsk(TPrice(99), TSize(10), 0, 1);    
    impl.onQuote(quote);    
    quote.clearFlag();
    
    ASSERT_EQ(infos.size(), 3UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(9)), TPriceSize(TPrice(99), TSize(0)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[2]));    
    
    
    quote.setTrade(TPrice(99), TSize(4));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideBuy;
    quote.setBid(TPrice(98), TSize(8), 0, 1);
    quote.setAsk(TPrice(99), TSize(9), 0, 1);    
    impl.onQuote(quote);    
    quote.clearFlag();
    
    ASSERT_EQ(infos.size(), 3UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(9)), TPriceSize(TPrice(99), TSize(4)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[2]));    
    
    
    quote.setTrade(TPrice(100), TSize(7));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideBuy;    
    impl.onQuote(quote);    
    quote.clearFlag();
    
    ASSERT_EQ(infos.size(), 4UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(100), TSize(7)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(9)), TPriceSize(TPrice(99), TSize(4)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[2]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[3]));    
    
    quote.setTrade(TPrice(101), TSize(11));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideBuy;    
    impl.onQuote(quote);    
    quote.clearFlag();
    
    ASSERT_EQ(infos.size(), 5UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(101), TSize(11)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(100), TSize(7)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(9)), TPriceSize(TPrice(99), TSize(4)), infos[2]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[3]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[4]));
    
    quote.setBid(TPrice(100), TSize(50), 0, 1);
    quote.setAsk(TPrice(101), TSize(69), 0, 1);    
    impl.onQuote(quote);    
    quote.clearFlag();
    
    ASSERT_EQ(infos.size(), 5UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(0)), TPriceSize(TPrice(101), TSize(11)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(100), TSize(7)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(9)), TPriceSize(TPrice(99), TSize(4)), infos[2]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[3]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[4]));
    
    
    quote.setBid(TPrice(100), TSize(60), 0, 1);
    quote.setAsk(TPrice(101), TSize(79), 0, 1);    
    impl.onQuote(quote);    
    quote.clearFlag();
    
    ASSERT_EQ(infos.size(), 5UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(0)), TPriceSize(TPrice(101), TSize(11)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(100), TSize(7)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(9)), TPriceSize(TPrice(99), TSize(4)), infos[2]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[3]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[4]));
    
    quote.setTrade(TPrice(101), TSize(11));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideUnknown;  
    impl.onQuote(quote);    
    quote.clearFlag();
    
    
    ASSERT_EQ(infos.size(), 5UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(0)), TPriceSize(TPrice(101), TSize(22)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(100), TSize(7)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(9)), TPriceSize(TPrice(99), TSize(4)), infos[2]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[3]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[4]));
    
    quote.setTrade(TPrice(100), TSize(12));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideUnknown;  
    impl.onQuote(quote);    
    quote.clearFlag();
    
    
    ASSERT_EQ(infos.size(), 5UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(12)), TPriceSize(TPrice(101), TSize(22)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(100), TSize(7)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(9)), TPriceSize(TPrice(99), TSize(4)), infos[2]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[3]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[4]));
    
    
    quote.setTrade(TPrice(102), TSize(25));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideUnknown;  
    impl.onQuote(quote);    
    quote.clearFlag();    
    
    ASSERT_EQ(infos.size(), 6UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(102), TSize(25)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(12)), TPriceSize(TPrice(101), TSize(22)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(100), TSize(7)), infos[2]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(9)), TPriceSize(TPrice(99), TSize(4)), infos[3]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[4]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[5]));
    
    quote.setBid(TPrice(101), TSize(60), 0, 1);
    quote.setAsk(TPrice(102), TSize(79), 0, 1);    
    impl.onQuote(quote);    
    quote.clearFlag();    
    
    ASSERT_EQ(infos.size(), 6UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(101), TSize(0)), TPriceSize(TPrice(102), TSize(25)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(12)), TPriceSize(TPrice(101), TSize(22)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(100), TSize(7)), infos[2]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(9)), TPriceSize(TPrice(99), TSize(4)), infos[3]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[4]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[5]));
    
    
    quote.setTrade(TPrice(100), TSize(28));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideUnknown;  
    impl.onQuote(quote);    
    quote.clearFlag();    
    
    ASSERT_EQ(infos.size(), 7UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(28)), TPriceSize(TPrice(), TSize(0)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(101), TSize(0)), TPriceSize(TPrice(102), TSize(25)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(12)), TPriceSize(TPrice(101), TSize(22)), infos[2]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(100), TSize(7)), infos[3]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(9)), TPriceSize(TPrice(99), TSize(4)), infos[4]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[5]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[6]));
    
    
    quote.setTrade(TPrice(101), TSize(38));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideUnknown;  
    impl.onQuote(quote);    
    quote.clearFlag();    
    
    ASSERT_EQ(infos.size(), 8UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(101), TSize(38)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(28)), TPriceSize(TPrice(), TSize(0)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(101), TSize(0)), TPriceSize(TPrice(102), TSize(25)), infos[2]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(12)), TPriceSize(TPrice(101), TSize(22)), infos[3]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(100), TSize(7)), infos[4]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(9)), TPriceSize(TPrice(99), TSize(4)), infos[5]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[6]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[7]));
    
    quote.setTrade(TPrice(102), TSize(48));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideSell;  
    impl.onQuote(quote);    
    quote.clearFlag();    
    
    ASSERT_EQ(infos.size(), 9UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(102), TSize(48)), TPriceSize(TPrice(), TSize(0)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(101), TSize(38)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(28)), TPriceSize(TPrice(), TSize(0)), infos[2]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(101), TSize(0)), TPriceSize(TPrice(102), TSize(25)), infos[3]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(12)), TPriceSize(TPrice(101), TSize(22)), infos[4]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(100), TSize(7)), infos[5]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(9)), TPriceSize(TPrice(99), TSize(4)), infos[6]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[7]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[8]));
    
    
    quote.setTrade(TPrice(100), TSize(46));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideBuy;  
    impl.onQuote(quote);    
    quote.clearFlag();    
    
    ASSERT_EQ(infos.size(), 10UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize()), TPriceSize(TPrice(100), TSize(46)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(102), TSize(48)), TPriceSize(TPrice(), TSize(0)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(101), TSize(38)), infos[2]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(28)), TPriceSize(TPrice(), TSize(0)), infos[3]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(101), TSize(0)), TPriceSize(TPrice(102), TSize(25)), infos[4]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(12)), TPriceSize(TPrice(101), TSize(22)), infos[5]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(100), TSize(7)), infos[6]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(9)), TPriceSize(TPrice(99), TSize(4)), infos[7]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[8]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[9]));
    
    
}





TEST(CommonTradeLibTestSuit, priceVolTracker_test_vol_by_time_processing)
{   
    TPriceVolTracker2 impl;
    uint64_t till = 10000;;
    
    const TPriceVolTracker2::TInfos& infos = impl.getInfos();
    TBuySellVolInfo2 bsInfo;
    
    ASSERT_EQ(infos.size(), 0UL);
    
    // Init quotes
    //
    tw::price::QuoteStore::TQuote quote;        
    tw::instr::InstrumentPtr instrNQM2 = InstrHelper::getNQM2();
    quote.setInstrument(instrNQM2);
    
    quote.setBid(TPrice(100), TSize(10), 0, 1);
    quote.setAsk(TPrice(101), TSize(10), 0, 1);    
    impl.onQuote(quote);    
    quote.clearFlag();
    ASSERT_EQ(infos.size(), 1UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(0)), TPriceSize(TPrice(101), TSize(0)), infos[0]));    
    
    ASSERT_TRUE(checkBuySellVolInfo(TSize(0), TSize(0), impl.getBuySellVolInfo(till)));    
    
    quote.setTrade(TPrice(101), TSize(3));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideBuy;
    impl.onQuote(quote);
    ASSERT_EQ(infos.size(), 1UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(0)), TPriceSize(TPrice(101), TSize(3)), infos[0]));
    
    ASSERT_TRUE(checkBuySellVolInfo(TSize(3), TSize(0), impl.getBuySellVolInfo(till)));
    
    quote.setTrade(TPrice(100), TSize(8));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideSell;
    impl.onQuote(quote);
    ASSERT_EQ(infos.size(), 1UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(8)), TPriceSize(TPrice(101), TSize(3)), infos[0]));
    
    ASSERT_TRUE(checkBuySellVolInfo(TSize(3), TSize(8), impl.getBuySellVolInfo(till)));
    
    quote.setTrade(TPrice(101), TSize(6));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideBuy;
    impl.onQuote(quote);
    ASSERT_EQ(infos.size(), 1UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(8)), TPriceSize(TPrice(101), TSize(9)), infos[0]));
    
    ASSERT_TRUE(checkBuySellVolInfo(TSize(9), TSize(8), impl.getBuySellVolInfo(till)));
    
    quote.setTrade(TPrice(100), TSize(2));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideSell;
    impl.onQuote(quote);
    ASSERT_EQ(infos.size(), 1UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[0]));
    
    ASSERT_TRUE(checkBuySellVolInfo(TSize(9), TSize(10), impl.getBuySellVolInfo(till)));
    
    quote.setTrade(TPrice(99), TSize(2));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideSell;
    impl.onQuote(quote);
    ASSERT_EQ(infos.size(), 2UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[1]));
    
    ASSERT_TRUE(checkBuySellVolInfo(TSize(9), TSize(12), impl.getBuySellVolInfo(till)));
    
    quote.setTrade(TPrice(98), TSize(5));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideSell;
    impl.onQuote(quote);
    ASSERT_EQ(infos.size(), 3UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(5)), TPriceSize(TPrice(), TSize(0)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[2]));
    
    ASSERT_TRUE(checkBuySellVolInfo(TSize(9), TSize(17), impl.getBuySellVolInfo(till)));
    
    quote.clearFlag();
    quote.setTrade(TPrice(98), TSize(4));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideSell;
    quote.setBid(TPrice(98), TSize(10), 0, 1);
    quote.setAsk(TPrice(99), TSize(10), 0, 1);    
    impl.onQuote(quote);    
    quote.clearFlag();
    
    ASSERT_EQ(infos.size(), 3UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(9)), TPriceSize(TPrice(99), TSize(0)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[2]));

    ASSERT_TRUE(checkBuySellVolInfo(TSize(9), TSize(21), impl.getBuySellVolInfo(till)));
    
    
    quote.setTrade(TPrice(99), TSize(4));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideBuy;
    quote.setBid(TPrice(98), TSize(8), 0, 1);
    quote.setAsk(TPrice(99), TSize(9), 0, 1);    
    impl.onQuote(quote);    
    quote.clearFlag();
    
    ASSERT_EQ(infos.size(), 3UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(9)), TPriceSize(TPrice(99), TSize(4)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[2]));    
    
    ASSERT_TRUE(checkBuySellVolInfo(TSize(13), TSize(21), impl.getBuySellVolInfo(till)));
    
    quote.setTrade(TPrice(100), TSize(7));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideBuy;    
    impl.onQuote(quote);    
    quote.clearFlag();
    
    ASSERT_EQ(infos.size(), 4UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(100), TSize(7)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(9)), TPriceSize(TPrice(99), TSize(4)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[2]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[3]));

    ASSERT_TRUE(checkBuySellVolInfo(TSize(20), TSize(21), impl.getBuySellVolInfo(till)));
    
    quote.setTrade(TPrice(101), TSize(11));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideBuy;    
    impl.onQuote(quote);    
    quote.clearFlag();
    
    ASSERT_EQ(infos.size(), 5UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(101), TSize(11)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(100), TSize(7)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(9)), TPriceSize(TPrice(99), TSize(4)), infos[2]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[3]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[4]));
    
    ASSERT_TRUE(checkBuySellVolInfo(TSize(31), TSize(21), impl.getBuySellVolInfo(till)));
    
    quote.setBid(TPrice(100), TSize(50), 0, 1);
    quote.setAsk(TPrice(101), TSize(69), 0, 1);    
    impl.onQuote(quote);    
    quote.clearFlag();
    
    ASSERT_EQ(infos.size(), 5UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(0)), TPriceSize(TPrice(101), TSize(11)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(100), TSize(7)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(9)), TPriceSize(TPrice(99), TSize(4)), infos[2]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[3]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[4]));
    
    ASSERT_TRUE(checkBuySellVolInfo(TSize(31), TSize(21), impl.getBuySellVolInfo(till)));
    
    
    quote.setBid(TPrice(100), TSize(60), 0, 1);
    quote.setAsk(TPrice(101), TSize(79), 0, 1);    
    impl.onQuote(quote);    
    quote.clearFlag();
    
    ASSERT_EQ(infos.size(), 5UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(0)), TPriceSize(TPrice(101), TSize(11)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(100), TSize(7)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(9)), TPriceSize(TPrice(99), TSize(4)), infos[2]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[3]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[4]));
    
    ASSERT_TRUE(checkBuySellVolInfo(TSize(31), TSize(21), impl.getBuySellVolInfo(till)));
    
    quote.setTrade(TPrice(101), TSize(11));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideUnknown;  
    impl.onQuote(quote);    
    quote.clearFlag();
    
    
    ASSERT_EQ(infos.size(), 5UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(0)), TPriceSize(TPrice(101), TSize(22)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(100), TSize(7)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(9)), TPriceSize(TPrice(99), TSize(4)), infos[2]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[3]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[4]));
    
    ASSERT_TRUE(checkBuySellVolInfo(TSize(42), TSize(21), impl.getBuySellVolInfo(till)));
    
    quote.setTrade(TPrice(100), TSize(12));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideUnknown;  
    impl.onQuote(quote);    
    quote.clearFlag();
    
    
    ASSERT_EQ(infos.size(), 5UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(12)), TPriceSize(TPrice(101), TSize(22)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(100), TSize(7)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(9)), TPriceSize(TPrice(99), TSize(4)), infos[2]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[3]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[4]));
    
    ASSERT_TRUE(checkBuySellVolInfo(TSize(42), TSize(33), impl.getBuySellVolInfo(till)));
    
    
    quote.setTrade(TPrice(102), TSize(25));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideUnknown;  
    impl.onQuote(quote);    
    quote.clearFlag();    
    
    ASSERT_EQ(infos.size(), 6UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(102), TSize(25)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(12)), TPriceSize(TPrice(101), TSize(22)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(100), TSize(7)), infos[2]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(9)), TPriceSize(TPrice(99), TSize(4)), infos[3]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[4]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[5]));
    
    ASSERT_TRUE(checkBuySellVolInfo(TSize(67), TSize(33), impl.getBuySellVolInfo(till)));
    
    quote.setBid(TPrice(101), TSize(60), 0, 1);
    quote.setAsk(TPrice(102), TSize(79), 0, 1);    
    impl.onQuote(quote);    
    quote.clearFlag();    
    
    ASSERT_EQ(infos.size(), 6UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(101), TSize(0)), TPriceSize(TPrice(102), TSize(25)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(12)), TPriceSize(TPrice(101), TSize(22)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(100), TSize(7)), infos[2]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(9)), TPriceSize(TPrice(99), TSize(4)), infos[3]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[4]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[5]));
    
    ASSERT_TRUE(checkBuySellVolInfo(TSize(67), TSize(33), impl.getBuySellVolInfo(till)));
    
    
    quote.setTrade(TPrice(100), TSize(28));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideUnknown;  
    impl.onQuote(quote);    
    quote.clearFlag();    
    
    ASSERT_EQ(infos.size(), 7UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(28)), TPriceSize(TPrice(), TSize(0)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(101), TSize(0)), TPriceSize(TPrice(102), TSize(25)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(12)), TPriceSize(TPrice(101), TSize(22)), infos[2]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(100), TSize(7)), infos[3]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(9)), TPriceSize(TPrice(99), TSize(4)), infos[4]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[5]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[6]));
    
    ASSERT_TRUE(checkBuySellVolInfo(TSize(67), TSize(61), impl.getBuySellVolInfo(till)));
    
    
    quote.setTrade(TPrice(101), TSize(38));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideUnknown;  
    impl.onQuote(quote);    
    quote.clearFlag();    
    
    ASSERT_EQ(infos.size(), 8UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(101), TSize(38)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(28)), TPriceSize(TPrice(), TSize(0)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(101), TSize(0)), TPriceSize(TPrice(102), TSize(25)), infos[2]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(12)), TPriceSize(TPrice(101), TSize(22)), infos[3]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(100), TSize(7)), infos[4]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(9)), TPriceSize(TPrice(99), TSize(4)), infos[5]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[6]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[7]));
    
    ASSERT_TRUE(checkBuySellVolInfo(TSize(105), TSize(61), impl.getBuySellVolInfo(till)));
    
    quote.setTrade(TPrice(102), TSize(48));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideSell;  
    impl.onQuote(quote);    
    quote.clearFlag();    
    
    ASSERT_EQ(infos.size(), 9UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(102), TSize(48)), TPriceSize(TPrice(), TSize(0)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(101), TSize(38)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(28)), TPriceSize(TPrice(), TSize(0)), infos[2]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(101), TSize(0)), TPriceSize(TPrice(102), TSize(25)), infos[3]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(12)), TPriceSize(TPrice(101), TSize(22)), infos[4]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(100), TSize(7)), infos[5]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(9)), TPriceSize(TPrice(99), TSize(4)), infos[6]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[7]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[8]));
    
    ASSERT_TRUE(checkBuySellVolInfo(TSize(105), TSize(109), impl.getBuySellVolInfo(till)));
    
    
    quote.setTrade(TPrice(100), TSize(46));
    quote._trade._aggressorSide = tw::price::Trade::kAggressorSideBuy;  
    impl.onQuote(quote);    
    quote.clearFlag();    
    
    ASSERT_EQ(infos.size(), 10UL);
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize()), TPriceSize(TPrice(100), TSize(46)), infos[0]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(102), TSize(48)), TPriceSize(TPrice(), TSize(0)), infos[1]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(101), TSize(38)), infos[2]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(28)), TPriceSize(TPrice(), TSize(0)), infos[3]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(101), TSize(0)), TPriceSize(TPrice(102), TSize(25)), infos[4]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(12)), TPriceSize(TPrice(101), TSize(22)), infos[5]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(), TSize(0)), TPriceSize(TPrice(100), TSize(7)), infos[6]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(98), TSize(9)), TPriceSize(TPrice(99), TSize(4)), infos[7]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(99), TSize(2)), TPriceSize(TPrice(), TSize(0)), infos[8]));
    ASSERT_TRUE(checkLevel(TPriceSize(TPrice(100), TSize(10)), TPriceSize(TPrice(101), TSize(9)), infos[9]));
    
    ASSERT_TRUE(checkBuySellVolInfo(TSize(151), TSize(109), impl.getBuySellVolInfo(till)));
    
    // Additional time frames checking
    //
    till = 0;
    ASSERT_TRUE(checkBuySellVolInfo(TSize(0), TSize(0), impl.getBuySellVolInfo(till)));
    
    till = TestHighResolutionTimer::_now - impl.getTimeVolInfos()[0]._timestamp._time+1;
    ASSERT_TRUE(checkBuySellVolInfo(TSize(46), TSize(0), impl.getBuySellVolInfo(till)));
    
    till = TestHighResolutionTimer::_now - impl.getTimeVolInfos()[1]._timestamp._time+1;
    ASSERT_TRUE(checkBuySellVolInfo(TSize(46), TSize(48), impl.getBuySellVolInfo(till)));
    
    
}


