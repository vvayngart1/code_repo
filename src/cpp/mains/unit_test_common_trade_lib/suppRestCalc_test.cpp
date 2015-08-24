#include <tw/common_trade/suppRestCalc.h>

#include <gtest/gtest.h>

namespace tw {
namespace common_trade {
    
const double EPSILON = 0.0001;

typedef tw::price::Ticks TTicks;

typedef SuppRestCalc TImpl;

TEST(CommonTradeLibTestSuit, suppRestCalc_test)
{
    TTicks h;
    TTicks l;
    double c = 0.0;
    
    double x = 0.0;
    double pp = 0.0;
    
    // Test PP calc
    //
    ASSERT_TRUE(!TImpl::calcPP(h, l, c, x));
    ASSERT_NEAR(x, 0.0, EPSILON);
    
    h.set(30);
    ASSERT_TRUE(!TImpl::calcPP(h, l, c, x));
    ASSERT_NEAR(x, 0.0, EPSILON);
    
    l.set(24);
    c = 28;
    ASSERT_TRUE(TImpl::calcPP(h, l, c, x));
    ASSERT_NEAR(x, 27.3333, EPSILON);
    
    pp = x;

    // Test S1
    //
    h.clear();
    ASSERT_TRUE(!TImpl::calcS1(h, pp, x));
    ASSERT_NEAR(x, 0.0, EPSILON);
    
    h.set(30);
    ASSERT_TRUE(TImpl::calcS1(h, pp, x));
    ASSERT_NEAR(x, 24.6667, EPSILON);
    
    // Test R1
    //
    l.clear();
    ASSERT_TRUE(!TImpl::calcR1(l, pp, x));
    ASSERT_NEAR(x, 0.0, EPSILON);
    
    l.set(24);
    ASSERT_TRUE(TImpl::calcR1(l, pp, x));
    ASSERT_NEAR(x, 30.6667, EPSILON);
    
    
    // Test S2
    //
    h.clear();
    l.clear();
    ASSERT_TRUE(!TImpl::calcS2(h, l, pp, x));
    ASSERT_NEAR(x, 0.0, EPSILON);
    
    h.set(30);
    ASSERT_TRUE(!TImpl::calcS2(h, l, pp, x));
    ASSERT_NEAR(x, 0.0, EPSILON);
    
    l.set(24);
    ASSERT_TRUE(TImpl::calcS2(h, l, pp, x));
    ASSERT_NEAR(x, 21.3333, EPSILON);
    
    
    // Test R2
    //
    h.clear();
    l.clear();
    ASSERT_TRUE(!TImpl::calcR2(h, l, pp, x));
    ASSERT_NEAR(x, 0.0, EPSILON);
    
    h.set(30);
    ASSERT_TRUE(!TImpl::calcR2(h, l, pp, x));
    ASSERT_NEAR(x, 0.0, EPSILON);
    
    l.set(24);
    ASSERT_TRUE(TImpl::calcR2(h, l, pp, x));
    ASSERT_NEAR(x, 33.3333, EPSILON);
            
}



} // common_trade
} // tw
