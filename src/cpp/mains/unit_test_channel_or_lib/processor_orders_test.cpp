#include <tw/channel_or/processor_orders.h>
#include <tw/common_strat/consumer_proxy.h>
#include <tw/common_thread/utils.h>

#include "order_helper.h"

#include <gtest/gtest.h>

typedef tw::channel_or::TOrderPtr TOrderPtr;
typedef tw::channel_or::Reject Reject;
typedef tw::channel_or::Fill Fill;
typedef tw::channel_or::PosUpdate PosUpdate;
typedef tw::channel_or::Alert Alert;
typedef tw::channel_or::TOrders TOrders;

typedef tw::channel_or::ProcessorOrders TProcessor;

class CallbackClient {
public:
    std::vector<tw::channel_or::Alert> _alerts;
    void onAlert(const tw::channel_or::Alert& alert) {
        _alerts.push_back(alert);
    }
};

TEST(ChannelOrLibTestSuit, processor_orders)
{
    // Structs for passing to processor
    //
    TOrderPtr o1;
    TOrderPtr o2;
    Fill fill;
    Reject rej;
    
    // Instantiation of processor
    //
    TProcessor& p = TProcessor::instance();
    p.clear();
    ASSERT_TRUE(p.start());
    
    // Check SendNew/OnNewAck/OnNewRej methods
    //
    o1 = p.createOrder();
    ASSERT_TRUE(o1.get() != NULL);
    ASSERT_TRUE(o1->_orderId != tw::common::TUuidBuffer());
    
    o1->_qty.set(5);
    o1->_price.set(10);
    o1->_accountId = 1;
    o1->_strategyId = 2;
    o1->_instrumentId = 5;
    
    ASSERT_TRUE(p.sendNew(o1, rej));
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kPending);
    ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrNew);
    ASSERT_EQ(o1->_modCounter, 0U);
    
    ASSERT_EQ(p.getAll().size(), 1U);
    
    rej.clear();
    o1->_action = tw::common::eCommandSubType::kUnknown;
    ASSERT_TRUE(!p.sendNew(o1, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorOrders);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kInvalidOrderState);
    ASSERT_EQ(o1->_modCounter, 0U);
    ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrNew);
    
    o2 = p.createOrder();
    ASSERT_TRUE(o2.get() != NULL);
    ASSERT_TRUE(o2->_orderId != tw::common::TUuidBuffer());    
    
    o2->_accountId = 2;
    o2->_strategyId = 3;
    o2->_instrumentId = 5;
    
    rej.clear();
    ASSERT_TRUE(!p.sendNew(o2, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorOrders);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kInvalidOrderQty);
    ASSERT_EQ(o2->_modCounter, 0U);
    ASSERT_EQ(o2->_action, tw::common::eCommandSubType::kOrNewRej);
    
    // Check that can reject newly placed order
    //
    p.onNewRej(o2, rej);
    ASSERT_EQ(o2->_state, tw::channel_or::eOrderState::kRejected);
    
    o2->_qty.set(7);
    o2->_action = tw::common::eCommandSubType::kUnknown;
    o2->_state = tw::channel_or::eOrderState::kUnknown;
    ASSERT_TRUE(p.sendNew(o2, rej));
    ASSERT_EQ(o2->_state, tw::channel_or::eOrderState::kPending);
    ASSERT_EQ(o2->_modCounter, 0U);
    ASSERT_EQ(o2->_action, tw::common::eCommandSubType::kOrNew);
    
    ASSERT_EQ(p.getAll().size(), 2U);
    ASSERT_EQ(p.getAllForAccount(1).size(), 1U);
    ASSERT_EQ(p.getAllForAccountStrategy(2, 3).size(), 1U);
    ASSERT_EQ(p.getAllForAccountStrategyInstr(2, 3, 5).size(), 1U);
    ASSERT_EQ(p.getAllForAccountStrategyInstr(2, 3, 4).size(), 0U);
    ASSERT_EQ(p.getAllForAccountStrategyInstr(1, 2, 5).size(), 1U);
    ASSERT_EQ(p.getAllForAccountStrategyInstr(1, 1, 5).size(), 0U);
    ASSERT_EQ(p.getAllForInstrument(5).size(), 2U);
    
    p.onNewAck(o1);
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kWorking);
    ASSERT_EQ(o1->_modCounter, 0U);
    ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrNewAck);
    
    p.onNewRej(o2, rej);
    ASSERT_EQ(o2->_state, tw::channel_or::eOrderState::kRejected); 
    ASSERT_EQ(p.getAll().size(), 1U);
    ASSERT_EQ(o2->_modCounter, 0U);
    ASSERT_EQ(o2->_action, tw::common::eCommandSubType::kOrNewRej);
    
    o2 = p.createOrder();
    ASSERT_TRUE(o2.get() != NULL);
    ASSERT_TRUE(o2->_orderId != tw::common::TUuidBuffer());    
    
    o2->_qty.set(7);
    o2->_price.set(20);
    o2->_accountId = 2;
    o2->_strategyId = 3;
    o2->_instrumentId = 5;
    
    ASSERT_TRUE(p.sendNew(o2, rej));
    ASSERT_EQ(o2->_state, tw::channel_or::eOrderState::kPending);
    ASSERT_EQ(p.getAll().size(), 2U);
    ASSERT_EQ(o2->_modCounter, 0U);
    ASSERT_EQ(o2->_action, tw::common::eCommandSubType::kOrNew);
    
    p.onNewAck(o2);
    ASSERT_EQ(o2->_state, tw::channel_or::eOrderState::kWorking);
    ASSERT_EQ(o2->_action, tw::common::eCommandSubType::kOrNewAck);
    ASSERT_EQ(o2->_modCounter, 0U);
    
    // Check SendMod/OnModAck/OnModRej methods
    //
    o1->_newPrice.set(11);
    rej.clear();
    
    ASSERT_TRUE(p.sendMod(o1, rej));
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kModifying);
    ASSERT_EQ(o1->_qty, 5U);
    ASSERT_EQ(o1->_price, 10);
    ASSERT_EQ(o1->_newPrice, 11);
    ASSERT_EQ(o1->_modCounter, 1U);
    ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrMod);
    
    o1->_newPrice.set(12);
    o1->_action = tw::common::eCommandSubType::kUnknown;
    ASSERT_TRUE(p.sendMod(o1, rej));    
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kModifying);
    ASSERT_EQ(o1->_qty, 5U);
    ASSERT_EQ(o1->_price, 10);
    ASSERT_EQ(o1->_newPrice, 12);    
    ASSERT_EQ(o1->_modCounter, 2U);
    ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrMod);
    
    p.onModRej(o1, rej);
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kModifying);
    ASSERT_EQ(o1->_qty, 5U);
    ASSERT_EQ(o1->_price, 10);
    ASSERT_EQ(o1->_newPrice, 12);
    ASSERT_EQ(o1->_modCounter, 2U);
    ASSERT_EQ(o1->_modRejCounter, 1U);
    ASSERT_EQ(o1->_cxlRejCounter, 0U);
    ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrModRej);
    
    
    p.onModRejPost(o1, rej);
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kModifying);
    ASSERT_EQ(o1->_qty, 5U);
    ASSERT_EQ(o1->_price, 10);
    ASSERT_EQ(o1->_newPrice, tw::price::Ticks::INVALID_VALUE());
    ASSERT_EQ(o1->_modCounter, 1U);
    
    o1->_newPrice.set(12);
    o1->_action = tw::common::eCommandSubType::kUnknown;
    p.onModAck(o1);
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kModifying);
    ASSERT_EQ(o1->_qty, 5U);
    ASSERT_EQ(o1->_price, 10);
    ASSERT_EQ(o1->_newPrice, 12);
    ASSERT_EQ(o1->_modCounter, 1U);
    ASSERT_EQ(o1->_modRejCounter, 0U);
    ASSERT_EQ(o1->_cxlRejCounter, 0U);
    ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrModAck);
    
    p.onModAckPost(o1);
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kWorking);
    ASSERT_EQ(o1->_qty, 5U);
    ASSERT_EQ(o1->_price, 12);
    ASSERT_EQ(o1->_newPrice, tw::price::Ticks::INVALID_VALUE());
    ASSERT_EQ(o1->_modCounter, 0U);
    
    o1->_newPrice.set(11);
    
    ASSERT_TRUE(p.sendMod(o1, rej));
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kModifying);
    ASSERT_EQ(o1->_qty, 5U);
    ASSERT_EQ(o1->_price, 12);
    ASSERT_EQ(o1->_newPrice, 11);
    ASSERT_EQ(o1->_modCounter, 1U);
    ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrMod);
    
    o1->_newPrice.set(10);
    o1->_action = tw::common::eCommandSubType::kUnknown;
    ASSERT_TRUE(p.sendMod(o1, rej));
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kModifying);
    ASSERT_EQ(o1->_qty, 5U);
    ASSERT_EQ(o1->_price, 12);
    ASSERT_EQ(o1->_newPrice, 10);
    ASSERT_EQ(o1->_modCounter, 2U);
    ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrMod);
    
    p.onModAck(o1);
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kModifying);
    ASSERT_EQ(o1->_qty, 5U);
    ASSERT_EQ(o1->_price, 12);
    ASSERT_EQ(o1->_newPrice, 10);
    ASSERT_EQ(o1->_modCounter, 2U);
    ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrModAck);
    
    o1->_newPrice.set(11);
    
    p.onModAckPost(o1);
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kModifying);
    ASSERT_EQ(o1->_qty, 5U);
    ASSERT_EQ(o1->_price, 11);
    ASSERT_EQ(o1->_newPrice, tw::price::Ticks::INVALID_VALUE());
    ASSERT_EQ(o1->_modCounter, 1U);
    
    o1->_newPrice.set(10);
    
    p.onModAckPost(o1);
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kWorking);
    ASSERT_EQ(o1->_qty, 5U);
    ASSERT_EQ(o1->_price, 10);
    ASSERT_EQ(o1->_newPrice, tw::price::Ticks::INVALID_VALUE());
    ASSERT_EQ(o1->_modCounter, 0U);
    
    // Check SendCxl/OnCxl
    //    
    ASSERT_TRUE(p.sendCxl(o1, rej));
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kCancelling);
    ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrCxl);
    ASSERT_EQ(p.getAll().size(), 2U);
    
    ASSERT_TRUE(p.sendCxl(o1, rej));
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kCancelling);
    ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrCxl);
    ASSERT_EQ(p.getAll().size(), 2U);
    
    p.onCxlRej(o1, rej);
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kWorking);
    ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrCxlRej);
    ASSERT_EQ(o1->_modRejCounter, 0U);
    ASSERT_EQ(o1->_cxlRejCounter, 1U);
    ASSERT_EQ(p.getAll().size(), 2U);
    
    p.onCxlAck(o1);
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kCancelled);
    ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrCxlAck);
    ASSERT_EQ(o1->_modRejCounter, 0U);
    ASSERT_EQ(o1->_cxlRejCounter, 1U);
    ASSERT_EQ(p.getAll().size(), 1U);
    
    o1.reset();
    
    // Check onFill
    //
    fill._type = tw::channel_or::eFillType::kNormal;
    fill._side = tw::channel_or::eOrderSide::kBuy;
    fill._orderId = o2->_orderId;
    fill._order = o2;
    fill._qty.set(2);    
    
    ASSERT_TRUE(p.get(o2->_orderId).get() != NULL);
    ASSERT_TRUE(p.get(o2->_orderId)== o2);
    
    ASSERT_EQ(o2->_cumQty, 0U);
    p.onFill(fill);
    ASSERT_EQ(o2->_state, tw::channel_or::eOrderState::kWorking);
    ASSERT_EQ(o2->_qty, 7U);
    ASSERT_EQ(o2->_cumQty, 2U);
    ASSERT_TRUE(fill._order.get()==o2.get());
    ASSERT_EQ(fill._posAccount, 2);
    
    ASSERT_TRUE(p.get(o2->_orderId).get() != NULL);
    ASSERT_TRUE(p.get(o2->_orderId)== o2);
    
    fill._type = tw::channel_or::eFillType::kBusted;
    p.onFill(fill);
    ASSERT_EQ(o2->_state, tw::channel_or::eOrderState::kWorking);
    ASSERT_EQ(o2->_qty, 7U);
    ASSERT_EQ(o2->_cumQty, 0U);
    ASSERT_TRUE(fill._order.get()==o2.get());
    ASSERT_EQ(fill._posAccount, 0);
    
    ASSERT_TRUE(p.get(o2->_orderId).get() != NULL);
    ASSERT_TRUE(p.get(o2->_orderId)== o2);
    
    fill._type = tw::channel_or::eFillType::kNormal;
    p.onFill(fill);
    ASSERT_EQ(o2->_state, tw::channel_or::eOrderState::kWorking);
    ASSERT_EQ(o2->_qty, 7U);
    ASSERT_EQ(o2->_cumQty, 2U);
    ASSERT_TRUE(fill._order.get()==o2.get());
    ASSERT_EQ(fill._posAccount, 2);
    
    ASSERT_TRUE(p.get(o2->_orderId).get() != NULL);
    ASSERT_TRUE(p.get(o2->_orderId)== o2);
    
    fill._qty.set(3);
    p.onFill(fill);
    ASSERT_EQ(o2->_state, tw::channel_or::eOrderState::kWorking);
    ASSERT_EQ(o2->_qty, 7U);
    ASSERT_EQ(o2->_cumQty, 5U);
    ASSERT_TRUE(fill._order.get()==o2.get());
    ASSERT_EQ(fill._posAccount, 5);
    
    ASSERT_TRUE(p.get(o2->_orderId).get() != NULL);
    ASSERT_TRUE(p.get(o2->_orderId)== o2);
    
    fill._qty.set(1);
    p.onFill(fill);
    ASSERT_EQ(o2->_state, tw::channel_or::eOrderState::kWorking);
    ASSERT_EQ(o2->_qty, 7U);
    ASSERT_EQ(o2->_cumQty, 6U);    
    ASSERT_EQ(p.getAll().size(), 1U);
    ASSERT_TRUE(fill._order.get()==o2.get());
    ASSERT_EQ(fill._posAccount, 6);
    
    ASSERT_TRUE(p.get(o2->_orderId).get() != NULL);
    ASSERT_TRUE(p.get(o2->_orderId)== o2);
    
    fill._qty.set(1);
    p.onFill(fill);
    ASSERT_EQ(o2->_state, tw::channel_or::eOrderState::kFilled);
    ASSERT_EQ(o2->_qty, 7U);
    ASSERT_EQ(o2->_cumQty, 7U);
    ASSERT_EQ(p.getAll().size(), 0U);
    ASSERT_TRUE(fill._order.get()==o2.get());
    ASSERT_EQ(fill._posAccount, 7);
    
    ASSERT_TRUE(p.get(o2->_orderId).get() == NULL);
    
    p.stop();
    
}

