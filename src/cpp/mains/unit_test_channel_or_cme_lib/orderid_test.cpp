#include <tw/channel_or_cme/id_factory.h>

#include <gtest/gtest.h>

typedef tw::channel_or_cme::Id TOrderId;
typedef tw::channel_or_cme::IdFactory TOrderIdFactory;

TEST(ChannelOrCmeLibTestSuit, orderId)
{
    TOrderId::Type lastOrderId = 0;
    TOrderId next;
    std::string temp;
    
    lastOrderId = 1;
    next.generate(lastOrderId);
    temp = next.c_str();
    ASSERT_EQ(temp.size(), 1U);
    ASSERT_EQ(temp, "1");    
    
    lastOrderId = 2;    
    next.generate(lastOrderId);
    temp = next.c_str();
    ASSERT_EQ(temp.size(), 1U);
    ASSERT_EQ(temp, "2");
    
    lastOrderId = 35;    
    next.generate(lastOrderId);
    temp = next.c_str();
    ASSERT_EQ(temp.size(), 1U);
    ASSERT_EQ(temp, "Z");
    
    lastOrderId = 36;    
    next.generate(lastOrderId);
    temp = next.c_str();
    ASSERT_EQ(temp.size(), 2U);
    ASSERT_EQ(temp, "10");
    
    lastOrderId = 71;    
    next.generate(lastOrderId);
    temp = next.c_str();
    ASSERT_EQ(temp.size(), 2U);
    ASSERT_EQ(temp, "1Z");
    
    lastOrderId = 36*36;    
    next.generate(lastOrderId);
    temp = next.c_str();
    ASSERT_EQ(temp.size(), 3U);
    ASSERT_EQ(temp, "100");    
    
    next.generate(lastOrderId+1);
    temp = next.c_str();
    ASSERT_EQ(temp.size(), 3U);
    ASSERT_EQ(temp, "101");        
    
    next.generate(lastOrderId+35);
    temp = next.c_str();
    ASSERT_EQ(temp.size(), 3U);
    ASSERT_EQ(temp, "10Z");
    
    lastOrderId = 36*36*36;    
    next.generate(lastOrderId);
    temp = next.c_str();
    ASSERT_EQ(temp.size(), 4U);
    ASSERT_EQ(temp, "1000");
    
    next.generate(lastOrderId+1);
    temp = next.c_str();
    ASSERT_EQ(temp.size(), 4U);
    ASSERT_EQ(temp, "1001");        
    
    next.generate(lastOrderId+35);
    temp = next.c_str();
    ASSERT_EQ(temp.size(), 4U);
    ASSERT_EQ(temp, "100Z");
    
    
    lastOrderId = 36*36*36*36;    
    next.generate(lastOrderId);
    temp = next.c_str();
    ASSERT_EQ(temp.size(), 5U);
    ASSERT_EQ(temp, "10000");
    
    next.generate(lastOrderId+1);
    temp = next.c_str();
    ASSERT_EQ(temp.size(), 5U);
    ASSERT_EQ(temp, "10001");        
    
    next.generate(lastOrderId+35);
    temp = next.c_str();
    ASSERT_EQ(temp.size(), 5U);
    ASSERT_EQ(temp, "1000Z");
    
    
    lastOrderId = 36*36*36*36*36;    
    next.generate(lastOrderId);
    temp = next.c_str();
    ASSERT_EQ(temp.size(), 6U);
    ASSERT_EQ(temp, "100000");
    
    next.generate(lastOrderId+1);
    temp = next.c_str();
    ASSERT_EQ(temp.size(), 6U);
    ASSERT_EQ(temp, "100001");        
    
    next.generate(lastOrderId+35);
    temp = next.c_str();
    ASSERT_EQ(temp.size(), 6U);
    ASSERT_EQ(temp, "10000Z");
    
    
    lastOrderId = 36LL*36LL*36LL*36LL*36LL*36LL*36LL*36LL*36LL*36LL*36LL*36LL;    
    next.generate(lastOrderId);
    temp = next.c_str();
    ASSERT_EQ(temp.size(), 13U);
    ASSERT_EQ(temp, "1000000000000");
    
    next.generate(lastOrderId+1);
    temp = next.c_str();
    ASSERT_EQ(temp.size(), 13U);
    ASSERT_EQ(temp, "1000000000001");       
    
    next.generate(lastOrderId+35);
    temp = next.c_str();
    ASSERT_EQ(temp.size(), 13U);
    ASSERT_EQ(temp, "100000000000Z");
}

TEST(ChannelOrCmeLibTestSuit, orderIdFactory)
{
    TOrderIdFactory factory;
    std::string temp;
    TOrderId::Type lastOrderId;
    
    lastOrderId = 0;
    ASSERT_TRUE(factory.start(lastOrderId, 1024));
    
    temp = factory.get().c_str();
    ASSERT_EQ(temp.size(), 1U);
    ASSERT_EQ(temp, "1");
    
    temp = factory.get().c_str();
    ASSERT_EQ(temp.size(), 1U);
    ASSERT_EQ(temp, "2");
    
    factory.stop();
    
    lastOrderId = 36*36*36*36*36;
    lastOrderId *= 36;
    lastOrderId += 34;
    ASSERT_TRUE(factory.start(lastOrderId, 1024));
    
    temp = factory.get().c_str();
    ASSERT_EQ(temp.size(), 7U);
    ASSERT_EQ(temp, "100000Z");
    
    temp = factory.get().c_str();
    ASSERT_EQ(temp.size(), 7U);
    ASSERT_EQ(temp, "1000010");
    
    
    factory.stop();
}
