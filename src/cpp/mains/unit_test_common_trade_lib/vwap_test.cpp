#include <tw/common_trade/vwap.h>
#include <tw/price/quote_store.h>
#include "../unit_test_price_lib/instr_helper.h"

#include <gtest/gtest.h>

typedef tw::price::QuoteStore::TQuote TQuote;
typedef tw::price::Ticks TTicks;
typedef tw::price::Size TSize;

typedef tw::common_trade::Vwap TVwap;

static const double PRICE_EPSILON = 0.005;

TEST(CommonTradeLibTestSuit, Vwap_SI)
{
    TVwap tv;
    TQuote quote;
    TTicks price;
    TSize size;
    
    quote.setInstrument(InstrHelper::getSIM2());
    ASSERT_TRUE(quote.isValid());
    
    ASSERT_TRUE(!tv.isValid());
    
    tv.onQuote(quote);
    ASSERT_TRUE(!tv.isValid());
    
    price.set(6348);
    size.set(2000);
    quote.setBid(price, size, 0, 1);
    
    price.set(6350);
    size.set(1000);
    quote.setAsk(price, size, 0, 1);
    
    tv.onQuote(quote);
    ASSERT_TRUE(!tv.isValid());
    
    price.set(6348);
    size.set(10);
    quote.setTrade(price, size);
    tv.onQuote(quote);
    ASSERT_TRUE(tv.isValid());        
    ASSERT_NEAR(tv.getTv(), 31740.0, PRICE_EPSILON);
    
    price.set(6350);
    size.set(30);
    quote.setTrade(price, size);
    tv.onQuote(quote);
    ASSERT_TRUE(tv.isValid());        
    ASSERT_NEAR(tv.getTv(), 31747.5, PRICE_EPSILON);
    
    price.set(6350);
    size.set(50);
    quote.setTrade(price, size);
    tv.onQuote(quote);
    ASSERT_TRUE(tv.isValid());        
    ASSERT_NEAR(tv.getTv(), 31748.8889, PRICE_EPSILON);
}

TEST(CommonTradeLibTestSuit, Vwap_ZF)
{
    TVwap tv;
    TQuote quote;
    TTicks price;
    TSize size;
    
    quote.setInstrument(InstrHelper::getZFM2());
    ASSERT_TRUE(quote.isValid());
    
    ASSERT_TRUE(!tv.isValid());
    
    tv.onQuote(quote);
    ASSERT_TRUE(!tv.isValid());
    
    price.set(18108);
    size.set(2000);
    quote.setBid(price, size, 0, 1);
    
    price.set(18124);
    size.set(1000);
    quote.setAsk(price, size, 0, 1);
    
    tv.onQuote(quote);
    ASSERT_TRUE(!tv.isValid());
    
    price.set(18108);
    size.set(10);
    quote.setTrade(price, size);
    tv.onQuote(quote);
    ASSERT_TRUE(tv.isValid());        
    ASSERT_NEAR(tv.getTv(), 141.46875, PRICE_EPSILON);
    
    price.set(18124);
    size.set(30);
    quote.setTrade(price, size);
    tv.onQuote(quote);
    ASSERT_TRUE(tv.isValid());        
    ASSERT_NEAR(tv.getTv(), 141.5625, PRICE_EPSILON);
    
    price.set(18124);
    size.set(50);
    quote.setTrade(price, size);
    tv.onQuote(quote);
    ASSERT_TRUE(tv.isValid());        
    ASSERT_NEAR(tv.getTv(), 141.5798, PRICE_EPSILON);
    
}

template <typename TOp1, typename TOp2>
bool checkMarketExitGoodBoth(double fillPrice1, double bookPrice1, double tv1, int32_t sign1, TOp1 op1,
                             double fillPrice2, double bookPrice2, double tv2, int32_t sign2, TOp2 op2,
                             double box_offset, double box_vwap) {
    double totalOffset = sign1*(bookPrice1-fillPrice1)+sign2*(bookPrice2-fillPrice2);
    double tvOffset1 = sign1*(tv1-fillPrice1);
    double tvOffset2 = sign2*(tv2-fillPrice2);

    if ( op1(totalOffset, box_offset) && (op1(tvOffset1, box_vwap) || (op2(tvOffset2, box_vwap))) )
        return true;

    return false;
}

TEST(CommonTradeLibTestSuit, Vwap_checkMarketExitGoodBoth)
{
    // Buy test - pass
    //
    ASSERT_TRUE(checkMarketExitGoodBoth(12701.0, 12702.0, 12701.4, 1, std::greater_equal<double>(),
                                        12345.0, 12344.0, 12344.4, -1, std::greater_equal<double>(),
                                        2.0, 0.5));
    
    // Sell test - pass
    //
    ASSERT_TRUE(checkMarketExitGoodBoth(12701.0, 12700.0, 12700.4, -1, std::greater_equal<double>(),
                                        12345.0, 12346.0, 12345.4, 1, std::greater_equal<double>(),
                                        2.0, 0.5));
    
    // Buy test - don't pass
    //
    ASSERT_TRUE(!checkMarketExitGoodBoth(12701.0, 12702.0, 12701.4, 1, std::greater_equal<double>(),
                                        12345.0, 12344.0, 12344.6, -1, std::greater_equal<double>(),
                                        2.0, 0.5));
    
    // Sell test - don't pass
    //
    ASSERT_TRUE(!checkMarketExitGoodBoth(12701.0, 12700.0, 12700.4, -1, std::greater_equal<double>(),
                                        12345.0, 12345.0, 12345.6, 1, std::greater_equal<double>(),
                                        2.0, 0.5));
}

