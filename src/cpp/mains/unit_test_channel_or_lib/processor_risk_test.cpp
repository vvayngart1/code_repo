#include <tw/channel_or/processor_orders.h>
#include <tw/channel_or/processor_risk.h>
#include <tw/channel_or/processor.h>

#include "order_helper.h"

#include <gtest/gtest.h>

typedef tw::channel_or::TOrderPtr TOrderPtr;
typedef tw::channel_or::Reject Reject;
typedef tw::channel_or::Fill Fill;
typedef tw::channel_or::PosUpdate PosUpdate;
typedef tw::channel_or::Alert Alert;
typedef tw::channel_or::TOrders TOrders;

typedef tw::channel_or::ProcessorOrders TProcessorOrders;
typedef tw::channel_or::ProcessorRisk TProcessorRisk;

// NOTE: the order of template initialization is from last TProcessorRisk
// to first one
//
typedef tw::channel_or::ProcessorOut<TProcessorRisk> TProcessorOutRisk;
typedef tw::channel_or::ProcessorOut<TProcessorOrders, TProcessorOutRisk> TProcessorOut;

TEST(ChannelOrLibTestSuit, processor_risk)
{
    // Structs for passing to processor
    //
    TOrderPtr s1_b1;
    TOrderPtr s1_b2;
    TOrderPtr s1_b3;
    TOrderPtr s1_b4;
    TOrderPtr s1_a1;
    TOrderPtr s1_a2;    
    
    TOrderPtr s2_b1;
    TOrderPtr s2_a1;
    
    Fill fill;
    Reject rej;    
    
    tw::instr::InstrumentPtr instrNQH2;
    tw::instr::InstrumentPtr instrNQU2;
    
    tw::risk::Account account;
    std::vector<tw::risk::AccountRiskParams> symbolsParams;
    
    // Init processors
    //
    TProcessorOrders& p_orders = TProcessorOrders::instance();
    TProcessorRisk& p_risk = TProcessorRisk::instance();
    
    TProcessorOutRisk p_out_risk(p_risk);
    TProcessorOut p_out(p_orders, p_out_risk);
    
    // Init instruments
    //
    instrNQH2 = InstrHelper::getNQH2();
    instrNQU2 = InstrHelper::getNQU2();
    
    // Init risk parameters
    //
    account._id = 1;
    account._name = "test";
    account._tradeEnabled = false;
    
    symbolsParams.resize(2);
    
    symbolsParams[0]._accountId = account._id;
    symbolsParams[0]._displayName = instrNQH2->_displayName;
    symbolsParams[0]._exchange = instrNQH2->_exchange;
    symbolsParams[0]._maxPos = 0;
    symbolsParams[0]._clipSize = 1;
    symbolsParams[0]._tradeEnabled = false;
    
    symbolsParams[1]._accountId = account._id;
    symbolsParams[1]._displayName = instrNQU2->_displayName;
    symbolsParams[1]._exchange = instrNQU2->_exchange;
    symbolsParams[1]._maxPos = 0;
    symbolsParams[1]._clipSize = 1;
    symbolsParams[1]._tradeEnabled = false;
    
    // Init orders
    //
    
    // NQH2 orders
    //
    s1_b1 = OrderHelper::getEmptyWithOrderId(instrNQH2);
    s1_b2 = OrderHelper::getEmptyWithOrderId(instrNQH2);
    s1_b3 = OrderHelper::getEmptyWithOrderId(instrNQH2);
    s1_b4 = OrderHelper::getEmptyWithOrderId(instrNQH2);
    s1_a1 = OrderHelper::getEmptyWithOrderId(instrNQH2);
    s1_a2 = OrderHelper::getEmptyWithOrderId(instrNQH2);
    
    s1_b1->_side = s1_b2->_side = s1_b3->_side = s1_b4->_side = tw::channel_or::eOrderSide::kBuy;
    s1_a1->_side = s1_a2->_side = tw::channel_or::eOrderSide::kSell;
    
    s1_b1->_accountId = s1_b2->_accountId = s1_b3->_accountId = s1_b4->_accountId = s1_a1->_accountId = s1_a2->_accountId = account._id;
    
    // Total bids = 11
    //
    s1_b1->_qty.set(6);
    s1_b2->_qty.set(2);
    s1_b3->_qty.set(3);
    
    // Total asks = 12
    //
    s1_a1->_qty.set(7);
    s1_a2->_qty.set(5);    
    
    // NQU2 orders
    //
    s2_b1 = OrderHelper::getEmptyWithOrderId(instrNQU2);
    s2_a1 = OrderHelper::getEmptyWithOrderId(instrNQU2);
    
    s2_b1->_side = tw::channel_or::eOrderSide::kBuy;
    s2_a1->_side = tw::channel_or::eOrderSide::kSell;
    
    s2_b1->_accountId = s2_a1->_accountId = account._id;
    
    s2_b1->_qty.set(12);    
    s2_a1->_qty.set(12);
    
    
    // Test disabled account and not configured/disabled symbols rejects
    //
    ASSERT_TRUE(p_risk.init(account, symbolsParams));
    
    s1_b1->_accountId = 0;
    
    ASSERT_TRUE(!p_out.sendNew(s1_b1, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorRisk);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kAccountNotConfigured);
    
    s1_b1->_state = tw::channel_or::eOrderState::kUnknown;
    s1_b1->_accountId = account._id;
    
    ASSERT_TRUE(!p_out.sendNew(s1_b1, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorRisk);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kAccountDisabled);
    
    
    account._tradeEnabled = true;
    ASSERT_TRUE(p_risk.init(account, symbolsParams));
    
    s1_b1->_state = tw::channel_or::eOrderState::kUnknown;
    s1_b1->_instrumentId = 0;
    
    ASSERT_TRUE(!p_out.sendNew(s1_b1, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorRisk);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kSymbolNotConfigured);
    
    s1_b1->_state = tw::channel_or::eOrderState::kUnknown;
    s1_b1->_instrumentId = s1_b1->_instrument->_keyId;
    
    ASSERT_TRUE(!p_out.sendNew(s1_b1, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorRisk);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kSymbolDisabled);
    
    symbolsParams[0]._tradeEnabled = true;
    symbolsParams[1]._tradeEnabled = true;
    
    ASSERT_TRUE(p_risk.init(account, symbolsParams));
    
    // Clip size 6 > 1 (max clip size)
    //
    s1_b1->_state = tw::channel_or::eOrderState::kUnknown;
    ASSERT_TRUE(!p_out.sendNew(s1_b1, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorRisk);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kSymbolClipSize);
    
    symbolsParams[0]._clipSize = 2*10;
    symbolsParams[1]._clipSize = 2*11;
    ASSERT_TRUE(p_risk.init(account, symbolsParams));
    
    // open_bids = 6 > 0 (maxpos) - reject
    //
    s1_b1->_state = tw::channel_or::eOrderState::kUnknown;
    ASSERT_TRUE(!p_out.sendNew(s1_b1, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorRisk);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kSymbolMaxPos);
    
    
    // Test normal order flow
    //
    symbolsParams[0]._maxPos = 10;
    symbolsParams[1]._maxPos = 11;
    ASSERT_TRUE(p_risk.init(account, symbolsParams));
    
    // Bids for NQH2
    //
    
    // open_bids = 6 < 10 (maxpos)
    //
    s1_b1->_state = tw::channel_or::eOrderState::kUnknown;
    ASSERT_TRUE(p_out.sendNew(s1_b1, rej));
    
    // open_bids = 6+2=8 < 10 (maxpos)
    //
    ASSERT_TRUE(p_out.sendNew(s1_b2, rej));
    
    // open_bids = 6+2+3=11 > 10 (maxpos) - reject
    //
    ASSERT_TRUE(!p_out.sendNew(s1_b3, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorRisk);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kSymbolMaxPos);
    
    s1_b3->_state = tw::channel_or::eOrderState::kUnknown;
    s1_b3->_qty.set(2);
    
    // open_bids = 6+2+2=10 = 10 (maxpos)
    //
    ASSERT_TRUE(p_out.sendNew(s1_b3, rej));
    
    
    // Check fills for bids for NQH2
    //
    p_orders.onNewAck(s1_b1);
    p_orders.onNewAck(s1_b2);
    p_orders.onNewAck(s1_b3);
    
    // Fill for 6 longs => pos = 0+6=6
    //
    fill._type = tw::channel_or::eFillType::kNormal;
    fill._orderId = s1_b1->_orderId;
    fill._order = s1_b1;
    fill._qty = s1_b1->_qty;
    fill._side = s1_b1->_side;
    fill._accountId = s1_b1->_accountId;
    fill._instrumentId = s1_b1->_instrumentId;
    fill._order = s1_b1;
    
    p_orders.onFill(fill);
    
    // pos + open_bids = 6(pos)+2+2+6=16 > 10 (maxpos) - reject
    //
    s1_b1->_state = tw::channel_or::eOrderState::kUnknown;
    s1_b1->_cumQty.set(0);
    
    ASSERT_TRUE(!p_out.sendNew(s1_b1, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorRisk);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kSymbolMaxPos);
    
    
    // pos + open_bids = 6(pos)+2+3=11 > 10 (maxpos) - reject
    //
    p_orders.onCxlAck(s1_b2);
    
    s1_b2->_state = tw::channel_or::eOrderState::kUnknown;
    s1_b2->_qty.set(3);
    
    ASSERT_TRUE(!p_out.sendNew(s1_b2, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorRisk);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kSymbolMaxPos);
    
    // pos + open_bids = 6(pos)+2+2=10 = 10 (maxpos)
    //
    s1_b2->_state = tw::channel_or::eOrderState::kUnknown;
    s1_b2->_qty.set(2);
    
    ASSERT_TRUE(p_out.sendNew(s1_b2, rej));
    
    
    // Asks for NQH2
    //
    
    // open_asks = 7 < 10 (maxpos)
    //
    ASSERT_TRUE(p_out.sendNew(s1_a1, rej));
    
    // open_asks = 7+10=17 > 10(maxpos)+6(long pos)=16 - reject
    //
    s1_a2->_state = tw::channel_or::eOrderState::kUnknown;
    s1_a2->_qty.set(10);
    
    ASSERT_TRUE(!p_out.sendNew(s1_a2, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorRisk);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kSymbolMaxPos);
    
    s1_a2->_state = tw::channel_or::eOrderState::kUnknown;
    s1_a2->_qty.set(8);
    
    // open_asks = 7+8=15 < 10(maxpos)+6(long pos)=16
    //
    ASSERT_TRUE(p_out.sendNew(s1_a2, rej));
    
    
    // Check fills for asks for NQH2
    //
    p_orders.onNewAck(s1_a1);
    p_orders.onNewAck(s1_a2);
    
    // Fill for 7 shorts => pos = 6-7=-1
    //
    fill._type = tw::channel_or::eFillType::kNormal;
    fill._orderId = s1_a1->_orderId;
    fill._order = s1_a1;
    fill._qty = s1_a1->_qty;
    fill._side = s1_a1->_side;
    fill._accountId = s1_a1->_accountId;
    fill._instrumentId = s1_a1->_instrumentId;
    fill._order = s1_a1;
    
    p_orders.onFill(fill);
    
    // pos + open_asks = (abs(6-7=-1(pos))+8+8=17 > 10 (maxpos) - reject
    //
    s1_a1->_state = tw::channel_or::eOrderState::kUnknown;
    s1_a1->_cumQty.set(0);
    s1_a1->_qty.set(8);
    
    ASSERT_TRUE(!p_out.sendNew(s1_a1, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorRisk);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kSymbolMaxPos);
    
    // pos + open_asks = (abs(6-7=-1(pos))+8+1=10 = 10 (maxpos)
    //
    s1_a1->_state = tw::channel_or::eOrderState::kUnknown;
    s1_a1->_qty.set(1);
    
    ASSERT_TRUE(p_out.sendNew(s1_a1, rej));
    
    // Bids for NQH2
    //
    
    // pos + open_bids = -1(pos)+2+2+8=11 > 10 (maxpos) - reject
    //
    s1_b4->_state = tw::channel_or::eOrderState::kUnknown;
    s1_b4->_qty.set(8);
    
    ASSERT_TRUE(!p_out.sendNew(s1_b4, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorRisk);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kSymbolMaxPos);
    
    // pos + open_bids = -1(pos)+2+2+7=10 = 10 (maxpos)
    //
    s1_b4->_state = tw::channel_or::eOrderState::kUnknown;
    s1_b4->_qty.set(7);
    
    ASSERT_TRUE(p_out.sendNew(s1_b4, rej));
    
    
    // Bids for NQU2
    //
    
    // open_bids = 12 > 11 (maxpos) - reject
    //
    ASSERT_TRUE(!p_out.sendNew(s2_b1, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorRisk);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kSymbolMaxPos);
    
    s2_b1->_state = tw::channel_or::eOrderState::kUnknown;
    s2_b1->_qty.set(11);
    
    // open_bids = 11 = 11 (maxpos)
    //
    ASSERT_TRUE(p_out.sendNew(s2_b1, rej));
    
    
    // Asks for NQU2
    //
    
    // open_asks = 12 > 11 (maxpos) - reject
    //
    ASSERT_TRUE(!p_out.sendNew(s2_a1, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorRisk);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kSymbolMaxPos);
    
    s2_a1->_state = tw::channel_or::eOrderState::kUnknown;
    s2_a1->_qty.set(11);
    
    // open_asks = 11 = 11 (maxpos)
    //
    ASSERT_TRUE(p_out.sendNew(s2_a1, rej));
    
    
    p_risk.clear();
    p_orders.clear();
}


