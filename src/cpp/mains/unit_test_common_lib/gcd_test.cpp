#include <tw/common/fractional.h>

#include <gtest/gtest.h>

TEST(CommonLibTestSuit, gcd)
{    
    tw::common::Fractional::TFraction f;
    f = tw::common::Fractional::getFraction(1.0/8.0);
    EXPECT_EQ(f.first, 1U);
    EXPECT_EQ(f.second, 8U);
    
    f = tw::common::Fractional::getFraction(1.0/32.0);
    EXPECT_EQ(f.first, 1U);
    EXPECT_EQ(f.second, 32U);    
    
    f = tw::common::Fractional::getFraction(1.0/64.0);
    EXPECT_EQ(f.first, 1U);
    EXPECT_EQ(f.second, 64U);
    
    f = tw::common::Fractional::getFraction(1.0/128.0);
    EXPECT_EQ(f.first, 1U);
    EXPECT_EQ(f.second, 128U);
    
    f = tw::common::Fractional::getFraction(1.0/100.0);
    EXPECT_EQ(f.first, 1U);
    EXPECT_EQ(f.second, 100U);
    
    f = tw::common::Fractional::getFraction(1.0/10000.0);
    EXPECT_EQ(f.first, 1U);
    EXPECT_EQ(f.second, 10000U);
    
    f = tw::common::Fractional::getFraction(1.0/100000000.0);
    EXPECT_EQ(f.first, 1U);
    EXPECT_EQ(f.second, 100000000U);    
    
    f = tw::common::Fractional::getFraction(1.0/4.0);
    EXPECT_EQ(f.first, 1U);
    EXPECT_EQ(f.second, 4U);
    
    f = tw::common::Fractional::getFraction(25.0);
    EXPECT_EQ(f.first, 25U);
    EXPECT_EQ(f.second, 1U);
    
}

TEST(CommonLibTestSuit, gcd_precision)
{    
    uint32_t p = 0;
    
    p = tw::common::Fractional::getPrecision(1, 8);
    EXPECT_EQ(p, 3U); 
    
    p = tw::common::Fractional::getPrecision(1, 32);
    EXPECT_EQ(p, 5U);    
    
    p = tw::common::Fractional::getPrecision(1, 64);
    EXPECT_EQ(p, 6U);
    
    p = tw::common::Fractional::getPrecision(1, 128);
    EXPECT_EQ(p, 7U);    
    
    p = tw::common::Fractional::getPrecision(1, 100);
    EXPECT_EQ(p, 2U);    
    
    p = tw::common::Fractional::getPrecision(1, 10000);
    EXPECT_EQ(p, 4U);    
    
    p = tw::common::Fractional::getPrecision(1, 1000000);
    EXPECT_EQ(p, 6U);
    
    p = tw::common::Fractional::getPrecision(1, 10000000);
    EXPECT_EQ(p, 7U);
    
    p = tw::common::Fractional::getPrecision(1, 4);
    EXPECT_EQ(p, 2U);
    
    p = tw::common::Fractional::getPrecision(1, 1);
    EXPECT_EQ(p, 0U);
    
    p = tw::common::Fractional::getPrecision(25, 1);
    EXPECT_EQ(p, 0U);
    
}

