#include <tw/common/type_wrap.h>

#include <gtest/gtest.h>
#include <sstream>


TEST(CommonLibTestSuit, type_wrap_int32_t)
{   
    typedef tw::common::TypeWrap<int32_t> TTypeWrap;
    int32_t invalidValue = std::numeric_limits<int32_t>::min();
    
    int32_t value = 0;
    double d = 0.0;        
    
    // Test construction/assignment
    //
    TTypeWrap t1;
    TTypeWrap t2(value);
    TTypeWrap t3(t2);
    TTypeWrap t4 = t3;
    
    EXPECT_TRUE(!t1.isValid());
    EXPECT_TRUE(t2.isValid());
    EXPECT_TRUE(t3.isValid());
    EXPECT_TRUE(t4.isValid());    
    
    EXPECT_EQ(t1.get(), invalidValue);
    EXPECT_EQ(t2.get(), value);
    EXPECT_EQ(t3.get(), value);
    EXPECT_EQ(t3.get(), value);
    
    // Test INVALID VALUE
    //
    value = 1;
    t2.set(1);
    t2 = t1 + value;
    EXPECT_TRUE(!t2.isValid());
    
    t2.set(1);
    t2 = t1 + t2;
    EXPECT_TRUE(!t2.isValid());
    
    t2.set(1);
    t2 = t2 + t1;
    EXPECT_TRUE(!t2.isValid());
    
    t2.set(1);
    t2 = t1 - value;
    EXPECT_TRUE(!t2.isValid());
    
    t2.set(1);
    t2 = t1 - t2;
    EXPECT_TRUE(!t2.isValid());
    
    t2.set(1);
    t2 = t2 - t1;
    EXPECT_TRUE(!t2.isValid());    
    
    t2.set(1);
    t2 += t1;
    EXPECT_TRUE(!t2.isValid());
    
    t2.set(1);
    t2 += t1;
    EXPECT_TRUE(!t2.isValid());
    
    t2.set(1);
    t2 -= t1;
    EXPECT_TRUE(!t2.isValid());
    
    t2.set(1);
    t2 = t1*value;
    EXPECT_TRUE(!t2.isValid());
    
    t2.set(1);
    t2 = t1*t2;
    EXPECT_TRUE(!t2.isValid());
    
    t2.set(1);
    t2 = t2*t1;
    EXPECT_TRUE(!t2.isValid());
    
    t2.set(1);
    t2 = t1/value;
    EXPECT_TRUE(!t2.isValid());
    
    t2.set(1);
    t2 = t1/t2;
    EXPECT_TRUE(!t2.isValid());
    
    t2.set(1);
    t2 = t2/t1;
    EXPECT_TRUE(!t2.isValid());
    
    t2.set(1);
    t2 *= t1;
    EXPECT_TRUE(!t2.isValid());
    
    t2.set(1);
    t2 /= t1;
    EXPECT_TRUE(!t2.isValid());
    
    t2.clear();
    ++t2;
    EXPECT_TRUE(!t2.isValid());
    
    t2.clear();
    --t2;
    EXPECT_TRUE(!t2.isValid());    
    
    {
        std::stringstream s;
        s << t2;
        EXPECT_EQ(s.str(), "InV");

        t2.set(1);
        EXPECT_TRUE(t2.isValid());

        s >> t2;
        EXPECT_TRUE(!t2.isValid());
    }
    
    // Test operator+
    //
    value = 1;
    t2.set(value);
    EXPECT_EQ(t2.get(), 1);
    
    t3 = t2 + value;
    EXPECT_EQ(t3.get(), 2);
    
    t4 = t2 + t3;
    EXPECT_EQ(t4.get(), 3);
    
    // Test operator-
    //
    value = 5;
    t1 = t4 - value;
    EXPECT_EQ(t1.get(), -2);
    
    t2 = t4 - t1;
    EXPECT_EQ(t2.get(), 5);
    
    // Test operator+=
    //
    t1.set(0);
    t2.set(1);
    
    value = 5;
    t1 += value;
    EXPECT_EQ(t1.get(), 5);
    
    t2 += t1;
    EXPECT_EQ(t2.get(), 6);
    
    // Test operator-=
    //
    t1.set(0);
    t2.set(1);
    
    value = 5;
    t1 -= value;
    EXPECT_EQ(t1.get(), -5);
    
    t2 -= t1;
    EXPECT_EQ(t2.get(), 6);    
    
    // Test operator*
    //
    t1.set(2);
    t2.set(3);
    t3.set(0);
    t4.set(0);
    
    value = 5;
    t3 = t1*value;
    EXPECT_EQ(t3.get(), 10);
    
    t4 = t1*t2;
    EXPECT_EQ(t4.get(), 6);
    
    // Test operator/
    //
    t1.set(12);
    t2.set(3);
    t3.set(0);
    t4.set(0);
    
    value = 4;
    t3 = t1/value;
    EXPECT_EQ(t3.get(), 3);
    
    t4 = t1/t2;
    EXPECT_EQ(t4.get(), 4);
    
    // Test operator*=
    //
    t1.set(2);
    t2.set(3);
    
    value = 5;
    t1 *= value;
    EXPECT_EQ(t1.get(), 10);
    
    t2 *= t1;
    EXPECT_EQ(t2.get(), 30);
    
    // Test operator/=
    //
    t1.set(36);
    t2.set(18);    
    
    value = 4;
    t1 /= value;
    EXPECT_EQ(t1.get(), 9);
    
    t2 /= t1;
    EXPECT_EQ(t2.get(), 2);
    
    // Test operator++
    //
    t1.set(36);
    ++t1;
    
    EXPECT_EQ(t1.get(), 37);
    
    // Test operator--
    //
    t1.set(36);
    --t1;
    
    EXPECT_EQ(t1.get(), 35);
    
    // Test operator-
    //
    t1.set(36);
    t2 = -t1;
    
    EXPECT_EQ(t2.get(), -36);    
    
    // Test comparison operators
    //
    t1.set(36);
    t2.set(36);
    t3.set(35);
    t4.set(37);
    
    EXPECT_TRUE(t1 == t2);
    EXPECT_TRUE(t1 != t3);
    EXPECT_TRUE(t1 > t3);
    EXPECT_TRUE(t1 < t4);
    
    // Test to/from double
    //
    t1.set(36);
    d = t1.toDouble();
    EXPECT_EQ(d, 36.0);
    
    d = 48.982;
    t1.fromDouble(d);
    EXPECT_EQ(t1.get(), 48);
    
    // Test I/O operators
    //
    {
        std::stringstream s;
    
        t1.set(36);
        s << t1;
        EXPECT_EQ(s.str(), "36");

        t2.set(0);
        s >> t2;
        EXPECT_EQ(t2.get(), 36);
    }
    
    
    // Test gap determination
    //
    t1.set(20);
    t2.set(25);
    
    EXPECT_TRUE(!t1.isGapped(t2, 22));
    EXPECT_TRUE(t2.isGapped(t1, 22));
}