TEST(ChannelOrLibTestSuit, processor_risk_bug_long_pos)
{
    // Structs for passing to processor
    //
    TOrderPtr s1_b1;
    TOrderPtr s1_a1;
    TOrderPtr s1_a2;
    
    Fill fill;
    Reject rej;    
    
    tw::instr::InstrumentPtr instrNQH2;
    
    tw::risk::Account account;
    std::vector<tw::risk::AccountRiskParams> symbolsParams;
    
    // Init processors
    //
    TProcessorOrders& p_orders = TProcessorOrders::instance();
    TProcessorRisk& p_risk = TProcessorRisk::instance();
    
    TProcessorOutRisk p_out_risk(p_risk);
    TProcessorOut p_out(p_orders, p_out_risk);
    
    p_risk.clear();
    p_orders.clear();
    
    // Init instruments
    //
    instrNQH2 = InstrHelper::getNQH2();
    
    // Init risk parameters
    //
    account._id = 1;
    account._name = "test";
    account._tradeEnabled = true;
    
    symbolsParams.resize(1);
    
    symbolsParams[0]._accountId = account._id;
    symbolsParams[0]._displayName = instrNQH2->_displayName;
    symbolsParams[0]._exchange = instrNQH2->_exchange;
    symbolsParams[0]._maxPos = 0;
    symbolsParams[0]._tradeEnabled = false;        
    
    // Init orders
    //
    
    // NQH2 orders
    //
    s1_b1 = OrderHelper::getEmptyWithOrderId(instrNQH2);
    s1_a1 = OrderHelper::getEmptyWithOrderId(instrNQH2);
    s1_a2 = OrderHelper::getEmptyWithOrderId(instrNQH2);
    
    s1_b1->_side = tw::channel_or::eOrderSide::kBuy;
    s1_a1->_side = s1_a2->_side = tw::channel_or::eOrderSide::kSell;
    
    s1_b1->_accountId = s1_a1->_accountId = s1_a2->_accountId = account._id;
    s1_b1->_state = s1_a1->_state = tw::channel_or::eOrderState::kUnknown;
    s1_b1->_instrumentId = s1_a1->_instrumentId = instrNQH2->_keyId;
    
    s1_b1->_qty.set(2);
    s1_a1->_qty.set(1);
    
    // Init risk system
    //        
    symbolsParams[0]._tradeEnabled = true;
    symbolsParams[0]._clipSize = 2*10;
    symbolsParams[0]._maxPos = 10;
    ASSERT_TRUE(p_risk.init(account, symbolsParams));    
    
    // Bids for NQH2
    //
    
    // open_bids = 2 < 10 (maxpos)
    //
    s1_b1->_state = tw::channel_or::eOrderState::kUnknown;
    ASSERT_TRUE(p_out.sendNew(s1_b1, rej));
    p_orders.onNewAck(s1_b1);
    
    // Check fills for bids for NQH2
    //    
    
    // Fill for 2 longs => pos = 0+2=2
    //
    fill._type = tw::channel_or::eFillType::kNormal;
    fill._orderId = s1_b1->_orderId;
    fill._order = s1_b1;
    fill._qty = s1_b1->_qty;
    fill._side = s1_b1->_side;
    fill._accountId = s1_b1->_accountId;
    fill._instrumentId = s1_b1->_instrumentId;
    fill._order = s1_b1;
    
    p_orders.onFill(fill);
    
    // Asks for NQH2
    //
    
    // open_asks = 1 < 10(maxpos)+2(long pos)=12
    //
    ASSERT_TRUE(p_out.sendNew(s1_a1, rej));
    
    
    // open_asks = 13 > 10(maxpos)+2(long pos)=12
    //
    s1_a2->_qty.set(12);
    ASSERT_TRUE(!p_out.sendNew(s1_a2, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorRisk);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kSymbolMaxPos);
    
    s1_a2->_state = tw::channel_or::eOrderState::kUnknown;
    s1_a2->_qty.set(11);
    
    // open_asks = 12 = 10(maxpos)+2(long pos)=12
    //
    s1_a2->_qty.set(11);
    s1_a2->_state = tw::channel_or::eOrderState::kUnknown;
    ASSERT_TRUE(p_out.sendNew(s1_a2, rej));
    
}


