#include <tw/channel_or/orders_table.h>

#include <gtest/gtest.h>

typedef tw::channel_or::OrderTable<> TOrderTable;

TEST(ChannelOrLibTestSuit, ordersTable)
{
    tw::channel_or::TOrderPtr o1(new tw::channel_or::Order());
    tw::channel_or::TOrderPtr o2(new tw::channel_or::Order());
    tw::channel_or::TOrderPtr o3(new tw::channel_or::Order());
    tw::channel_or::TOrderPtr o4(new tw::channel_or::Order());
    
    o1->_orderId = tw::common::generateUuid();
    o2->_orderId = tw::common::generateUuid();
    o3->_orderId = tw::common::generateUuid();
    o4->_orderId = tw::common::generateUuid();
    
    tw::common::TUuidBuffer uuid = tw::common::generateUuid();
    
    o1->_accountId = 1;
    o2->_accountId = 1;
    o3->_accountId = 1;
    o4->_accountId = 2;
    
    o1->_strategyId = 3;
    o2->_strategyId = 3;
    o3->_strategyId = 4;
    o4->_strategyId = 5;
    
    o1->_instrumentId = 6;
    o2->_instrumentId = 6;
    o3->_instrumentId = 6;
    o4->_instrumentId = 7;
    
    TOrderTable table;
    tw::channel_or::TOrders orders;
    tw::channel_or::TOrders::iterator ordersIter;
    
    // Test add/exist/get
    //
    ASSERT_TRUE(table.add(o1->_orderId, o1));
    ASSERT_TRUE(!table.add(o1->_orderId, o1));
    
    ASSERT_TRUE(table.exist(o1->_orderId));
    ASSERT_TRUE(!table.exist(o2->_orderId));
    ASSERT_TRUE(!table.exist(o3->_orderId));
    ASSERT_TRUE(!table.exist(o4->_orderId));
    ASSERT_TRUE(!table.exist(uuid));
    
    ASSERT_TRUE(table.add(o2->_orderId, o2));
    ASSERT_TRUE(table.add(o3->_orderId, o3));
    ASSERT_TRUE(table.add(o4->_orderId, o4));
    
    ASSERT_TRUE(table.exist(o1->_orderId));
    ASSERT_TRUE(table.exist(o2->_orderId));
    ASSERT_TRUE(table.exist(o3->_orderId));
    ASSERT_TRUE(table.exist(o4->_orderId));
    ASSERT_TRUE(!table.exist(uuid));
    
    ASSERT_EQ(table.get(o1->_orderId), o1);
    ASSERT_EQ(table.get(o2->_orderId), o2);
    ASSERT_EQ(table.get(o3->_orderId), o3);
    ASSERT_EQ(table.get(o4->_orderId), o4);
    
    // Test filtering
    //
    
    // Null filter - all orders
    //
    orders = table.get(tw::channel_or::FilterNull());
    ASSERT_EQ(orders.size(), 4U);
    
    ordersIter = std::find(orders.begin(), orders.end(), o1);
    ASSERT_TRUE(ordersIter != orders.end());
    ASSERT_EQ((*ordersIter).get(), o1.get());
    
    ordersIter = std::find(orders.begin(), orders.end(), o2);
    ASSERT_TRUE(ordersIter != orders.end());
    ASSERT_EQ((*ordersIter).get(), o2.get());
    
    ordersIter = std::find(orders.begin(), orders.end(), o3);
    ASSERT_TRUE(ordersIter != orders.end());
    ASSERT_EQ((*ordersIter).get(), o3.get());
    
    ordersIter = std::find(orders.begin(), orders.end(), o4);
    ASSERT_TRUE(ordersIter != orders.end());
    ASSERT_EQ((*ordersIter).get(), o4.get());
    
    // AccountId filter
    //
    orders = table.get(tw::channel_or::FilterAccountId(1));
    ASSERT_EQ(orders.size(), 3U);
    
    ordersIter = std::find(orders.begin(), orders.end(), o1);
    ASSERT_TRUE(ordersIter != orders.end());
    ASSERT_EQ((*ordersIter).get(), o1.get());
    
    ordersIter = std::find(orders.begin(), orders.end(), o2);
    ASSERT_TRUE(ordersIter != orders.end());
    ASSERT_EQ((*ordersIter).get(), o2.get());
    
    ordersIter = std::find(orders.begin(), orders.end(), o3);
    ASSERT_TRUE(ordersIter != orders.end());
    ASSERT_EQ((*ordersIter).get(), o3.get());
    
    ordersIter = std::find(orders.begin(), orders.end(), o4);
    ASSERT_TRUE(ordersIter == orders.end());
    
    // StrategyId/AccountId filter
    //
    orders = table.get(tw::channel_or::FilterAccountIdStrategyId(1, 3));
    ASSERT_EQ(orders.size(), 2U);
    
    ordersIter = std::find(orders.begin(), orders.end(), o1);
    ASSERT_TRUE(ordersIter != orders.end());
    ASSERT_EQ((*ordersIter).get(), o1.get());
    
    ordersIter = std::find(orders.begin(), orders.end(), o2);
    ASSERT_TRUE(ordersIter != orders.end());
    ASSERT_EQ((*ordersIter).get(), o2.get());
    
    ordersIter = std::find(orders.begin(), orders.end(), o3);
    ASSERT_TRUE(ordersIter == orders.end());
    
    ordersIter = std::find(orders.begin(), orders.end(), o4);
    ASSERT_TRUE(ordersIter == orders.end());    
    
    // InstrumentId filter
    //
    orders = table.get(tw::channel_or::FilterInstrumentId(7));
    ASSERT_EQ(orders.size(), 1U);
    
    ordersIter = std::find(orders.begin(), orders.end(), o1);
    ASSERT_TRUE(ordersIter == orders.end());
    
    ordersIter = std::find(orders.begin(), orders.end(), o2);
    ASSERT_TRUE(ordersIter == orders.end());    
    
    ordersIter = std::find(orders.begin(), orders.end(), o3);
    ASSERT_TRUE(ordersIter == orders.end());
    
    ordersIter = std::find(orders.begin(), orders.end(), o4);
    ASSERT_TRUE(ordersIter != orders.end());
    ASSERT_EQ((*ordersIter).get(), o4.get());
    
    // Test rem
    //
    ASSERT_TRUE(table.rem(o1->_orderId));
    ASSERT_TRUE(!table.exist(o1->_orderId));
    ASSERT_TRUE(!table.rem(o1->_orderId));
    
    orders = table.get(tw::channel_or::FilterNull());
    ASSERT_EQ(orders.size(), 3U);
    
    ASSERT_TRUE(table.rem(o2->_orderId));
    orders = table.get(tw::channel_or::FilterNull());
    ASSERT_EQ(orders.size(), 2U);
    
    ASSERT_TRUE(table.rem(o3->_orderId));
    orders = table.get(tw::channel_or::FilterNull());
    ASSERT_EQ(orders.size(), 1U);
    
    ASSERT_TRUE(table.rem(o4->_orderId));    
    orders = table.get(tw::channel_or::FilterNull());
    ASSERT_EQ(orders.size(), 0U);
    
}
