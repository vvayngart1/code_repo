#include <tw/common_trade/wbo.h>
#include <tw/price/quote_store.h>
#include "../unit_test_price_lib/instr_helper.h"

#include <gtest/gtest.h>

typedef tw::price::QuoteStore::TQuote TQuote;
typedef tw::price::Ticks TTicks;
typedef tw::price::Size TSize;

typedef tw::common_trade::Wbo<tw::common_trade::Wbo_calc1> TWbo1;
typedef tw::common_trade::Wbo<tw::common_trade::Wbo_calc2> TWbo2;

static const double PRICE_EPSILON = 0.005;

TEST(CommonTradeLibTestSuit, wbo1_SI)
{
    TWbo1 wbo;
    TQuote quote;
    TTicks price;
    TSize size;
    
    quote.setInstrument(InstrHelper::getSIM2());
    ASSERT_TRUE(quote.isValid());
    
    ASSERT_TRUE(!wbo.isValid());
    
    wbo.onQuote(quote);
    ASSERT_TRUE(!wbo.isValid());
    
    price.set(6348);
    size.set(2);
    quote.setBid(price, size, 0, 1);
    
    price.set(6350);
    size.set(10);
    quote.setAsk(price, size, 0, 1);
    
    wbo.onQuote(quote);
    ASSERT_TRUE(wbo.isValid());        
    ASSERT_NEAR(wbo.getTv(), 31743.3333, PRICE_EPSILON);
    ASSERT_NEAR(wbo.getTvInTicks(), 6348.6667, PRICE_EPSILON);
    
    
    price.set(6348);
    size.set(10);
    quote.setBid(price, size, 0, 1);
    
    price.set(6350);
    size.set(2);
    quote.setAsk(price, size, 0, 1);
    
    wbo.onQuote(quote);
    ASSERT_TRUE(wbo.isValid());        
    ASSERT_NEAR(wbo.getTv(), 31746.6667, PRICE_EPSILON);
    ASSERT_NEAR(wbo.getTvInTicks(), 6349.33334, PRICE_EPSILON);
    
    price.set(6348);
    size.set(2);
    quote.setBid(price, size, 0, 1);
    
    price.set(6350);
    size.set(10000);
    quote.setAsk(price, size, 0, 1);
    
    wbo.onQuote(quote);
    ASSERT_TRUE(wbo.isValid());        
    ASSERT_NEAR(wbo.getTv(), 31742.5010, PRICE_EPSILON);
    ASSERT_NEAR(wbo.getTvInTicks(), 6348.5002, PRICE_EPSILON);
    
    
    price.set(6348);
    size.set(10000);
    quote.setBid(price, size, 0, 1);
    
    price.set(6350);
    size.set(2);
    quote.setAsk(price, size, 0, 1);
    
    wbo.onQuote(quote);
    ASSERT_TRUE(wbo.isValid());        
    ASSERT_NEAR(wbo.getTv(), 31747.4990, PRICE_EPSILON);
    ASSERT_NEAR(wbo.getTvInTicks(), 6349.4998, PRICE_EPSILON);
    
}

TEST(CommonTradeLibTestSuit, wbo1_ZF)
{
    TWbo1 wbo;
    TQuote quote;
    TTicks price;
    TSize size;
    
    quote.setInstrument(InstrHelper::getZFM2());
    ASSERT_TRUE(quote.isValid());
    
    ASSERT_TRUE(!wbo.isValid());
    
    wbo.onQuote(quote);
    ASSERT_TRUE(!wbo.isValid());
    
    price.set(18108);
    size.set(2);
    quote.setBid(price, size, 0, 1);
    
    price.set(18124);
    size.set(10);
    quote.setAsk(price, size, 0, 1);
    
    wbo.onQuote(quote);
    ASSERT_TRUE(wbo.isValid());        
    ASSERT_NEAR(wbo.getTv(), 141.5286, PRICE_EPSILON);
    
    
    price.set(18108);
    size.set(10);
    quote.setBid(price, size, 0, 1);
    
    price.set(18124);
    size.set(2);
    quote.setAsk(price, size, 0, 1);
    
    wbo.onQuote(quote);
    ASSERT_TRUE(wbo.isValid());        
    ASSERT_NEAR(wbo.getTv(), 141.5339, PRICE_EPSILON);
    
    price.set(18108);
    size.set(2);
    quote.setBid(price, size, 0, 1);
    
    price.set(18124);
    size.set(10000);
    quote.setAsk(price, size, 0, 1);
    
    wbo.onQuote(quote);
    ASSERT_TRUE(wbo.isValid());        
    ASSERT_NEAR(wbo.getTv(), 141.5273, PRICE_EPSILON);
    
    
    price.set(18108);
    size.set(10000);
    quote.setBid(price, size, 0, 1);
    
    price.set(18124);
    size.set(2);
    quote.setAsk(price, size, 0, 1);
    
    wbo.onQuote(quote);
    ASSERT_TRUE(wbo.isValid());        
    ASSERT_NEAR(wbo.getTv(), 141.5352, PRICE_EPSILON);
    
}


