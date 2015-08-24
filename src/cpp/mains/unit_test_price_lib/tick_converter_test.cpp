#include <gtest/gtest.h>

#include "instr_helper.h"

TEST(PriceLibTestSuit, tickConverter)
{   
    TInstrumentPtr instrument = InstrHelper::getNQH2();
    ASSERT_TRUE(instrument->isValid());
    
    TTickConverter tc(instrument);
    double p = 0.0;
    double fp = 0.0;
    TTicks t1, t2, t3;
    
    // Check empty (num == denom) price converter
    //
    instrument->_tickNumerator = 1;
    instrument->_tickDenominator = 1;    
    tc.resetWithNewInstrument(instrument);
    
    p = 5046; t1 = TTicks(5046);
    EXPECT_EQ(tc.fromExchangePrice(p), t1);
    EXPECT_EQ(tc.toExchangePrice(t1), p);
    EXPECT_EQ(tc.toFractionalExchangePrice(t1.toDouble()), p);
    
    t2 = t1-TTicks(3); p = 5046-3;
    EXPECT_EQ(t2, TTicks(5046-3));    
    EXPECT_EQ(tc.toExchangePrice(t2), p);
    EXPECT_EQ(tc.toFractionalExchangePrice(t2.toDouble()), p);
    
    t3 = t1+TTicks(2); p = 5046+2;
    EXPECT_EQ(t3, TTicks(5046+2));    
    EXPECT_EQ(tc.toExchangePrice(t3), p);
    EXPECT_EQ(tc.toFractionalExchangePrice(t3.toDouble()), p);
    
    // Check num (denom == 1) price converter
    //
    instrument->_tickNumerator = 25;
    instrument->_tickDenominator = 1;    
    tc.resetWithNewInstrument(instrument);
    
    p = 126150; t1 = TTicks(5046);
    EXPECT_EQ(tc.fromExchangePrice(p), t1);
    EXPECT_EQ(tc.toExchangePrice(t1), p);
    EXPECT_EQ(tc.toFractionalExchangePrice(t1.toDouble()), p);
    
    t2 = t1-TTicks(3); p = 126150-75;
    EXPECT_EQ(t2, TTicks(5046-3));    
    EXPECT_EQ(tc.toExchangePrice(t2), p);
    EXPECT_EQ(tc.toFractionalExchangePrice(t2.toDouble()), p);
    
    t3 = t1+TTicks(2); p = 126150+50;
    EXPECT_EQ(t3, TTicks(5046+2));    
    EXPECT_EQ(tc.toExchangePrice(t3), p);
    EXPECT_EQ(tc.toFractionalExchangePrice(t3.toDouble()), p);    
    
    // Check denom (num == 1) price converter
    //
    instrument->_tickNumerator = 1;
    instrument->_tickDenominator = 4;    
    tc.resetWithNewInstrument(instrument);
    
    p = 1261.50; t1 = TTicks(5046);
    EXPECT_EQ(tc.fromExchangePrice(p), t1);
    EXPECT_EQ(tc.toExchangePrice(t1), p);
    EXPECT_EQ(tc.toFractionalExchangePrice(t1.toDouble()), p);
    
    t2 = t1-TTicks(3); p = 1260.75;
    EXPECT_EQ(t2, TTicks(5046-3));    
    EXPECT_EQ(tc.toExchangePrice(t2), p);
    EXPECT_EQ(tc.toFractionalExchangePrice(t2.toDouble()), p);
    
    t3 = t1+TTicks(2); p = 1262.00;
    EXPECT_EQ(t3, TTicks(5046+2));    
    EXPECT_EQ(tc.toExchangePrice(t3), p);
    EXPECT_EQ(tc.toFractionalExchangePrice(t3.toDouble()), p);
    
    // Check full (num != denom 1) price converter
    //
    instrument->_tickNumerator = 2;
    instrument->_tickDenominator = 5;    
    tc.resetWithNewInstrument(instrument);
    
    p = 12; t1 = TTicks(30);
    EXPECT_EQ(tc.fromExchangePrice(p), t1);
    EXPECT_EQ(tc.toExchangePrice(t1), p);
    EXPECT_EQ(tc.toFractionalExchangePrice(t1.toDouble()), p);
    
    t2 = t1-TTicks(3); p = 10.80;
    EXPECT_EQ(t2, TTicks(30-3));    
    EXPECT_EQ(tc.toExchangePrice(t2), p);
    EXPECT_EQ(tc.toFractionalExchangePrice(t2.toDouble()), p);
    
    t3 = t1+TTicks(2); p = 12.80;
    EXPECT_EQ(t3, TTicks(30+2));    
    EXPECT_EQ(tc.toExchangePrice(t3), p);
    EXPECT_EQ(tc.toFractionalExchangePrice(t3.toDouble()), p);
    
    
    // Reset values
    //
    instrument->_tickNumerator = 1;
    instrument->_tickDenominator = 4;    
    tc.resetWithNewInstrument(instrument);    

    t1 = TTicks(5046);
    t2 = t1-TTicks(3);
    t3 = t1+TTicks(2);

    // Fractional price - 5045.6666666667 - is closer to the tick above
    //
    fp = (t1+t2+t3).toDouble()/3.0; // 5045.6666666667 in ticks    
    EXPECT_EQ(tc.nearestTick(fp), TTicks(5046));    
    
    EXPECT_EQ(tc.nearestTickAbove(fp), TTicks(5046));
    EXPECT_EQ(tc.nearestTickBelow(fp), TTicks(5045));
    
    EXPECT_EQ(tc.nextTickAbove(fp), TTicks(5046+1));
    EXPECT_EQ(tc.nextTickBelow(fp), TTicks(5045-1));
    
    EXPECT_EQ(tc.nTicksAbove(fp, TTicks(5)), TTicks(5046+5));
    EXPECT_EQ(tc.nTicksBelow(fp, TTicks(5)), TTicks(5045-5));
    
    EXPECT_EQ(tc.nTicksAbove(fp, TTicks(-5)), TTicks(5045-5));
    EXPECT_EQ(tc.nTicksBelow(fp, TTicks(-5)), TTicks(5046+5));
    
    // The following tests with tolerance demonstrate potential
    // use of the method with edges (in ticks) for calculating bids/asks
    // to send to the market:
    //          bid = nearestTick(fp - egde), ask = nearestTick(fpr+edge)
    //
    // edge = 0.5 
    //
    EXPECT_EQ(tc.nearestTick(fp, -.5), TTicks(5045));   // bid
    EXPECT_EQ(tc.nearestTick(fp, .5), TTicks(5047));    // ask
    
    // edge = 0.25 
    //
    EXPECT_EQ(tc.nearestTick(fp, -.25), TTicks(5045));   // bid
    EXPECT_EQ(tc.nearestTick(fp, .25), TTicks(5046));    // ask
    
    // edge = 0.75 
    //
    EXPECT_EQ(tc.nearestTick(fp, -.75), TTicks(5044));   // bid
    EXPECT_EQ(tc.nearestTick(fp, .75), TTicks(5047));    // ask
    
    
    // Fractional price - 5045.3333333333 - is closer to the tick below
    //
    fp = 5045.3333333333;
    EXPECT_EQ(tc.nearestTick(fp), TTicks(5045));
        
    EXPECT_EQ(tc.nearestTickAbove(fp), TTicks(5046));
    EXPECT_EQ(tc.nearestTickBelow(fp), TTicks(5045));
    
    EXPECT_EQ(tc.nextTickAbove(fp), TTicks(5046+1));
    EXPECT_EQ(tc.nextTickBelow(fp), TTicks(5045-1));
    
    EXPECT_EQ(tc.nTicksAbove(fp, TTicks(5)), TTicks(5046+5));
    EXPECT_EQ(tc.nTicksBelow(fp, TTicks(5)), TTicks(5045-5));
    
    EXPECT_EQ(tc.nTicksAbove(fp, TTicks(-5)), TTicks(5045-5));
    EXPECT_EQ(tc.nTicksBelow(fp, TTicks(-5)), TTicks(5046+5));
    
    // The following tests with tolerance demonstrate potential
    // use of the method with edges (in ticks) for calculating bids/asks
    // to send to the market:
    //          bid = nearestTick(fp - egde), ask = nearestTick(fpr+edge)
    //
    // edge = 0.5 
    //
    EXPECT_EQ(tc.nearestTick(fp, -.5), TTicks(5044));   // bid
    EXPECT_EQ(tc.nearestTick(fp, .5), TTicks(5046));    // ask
    
    // edge = 0.25 
    //
    EXPECT_EQ(tc.nearestTick(fp, -.25), TTicks(5045));   // bid
    EXPECT_EQ(tc.nearestTick(fp, .25), TTicks(5046));    // ask
    
    // edge = 0.75 
    //
    EXPECT_EQ(tc.nearestTick(fp, -.75), TTicks(5044));   // bid
    EXPECT_EQ(tc.nearestTick(fp, .75), TTicks(5047));    // ask
    
    
    // Fractional price - 5045 - is exactly tick
    //
    fp = 5045;
    EXPECT_EQ(tc.nearestTick(fp), TTicks(5045));
    
    EXPECT_EQ(tc.nearestTickAbove(fp), TTicks(5045));
    EXPECT_EQ(tc.nearestTickBelow(fp), TTicks(5045));
    
    EXPECT_EQ(tc.nextTickAbove(fp), TTicks(5045+1));
    EXPECT_EQ(tc.nextTickBelow(fp), TTicks(5045-1));
    
    EXPECT_EQ(tc.nTicksAbove(fp, TTicks(5)), TTicks(5045+5));
    EXPECT_EQ(tc.nTicksBelow(fp, TTicks(5)), TTicks(5045-5));
    
    EXPECT_EQ(tc.nTicksAbove(fp, TTicks(-5)), TTicks(5045-5));
    EXPECT_EQ(tc.nTicksBelow(fp, TTicks(-5)), TTicks(5045+5));
    
    // The following tests with tolerance demonstrate potential
    // use of the method with edges (in ticks) for calculating bids/asks
    // to send to the market:
    //          bid = nearestTick(fp - egde), ask = nearestTick(fpr+edge)
    //
    // edge = 0.5 
    //
    EXPECT_EQ(tc.nearestTick(fp, -.5), TTicks(5044));   // bid
    EXPECT_EQ(tc.nearestTick(fp, .5), TTicks(5046));    // ask
    
    // edge = 0.25 
    //
    EXPECT_EQ(tc.nearestTick(fp, -.25), TTicks(5044));   // bid
    EXPECT_EQ(tc.nearestTick(fp, .25), TTicks(5046));    // ask
    
    // edge = 0.75 
    //
    EXPECT_EQ(tc.nearestTick(fp, -.75), TTicks(5044));   // bid
    EXPECT_EQ(tc.nearestTick(fp, .75), TTicks(5046));    // ask   
}