TEST(ChannelOrLibTestSuit, processor_orders_open_orders_pos)
{
    // Structs for passing to processor
    //
    TOrderPtr o1;
    TOrderPtr o2;
    TOrderPtr o3;
    Fill fill;
    Reject rej;
    
    // Instantiation of processor
    //
    TProcessor& p = TProcessor::instance();
    ASSERT_TRUE(p.start());
    
    o1 = OrderHelper::getEmpty();
    
    o1->_side = tw::channel_or::eOrderSide::kBuy;
    o1->_qty.set(5);
    o1->_price.set(10);
    o1->_accountId = 1;
    o1->_strategyId = 2;
    o1->_orderId = tw::common::generateUuid();
    
    o2 = OrderHelper::getEmpty();
    o2->_side = tw::channel_or::eOrderSide::kSell;
    o2->_qty.set(10);
    o2->_price.set(10);
    o2->_accountId = 1;
    o2->_strategyId = 3;
    o2->_orderId = tw::common::generateUuid();
    
    o3 = OrderHelper::getEmpty(InstrHelper::getNQU2());
    o3->_side = tw::channel_or::eOrderSide::kBuy;
    o3->_qty.set(15);
    o3->_price.set(10);
    o3->_accountId = 1;
    o3->_strategyId = 3;
    o3->_orderId = tw::common::generateUuid();
    
    tw::channel_or::PosUpdate posUpdate;
    posUpdate._accountId = o1->_accountId;
    posUpdate._displayName = o1->_instrument->_displayName;
    posUpdate._exchange = o1->_instrument->_exchange;
    posUpdate._strategyId = o1->_strategyId;
    
    // Get account instrument open orders/positions
    //
    const tw::channel_or::AccountInstrOpenOrdersPos& v = p.getInstrOpenOrdersPosForAccountInstr(o1->_accountId, o1->_instrumentId);
    ASSERT_EQ(v._accountId, o1->_accountId);
    ASSERT_EQ(v._instrumentId, o1->_instrumentId);
    
    const tw::channel_or::AccountInstrOpenOrdersPos& v2 = p.getInstrOpenOrdersPosForAccountInstr(o3->_accountId, o3->_instrumentId);
    ASSERT_EQ(v2._accountId, o3->_accountId);
    ASSERT_EQ(v2._instrumentId, o3->_instrumentId);
    
    
    // Get strategy instrument open orders/positions
    //
    const tw::channel_or::StrategyInstrOpenOrdersPos& s1 = p.getInstrOpenOrdersPosForStrategyInstr(o1->_strategyId, o1->_instrumentId);
    ASSERT_EQ(s1._strategyId, o1->_strategyId);
    ASSERT_EQ(s1._instrumentId, o1->_instrumentId);
    
    const tw::channel_or::StrategyInstrOpenOrdersPos& s2 = p.getInstrOpenOrdersPosForStrategyInstr(o2->_strategyId, o2->_instrumentId);
    ASSERT_EQ(s2._strategyId, o2->_strategyId);
    ASSERT_EQ(s2._instrumentId, o2->_instrumentId);
    
    const tw::channel_or::StrategyInstrOpenOrdersPos& s3 = p.getInstrOpenOrdersPosForStrategyInstr(o3->_strategyId, o3->_instrumentId);
    ASSERT_EQ(s3._strategyId, o3->_strategyId);
    ASSERT_EQ(s3._instrumentId, o3->_instrumentId);
    
    // Test crash recovery methods
    //
    tw::instr::InstrumentConstPtr o1_instr = o1->_instrument;
    o1->_instrument.reset();
    ASSERT_TRUE(p.rebuildOrder(o1, rej));
    ASSERT_TRUE(o1->_instrument.get() != NULL);
    ASSERT_TRUE(o1->_instrument.get() == o1_instr.get());
    
    ASSERT_EQ(v._bids.get(), 5U);
    ASSERT_EQ(v._asks.get(), 0U);
    ASSERT_EQ(v._pos, 0);
    
    ASSERT_EQ(s1._bids.get(), 5U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    p.onRebuildOrderRej(o1, rej);
    ASSERT_EQ(v._bids.get(), 0U);
    ASSERT_EQ(v._asks.get(), 0U);
    ASSERT_EQ(v._pos, 0);
    
    ASSERT_EQ(s1._bids.get(), 0U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    posUpdate._pos = 5;
    p.rebuildPos(posUpdate);
    ASSERT_EQ(v._bids.get(), 0U);
    ASSERT_EQ(v._asks.get(), 0U);
    ASSERT_EQ(v._pos, 5);
    
    ASSERT_EQ(s1._bids.get(), 0U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 5);
    
    posUpdate._pos = -5;
    p.rebuildPos(posUpdate);
    ASSERT_EQ(v._bids.get(), 0U);
    ASSERT_EQ(v._asks.get(), 0U);
    ASSERT_EQ(v._pos, 0);
    
    ASSERT_EQ(s1._bids.get(), 0U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    
    // Check SendNew/OnNewAck/OnNewRej methods
    //
    ASSERT_TRUE(p.sendNew(o1, rej));
    ASSERT_EQ(v._bids.get(), 5U);
    ASSERT_EQ(v._asks.get(), 0U);
    ASSERT_EQ(v._pos, 0);
    
    ASSERT_EQ(s1._bids.get(), 5U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_TRUE(!p.sendNew(o1, rej));
    ASSERT_EQ(v._bids.get(), 5U);
    ASSERT_EQ(v._asks.get(), 0U);
    ASSERT_EQ(v._pos, 0);
    
    ASSERT_EQ(s1._bids.get(), 5U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_TRUE(p.sendNew(o2, rej));
    ASSERT_EQ(v._bids.get(), 5U);
    ASSERT_EQ(v._asks.get(), 10U);
    ASSERT_EQ(v._pos, 0);
    
    ASSERT_EQ(s1._bids.get(), 5U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_EQ(s2._bids.get(), 0U);
    ASSERT_EQ(s2._asks.get(), 10U);
    ASSERT_EQ(s2._pos, 0);
    
    p.onNewAck(o1);
    ASSERT_EQ(v._bids.get(), 5U);
    ASSERT_EQ(v._asks.get(), 10U);
    ASSERT_EQ(v._pos, 0);
    
    ASSERT_EQ(s1._bids.get(), 5U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_EQ(s2._bids.get(), 0U);
    ASSERT_EQ(s2._asks.get(), 10U);
    ASSERT_EQ(s2._pos, 0);
    
    p.onNewRej(o2, rej);
    ASSERT_EQ(v._bids.get(), 5U);
    ASSERT_EQ(v._asks.get(), 0U);
    ASSERT_EQ(v._pos, 0);
    
    ASSERT_EQ(s1._bids.get(), 5U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_EQ(s2._bids.get(), 0U);
    ASSERT_EQ(s2._asks.get(), 0U);
    ASSERT_EQ(s2._pos, 0);
    
    o2->_state = tw::channel_or::eOrderState::kUnknown;
    o2->_qty.set(7);
    ASSERT_TRUE(p.sendNew(o2, rej));
    ASSERT_EQ(v._bids.get(), 5U);
    ASSERT_EQ(v._asks.get(), 7U);
    ASSERT_EQ(v._pos, 0);
    
    ASSERT_EQ(s1._bids.get(), 5U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_EQ(s2._bids.get(), 0U);
    ASSERT_EQ(s2._asks.get(), 7U);
    ASSERT_EQ(s2._pos, 0);
    
    p.onNewAck(o2);
    ASSERT_EQ(v._bids.get(), 5U);
    ASSERT_EQ(v._asks.get(), 7U);
    ASSERT_EQ(v._pos, 0);
    
    ASSERT_EQ(s1._bids.get(), 5U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_EQ(s2._bids.get(), 0U);
    ASSERT_EQ(s2._asks.get(), 7U);
    ASSERT_EQ(s2._pos, 0);
    
    
    
    // Check SendMod/OnModAck/OnModRej methods
    //
    o1->_newPrice.set(11);
    
    ASSERT_TRUE(p.sendMod(o1, rej));
    ASSERT_EQ(v._bids.get(), 5U);
    ASSERT_EQ(v._asks.get(), 7U);
    ASSERT_EQ(v._pos, 0);
    
    ASSERT_EQ(s1._bids.get(), 5U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_EQ(s2._bids.get(), 0U);
    ASSERT_EQ(s2._asks.get(), 7U);
    ASSERT_EQ(s2._pos, 0);
    
    o1->_newPrice.set(12);
    
    ASSERT_TRUE(p.sendMod(o1, rej));    
    ASSERT_EQ(v._bids.get(), 5U);
    ASSERT_EQ(v._asks.get(), 7U);
    ASSERT_EQ(v._pos, 0);
    
    ASSERT_EQ(s1._bids.get(), 5U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_EQ(s2._bids.get(), 0U);
    ASSERT_EQ(s2._asks.get(), 7U);
    ASSERT_EQ(s2._pos, 0);
    
    p.onModRej(o1, rej);
    ASSERT_EQ(v._bids.get(), 5U);
    ASSERT_EQ(v._asks.get(), 7U);
    ASSERT_EQ(v._pos, 0);
    
    ASSERT_EQ(s1._bids.get(), 5U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_EQ(s2._bids.get(), 0U);
    ASSERT_EQ(s2._asks.get(), 7U);
    ASSERT_EQ(s2._pos, 0);
    
    p.onModRejPost(o1, rej);
    ASSERT_EQ(v._bids.get(), 5U);
    ASSERT_EQ(v._asks.get(), 7U);
    ASSERT_EQ(v._pos, 0);
    
    ASSERT_EQ(s1._bids.get(), 5U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_EQ(s2._bids.get(), 0U);
    ASSERT_EQ(s2._asks.get(), 7U);
    ASSERT_EQ(s2._pos, 0);
    
    o1->_newPrice.set(12);
    p.onModAck(o1);
    ASSERT_EQ(v._bids.get(), 5U);
    ASSERT_EQ(v._asks.get(), 7U);
    ASSERT_EQ(v._pos, 0);
    
    
    ASSERT_EQ(s1._bids.get(), 5U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_EQ(s2._bids.get(), 0U);
    ASSERT_EQ(s2._asks.get(), 7U);
    ASSERT_EQ(s2._pos, 0);
    
    p.onModAckPost(o1);
    ASSERT_EQ(v._bids.get(), 5U);
    ASSERT_EQ(v._asks.get(), 7U);
    ASSERT_EQ(v._pos, 0);
    
    ASSERT_EQ(s1._bids.get(), 5U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_EQ(s2._bids.get(), 0U);
    ASSERT_EQ(s2._asks.get(), 7U);
    ASSERT_EQ(s2._pos, 0);
    
    // Check SendCxl/OnCxl
    //    
    ASSERT_TRUE(p.sendCxl(o1, rej));
    ASSERT_EQ(v._bids.get(), 5U);
    ASSERT_EQ(v._asks.get(), 7U);
    ASSERT_EQ(v._pos, 0);
    
    ASSERT_EQ(s1._bids.get(), 5U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_EQ(s2._bids.get(), 0U);
    ASSERT_EQ(s2._asks.get(), 7U);
    ASSERT_EQ(s2._pos, 0);
    
    ASSERT_TRUE(p.sendCxl(o1, rej));
    ASSERT_EQ(v._bids.get(), 5U);
    ASSERT_EQ(v._asks.get(), 7U);
    ASSERT_EQ(v._pos, 0);
    
    ASSERT_EQ(s1._bids.get(), 5U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_EQ(s2._bids.get(), 0U);
    ASSERT_EQ(s2._asks.get(), 7U);
    ASSERT_EQ(s2._pos, 0);
    
    p.onCxlRej(o1, rej);
    ASSERT_EQ(v._bids.get(), 5U);
    ASSERT_EQ(v._asks.get(), 7U);
    ASSERT_EQ(v._pos, 0);
    
    ASSERT_EQ(s1._bids.get(), 5U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_EQ(s2._bids.get(), 0U);
    ASSERT_EQ(s2._asks.get(), 7U);
    ASSERT_EQ(s2._pos, 0);
    
    p.onCxlAck(o1);
    ASSERT_EQ(v._bids.get(), 0U);
    ASSERT_EQ(v._asks.get(), 7U);
    ASSERT_EQ(v._pos, 0);
    
    ASSERT_EQ(s1._bids.get(), 0U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_EQ(s2._bids.get(), 0U);
    ASSERT_EQ(s2._asks.get(), 7U);
    ASSERT_EQ(s2._pos, 0);
        
    // Check onFill
    //
    fill._type = tw::channel_or::eFillType::kNormal;
    fill._orderId = o2->_orderId;
    fill._order = o2;
    fill._qty.set(2);
    fill._side = o2->_side;
    fill._accountId = o2->_accountId;
    fill._strategyId = o2->_strategyId;
    fill._instrumentId = o2->_instrumentId;
    
    p.onFill(fill);
    
    ASSERT_EQ(v._bids.get(), 0U);
    ASSERT_EQ(v._asks.get(), 5U);
    ASSERT_EQ(v._pos, -2);
    
    ASSERT_EQ(s1._bids.get(), 0U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_EQ(s2._bids.get(), 0U);
    ASSERT_EQ(s2._asks.get(), 5U);
    ASSERT_EQ(s2._pos, -2);
    
    fill._type = tw::channel_or::eFillType::kBusted;    
    p.onFill(fill);
    
    ASSERT_EQ(v._bids.get(), 0U);
    ASSERT_EQ(v._asks.get(), 7U);
    ASSERT_EQ(v._pos, 0);
    
    ASSERT_EQ(s1._bids.get(), 0U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_EQ(s2._bids.get(), 0U);
    ASSERT_EQ(s2._asks.get(), 7U);
    ASSERT_EQ(s2._pos, 0);
    
    
    fill._type = tw::channel_or::eFillType::kNormal;
    p.onFill(fill);
    
    ASSERT_EQ(v._bids.get(), 0U);
    ASSERT_EQ(v._asks.get(), 5U);
    ASSERT_EQ(v._pos, -2);
    
    ASSERT_EQ(s1._bids.get(), 0U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_EQ(s2._bids.get(), 0U);
    ASSERT_EQ(s2._asks.get(), 5U);
    ASSERT_EQ(s2._pos, -2);
    
    
    fill._qty.set(3);
    p.onFill(fill);
    
    ASSERT_EQ(v._bids.get(), 0U);
    ASSERT_EQ(v._asks.get(), 2U);
    ASSERT_EQ(v._pos, -5);
    
    ASSERT_EQ(s1._bids.get(), 0U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_EQ(s2._bids.get(), 0U);
    ASSERT_EQ(s2._asks.get(), 2U);
    ASSERT_EQ(s2._pos, -5);
    
    fill._qty.set(1);
    p.onFill(fill);
    
    ASSERT_EQ(v._bids.get(), 0U);
    ASSERT_EQ(v._asks.get(), 1U);
    ASSERT_EQ(v._pos, -6);
    
    ASSERT_EQ(s1._bids.get(), 0U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_EQ(s2._bids.get(), 0U);
    ASSERT_EQ(s2._asks.get(), 1U);
    ASSERT_EQ(s2._pos, -6);
    
    fill._qty.set(1);
    p.onFill(fill);
    
    ASSERT_EQ(v._bids.get(), 0U);
    ASSERT_EQ(v._asks.get(), 0U);
    ASSERT_EQ(v._pos, -7);
    
    ASSERT_EQ(s1._bids.get(), 0U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_EQ(s2._bids.get(), 0U);
    ASSERT_EQ(s2._asks.get(), 0U);
    ASSERT_EQ(s2._pos, -7);
    
    
    o2->_state = tw::channel_or::eOrderState::kUnknown;
    o2->_qty.set(7);
    o2->_cumQty.set(0);
    ASSERT_TRUE(p.sendNew(o2, rej));
    ASSERT_EQ(v._bids.get(), 0U);
    ASSERT_EQ(v._asks.get(), 7U);
    ASSERT_EQ(v._pos, -7);
    
    ASSERT_EQ(s1._bids.get(), 0U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_EQ(s2._bids.get(), 0U);
    ASSERT_EQ(s2._asks.get(), 7U);
    ASSERT_EQ(s2._pos, -7);
    
    p.onNewAck(o2);
    ASSERT_EQ(v._bids.get(), 0U);
    ASSERT_EQ(v._asks.get(), 7U);
    ASSERT_EQ(v._pos, -7);
    
    ASSERT_EQ(s1._bids.get(), 0U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_EQ(s2._bids.get(), 0U);
    ASSERT_EQ(s2._asks.get(), 7U);
    ASSERT_EQ(s2._pos, -7);
    
    fill._qty.set(5);
    p.onFill(fill);
    
    ASSERT_EQ(v._bids.get(), 0U);
    ASSERT_EQ(v._asks.get(), 2U);
    ASSERT_EQ(v._pos, -12);
    
    ASSERT_EQ(s1._bids.get(), 0U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_EQ(s2._bids.get(), 0U);
    ASSERT_EQ(s2._asks.get(), 2U);
    ASSERT_EQ(s2._pos, -12);
    
    
    ASSERT_TRUE(p.sendCxl(o2, rej));
    ASSERT_EQ(v._bids.get(), 0U);
    ASSERT_EQ(v._asks.get(), 2U);
    ASSERT_EQ(v._pos, -12);
    
    
    ASSERT_EQ(s1._bids.get(), 0U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_EQ(s2._bids.get(), 0U);
    ASSERT_EQ(s2._asks.get(), 2U);
    ASSERT_EQ(s2._pos, -12);
    
    p.onCxlAck(o2);
    ASSERT_EQ(v._bids.get(), 0U);
    ASSERT_EQ(v._asks.get(), 0U);
    ASSERT_EQ(v._pos, -12);
    
    ASSERT_EQ(s1._bids.get(), 0U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_EQ(s2._bids.get(), 0U);
    ASSERT_EQ(s2._asks.get(), 0U);
    ASSERT_EQ(s2._pos, -12);
    
    // Test external fill
    //
    fill._type = tw::channel_or::eFillType::kExternal;
    fill._qty.set(5);
    p.onFill(fill);
    
    ASSERT_EQ(v._bids.get(), 0U);
    ASSERT_EQ(v._asks.get(), 0U);
    ASSERT_EQ(v._pos, -17);
    
    
    ASSERT_EQ(s1._bids.get(), 0U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_EQ(s2._bids.get(), 0U);
    ASSERT_EQ(s2._asks.get(), 0U);
    ASSERT_EQ(s2._pos, -17);
    
    // Test another instrument
    //
    ASSERT_TRUE(p.sendNew(o3, rej));
    
    ASSERT_EQ(v._bids.get(), 0U);
    ASSERT_EQ(v._asks.get(), 0U);
    ASSERT_EQ(v._pos, -17);
    
    ASSERT_EQ(v2._bids.get(), 15U);
    ASSERT_EQ(v2._asks.get(), 0U);
    ASSERT_EQ(v2._pos, 0);
    
    
    ASSERT_EQ(s1._bids.get(), 0U);
    ASSERT_EQ(s1._asks.get(), 0U);
    ASSERT_EQ(s1._pos, 0);
    
    ASSERT_EQ(s2._bids.get(), 0U);
    ASSERT_EQ(s2._asks.get(), 0U);
    ASSERT_EQ(s2._pos, -17);
    
    ASSERT_EQ(s3._bids.get(), 15U);
    ASSERT_EQ(s3._asks.get(), 0U);
    ASSERT_EQ(s3._pos, 0);
    
    p.stop();
    
}



TEST(ChannelOrLibTestSuit, processor_orders_stuck_orders)
{
    // Callback client to track alerts
    //
    CallbackClient c;
    tw::common_strat::ConsumerProxy::instance().registerCallbackAlert(&c);
    
    // Structs for passing to processor
    //
    TOrderPtr o1;
    TOrderPtr o2;
    Reject rej;
    
    // Instantiation of processor
    //
    TProcessor& p = TProcessor::instance();
    p.clear();
    ASSERT_TRUE(p.start());
    
    // Put 2 new orders
    //
    o1 = p.createOrder();
    ASSERT_TRUE(o1.get() != NULL);
    ASSERT_TRUE(o1->_orderId != tw::common::TUuidBuffer());
    
    o1->_qty.set(5);
    o1->_price.set(10);
    o1->_accountId = 1;
    o1->_strategyId = 2;
    o1->_instrumentId = 5;
    o1->_timestamp1.setToNow();
    
    ASSERT_TRUE(p.sendNew(o1, rej));
    ASSERT_EQ(p.getAll().size(), 1U);
    
    o2 = p.createOrder();
    ASSERT_TRUE(o2.get() != NULL);
    ASSERT_TRUE(o2->_orderId != tw::common::TUuidBuffer());    
    
    o2->_accountId = 2;
    o2->_strategyId = 3;
    o2->_instrumentId = 5;
    o2->_qty.set(7);
    o2->_action = tw::common::eCommandSubType::kUnknown;
    o2->_state = tw::channel_or::eOrderState::kUnknown;
    o2->_timestamp1.setToNow();
    
    ASSERT_TRUE(p.sendNew(o2, rej));       
    ASSERT_EQ(p.getAll().size(), 2U);
    
    ASSERT_TRUE(c._alerts.empty());
    
    p.onTimeout(1);
    ASSERT_EQ(c._alerts.size(), 2U);
    ASSERT_EQ(c._alerts.front()._type, tw::channel_or::eAlertType::kStuckOrders);
    ASSERT_EQ(c._alerts.front()._strategyId, o1->_strategyId);
    ASSERT_EQ(c._alerts.back()._type, tw::channel_or::eAlertType::kStuckOrders);
    ASSERT_EQ(c._alerts.back()._strategyId, o2->_strategyId);
    
    c._alerts.clear();
    p.onNewAck(o1);
    
    p.onTimeout(1);    
    ASSERT_EQ(c._alerts.size(), 1U);    
    ASSERT_EQ(c._alerts.back()._type, tw::channel_or::eAlertType::kStuckOrders);
    ASSERT_EQ(c._alerts.back()._strategyId, o2->_strategyId);
    
    c._alerts.clear();
    p.onNewAck(o2);
    
    p.onTimeout(1);    
    ASSERT_EQ(c._alerts.size(), 0U);
    
    o1->_newPrice.set(11);
    ASSERT_TRUE(p.sendMod(o1, rej));
    
    p.onTimeout(1);    
    ASSERT_EQ(c._alerts.size(), 1U);
    ASSERT_EQ(c._alerts.front()._type, tw::channel_or::eAlertType::kStuckOrders);
    ASSERT_EQ(c._alerts.front()._strategyId, o1->_strategyId);
    
    c._alerts.clear();
    ASSERT_TRUE(p.sendCxl(o2, rej));
    
    p.onTimeout(1);
    ASSERT_EQ(c._alerts.size(), 2U);
    ASSERT_EQ(c._alerts.front()._type, tw::channel_or::eAlertType::kStuckOrders);
    ASSERT_EQ(c._alerts.front()._strategyId, o1->_strategyId);
    ASSERT_EQ(c._alerts.back()._type, tw::channel_or::eAlertType::kStuckOrders);
    ASSERT_EQ(c._alerts.back()._strategyId, o2->_strategyId);
    
    p.stop();
    
}


TEST(ChannelOrLibTestSuit, processor_orders_stuck_orders_w_timeouts)
{
    // Callback client to track alerts
    //
    CallbackClient c;
    tw::common_strat::ConsumerProxy::instance().registerCallbackAlert(&c);
    
    // Structs for passing to processor
    //
    TOrderPtr o1;
    TOrderPtr o2;
    Reject rej;
    
    // Instantiation of processor
    //
    TProcessor& p = TProcessor::instance();
    p.clear();
    
    tw::common::Settings settings;
    settings._strategy_container_stuck_orders_timeout = 1500; // 1500 ms - 1.5 secs
    
    ASSERT_TRUE(p.init(settings));
    ASSERT_TRUE(p.start());
    
    // Put 2 new orders
    //
    o1 = p.createOrder();
    ASSERT_TRUE(o1.get() != NULL);
    ASSERT_TRUE(o1->_orderId != tw::common::TUuidBuffer());
    
    o1->_qty.set(5);
    o1->_price.set(10);
    o1->_accountId = 1;
    o1->_strategyId = 2;
    o1->_instrumentId = 5;
    o1->_timestamp1.setToNow();
    
    ASSERT_TRUE(p.sendNew(o1, rej));
    ASSERT_EQ(p.getAll().size(), 1U);
    
    o2 = p.createOrder();
    ASSERT_TRUE(o2.get() != NULL);
    ASSERT_TRUE(o2->_orderId != tw::common::TUuidBuffer());    
    
    o2->_accountId = 2;
    o2->_strategyId = 3;
    o2->_instrumentId = 5;
    o2->_qty.set(7);
    o2->_action = tw::common::eCommandSubType::kUnknown;
    o2->_state = tw::channel_or::eOrderState::kUnknown;
    o2->_timestamp1.setToNow();
    
    ASSERT_TRUE(p.sendNew(o2, rej));       
    ASSERT_EQ(p.getAll().size(), 2U);
    
    ASSERT_TRUE(c._alerts.empty());
    
    p.onTimeout(1);
    ASSERT_TRUE(c._alerts.empty());    
    
    // Sleep for 2 seconds
    //
    tw::common_thread::sleep(2000);
    
    p.onTimeout(1);
    ASSERT_EQ(c._alerts.size(), 2U);
    ASSERT_EQ(c._alerts.front()._type, tw::channel_or::eAlertType::kStuckOrders);
    ASSERT_EQ(c._alerts.front()._strategyId, o1->_strategyId);
    ASSERT_EQ(c._alerts.front()._text, o1->_orderId.toString());
    ASSERT_EQ(c._alerts.back()._type, tw::channel_or::eAlertType::kStuckOrders);
    ASSERT_EQ(c._alerts.back()._strategyId, o2->_strategyId);
    ASSERT_EQ(c._alerts.back()._text, o2->_orderId.toString());
    
    c._alerts.clear();
    p.onNewAck(o1);
    
    p.onTimeout(1);    
    ASSERT_EQ(c._alerts.size(), 1U);    
    ASSERT_EQ(c._alerts.back()._type, tw::channel_or::eAlertType::kStuckOrders);
    ASSERT_EQ(c._alerts.back()._strategyId, o2->_strategyId);
    ASSERT_EQ(c._alerts.back()._text, o2->_orderId.toString());
    
    c._alerts.clear();
    p.onNewAck(o2);
    
    p.onTimeout(1);    
    ASSERT_EQ(c._alerts.size(), 0U);    
    
    
    p.stop();
    
}


TEST(ChannelOrLibTestSuit, processor_orders_mod_rej_and_mod_rej_counters)
{
    // Structs for passing to processor
    //
    TOrderPtr o1;
    Reject rej;
    
    // Instantiation of processor
    //
    TProcessor& p = TProcessor::instance();
    p.clear();
    ASSERT_TRUE(p.start());
    
    // SendNew <--> OnNewAck prep of order
    //
    o1 = p.createOrder();        
    o1->_qty.set(5);
    o1->_price.set(10);
    o1->_accountId = 1;
    o1->_strategyId = 2;
    o1->_instrumentId = 5;
    
    ASSERT_TRUE(p.sendNew(o1, rej));
    ASSERT_EQ(p.getAll().size(), 1U);
    p.onNewAck(o1);    
    
    // SendMod <--> OnModRej test
    //
   
    // Test mod rej when new price is invalid
    //
    rej.clear();
    ASSERT_TRUE(!p.sendMod(o1, rej));
    ASSERT_EQ(p.getAll().size(), 1U);
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kWorking);
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorOrders);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kInvalidOrderPrice);
    ASSERT_EQ(o1->_modCounter, 0U);
    ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrMod);
    
    p.onModRej(o1, rej);
    p.onModRejPost(o1, rej);
    ASSERT_EQ(p.getAll().size(), 1U);
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kWorking);
    ASSERT_EQ(o1->_modCounter, 0U);
    ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrModRej);
    ASSERT_EQ(o1->_modRejCounter, 1U);
    
    // Test mod rej when new price is the same as price
    //
    rej.clear();
    o1->_newPrice = o1->_price;
    ASSERT_TRUE(!p.sendMod(o1, rej));
    ASSERT_EQ(p.getAll().size(), 1U);
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kWorking);
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorOrders);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kSameOrderPrice);
    ASSERT_EQ(o1->_modCounter, 0U);
    ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrMod);
    ASSERT_EQ(o1->_price, o1->_newPrice);
    
    p.onModRej(o1, rej);
    p.onModRejPost(o1, rej);
    ASSERT_EQ(p.getAll().size(), 1U);
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kWorking);
    ASSERT_EQ(o1->_modCounter, 0U);
    ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrModRej);
    ASSERT_TRUE(!o1->_newPrice.isValid());
    ASSERT_EQ(o1->_modRejCounter, 2U);
    
    // Test mod requests > 5 
    //
    for ( size_t i = 1; i < 6; ++i ) {
        rej.clear();
        o1->_newPrice = o1->_price + tw::price::Ticks(i);
        ASSERT_TRUE(p.sendMod(o1, rej));
        ASSERT_EQ(p.getAll().size(), 1U);
        ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kModifying);
        ASSERT_EQ(o1->_modCounter, i);
        ASSERT_EQ(o1->_modRejCounter, 2U);
        ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrMod);
    }
    
    rej.clear();
    o1->_newPrice = o1->_price + tw::price::Ticks(6);
    ASSERT_TRUE(!p.sendMod(o1, rej));
    ASSERT_EQ(p.getAll().size(), 1U);
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kModifying);
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorOrders);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kMaxModCounter);    
    ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrMod);
    ASSERT_EQ(o1->_modCounter, 5U);
    ASSERT_EQ(o1->_modRejCounter, 2U);
    
    p.onModRej(o1, rej);
    p.onModRejPost(o1, rej);
    ASSERT_EQ(p.getAll().size(), 1U);
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kModifying);
    ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrModRej);
    ASSERT_EQ(o1->_modCounter, 5U);    
    ASSERT_EQ(o1->_modRejCounter, 3U);

    // Test mod rejects > 5 
    //
    for ( size_t i = 3; i < 6; ++i ) {
        rej.clear();
        o1->_newPrice = o1->_price + tw::price::Ticks(6);
        ASSERT_TRUE(!p.sendMod(o1, rej));
        ASSERT_EQ(p.getAll().size(), 1U);
        ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kModifying);
        ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
        ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorOrders);
        ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kMaxModCounter);    
        ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrMod);
        ASSERT_EQ(o1->_modCounter, 5U);
        ASSERT_EQ(o1->_modRejCounter, i);

        p.onModRej(o1, rej);
        p.onModRejPost(o1, rej);
        ASSERT_EQ(p.getAll().size(), 1U);
        ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kModifying);
        ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrModRej);
        ASSERT_EQ(o1->_modCounter, 5U);    
        ASSERT_EQ(o1->_modRejCounter, i+1);
    }
    
    
    rej.clear();
    o1->_newPrice = o1->_price + tw::price::Ticks(6);
    ASSERT_TRUE(!p.sendMod(o1, rej));
    ASSERT_EQ(p.getAll().size(), 1U);
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kModifying);
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorOrders);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kMaxModRejCounter);    
    ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrMod);
    ASSERT_EQ(o1->_modCounter, 5U);
    ASSERT_EQ(o1->_modRejCounter, 6U);
    
    p.onModRej(o1, rej);
    p.onModRejPost(o1, rej);
    ASSERT_EQ(p.getAll().size(), 1U);
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kModifying);
    ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrModRej);
    ASSERT_EQ(o1->_modCounter, 5U);    
    ASSERT_EQ(o1->_modRejCounter, 7U);
    
    // Test can do normal modifies after mod ack
    //
    o1->_newPrice = o1->_price + tw::price::Ticks(1);
    p.onModAck(o1);
    p.onModAckPost(o1);
    ASSERT_EQ(p.getAll().size(), 1U);
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kModifying);
    ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrModAck);
    ASSERT_EQ(o1->_modCounter, 4U);    
    ASSERT_EQ(o1->_modRejCounter, 0U);
    
    rej.clear();
    o1->_newPrice = o1->_price + tw::price::Ticks(2);
    ASSERT_TRUE(p.sendMod(o1, rej));
    ASSERT_EQ(p.getAll().size(), 1U);
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kModifying);
    ASSERT_EQ(o1->_modCounter, 5U);
    ASSERT_EQ(o1->_modRejCounter, 0U);
    ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrMod);
    
}