TEST(ChannelOrLibTestSuit, processor_risk_bug_short_pos)
{
    // Structs for passing to processor
    //
    TOrderPtr s1_b1;
    TOrderPtr s1_b2;
    TOrderPtr s1_a1;
    
    Fill fill;
    Reject rej;    
    
    tw::instr::InstrumentPtr instrNQH2;
    
    tw::risk::Account account;
    std::vector<tw::risk::AccountRiskParams> symbolsParams;
    
    // Init processors
    //
    TProcessorOrders& p_orders = TProcessorOrders::instance();
    TProcessorRisk& p_risk = TProcessorRisk::instance();
    
    TProcessorOutRisk p_out_risk(p_risk);
    TProcessorOut p_out(p_orders, p_out_risk);
    
    p_risk.clear();
    p_orders.clear();
    
    // Init instruments
    //
    instrNQH2 = InstrHelper::getNQH2();
    
    // Init risk parameters
    //
    account._id = 1;
    account._name = "test";
    account._tradeEnabled = true;
    
    symbolsParams.resize(1);
    
    symbolsParams[0]._accountId = account._id;
    symbolsParams[0]._displayName = instrNQH2->_displayName;
    symbolsParams[0]._exchange = instrNQH2->_exchange;
    symbolsParams[0]._maxPos = 0;
    symbolsParams[0]._tradeEnabled = false;        
    
    // Init orders
    //
    
    // NQH2 orders
    //
    s1_b1 = OrderHelper::getEmptyWithOrderId(instrNQH2);
    s1_b2 = OrderHelper::getEmptyWithOrderId(instrNQH2);
    s1_a1 = OrderHelper::getEmptyWithOrderId(instrNQH2);
    
    s1_b1->_side = tw::channel_or::eOrderSide::kBuy;
    s1_b2->_side = tw::channel_or::eOrderSide::kBuy;
    s1_a1->_side = tw::channel_or::eOrderSide::kSell;
    
    s1_b1->_accountId = s1_b2->_accountId = s1_a1->_accountId = account._id;
    s1_b1->_state = s1_b2->_state = s1_a1->_state = tw::channel_or::eOrderState::kUnknown;
    s1_b1->_instrumentId = s1_b2->_instrumentId = s1_a1->_instrumentId = instrNQH2->_keyId;
    
    s1_b1->_qty.set(1);
    s1_a1->_qty.set(2);                        
    
    // Init risk system
    //        
    symbolsParams[0]._tradeEnabled = true;
    symbolsParams[0]._clipSize = 2*10;
    symbolsParams[0]._maxPos = 10;
    ASSERT_TRUE(p_risk.init(account, symbolsParams));    
    
    // Asks for NQH2
    //
    
    // open_asks = 2 < 10 (maxpos)
    //
    s1_a1->_state = tw::channel_or::eOrderState::kUnknown;
    ASSERT_TRUE(p_out.sendNew(s1_a1, rej));
    p_orders.onNewAck(s1_a1);
    
    // Check fills for bids for NQH2
    //    
    
    // Fill for 2 shorts => pos = 0+2=2
    //
    fill._type = tw::channel_or::eFillType::kNormal;
    fill._orderId = s1_a1->_orderId;
    fill._order = s1_a1;
    fill._qty = s1_a1->_qty;
    fill._side = s1_a1->_side;
    fill._accountId = s1_a1->_accountId;
    fill._instrumentId = s1_a1->_instrumentId;
    fill._order = s1_a1;
    
    p_orders.onFill(fill);
    
    // Bids for NQH2
    //
    
    // open_bids = 1 < 10(maxpos)+2(short pos)=12
    //
    ASSERT_TRUE(p_out.sendNew(s1_b1, rej));
    
    
    // open_bids = 13 > 10(maxpos)+2(long pos)=12
    //
    s1_b2->_qty.set(12);
    ASSERT_TRUE(!p_out.sendNew(s1_b2, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorRisk);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kSymbolMaxPos);
    
    s1_b2->_state = tw::channel_or::eOrderState::kUnknown;
    s1_b2->_qty.set(11);
    
    // open_bids = 12 = 10(maxpos)+2(long pos)=12
    //
    s1_b2->_qty.set(11);
    s1_b2->_state = tw::channel_or::eOrderState::kUnknown;
    ASSERT_TRUE(p_out.sendNew(s1_b2, rej));
    
}


