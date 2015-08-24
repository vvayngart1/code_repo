#include <tw/common_trade/atr.h>

#include <gtest/gtest.h>

namespace tw {
namespace common_trade {
    
const double EPSILON = 0.0001;

typedef tw::price::Ticks TTicks;

TEST(CommonTradeLibTestSuit, atr_test)
{
    Atr atr;
    
    // numOfPeriods = 0
    //
    atr.setNumOfPeriods(0);
    ASSERT_EQ(atr.getNumOfPeriods(), 0);
    
    atr.addRange(TTicks(4));
    ASSERT_NEAR(atr.getAtr(), 0.0, EPSILON);
    
    atr.addRange(TTicks(5));
    ASSERT_NEAR(atr.getAtr(), 0.0, EPSILON);
    
    // numOfPeriods = 4
    //
    atr.setNumOfPeriods(4);
    ASSERT_EQ(atr.getNumOfPeriods(), 4);
    ASSERT_NEAR(atr.getAtr(), 0.0, EPSILON);
    
    atr.addRange(TTicks(4));
    ASSERT_NEAR(atr.getAtr(), 4.0, EPSILON);
    
    atr.addRange(TTicks(5));
    ASSERT_NEAR(atr.getAtr(), 4.5, EPSILON);
    
    atr.addRange(TTicks(9));
    ASSERT_NEAR(atr.getAtr(), 6.0, EPSILON);
    
    atr.addRange(TTicks(2));
    ASSERT_NEAR(atr.getAtr(), 5.0, EPSILON);
    
    atr.addRange(TTicks(2));
    ASSERT_NEAR(atr.getAtr(), 4.5, EPSILON);
    
    atr.addRange(TTicks(7));
    ASSERT_NEAR(atr.getAtr(), 5.0, EPSILON);
    
    // numOfPeriods = 5
    //
    atr.setNumOfPeriods(5);
    ASSERT_EQ(atr.getNumOfPeriods(), 5);
    ASSERT_NEAR(atr.getAtr(), 5.0, EPSILON);
    
    atr.addRange(TTicks(10));
    ASSERT_NEAR(atr.getAtr(), 6.0, EPSILON);
    
    // numOfPeriods = 3
    //
    atr.setNumOfPeriods(3);
    ASSERT_EQ(atr.getNumOfPeriods(), 3);
    ASSERT_NEAR(atr.getAtr(), 6.3333, EPSILON);
    
    atr.addRange(TTicks(4));
    ASSERT_NEAR(atr.getAtr(), 7.0, EPSILON);
            
}



} // common_trade
} // tw
