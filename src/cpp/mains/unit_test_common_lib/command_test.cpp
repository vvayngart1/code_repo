#include <tw/common/command.h>
#include <tw/generated/channel_or_defs.h>

#include <gtest/gtest.h>


TEST(CommonLibTestSuit, command)
{   
    tw::common::Command c;
    std::string message;
    
    std::string temp;
    int32_t i = 0;
    double d = 0.0;
    
    message = "ChannelOr,Logon,user=test;passwd=1234;token1=5;token2=6.5";
    ASSERT_TRUE(c.fromString(message));
    
    ASSERT_EQ(c._type, tw::common::eCommandType::kChannelOr);
    ASSERT_EQ(c._subType, tw::common::eCommandSubType::kLogon);
    
    ASSERT_TRUE(c.has("user"));
    
    // Case insensitive compare
    //
    ASSERT_TRUE(c.has("uSeR"));
    
    ASSERT_TRUE(c.has("passwd"));
    ASSERT_TRUE(c.has("token1"));
    ASSERT_TRUE(c.has("token2"));
    ASSERT_TRUE(!c.has("token3"));
        
    ASSERT_TRUE(c.get("user", temp));
    ASSERT_EQ(temp, "test");
    
    ASSERT_TRUE(c.get("passwd", temp));
    ASSERT_EQ(temp, "1234");
    
    ASSERT_TRUE(c.get("token1", i));
    ASSERT_EQ(i, 5);
    
    ASSERT_TRUE(c.get("token2", d));
    ASSERT_EQ(d, 6.5);
    
    ASSERT_TRUE(!c.get("token3", d));
}

TEST(CommonLibTestSuit, commandExtOrder)
{   
    tw::common::Command c;
    std::string message;
    
    message = "ChannelOr,ExtNew,type=Limit;side=Buy;timeInForce=Day;instrumentId=3;qty=5;price=125725";
    ASSERT_TRUE(c.fromString(message));
    
    tw::channel_or::OrderWire order = tw::channel_or::OrderWire::fromCommand(c);
    ASSERT_EQ(order._type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(order._side, tw::channel_or::eOrderSide::kBuy);
    ASSERT_EQ(order._timeInForce, tw::channel_or::eTimeInForce::kDay);
    ASSERT_EQ(order._instrumentId, 3U);
    ASSERT_EQ(order._qty, 5U);
    ASSERT_EQ(order._price, 125725);
}

TEST(CommonLibTestSuit, commandOperatorPlus)
{   
    tw::common::Command c;
    tw::common::Command cmndToAdd;
    std::string message;
    
    std::string temp;
    int32_t i = 0;
    double d = 0.0;
    
    message = "ChannelOr,Logon,user=test;passwd=1234;token1=5;token2=6.5";
    ASSERT_TRUE(c.fromString(message));
    
    cmndToAdd.addParams("token3", 7.8);
    cmndToAdd.addParams("token4", "info");
    
    c += cmndToAdd;
    
    ASSERT_EQ(c._type, tw::common::eCommandType::kChannelOr);
    ASSERT_EQ(c._subType, tw::common::eCommandSubType::kLogon);
    
    ASSERT_TRUE(c.has("user"));
    
    // Case insensitive compare
    //
    ASSERT_TRUE(c.has("uSeR"));
    
    ASSERT_TRUE(c.has("passwd"));
    ASSERT_TRUE(c.has("token1"));
    ASSERT_TRUE(c.has("token2"));
    ASSERT_TRUE(c.has("token3"));
    ASSERT_TRUE(c.has("token4"));
        
    ASSERT_TRUE(c.get("user", temp));
    ASSERT_EQ(temp, "test");
    
    ASSERT_TRUE(c.get("passwd", temp));
    ASSERT_EQ(temp, "1234");
    
    ASSERT_TRUE(c.get("token1", i));
    ASSERT_EQ(i, 5);
    
    ASSERT_TRUE(c.get("token2", d));
    ASSERT_EQ(d, 6.5);
    
    ASSERT_TRUE(c.get("token3", d));
    ASSERT_EQ(d, 7.8);
    
    ASSERT_TRUE(c.get("token4", temp));
    ASSERT_EQ(temp, "info");
}

TEST(CommonLibTestSuit, command_type_subtype_only)
{   
    tw::common::Command c;
    std::string message;
    
    message = "ChannelOr,Logon";
    ASSERT_TRUE(c.fromString(message));
    
    ASSERT_EQ(c._type, tw::common::eCommandType::kChannelOr);
    ASSERT_EQ(c._subType, tw::common::eCommandSubType::kLogon);
}