TEST(ChannelOrLibTestSuit, processor_risk_can_send)
{
    // Structs for passing to processor
    //
    TOrderPtr s1_b1;
    TOrderPtr s1_b2;
    TOrderPtr s1_a1;
    TOrderPtr s1_a2;
    
    Reject rej;
    Fill fill;
    
    tw::instr::InstrumentPtr instrNQH2;
    
    tw::risk::Account account;
    std::vector<tw::risk::AccountRiskParams> symbolsParams;
    
    // Init processors
    //
    TProcessorOrders& p_orders = TProcessorOrders::instance();
    TProcessorRisk& p_risk = TProcessorRisk::instance();
    
    TProcessorOutRisk p_out_risk(p_risk);
    TProcessorOut p_out(p_orders, p_out_risk);
    
    p_risk.clear();
    p_orders.clear();
    
    // Init instruments
    //
    instrNQH2 = InstrHelper::getNQH2();
    
    // Init risk parameters
    //
    account._id = 1;
    account._name = "test";
    account._tradeEnabled = true;
    
    symbolsParams.resize(1);
    
    symbolsParams[0]._accountId = account._id;
    symbolsParams[0]._displayName = instrNQH2->_displayName;
    symbolsParams[0]._exchange = instrNQH2->_exchange;
    symbolsParams[0]._maxPos = 0;
    symbolsParams[0]._tradeEnabled = false;        
    
    // Init orders
    //
    
    // NQH2 orders
    //
    s1_b1 = OrderHelper::getEmptyWithOrderId(instrNQH2);
    s1_b2 = OrderHelper::getEmptyWithOrderId(instrNQH2);
    s1_a1 = OrderHelper::getEmptyWithOrderId(instrNQH2);
    s1_a2 = OrderHelper::getEmptyWithOrderId(instrNQH2);
    
    s1_b1->_side = tw::channel_or::eOrderSide::kBuy;
    s1_b2->_side = tw::channel_or::eOrderSide::kBuy;
    s1_a1->_side = tw::channel_or::eOrderSide::kSell;
    s1_a2->_side = tw::channel_or::eOrderSide::kSell;
    
    s1_b1->_accountId = s1_b2->_accountId = s1_a1->_accountId = s1_a2->_accountId = account._id;
    s1_b1->_state = s1_b2->_state = s1_a1->_state = s1_a2->_state = tw::channel_or::eOrderState::kUnknown;
    s1_b1->_instrumentId = s1_b2->_instrumentId = s1_a1->_instrumentId = s1_a2->_instrumentId = instrNQH2->_keyId;
    
    // Init risk system
    //        
    symbolsParams[0]._tradeEnabled = true;
    symbolsParams[0]._clipSize = 2*10;
    symbolsParams[0]._maxPos = 10;
    ASSERT_TRUE(p_risk.init(account, symbolsParams)); 
    
    // Check 'canSend'
    //
    
    // Bids
    //
    s1_b1->_qty.set(10);
    ASSERT_TRUE(p_risk.canSend(s1_b1));
    
    s1_b1->_qty.set(11);
    ASSERT_TRUE(!p_risk.canSend(s1_b1));
    
    // Asks
    //
    s1_a1->_qty.set(10);
    ASSERT_TRUE(p_risk.canSend(s1_a1));
    
    s1_a1->_qty.set(11);
    ASSERT_TRUE(!p_risk.canSend(s1_a1));
    
    // Fill for 'long' side
    
    // open_bids = 2 < 10 (maxpos)
    //
    s1_b1->_qty.set(2);
    s1_b1->_state = tw::channel_or::eOrderState::kUnknown;
    ASSERT_TRUE(p_out.sendNew(s1_b1, rej));
    p_orders.onNewAck(s1_b1);
    
    // Fill for 2 longs => pos = 0+2=2
    //
    fill._type = tw::channel_or::eFillType::kNormal;
    fill._orderId = s1_b1->_orderId;
    fill._order = s1_b1;
    fill._qty = s1_b1->_qty;
    fill._side = s1_b1->_side;
    fill._accountId = s1_b1->_accountId;
    fill._instrumentId = s1_b1->_instrumentId;
    fill._order = s1_b1;
    
    p_orders.onFill(fill);
    
    
    // Check 'canSend'
    //
    
    // Bids
    //
    s1_b2->_qty.set(8);
    ASSERT_TRUE(p_risk.canSend(s1_b2));
    
    s1_b2->_qty.set(9);
    ASSERT_TRUE(!p_risk.canSend(s1_b2));
    
    // Asks
    //
    s1_a1->_qty.set(12);
    ASSERT_TRUE(p_risk.canSend(s1_a1));
    
    s1_a1->_qty.set(13);
    ASSERT_TRUE(!p_risk.canSend(s1_a1));
    
    
    // Fill for 'short' side
    
    // open_asks = 3 < 10 (maxpos)
    //
    s1_a1->_qty.set(5);
    s1_a1->_state = tw::channel_or::eOrderState::kUnknown;
    ASSERT_TRUE(p_out.sendNew(s1_a1, rej));
    p_orders.onNewAck(s1_b1);
    
    // Fill for 5 shorts => pos = 2-5=-3
    //
    fill._type = tw::channel_or::eFillType::kNormal;
    fill._orderId = s1_a1->_orderId;
    fill._order = s1_a1;
    fill._qty = s1_a1->_qty;
    fill._side = s1_a1->_side;
    fill._accountId = s1_a1->_accountId;
    fill._instrumentId = s1_a1->_instrumentId;
    fill._order = s1_a1;
    
    p_orders.onFill(fill);
    
    
    // Check 'canSend'
    //
    
    // Bids
    //
    s1_b2->_qty.set(13);
    ASSERT_TRUE(p_risk.canSend(s1_b2));
    
    s1_b2->_qty.set(14);
    ASSERT_TRUE(!p_risk.canSend(s1_b2));
    
    // Asks
    //
    s1_a2->_qty.set(7);
    ASSERT_TRUE(p_risk.canSend(s1_a2));
    
    s1_a2->_qty.set(8);
    ASSERT_TRUE(!p_risk.canSend(s1_a2));
    
    
    
}

