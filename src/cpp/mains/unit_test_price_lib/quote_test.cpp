#include <gtest/gtest.h>

#include <tw/price/quote_store.h>
#include "instr_helper.h"

typedef tw::price::QuoteStore::TQuote TQuote;
typedef tw::price::Ticks TTicks;
typedef tw::price::Size TSize;

const uint32_t CHECK_NOTHING = 0x0;
const uint32_t CHECK_PRICE = 0x1;
const uint32_t CHECK_SIZE = 0x2;

enum eBidAsk {
    kBID = 1,
    kASK = 2
};

bool checkLevel(TQuote& quote, size_t level, eBidAsk side, uint32_t checkFlag) {
    if ( level >= TQuote::SIZE )
        return false;
    
    if ( checkFlag & CHECK_PRICE ) {
        if ( kBID == side ) {
            if ( !quote.isBidUpdatePrice(level) )
                return false;
        } else {
            if ( !quote.isAskUpdatePrice(level) )
                return false;
        } 
    } else {
        
        if ( kBID == side ) {
            if ( quote.isBidUpdatePrice(level) )
                return false;
        } else {
            if ( quote.isAskUpdatePrice(level) )
                return false;
        }
    }
    
    if ( checkFlag & CHECK_SIZE ) {
        if ( kBID == side ) {
            if ( !quote.isBidUpdateSize(level) )
                return false;
        } else {
            if ( !quote.isAskUpdateSize(level) )
                return false;
        } 
    } else {
        if ( kBID == side ) {
            if ( quote.isBidUpdateSize(level) )
                return false;
        } else {
            if ( quote.isAskUpdateSize(level) )
                return false;
        }
    }
    
    if ( checkFlag & (CHECK_PRICE | CHECK_SIZE ) ) {
        if ( kBID == side ) {
            if ( !quote.isBidUpdate(level) )
                return false;
        } else {
            if ( !quote.isAskUpdate(level) )
                return false;
        } 
    } else {
        if ( kBID == side ) {
            if ( quote.isBidUpdate(level) )
                return false;
        } else {
            if ( quote.isAskUpdate(level) )
                return false;
        }
    }
    
    return true;
}

bool checkBook(TQuote& quote, size_t level, eBidAsk side, uint32_t checkFlag) {
    for ( uint32_t counter = level; counter < TQuote::SIZE; ++counter ) {
        if ( !checkLevel(quote, counter, side, checkFlag) )
            return false;
    }
    
    return true;
}