TEST(ChannelOrLibTestSuit, processor_orders_cxl_rej_and_cxl_rej_counters)
{
    // Structs for passing to processor
    //
    TOrderPtr o1;
    Reject rej;
    
    // Instantiation of processor
    //
    TProcessor& p = TProcessor::instance();
    p.clear();
    ASSERT_TRUE(p.start());
    
    // SendNew <--> OnNewAck prep of order
    //
    o1 = p.createOrder();        
    o1->_qty.set(5);
    o1->_price.set(10);
    o1->_accountId = 1;
    o1->_strategyId = 2;
    o1->_instrumentId = 5;
    
    ASSERT_TRUE(p.sendNew(o1, rej));
    ASSERT_EQ(p.getAll().size(), 1U);
    p.onNewAck(o1);    
    
    // Test cxl rejects > 5 
    //
    for ( size_t i = 0; i < 6; ++i ) {
        rej.clear();
        ASSERT_TRUE(p.sendCxl(o1, rej));
        ASSERT_EQ(p.getAll().size(), 1U);
        ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kCancelling);        
        ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrCxl);
        ASSERT_EQ(o1->_cxlRejCounter, i);

        p.onCxlRej(o1, rej);
        p.onModRejPost(o1, rej);
        ASSERT_EQ(p.getAll().size(), 1U);
        ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kWorking);
        ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrCxlRej);
        ASSERT_EQ(o1->_cxlRejCounter, i+1);
    }
        
    rej.clear();
    ASSERT_TRUE(!p.sendCxl(o1, rej));
    ASSERT_EQ(p.getAll().size(), 1U);
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorOrders);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kMaxCxlRejCounter); 
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kWorking);
    ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrCxl);
    ASSERT_EQ(o1->_cxlRejCounter, 6U);
    
}


