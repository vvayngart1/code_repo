#include <tw/generated/channel_or_defs.h>

#include <boost/lexical_cast.hpp>

#include <gtest/gtest.h>

#include <map>

typedef tw::common::TUuid TOrderId;
typedef tw::common::TUuidBuffer TOrderIdBuffer;

typedef std::map<TOrderId, std::string> TOrderIds;
typedef std::map<TOrderIdBuffer, std::string> TOrderIdBuffers;

TEST(ChannelOrLibTestSuit, order_parse)
{
    tw::channel_or::Order order;

    order._state = tw::channel_or::eOrderState::kWorking;
    order._type = tw::channel_or::eOrderType::kLimit;
    order._side = tw::channel_or::eOrderSide::kBuy;
    order._timeInForce = tw::channel_or::eTimeInForce::kDay;
    order._accountId = 1111;
    order._strategyId = 2222;
    order._instrumentId = 3333;
    order._orderId = tw::common::generateUuid();
    order._qty = tw::price::Size(1);
    order._price = tw::price::Ticks(12567);
    order._newPrice = tw::price::Ticks(12568);
    order._cancelOnAck = false;

    order._exTimestamp.setToNow();
    order._trTimestamp.setToNow();
    order._timestamp1.setToNow();
    order._timestamp2.setToNow();
    order._timestamp3.setToNow();
    order._timestamp4.setToNow();

    tw::channel_or::Order order2;
    ASSERT_TRUE(order2.fromString(order.toString()));

    ASSERT_EQ(order.toString(),order2.toString());
}