TEST(PriceLibTestSuit, quote)
{
    TQuote quote;    
    
    quote.setInstrument(InstrHelper::getNQH2());
    ASSERT_TRUE(quote.isValid());
    
    
    EXPECT_TRUE(!quote.isChanged());    
    
    TTicks p = TTicks(5047);
    TSize s = TSize(10);    
    
    quote.setTrade(p, s);
    EXPECT_TRUE(quote.isChanged());
    EXPECT_TRUE(quote.isTrade());
    
    EXPECT_EQ(quote._trade._price, p);
    EXPECT_EQ(quote._trade._size, s);
        
    for ( size_t level = 0; level < TQuote::SIZE; ++level ) {         
    
        // Check the book from this level and up is not changed
        //
        EXPECT_TRUE(checkBook(quote, level, kBID, CHECK_NOTHING));
        EXPECT_TRUE(checkBook(quote, level, kASK, CHECK_NOTHING));

        // Check setting bid level
        //
        p = TTicks(5035+level);
        s = TSize(5+level);
        
        quote.setBidPrice(p, level);
        EXPECT_TRUE(checkLevel(quote, level, kBID, CHECK_PRICE));
        EXPECT_TRUE(checkBook(quote, level, kASK, CHECK_NOTHING));

        EXPECT_EQ(quote._book[level]._bid._price, p);
        EXPECT_EQ(quote._book[level]._bid._size.get(), TSize::type());

        quote.setBidSize(s, level);
        EXPECT_TRUE(checkLevel(quote, level, kBID, CHECK_PRICE|CHECK_SIZE));
        EXPECT_TRUE(checkBook(quote, level, kASK, CHECK_NOTHING));

        EXPECT_EQ(quote._book[level]._bid._price, p);
        EXPECT_EQ(quote._book[level]._bid._size, s);
        
        // Check setting ask level
        //
        p = TTicks(5085-level);
        s = TSize(45-level);
        
        quote.setAskPrice(p, level);
        EXPECT_TRUE(checkLevel(quote, level, kBID, CHECK_PRICE|CHECK_SIZE));
        EXPECT_TRUE(checkLevel(quote, level, kASK, CHECK_PRICE));

        EXPECT_EQ(quote._book[level]._ask._price, p);
        EXPECT_EQ(quote._book[level]._ask._size.get(), TSize::type());

        quote.setAskSize(s, level);
        EXPECT_TRUE(checkLevel(quote, level, kBID, CHECK_PRICE|CHECK_SIZE));
        EXPECT_TRUE(checkLevel(quote, level, kASK, CHECK_PRICE|CHECK_SIZE));

        EXPECT_EQ(quote._book[level]._ask._price, p);
        EXPECT_EQ(quote._book[level]._ask._size, s);   
    }
    
    EXPECT_EQ(quote.getInstrument()->_tc->toExchangePrice(quote._trade._price), 1261.75);
    
    EXPECT_TRUE(!quote.isOpen());
    EXPECT_TRUE(!quote.isHigh());
    EXPECT_TRUE(!quote.isLow());
    
    p = TTicks(5047);
    quote.setOpen(p);
    EXPECT_TRUE(quote.isOpen());
    EXPECT_EQ(quote.getInstrument()->_tc->toExchangePrice(quote._open), 1261.75);
    
    EXPECT_TRUE(!quote.isHigh());
    EXPECT_TRUE(!quote.isLow());
    
    quote.setHigh(++p);
    EXPECT_TRUE(quote.isHigh());
    EXPECT_EQ(quote.getInstrument()->_tc->toExchangePrice(quote._high), 1262.00);
    
    EXPECT_TRUE(quote.isOpen());    
    EXPECT_TRUE(!quote.isLow());
    
    quote.setLow(++p);
    EXPECT_TRUE(quote.isLow());
    EXPECT_EQ(quote.getInstrument()->_tc->toExchangePrice(quote._low), 1262.25);
    
    EXPECT_TRUE(quote.isOpen());
    EXPECT_TRUE(quote.isHigh());
}


TEST(PriceLibTestSuit, quote_print)
{
    TQuote quote;    
    
    quote.setInstrument(InstrHelper::getZNM2());
    ASSERT_TRUE(quote.isValid());
    
    const tw::price::TicksConverter::TConverterTo& c = quote.getInstrument()->_tc->getConverterTo();
    
    ASSERT_TRUE(!quote._book[0].isValid());
    ASSERT_TRUE(!quote._book[0]._bid.isValid());
    ASSERT_TRUE(!quote._book[0]._ask.isValid());
    
    TTicks p = TTicks(8313);
    TSize s;
    
    quote.setBid(p-2, s, 0, 5);
    quote.setAsk(p+2, s, 0, 4);
    ASSERT_TRUE(!quote._book[0].isValid());
    ASSERT_TRUE(!quote._book[0]._bid.isValid());
    ASSERT_TRUE(!quote._book[0]._ask.isValid());
    
    
    s = TSize(10);
    quote.setTrade(p, s);
    quote.setBid(p-2, s+1, 0, 5);
    quote.setAsk(p+2, s-1, 0, 4);
    
    ASSERT_TRUE(quote._book[0].isValid());
    ASSERT_TRUE(quote._book[0]._bid.isValid());
    ASSERT_TRUE(quote._book[0]._ask.isValid());
    
    EXPECT_EQ(quote._trade.toShortString(c), "10,129.890625,u,u,u,u");
    EXPECT_EQ(quote._book[0]._bid.toShortString(false, c), "5,11,129.859375");
    EXPECT_EQ(quote._book[0]._ask.toShortString(false, c), "4,9,129.921875");
}