TEST(ChannelOrLibTestSuit, processor_orders_new_rej_on_invalid_price)
{
    // Structs for passing to processor
    //
    TOrderPtr o1;
    Reject rej;
    
    // Instantiation of processor
    //
    TProcessor& p = TProcessor::instance();
    p.clear();
    ASSERT_TRUE(p.start());
    
    // SendNew <--> OnNewAck prep of order
    //
    o1 = p.createOrder();
    o1->_type = tw::channel_or::eOrderType::kLimit;
    o1->_qty.set(5);    
    o1->_accountId = 1;
    o1->_strategyId = 2;
    o1->_instrumentId = 5;
    
    ASSERT_TRUE(!p.sendNew(o1, rej));
    ASSERT_EQ(p.getAll().size(), 0U);
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorOrders);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kInvalidOrderPrice); 
    ASSERT_EQ(o1->_state, tw::channel_or::eOrderState::kRejected);
    ASSERT_EQ(o1->_action, tw::common::eCommandSubType::kOrNewRej);   
    
}

TEST(ChannelOrLibTestSuit, processor_orders_isOrderLive)
{
    // Structs for passing to processor
    //
    TOrderPtr o1;
    
    // Instantiation of processor
    //
    TProcessor& p = TProcessor::instance();
    p.clear();
    ASSERT_TRUE(p.start());
    
    ASSERT_TRUE(!p.isOrderLive(o1));    
    
    o1 = p.createOrder();
    ASSERT_TRUE(!p.isOrderLive(o1));
    
    o1->_state = tw::channel_or::eOrderState::kPending;
    ASSERT_TRUE(p.isOrderLive(o1));
    
    o1->_state = tw::channel_or::eOrderState::kWorking;
    ASSERT_TRUE(p.isOrderLive(o1));
    
    o1->_state = tw::channel_or::eOrderState::kModifying;
    ASSERT_TRUE(p.isOrderLive(o1));
    
    o1->_state = tw::channel_or::eOrderState::kCancelling;
    ASSERT_TRUE(p.isOrderLive(o1));
    
    o1->_state = tw::channel_or::eOrderState::kCancelled;
    ASSERT_TRUE(!p.isOrderLive(o1));
    
    o1->_state = tw::channel_or::eOrderState::kRejected;
    ASSERT_TRUE(!p.isOrderLive(o1));
    
    o1->_state = tw::channel_or::eOrderState::kFilled;
    ASSERT_TRUE(!p.isOrderLive(o1));
    
}