TEST(CommonTradeLibTestSuit, wbo2_SI)
{
    TWbo2 wbo;
    TQuote quote;
    TTicks price;
    TSize size;
    
    quote.setInstrument(InstrHelper::getSIM2());
    ASSERT_TRUE(quote.isValid());
    
    ASSERT_TRUE(!wbo.isValid());
    
    wbo.onQuote(quote);
    ASSERT_TRUE(!wbo.isValid());
    
    price.set(6348);
    size.set(2);
    quote.setBid(price, size, 0, 1);
    
    price.set(6350);
    size.set(10);
    quote.setAsk(price, size, 0, 1);
    
    wbo.onQuote(quote);
    ASSERT_TRUE(wbo.isValid());        
    ASSERT_NEAR(wbo.getTv(), 31741.6667, PRICE_EPSILON);
    
    
    price.set(6348);
    size.set(10);
    quote.setBid(price, size, 0, 1);
    
    price.set(6350);
    size.set(2);
    quote.setAsk(price, size, 0, 1);
    
    wbo.onQuote(quote);
    ASSERT_TRUE(wbo.isValid());        
    ASSERT_NEAR(wbo.getTv(), 31748.3333, PRICE_EPSILON);
    
    price.set(6348);
    size.set(2);
    quote.setBid(price, size, 0, 1);
    
    price.set(6350);
    size.set(10000);
    quote.setAsk(price, size, 0, 1);
    
    wbo.onQuote(quote);
    ASSERT_TRUE(wbo.isValid());        
    ASSERT_NEAR(wbo.getTv(), 31740.0020, PRICE_EPSILON);
    
    
    price.set(6348);
    size.set(10000);
    quote.setBid(price, size, 0, 1);
    
    price.set(6350);
    size.set(2);
    quote.setAsk(price, size, 0, 1);
    
    wbo.onQuote(quote);
    ASSERT_TRUE(wbo.isValid());        
    ASSERT_NEAR(wbo.getTv(), 31749.9980, PRICE_EPSILON);
    
}

TEST(CommonTradeLibTestSuit, wbo2_ZF)
{
    TWbo2 wbo;
    TQuote quote;
    TTicks price;
    TSize size;
    
    quote.setInstrument(InstrHelper::getZFM2());
    ASSERT_TRUE(quote.isValid());
    
    ASSERT_TRUE(!wbo.isValid());
    
    wbo.onQuote(quote);
    ASSERT_TRUE(!wbo.isValid());
    
    price.set(18108);
    size.set(2);
    quote.setBid(price, size, 0, 1);
    
    price.set(18124);
    size.set(10);
    quote.setAsk(price, size, 0, 1);
    
    wbo.onQuote(quote);
    ASSERT_TRUE(wbo.isValid());        
    ASSERT_NEAR(wbo.getTv(), 141.4896, PRICE_EPSILON);
    
    
    price.set(18108);
    size.set(10);
    quote.setBid(price, size, 0, 1);
    
    price.set(18124);
    size.set(2);
    quote.setAsk(price, size, 0, 1);
    
    wbo.onQuote(quote);
    ASSERT_TRUE(wbo.isValid());        
    ASSERT_NEAR(wbo.getTv(), 141.5729, PRICE_EPSILON);
    
    price.set(18108);
    size.set(2);
    quote.setBid(price, size, 0, 1);
    
    price.set(18124);
    size.set(10000);
    quote.setAsk(price, size, 0, 1);
    
    wbo.onQuote(quote);
    ASSERT_TRUE(wbo.isValid());        
    ASSERT_NEAR(wbo.getTv(), 141.4688, PRICE_EPSILON);
    
    
    price.set(18108);
    size.set(10000);
    quote.setBid(price, size, 0, 1);
    
    price.set(18124);
    size.set(2);
    quote.setAsk(price, size, 0, 1);
    
    wbo.onQuote(quote);
    ASSERT_TRUE(wbo.isValid());        
    ASSERT_NEAR(wbo.getTv(), 141.5937, PRICE_EPSILON);
    
}

TEST(CommonTradeLibTestSuit, wbo1_ZB)
{
    TWbo1 wbo;
    TQuote quote;
    
    quote.setInstrument(InstrHelper::getZBM3());
    ASSERT_TRUE(quote.isValid());
    
    ASSERT_TRUE(!wbo.isValid());
    
    wbo.onQuote(quote);
    ASSERT_TRUE(!wbo.isValid());
    
    // 4740 = 148.125
    // 4741 = 148.15625
    //
    quote.setBid(TTicks(4740), TSize(44), 0, 7);
    quote.setAsk(TTicks(4741), TSize(1100), 0, 51);
        
    wbo.onQuote(quote);
    ASSERT_TRUE(wbo.isValid());        
    ASSERT_NEAR(wbo.getTv(), 148.1262, PRICE_EPSILON);
    
    // 4741 = 148.15625
    // 4742 = 148.1875
    //
    quote.setBid(TTicks(4741), TSize(500), 0, 7);
    quote.setAsk(TTicks(4742), TSize(100), 0, 51);
        
    wbo.onQuote(quote);
    ASSERT_TRUE(wbo.isValid());        
    ASSERT_NEAR(wbo.getTv(), 148.182, PRICE_EPSILON);
    
    // 4739 = 148.09375
    // 4740 = 148.125
    //
    quote.setBid(TTicks(4739), TSize(711), 0, 123);
    quote.setAsk(TTicks(4740), TSize(45), 0, 8);
        
    wbo.onQuote(quote);
    ASSERT_TRUE(wbo.isValid());        
    ASSERT_NEAR(wbo.getTv(), 148.1231, PRICE_EPSILON);
    
}
