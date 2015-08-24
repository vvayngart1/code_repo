#include <tw/common/uuid.h>

#include <boost/lexical_cast.hpp>

#include <gtest/gtest.h>

#include <map>

typedef tw::common::TUuid TOrderId;
typedef tw::common::TUuidBuffer TOrderIdBuffer;

typedef std::map<TOrderId, std::string> TOrderIds;
typedef std::map<TOrderIdBuffer, std::string> TOrderIdBuffers;

TEST(ChannelOrLibTestSuit, orderId)
{    
    TOrderId o1 = tw::common::generateUuid();
    TOrderId o2 = tw::common::generateUuid();
    TOrderId o3 = tw::common::generateUuid();
    TOrderId o4 = tw::common::generateUuid();
    TOrderId o5 = tw::common::generateUuid();
    
    {
        TOrderIds orders;    
        orders[o1] = "1";
        orders[o2] = "2";
        
        ASSERT_EQ(orders.size(), 2UL);
        ASSERT_TRUE(orders[o1] == "1");
        ASSERT_TRUE(orders[o2] == "2");
    }
    
    {
        TOrderIdBuffers orders;    
        orders[o3] = "3";
        orders[o4] = "4";
        orders[o5] = "5";
        
        ASSERT_EQ(orders.size(), 3UL);
        ASSERT_TRUE(orders[o3] == "3");
        ASSERT_TRUE(orders[o4] == "4");
        ASSERT_TRUE(orders[o5] == "5");
        
        TOrderIdBuffer b1 = o3;
        TOrderIdBuffer b2 = o4;
        TOrderIdBuffer b3 = o5;
        
        ASSERT_TRUE(orders[b1] == "3");
        ASSERT_TRUE(orders[b2] == "4");
        ASSERT_TRUE(orders[b3] == "5");
    
    }
    
    TOrderIdBuffer b1 = o1;
    TOrderIdBuffer b2, b3;
    
    ASSERT_TRUE(b2 != b1);
    
    b2 = o1;    
    ASSERT_TRUE(b2 == b1);
    
    ASSERT_TRUE(b3 != b1);
    
    b3 = boost::lexical_cast<TOrderIdBuffer>(b1.toString());
    ASSERT_TRUE(b3 == b1);    
}
