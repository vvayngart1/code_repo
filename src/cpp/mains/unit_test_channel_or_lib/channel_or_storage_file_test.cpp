#include <tw/channel_or/channel_or_storage.h>
#include <tw/channel_or/processor_orders.h>

#include "order_helper.h"

#include <gtest/gtest.h>

typedef tw::common::Command TCommand;
typedef tw::channel_or::Order TOrder;
typedef tw::channel_or::TOrderPtr TOrderPtr;
typedef tw::channel_or::Reject TReject;
typedef tw::channel_or::Fill TFill;
typedef tw::channel_or::PosUpdate TPosUpdate;
typedef tw::channel_or::ChannelOrStorageItem TStorageItem;
typedef tw::channel_or::ChannelOrStorageFile TStorageFile;
typedef tw::channel_or::FixSessionCMEState TCMEState;
typedef tw::channel_or::FixSessionCMEMsg TCMEMsg;

typedef tw::channel_or::FillDropCopy TFillDropCopy;

TEST(ChannelOrLibTestSuit, channelOrStorageFile)
{    
    tw::channel_or::Settings settings;
    TStorageFile storage;
    
    TCommand command;
    TOrder order1;
    TOrder order2;
    TOrder order3;
    
    TOrderPtr order2_copy = tw::channel_or::ProcessorOrders::instance().createOrder(false);
    
    TReject rej;
    
    TFill fill1;
    TFill fill2;
    TPosUpdate pos;
    
    TCMEState stateCME;
    TCMEMsg msgCME;
    
    TFillDropCopy fillDropCopy;
    
    TStorageItem item;
    
    OrderHelper::getOrder(order1);
    OrderHelper::getOrder(order2);
    OrderHelper::getOrder(order3);
    
    OrderHelper::getRej(rej);
    
    OrderHelper::getFill(fill1, pos);
    OrderHelper::getFill(fill2, pos);
    
    OrderHelper::getFillDropCopy(fillDropCopy);
    
    
    OrderHelper::getStateCME(stateCME);
    OrderHelper::getMsgCME(msgCME);
    
    *order2_copy = order2;
    fill2._order = order2_copy;
    
    // Test writing to storage
    //
    ASSERT_TRUE(storage.init(settings));
    storage.remove(false);
    
    ASSERT_TRUE(!storage.exists());
    
    ASSERT_TRUE(storage.start());
    ASSERT_TRUE(storage.exists());
    
    
    command._connectionId = 1;
    command._type = tw::common::eCommandType::kChannelOr;
    command._subType = tw::common::eCommandSubType::kList;
    command.addParams("p1", "v1");
    command.addParams("p2", "v2");
    
    item.set(command);
    ASSERT_EQ(item._type, tw::common::eChannelOrStorageItemType::kCommand);
    ASSERT_TRUE(storage.write(item));
    
    item.set(order1);
    ASSERT_EQ(item._type, tw::common::eChannelOrStorageItemType::kOrder);
    ASSERT_TRUE(storage.write(item));
    
    item.set(order2, rej);
    ASSERT_EQ(item._type, tw::common::eChannelOrStorageItemType::kOrderAndRej);
    ASSERT_TRUE(storage.write(item));
    
    item.set(fill1);
    ASSERT_EQ(item._type, tw::common::eChannelOrStorageItemType::kFill);
    ASSERT_TRUE(storage.write(item));
    
    item.set(order3);
    ASSERT_EQ(item._type, tw::common::eChannelOrStorageItemType::kOrder);
    ASSERT_TRUE(storage.write(item));
    
    item.set(fill2);
    ASSERT_EQ(item._type, tw::common::eChannelOrStorageItemType::kFill);
    ASSERT_TRUE(storage.write(item));
    
    item.set(stateCME);
    ASSERT_EQ(item._type, tw::common::eChannelOrStorageItemType::kFixSessionCMEState);
    ASSERT_TRUE(storage.write(item));
    
    item.set(msgCME);
    ASSERT_EQ(item._type, tw::common::eChannelOrStorageItemType::kFixSessionCMEMsg);
    ASSERT_TRUE(storage.write(item));
    
    item.set(fillDropCopy);
    ASSERT_EQ(item._type, tw::common::eChannelOrStorageItemType::kFillDropCopy);
    ASSERT_TRUE(storage.write(item));
    
    ASSERT_TRUE(storage.stop());
    
    // Test reading from storage
    //
    ASSERT_TRUE(storage.start());
    
    item.clear();
    ASSERT_TRUE(storage.read(item));
    ASSERT_EQ(item._type, tw::common::eChannelOrStorageItemType::kCommand);
    ASSERT_EQ(item._command.toString(), command.toString());    
    
    ASSERT_TRUE(storage.read(item));
    ASSERT_EQ(item._type, tw::common::eChannelOrStorageItemType::kOrder);
    ASSERT_EQ(item._order.toString(), order1.toString());
    
    ASSERT_TRUE(storage.read(item));
    ASSERT_EQ(item._type, tw::common::eChannelOrStorageItemType::kOrderAndRej);
    ASSERT_EQ(item._order.toString(), order2.toString());
    ASSERT_EQ(item._rej.toString(), rej.toString());
    
    ASSERT_TRUE(storage.read(item));
    ASSERT_EQ(item._type, tw::common::eChannelOrStorageItemType::kFill);
    ASSERT_EQ(item._fill.toString(), fill1.toString());
    ASSERT_TRUE(item._fill._order == NULL);
    
    ASSERT_TRUE(storage.read(item));
    ASSERT_EQ(item._type, tw::common::eChannelOrStorageItemType::kOrder);
    ASSERT_EQ(item._order.toString(), order3.toString());
    
    ASSERT_TRUE(storage.read(item));
    ASSERT_EQ(item._type, tw::common::eChannelOrStorageItemType::kFill);
    ASSERT_EQ(item._fill.toString(), fill2.toString());
    ASSERT_TRUE(item._fill._order != NULL);
    ASSERT_EQ(item._fill._order->toString(), order2.toString());
    ASSERT_EQ(item._order.toString(), order2.toString());
    
    ASSERT_TRUE(storage.read(item));
    ASSERT_EQ(item._type, tw::common::eChannelOrStorageItemType::kFixSessionCMEState);
    ASSERT_EQ(item._stateCME.toString(), stateCME.toString());
    
    ASSERT_TRUE(storage.read(item));
    ASSERT_EQ(item._type, tw::common::eChannelOrStorageItemType::kFixSessionCMEMsg);
    ASSERT_EQ(item._msgCME.toString(), msgCME.toString());
    
    ASSERT_TRUE(storage.read(item));
    ASSERT_EQ(item._type, tw::common::eChannelOrStorageItemType::kFillDropCopy);
    ASSERT_EQ(item._fillDropCopy.toString(), fillDropCopy.toString());
    
    ASSERT_TRUE(storage.read(item));
    ASSERT_EQ(item._type, tw::common::eChannelOrStorageItemType::kUnknown);
    
    ASSERT_TRUE(storage.remove(false));
    ASSERT_TRUE(!storage.exists());
    
}
