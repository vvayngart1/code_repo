#include <tw/channel_or/processor_orders.h>
#include <tw/channel_or/processor_pnl.h>
#include <tw/channel_or/processor.h>
#include <tw/common_trade/wbo_manager.h>

#include "order_helper.h"

#include <gtest/gtest.h>

typedef tw::channel_or::TOrderPtr TOrderPtr;
typedef tw::channel_or::Reject Reject;
typedef tw::channel_or::Fill Fill;
typedef tw::channel_or::PosUpdate PosUpdate;
typedef tw::channel_or::Alert Alert;
typedef tw::channel_or::TOrders TOrders;

typedef tw::channel_or::ProcessorOrders TProcessorOrders;
typedef tw::channel_or::ProcessorPnL TProcessorPnL;

// NOTE: the order of template initialization for 'out processors'
// is from last TProcessorPnL to first TProcessorOrders
//
typedef tw::channel_or::ProcessorOut<TProcessorPnL> TProcessorOutPnL;
typedef tw::channel_or::ProcessorOut<TProcessorOrders, TProcessorOutPnL> TProcessorOut;

// NOTE: the order of template initialization for 'in processors'
// is from last TProcessorOrders to first TProcessorPnL
//
typedef tw::channel_or::ProcessorIn<tw::channel_or::ProcessorOrders> TProcessorInOrders;
typedef tw::channel_or::ProcessorIn<TProcessorPnL, TProcessorInOrders> TProcessorIn;


TEST(ChannelOrLibTestSuit, processor_pnl)
{
    // Init params used
    //
    tw::risk::Account accountParams;
    
    accountParams._id = 1;
    accountParams._name = "acc1";
    accountParams._maxRealizedLoss = 200.0;
    accountParams._maxUnrealizedLoss = 350.0;
    accountParams._maxTotalLoss = 10000.0;
    accountParams._maxRealizedDrawdown = 406.25;
    accountParams._maxUnrealizedDrawdown = 593.75;
    
    tw::risk::Strategy stratParams1;
    
    stratParams1._id = 11;
    stratParams1._name = "acc1_strat1";
    stratParams1._maxRealizedLoss = 150.0;
    stratParams1._maxUnrealizedLoss = 250.0;
    stratParams1._maxRealizedDrawdown = 300;
    stratParams1._maxUnrealizedDrawdown = 350;
    stratParams1._tradeEnabled = true;
    
    
    tw::risk::Strategy stratParams2;
    
    stratParams2._id = 12;
    stratParams2._name = "acc1_strat2";
    stratParams2._maxRealizedLoss = 140.0;
    stratParams2._maxUnrealizedLoss = 240.0;
    stratParams2._maxRealizedDrawdown = 281.25;
    stratParams2._maxUnrealizedDrawdown = 312.5;
    stratParams2._tradeEnabled = true;
    
    std::vector<tw::risk::Strategy> stratsParams;
    stratsParams.push_back(stratParams1);
    stratsParams.push_back(stratParams2);
    
    // Init processors
    //
    TProcessorOrders& p_orders = TProcessorOrders::instance();
    TProcessorPnL& p_pnl = TProcessorPnL::instance();
    
    p_orders.clear();
    p_pnl.clear();
    
    TProcessorOutPnL p_out_pnl(p_pnl);
    TProcessorOut p_out(p_orders, p_out_pnl);
    
    TProcessorInOrders p_in_orders(p_orders);
    TProcessorIn p_in(p_pnl, p_in_orders);
    
    // Structs for passing to processor
    //
    tw::instr::InstrumentPtr instrNQM2 = InstrHelper::getNQM2();
    tw::instr::InstrumentPtr instrZNM2 = InstrHelper::getZNM2();
    
    TOrderPtr s1_b1;
    TOrderPtr s1_b2;
    TOrderPtr s1_a1;
    TOrderPtr s1_a2;    
    
    TOrderPtr s2_b1;
    TOrderPtr s2_b2;
    TOrderPtr s2_a1;
    
    PosUpdate posUpdate1;
    PosUpdate posUpdate2;
    Fill fill1;
    Fill fill2;
    Reject rej;
    
    double price = 0;
    uint32_t qty = 0;
    tw::price::Ticks ticks(0);
    
    // Strat1 pos: 5
    //
    posUpdate1._accountId = accountParams._id;
    posUpdate1._strategyId = stratParams1._id;
    posUpdate1._exchange = instrNQM2->_exchange;
    posUpdate1._displayName = instrNQM2->_displayName;
    posUpdate1._pos = 5;
    posUpdate1._avgPrice = 274175;
    
    // Strat2 pos: -10
    //
    posUpdate2._accountId = accountParams._id;
    posUpdate2._strategyId = stratParams2._id;
    posUpdate2._exchange = instrZNM2->_exchange;
    posUpdate2._displayName = instrZNM2->_displayName;
    posUpdate2._pos = -10;
    posUpdate2._avgPrice = 130.984375;
    
    // Fill1
    //
    fill1._type = tw::channel_or::eFillType::kNormal;
    fill1._subType = tw::channel_or::eFillSubType::kOutright;
    fill1._accountId = accountParams._id;
    fill1._strategyId = stratParams1._id;
    fill1._instrumentId = instrNQM2->_keyId;
    fill1._liqInd = tw::channel_or::eLiqInd::kRem;
    
    // Fill2
    //
    fill2._type = tw::channel_or::eFillType::kNormal;
    fill2._subType = tw::channel_or::eFillSubType::kOutright;
    fill2._accountId = accountParams._id;
    fill2._strategyId = stratParams2._id;
    fill2._instrumentId = instrZNM2->_keyId;
    fill2._liqInd = tw::channel_or::eLiqInd::kRem;    
    
    // Get tvs
    //
    tw::common_trade::WboManager::TTv& tv1 = tw::common_trade::WboManager::instance().getOrCreateTv(instrNQM2->_keyId);
    tw::common_trade::WboManager::TTv& tv2 = tw::common_trade::WboManager::instance().getOrCreateTv(instrZNM2->_keyId);
            
    
    // Initialize ProcessorPnL
    //
    ASSERT_TRUE(p_pnl.init(accountParams, stratsParams));
    
    // Add posUpdates
    //
    p_out.rebuildPos(posUpdate1);
    p_out.rebuildPos(posUpdate2);
    
    // Get account and strats pnls
    //
    const tw::channel_or::TPnLComposite& accountPnl = p_pnl.getAccountPnL();
    const tw::channel_or::ProcessorPnL::TStratsPnL& stratsPnl = p_pnl.getStratsPnL();
    
    tw::channel_or::ProcessorPnL::TStratsPnL::const_iterator iter;
    
    iter = stratsPnl.find(stratParams1._id);
    ASSERT_TRUE(iter != stratsPnl.end());    
    const tw::channel_or::TPnLComposite& stratPnL1 = iter->second._stratPnL;
    
    iter = stratsPnl.find(stratParams2._id);
    ASSERT_TRUE(iter != stratsPnl.end());    
    const tw::channel_or::TPnLComposite& stratPnL2 = iter->second._stratPnL;
    
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedPnL(), 0.0);    
    ASSERT_DOUBLE_EQ(accountPnl.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedDrawdown(), 0.0);    
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExLiqRem, 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExClearing, 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeBrokerage, 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feePerTrade, 0.0);
    
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedPnL(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedDrawdown(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExLiqRem, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExClearing, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeBrokerage, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feePerTrade, 0.0);
    
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedPnL(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedDrawdown(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExLiqRem, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExClearing, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeBrokerage, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feePerTrade, 0.0);    
    
    
    // Start testing with orders
    //
    
    // pnl is '0' at this point - order should not be rejected
    //
    price = 274175;    
    ticks = instrNQM2->_tc->fromExchangePrice(price);
    qty = 1;
    s1_b1 = OrderHelper::getBuyLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams1._id, instrNQM2);
    
    ASSERT_TRUE(p_out.sendNew(s1_b1, rej));
    ASSERT_EQ(p_orders.getAll().size(), 1);
    p_in.onNewRej(s1_b1, rej);
    ASSERT_EQ(p_orders.getAll().size(), 0);
    
    // 'Update' stratPnl1 to '-275', which is bigger than strat's 'maxUnrelizedLoss'
    //
    price = 274175;
    ticks = instrNQM2->_tc->fromExchangePrice(price);
    price = instrNQM2->_tc->toExchangePrice(ticks-11);
    tv1.setTv(price);
    qty = 1;
    s1_b1 = OrderHelper::getBuyLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams1._id, instrNQM2);
    
    ASSERT_TRUE(!p_out.sendNew(s1_b1, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorPnL);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kMaxUnrealizedLossStrategy);
    ASSERT_EQ(rej._text, "275.00 :: 250");
    ASSERT_EQ(p_orders.getAll().size(), 0);
    
    // The other side order should be able to go through
    //
    s1_a2 = OrderHelper::getSellLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams1._id, instrNQM2);
    ASSERT_TRUE(p_out.sendNew(s1_a2, rej));
    ASSERT_EQ(p_orders.getAll().size(), 1);
    p_in.onNewRej(s1_a2, rej);
    ASSERT_EQ(p_orders.getAll().size(), 0);
    
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedPnL(), -275.0);    
    ASSERT_DOUBLE_EQ(accountPnl.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedDrawdown(), 275.0);    
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExLiqRem, 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExClearing, 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeBrokerage, 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feePerTrade, 0.0);
    
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedPnL(), -275.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedDrawdown(), 275.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExLiqRem, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExClearing, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeBrokerage, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feePerTrade, 0.0);
    
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedPnL(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedDrawdown(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExLiqRem, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExClearing, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeBrokerage, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feePerTrade, 0.0);
    
    
    // 'Update' stratPnl2 to '-156.25', which would bring total accPnl to '-431.25'
    // which is bigger than acc's 'maxUnrelizedLoss'
    //
    price = 130.984375;
    ticks = instrZNM2->_tc->fromExchangePrice(price);
    price = instrZNM2->_tc->toExchangePrice(ticks+1);
    tv2.setTv(price);    
    s2_a1 = OrderHelper::getSellLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams2._id, instrZNM2);
    
    ASSERT_TRUE(!p_out.sendNew(s2_a1, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorPnL);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kMaxUnrealizedLossAccount);
    ASSERT_EQ(rej._text, "431.25 :: 350");
    ASSERT_EQ(p_orders.getAll().size(), 0);
    
    // The other side order should be able to go through
    //
    s2_b1 = OrderHelper::getBuyLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams2._id, instrZNM2);
    ASSERT_TRUE(p_out.sendNew(s2_b1, rej));
    ASSERT_EQ(p_orders.getAll().size(), 1);
    p_in.onNewRej(s2_b1, rej);
    ASSERT_EQ(p_orders.getAll().size(), 0);
    
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedPnL(), -431.25);    
    ASSERT_DOUBLE_EQ(accountPnl.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedDrawdown(), 431.25);    
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExLiqRem, 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExClearing, 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeBrokerage, 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feePerTrade, 0.0);
    
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedPnL(), -275.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedDrawdown(), 275.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExLiqRem, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExClearing, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeBrokerage, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feePerTrade, 0.0);
    
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedPnL(), -156.25);    
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedDrawdown(), 156.25);    
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExLiqRem, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExClearing, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeBrokerage, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feePerTrade, 0.0);
    
    // Test maxRealizedLoss
    //    
    
    price = 274175;
    ticks = instrNQM2->_tc->fromExchangePrice(price)-10;
    price = instrNQM2->_tc->toExchangePrice(ticks);
    qty = 5;
    s1_a1 = OrderHelper::getSellLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams1._id, instrNQM2);
    
    ASSERT_TRUE(p_out.sendNew(s1_a1, rej));    
    ASSERT_EQ(p_orders.getAll().size(), 1);
    
    p_in.onNewAck(s1_a1);
    ASSERT_EQ(p_orders.getAll().size(), 1);
    
    fill1._order = s1_a1;
    fill1._orderId = s1_a1->_orderId;
    fill1._side = s1_a1->_side;
    fill1._qty.set(4);
    fill1._price = ticks;
    
    p_in.onFill(fill1);
    ASSERT_EQ(p_orders.getAll().size(), 1);
    ASSERT_EQ(fill1._posAccount, 1);
    ASSERT_EQ(fill1._posStrategy, 1);
    
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedPnL(), -200.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedPnL(), -211.25);    
    ASSERT_DOUBLE_EQ(accountPnl.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedDrawdown(), 200.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedDrawdown(), 211.25);    
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExLiqRem, 0.24);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExClearing, 0.28);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeBrokerage, 0.32);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feePerTrade, 0.03);
    
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -200.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedPnL(), -55.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 200.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedDrawdown(), 55.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExLiqRem, 0.24);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExClearing, 0.28);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeBrokerage, 0.32);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feePerTrade, 0.03);
    
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedPnL(), -156.25);    
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedDrawdown(), 156.25);    
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExLiqRem, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExClearing, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeBrokerage, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feePerTrade, 0.0);
    
    // Test acc's maxRealizedLoss
    //
    price = 274175;
    ticks = instrNQM2->_tc->fromExchangePrice(price);
    qty = 1;
    s1_b2 = OrderHelper::getBuyLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams1._id, instrNQM2);
    
    ASSERT_TRUE(!p_out.sendNew(s1_b2, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorPnL);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kMaxRealizedLossAccount);
    ASSERT_EQ(rej._text, "200.87 :: 200");
    ASSERT_EQ(p_orders.getAll().size(), 1);
    
    // The other side order should be able to go through
    //
    s1_a2 = OrderHelper::getSellLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams1._id, instrNQM2);
    ASSERT_TRUE(p_out.sendNew(s1_a2, rej));
    ASSERT_EQ(p_orders.getAll().size(), 2);
    p_in.onNewRej(s1_a2, rej);
    ASSERT_EQ(p_orders.getAll().size(), 1);
    
    // Test acc's maxTotalLoss
    //
    p_pnl.getAccount()._maxRealizedLoss = 10000.0;
    p_pnl.getAccount()._maxTotalLoss = 412.0;
    
    price = 274175;
    ticks = instrNQM2->_tc->fromExchangePrice(price);
    qty = 1;
    s1_b2 = OrderHelper::getBuyLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams1._id, instrNQM2);
    
    ASSERT_TRUE(!p_out.sendNew(s1_b2, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorPnL);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kMaxTotalLossAccount);
    ASSERT_EQ(rej._text, "412.12 :: 412");
    ASSERT_EQ(p_orders.getAll().size(), 1);
    
    // The other side order should be able to go through
    //
    s1_a2 = OrderHelper::getSellLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams1._id, instrNQM2);
    ASSERT_TRUE(p_out.sendNew(s1_a2, rej));
    ASSERT_EQ(p_orders.getAll().size(), 2);
    p_in.onNewRej(s1_a2, rej);
    ASSERT_EQ(p_orders.getAll().size(), 1);
    
    p_pnl.getAccount()._maxRealizedLoss = 200.0;
    p_pnl.getAccount()._maxTotalLoss = 10000;
    
    // Test strat's maxRealizedLoss
    //
    price = 274175;
    
    fill1._order = s1_a1;
    fill1._orderId = s1_a1->_orderId;
    fill1._side = s1_a1->_side;
    fill1._qty.set(1);
    fill1._price = instrNQM2->_tc->fromExchangePrice(price)+10;
    
    p_in.onFill(fill1);
    ASSERT_EQ(p_orders.getAll().size(), 0);
    ASSERT_EQ(fill1._posAccount, 0);
    ASSERT_EQ(fill1._posStrategy, 0);
    
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedPnL(), -150.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedPnL(), -156.25);    
    ASSERT_DOUBLE_EQ(accountPnl.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedDrawdown(), 150.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedDrawdown(), 156.25);    
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExLiqRem, 0.30);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExClearing, 0.35);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeBrokerage, 0.40);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feePerTrade, 0.06);
    
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -150.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedPnL(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 150.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedDrawdown(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExLiqRem, 0.30);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExClearing, 0.35);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeBrokerage, 0.40);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feePerTrade, 0.06);
    
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedPnL(), -156.25);    
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedDrawdown(), 156.25);    
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExLiqRem, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExClearing, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeBrokerage, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feePerTrade, 0.0);
    
    // Both buys and sells for strat1 should be rejected
    //
    
    // Buy reject
    //
    price = 274175;
    ticks = instrNQM2->_tc->fromExchangePrice(price);
    qty = 1;
    s1_b1 = OrderHelper::getBuyLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams1._id, instrNQM2);
    
    ASSERT_EQ(s1_b1->_side, tw::channel_or::eOrderSide::kBuy);
    ASSERT_TRUE(!p_out.sendNew(s1_b1, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorPnL);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kMaxRealizedLossStrategy);
    ASSERT_EQ(rej._text, "151.11 :: 150");
    ASSERT_EQ(p_orders.getAll().size(), 0);
    
    
    // Sell reject
    //
    price = 274175;
    ticks = instrNQM2->_tc->fromExchangePrice(price);
    qty = 1;
    s1_a1 = OrderHelper::getSellLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams1._id, instrNQM2);
    
    ASSERT_EQ(s1_a1->_side, tw::channel_or::eOrderSide::kSell);
    ASSERT_TRUE(!p_out.sendNew(s1_a1, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorPnL);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kMaxRealizedLossStrategy);
    ASSERT_EQ(rej._text, "151.11 :: 150");
    ASSERT_EQ(p_orders.getAll().size(), 0);
    
    // Test maxUnrealizedDrawdown
    //
    
    // Strat's maxUnrealizedDrawdown
    //
    
    // Set tv high and send new order
    //
    price = 130.984375;
    ticks = instrZNM2->_tc->fromExchangePrice(price);
    price = instrZNM2->_tc->toExchangePrice(ticks-2); // minus 2 ticks
    tv2.setTv(price);
    qty = 1;
    s2_b1 = OrderHelper::getBuyLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams2._id, instrZNM2);
    
    ASSERT_TRUE(p_out.sendNew(s2_b1, rej));
    ASSERT_EQ(p_orders.getAll().size(), 1);
    
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedPnL(), -150.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedPnL(), 312.5);    
    ASSERT_DOUBLE_EQ(accountPnl.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getMaxUnrealizedPnL(), 312.5);
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedDrawdown(), 150.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedDrawdown(), 0.0);    
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExLiqRem, 0.30);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExClearing, 0.35);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeBrokerage, 0.40);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feePerTrade, 0.06);
    
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -150.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedPnL(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 150.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedDrawdown(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExLiqRem, 0.30);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExClearing, 0.35);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeBrokerage, 0.40);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feePerTrade, 0.06);
    
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedPnL(), 312.5);    
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxUnrealizedPnL(), 312.5);
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedDrawdown(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExLiqRem, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExClearing, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeBrokerage, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feePerTrade, 0.0);
    
    
    // Get fill for that order
    //
    fill2._order = s2_b1;
    fill2._orderId = s2_b1->_orderId;
    fill2._side = s2_b1->_side;
    fill2._qty.set(1);
    fill2._price = s2_b1->_price;
    
    p_in.onFill(fill2);
    ASSERT_EQ(p_orders.getAll().size(), 0);
    ASSERT_EQ(fill2._posAccount, -9);
    ASSERT_EQ(fill2._posStrategy, -9);
    
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedPnL(), -150.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedPnL(), 281.25);    
    ASSERT_DOUBLE_EQ(accountPnl.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getMaxUnrealizedPnL(), 312.5);
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedDrawdown(), 150.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedDrawdown(), 31.25);    
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExLiqRem, 0.46);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExClearing, 0.52);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeBrokerage, 0.58);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feePerTrade, 0.19);
    
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -150.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedPnL(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 150.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedDrawdown(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExLiqRem, 0.30);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExClearing, 0.35);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeBrokerage, 0.40);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feePerTrade, 0.06);
    
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedPnL(), 281.25);    
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxUnrealizedPnL(), 312.5);
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedDrawdown(), 31.25);    
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExLiqRem, 0.16);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExClearing, 0.17);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeBrokerage, 0.18);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feePerTrade, 0.13);
    
    // Set tv 'low' and send new order, which will be rejected
    //
    price = 130.984375;
    ticks = instrZNM2->_tc->fromExchangePrice(price);
    tv2.setTv(price);
    qty = 1;
    s2_a1 = OrderHelper::getSellLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams2._id, instrZNM2);
    
    ASSERT_TRUE(!p_out.sendNew(s2_a1, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorPnL);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kMaxUnrealizedDrawdownStrategy);
    ASSERT_EQ(rej._text, "313.14 :: 312.5");
    ASSERT_EQ(p_orders.getAll().size(), 0);
    
    // The other side order should be able to go through
    //
    s2_b1 = OrderHelper::getBuyLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams2._id, instrZNM2);
    ASSERT_TRUE(p_out.sendNew(s2_b1, rej));
    ASSERT_EQ(p_orders.getAll().size(), 1);
    p_in.onNewRej(s2_b1, rej);
    ASSERT_EQ(p_orders.getAll().size(), 0);
    
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedPnL(), -150.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedPnL(), 0.0);    
    ASSERT_DOUBLE_EQ(accountPnl.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getMaxUnrealizedPnL(), 312.5);
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedDrawdown(), 150.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedDrawdown(), 312.5);    
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExLiqRem, 0.46);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExClearing, 0.52);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeBrokerage, 0.58);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feePerTrade, 0.19);
    
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -150.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedPnL(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 150.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedDrawdown(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExLiqRem, 0.30);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExClearing, 0.35);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeBrokerage, 0.40);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feePerTrade, 0.06);
    
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedPnL(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxUnrealizedPnL(), 312.5);
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedDrawdown(), 312.5);    
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExLiqRem, 0.16);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExClearing, 0.17);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeBrokerage, 0.18);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feePerTrade, 0.13);
    
    
    // Acc's maxUnrealizedDrawdown
    //
    
    // Set tv 'low' and send new order, which will be rejected
    //
    price = 130.984375;
    ticks = instrZNM2->_tc->fromExchangePrice(price);
    price = instrZNM2->_tc->toExchangePrice(ticks+2);
    tv2.setTv(price);
    qty = 1;
    s2_a1 = OrderHelper::getSellLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams2._id, instrZNM2);
    
    ASSERT_TRUE(!p_out.sendNew(s2_a1, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorPnL);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kMaxUnrealizedDrawdownAccount);
    ASSERT_EQ(rej._text, "595.50 :: 593.75");
    ASSERT_EQ(p_orders.getAll().size(), 0);
    
    // The other side order should be able to go through
    //
    s2_b1 = OrderHelper::getBuyLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams2._id, instrZNM2);
    ASSERT_TRUE(p_out.sendNew(s2_b1, rej));
    ASSERT_EQ(p_orders.getAll().size(), 1);
    p_in.onNewRej(s2_b1, rej);
    ASSERT_EQ(p_orders.getAll().size(), 0);
    
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedPnL(), -150.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedPnL(), -281.25);    
    ASSERT_DOUBLE_EQ(accountPnl.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getMaxUnrealizedPnL(), 312.5);
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedDrawdown(), 150.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedDrawdown(), 593.75);    
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExLiqRem, 0.46);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExClearing, 0.52);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeBrokerage, 0.58);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feePerTrade, 0.19);
    
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -150.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedPnL(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 150.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedDrawdown(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExLiqRem, 0.30);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExClearing, 0.35);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeBrokerage, 0.40);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feePerTrade, 0.06);
    
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedPnL(), -281.25);    
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxUnrealizedPnL(), 312.5);
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedDrawdown(), 593.75);    
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExLiqRem, 0.16);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExClearing, 0.17);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeBrokerage, 0.18);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feePerTrade, 0.13);
    
    // Test maxRealizedDrawdown
    //
    
    // Test strat's maxRealizedDrawdown
    //
    // Set tv 'high' and send new order
    //
    price = 130.984375;
    ticks = instrZNM2->_tc->fromExchangePrice(price);
    price = instrZNM2->_tc->toExchangePrice(ticks-2); // minus 2 ticks
    tv2.setTv(price);
    qty = 9;
    s2_b1 = OrderHelper::getBuyLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams2._id, instrZNM2);
    
    ASSERT_TRUE(p_out.sendNew(s2_b1, rej));
    ASSERT_EQ(p_orders.getAll().size(), 1);
    
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedPnL(), -150.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedPnL(), 281.25);    
    ASSERT_DOUBLE_EQ(accountPnl.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getMaxUnrealizedPnL(), 312.5);
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedDrawdown(), 150.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedDrawdown(), 31.25);    
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExLiqRem, 0.46);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExClearing, 0.52);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeBrokerage, 0.58);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feePerTrade, 0.19);
    
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -150.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedPnL(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 150.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedDrawdown(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExLiqRem, 0.30);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExClearing, 0.35);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeBrokerage, 0.40);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feePerTrade, 0.06);
    
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedPnL(), 281.25);    
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxUnrealizedPnL(), 312.5);
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedDrawdown(), 31.25);    
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExLiqRem, 0.16);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExClearing, 0.17);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeBrokerage, 0.18);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feePerTrade, 0.13);
    
    
    // Get partial fill for that order
    //
    price = 130.984375;
    fill2._order = s2_b1;
    fill2._orderId = s2_b1->_orderId;
    fill2._side = s2_b1->_side;
    fill2._qty.set(6);
    fill2._price = s2_b1->_price-6;  // 6 ticks lower then average for profit
    
    p_in.onFill(fill2);
    ASSERT_EQ(p_orders.getAll().size(), 1);
    ASSERT_EQ(fill2._posAccount, -3);
    ASSERT_EQ(fill2._posStrategy, -3);
    
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedPnL(), 412.5);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedPnL(), 93.75);    
    ASSERT_DOUBLE_EQ(accountPnl.getMaxRealizedPnL(), 412.5);
    ASSERT_DOUBLE_EQ(accountPnl.getMaxUnrealizedPnL(), 312.5);
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedDrawdown(), 218.75);    
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExLiqRem, 1.42);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExClearing, 1.54);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeBrokerage, 1.66);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feePerTrade, 0.32);
    
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -150.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedPnL(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 150.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedDrawdown(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExLiqRem, 0.30);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExClearing, 0.35);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeBrokerage, 0.40);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feePerTrade, 0.06);
    
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedPnL(), 562.5);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedPnL(), 93.75);    
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxRealizedPnL(), 562.5);
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxUnrealizedPnL(), 312.5);
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedDrawdown(), 218.75);    
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExLiqRem, 1.12);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExClearing, 1.19);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeBrokerage, 1.26);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feePerTrade, 0.26);
        
    // Get another partial fill for loss for that order
    //
    price = 130.984375;
    fill2._order = s2_b1;
    fill2._orderId = s2_b1->_orderId;
    fill2._side = s2_b1->_side;
    fill2._qty.set(2);
    fill2._price = s2_b1->_price+9;  // 9 ticks higher then average for loss
    
    p_in.onFill(fill2);
    ASSERT_EQ(p_orders.getAll().size(), 1);
    ASSERT_EQ(fill2._posAccount, -1);
    ASSERT_EQ(fill2._posStrategy, -1);
    
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedPnL(), 131.25);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedPnL(), 31.25);    
    ASSERT_DOUBLE_EQ(accountPnl.getMaxRealizedPnL(), 412.5);
    ASSERT_DOUBLE_EQ(accountPnl.getMaxUnrealizedPnL(), 312.5);
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedDrawdown(), 281.25);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedDrawdown(), 281.25);    
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExLiqRem, 1.74);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExClearing, 1.88);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeBrokerage, 2.02);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feePerTrade, 0.45);
    
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -150.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedPnL(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 150.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedDrawdown(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExLiqRem, 0.30);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExClearing, 0.35);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeBrokerage, 0.40);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feePerTrade, 0.06);
    
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedPnL(), 281.25);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedPnL(), 31.25);    
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxRealizedPnL(), 562.5);
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxUnrealizedPnL(), 312.5);
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedDrawdown(), 281.25);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedDrawdown(), 281.25);    
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExLiqRem, 1.44);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExClearing, 1.53);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeBrokerage, 1.62);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feePerTrade, 0.39);
    
    // Set tv 'low' Send new order, which will be rejected
    //    
    price = 130.984375;
    ticks = instrZNM2->_tc->fromExchangePrice(price);
    qty = 1;
    s2_a1 = OrderHelper::getSellLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams2._id, instrZNM2);
    
    ASSERT_TRUE(!p_out.sendNew(s2_a1, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorPnL);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kMaxRealizedDrawdownStrategy);
    ASSERT_EQ(rej._text, "286.23 :: 281.25");
    ASSERT_EQ(p_orders.getAll().size(), 1);
    
    // The other side order should be able to go through
    //
    s2_b2 = OrderHelper::getBuyLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams2._id, instrZNM2);
    ASSERT_TRUE(p_out.sendNew(s2_b2, rej));
    ASSERT_EQ(p_orders.getAll().size(), 2);
    p_in.onNewRej(s2_b2, rej);
    ASSERT_EQ(p_orders.getAll().size(), 1);
    
    
    // Get another partial fill for loss for that order, which fills that order completely
    //
    price = 130.984375;
    fill2._order = s2_b1;
    fill2._orderId = s2_b1->_orderId;
    fill2._side = s2_b1->_side;
    fill2._qty.set(1);
    fill2._price = s2_b1->_price+8;  // 8 ticks higher then average for loss
    
    p_in.onFill(fill2);
    ASSERT_EQ(p_orders.getAll().size(), 0);
    ASSERT_EQ(fill2._posAccount, 0);
    ASSERT_EQ(fill2._posStrategy, 0);
    
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedPnL(), 6.25);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedPnL(), 0.0);    
    ASSERT_DOUBLE_EQ(accountPnl.getMaxRealizedPnL(), 412.5);
    ASSERT_DOUBLE_EQ(accountPnl.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedDrawdown(), 406.25);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedDrawdown(), 0.0);    
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExLiqRem, 1.90);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeExClearing, 2.05);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feeBrokerage, 2.20);
    ASSERT_DOUBLE_EQ(accountPnl.getFeesPaid()._feePerTrade, 0.58);
    
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -150.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedPnL(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 150.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedDrawdown(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExLiqRem, 0.30);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeExClearing, 0.35);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feeBrokerage, 0.40);
    ASSERT_DOUBLE_EQ(stratPnL1.getFeesPaid()._feePerTrade, 0.06);
    
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedPnL(), 156.25);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedPnL(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxRealizedPnL(), 562.5);
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedDrawdown(), 406.25);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedDrawdown(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExLiqRem, 1.60);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeExClearing, 1.70);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feeBrokerage, 1.80);
    ASSERT_DOUBLE_EQ(stratPnL2.getFeesPaid()._feePerTrade, 0.52);
    
    
    // Acc's maxRealizedDrawdown was triggered, all orders should be rejected
    //
    price = 130.984375;
    ticks = instrZNM2->_tc->fromExchangePrice(price);
    qty = 1;
    s2_a1 = OrderHelper::getSellLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams2._id, instrZNM2);
    
    ASSERT_TRUE(!p_out.sendNew(s2_a1, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorPnL);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kMaxRealizedDrawdownAccount);
    ASSERT_EQ(rej._text, "412.98 :: 406.25");
    ASSERT_EQ(p_orders.getAll().size(), 0);
    
    // The other side order should be able to go through
    //
    s2_b2 = OrderHelper::getBuyLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams2._id, instrZNM2);
    ASSERT_TRUE(!p_out.sendNew(s2_b2, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorPnL);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kMaxRealizedDrawdownAccount);
    ASSERT_EQ(rej._text, "412.98 :: 406.25");
    ASSERT_EQ(p_orders.getAll().size(), 0);
    
}



TEST(ChannelOrLibTestSuit, processor_pnl_unrealizedPnLBug_account)
{
    // Init params used
    //
    tw::risk::Account accountParams;
    
    accountParams._id = 1;
    accountParams._name = "acc1";
    accountParams._maxRealizedLoss = 200.0;
    accountParams._maxUnrealizedLoss = 350.0;
    accountParams._maxRealizedDrawdown = 406.25;
    accountParams._maxUnrealizedDrawdown = 593.75;
    
    tw::risk::Strategy stratParams1;
    
    stratParams1._id = 11;
    stratParams1._name = "acc1_strat1";
    stratParams1._maxRealizedLoss = 150.0;
    stratParams1._maxUnrealizedLoss = 250.0;
    stratParams1._maxRealizedDrawdown = 300;
    stratParams1._maxUnrealizedDrawdown = 350;
    stratParams1._tradeEnabled = true;
    
    
    tw::risk::Strategy stratParams2;
    
    stratParams2._id = 12;
    stratParams2._name = "acc1_strat2";
    stratParams2._maxRealizedLoss = 140.0;
    stratParams2._maxUnrealizedLoss = 240.0;
    stratParams2._maxRealizedDrawdown = 281.25;
    stratParams2._maxUnrealizedDrawdown = 312.5;
    stratParams2._tradeEnabled = true;
    
    std::vector<tw::risk::Strategy> stratsParams;
    stratsParams.push_back(stratParams1);
    stratsParams.push_back(stratParams2);
    
    // Init processors
    //
    TProcessorOrders& p_orders = TProcessorOrders::instance();
    TProcessorPnL& p_pnl = TProcessorPnL::instance();
    
    p_orders.clear();
    p_pnl.clear();
    
    TProcessorOutPnL p_out_pnl(p_pnl);
    TProcessorOut p_out(p_orders, p_out_pnl);
    
    TProcessorInOrders p_in_orders(p_orders);
    TProcessorIn p_in(p_pnl, p_in_orders);
    
    // Structs for passing to processor
    //
    tw::instr::InstrumentPtr instrNQM2 = InstrHelper::getNQM2();
    tw::instr::InstrumentPtr instrZNM2 = InstrHelper::getZNM2();
    
    TOrderPtr s1_b1;    
    TOrderPtr s1_a1;
    
    TOrderPtr s2_b1;
    TOrderPtr s2_a1;
    
    Fill fill1;
    Fill fill2;
    Reject rej;
    
    double price = 0;
    uint32_t qty = 0;
    tw::price::Ticks ticks(0);
    
    // Fill1
    //
    fill1._type = tw::channel_or::eFillType::kNormal;
    fill1._subType = tw::channel_or::eFillSubType::kOutright;
    fill1._accountId = accountParams._id;
    fill1._strategyId = stratParams1._id;
    fill1._instrumentId = instrNQM2->_keyId;
    fill1._liqInd = tw::channel_or::eLiqInd::kRem;
    
    // Fill2
    //
    fill2._type = tw::channel_or::eFillType::kNormal;
    fill2._subType = tw::channel_or::eFillSubType::kOutright;
    fill2._accountId = accountParams._id;
    fill2._strategyId = stratParams2._id;
    fill2._instrumentId = instrZNM2->_keyId;
    fill2._liqInd = tw::channel_or::eLiqInd::kRem;    
    
    // Get tvs
    //
    tw::common_trade::WboManager::TTv& tv1 = tw::common_trade::WboManager::instance().getOrCreateTv(instrNQM2->_keyId);
    tw::common_trade::WboManager::TTv& tv2 = tw::common_trade::WboManager::instance().getOrCreateTv(instrZNM2->_keyId);
              
    // Initialize ProcessorPnL
    //
    ASSERT_TRUE(p_pnl.init(accountParams, stratsParams));
    
    // Get account and strats pnls
    //
    const tw::channel_or::TPnLComposite& accountPnl = p_pnl.getAccountPnL();
    const tw::channel_or::ProcessorPnL::TStratsPnL& stratsPnl = p_pnl.getStratsPnL();
    
    // Set TVs
    //
    
    // TV 1.5 tick above avgPrice for instrument 1
    //
    price = 274212.5;
    tv1.setTv(price);    
    
    // TV 1.5 tick above avgPrice for instrument 2
    //
    price = 131.0078125;
    tv2.setTv(price);
    
    // Test orders/fills
    //
    
    // Buy 5@274175 for instrument 1
    //
    price = 274175;
    ticks = instrNQM2->_tc->fromExchangePrice(price);    
    qty = 5;
    s1_b1 = OrderHelper::getBuyLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams1._id, instrNQM2);
    
    ASSERT_TRUE(p_out.sendNew(s1_b1, rej));    
    ASSERT_EQ(p_orders.getAll().size(), 1);
    
    p_in.onNewAck(s1_b1);
    ASSERT_EQ(p_orders.getAll().size(), 1);
    
    
    // Sell 5@130.984375 for instrument 2
    //
    price = 130.984375;
    ticks = instrZNM2->_tc->fromExchangePrice(price);    
    qty = 5;
    s2_a1 = OrderHelper::getSellLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams2._id, instrZNM2);
    
    ASSERT_TRUE(p_out.sendNew(s2_a1, rej));    
    ASSERT_EQ(p_orders.getAll().size(), 2);
    
    p_in.onNewAck(s2_a1);
    ASSERT_EQ(p_orders.getAll().size(), 2);
    
    // Get strategies' pnls
    //
    tw::channel_or::ProcessorPnL::TStratsPnL::const_iterator iter;
    
    iter = stratsPnl.find(stratParams1._id);
    ASSERT_TRUE(iter != stratsPnl.end());    
    const tw::channel_or::TPnLComposite& stratPnL1 = iter->second._stratPnL;
    
    iter = stratsPnl.find(stratParams2._id);
    ASSERT_TRUE(iter != stratsPnl.end());    
    const tw::channel_or::TPnLComposite& stratPnL2 = iter->second._stratPnL;
    
    
    // Fill buy 5@274175 for instrument 1
    //
    fill1._order = s1_b1;
    fill1._orderId = s1_b1->_orderId;
    fill1._side = s1_b1->_side;
    fill1._qty.set(5);
    fill1._price = s1_b1->_price;
    
    p_in.onFill(fill1);
    ASSERT_EQ(p_orders.getAll().size(), 1);
    
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedPnL(), 37.5);    
    ASSERT_DOUBLE_EQ(accountPnl.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getMaxUnrealizedPnL(), 37.5);
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedDrawdown(), 0.0);
    
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedPnL(), 37.5);    
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxUnrealizedPnL(), 37.5);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedDrawdown(), 0.0);
    
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedDrawdown(), 0.0);
    
    // Fill sell 5@274175 for instrument 2
    //
    fill2._order = s2_a1;
    fill2._orderId = s2_a1->_orderId;
    fill2._side = s2_a1->_side;
    fill2._qty.set(5);
    fill2._price = s2_a1->_price;
    
    p_in.onFill(fill2);
    ASSERT_EQ(p_orders.getAll().size(), 0);
    
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedPnL(), -79.6875);    
    ASSERT_DOUBLE_EQ(accountPnl.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getMaxUnrealizedPnL(), 37.5);
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedDrawdown(), 117.1875);
    
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedPnL(), 37.5);    
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxUnrealizedPnL(), 37.5);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedDrawdown(), 0.0);
    
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedPnL(), -117.1875);
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedDrawdown(), 117.1875);
    
    // Reverse positions
    //
    
    // Sell 5@274175 for instrument 1
    //
    price = 274175;
    ticks = instrNQM2->_tc->fromExchangePrice(price);    
    qty = 5;
    s1_a1 = OrderHelper::getSellLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams1._id, instrNQM2);
    
    ASSERT_TRUE(p_out.sendNew(s1_a1, rej));    
    ASSERT_EQ(p_orders.getAll().size(), 1);
    
    p_in.onNewAck(s1_a1);
    ASSERT_EQ(p_orders.getAll().size(), 1);
    
    
    // Buy 5@130.984375 for instrument 2
    //
    price = 130.984375;
    ticks = instrZNM2->_tc->fromExchangePrice(price);    
    qty = 5;
    s2_b1 = OrderHelper::getBuyLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams2._id, instrZNM2);
    
    ASSERT_TRUE(p_out.sendNew(s2_b1, rej));    
    ASSERT_EQ(p_orders.getAll().size(), 2);
    
    p_in.onNewAck(s2_b1);
    ASSERT_EQ(p_orders.getAll().size(), 2);
    
    
    
    // Fill sell 5@274175 for instrument 1
    //
    fill1._order = s1_a1;
    fill1._orderId = s1_a1->_orderId;
    fill1._side = s1_a1->_side;
    fill1._qty.set(5);
    fill1._price = s1_a1->_price;
    
    p_in.onFill(fill1);
    ASSERT_EQ(p_orders.getAll().size(), 1);
    
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedPnL(), -117.1875);    
    ASSERT_DOUBLE_EQ(accountPnl.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getMaxUnrealizedPnL(), 37.5);
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedDrawdown(), 154.6875);
    
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedPnL(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedDrawdown(), 0.0);
    
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedPnL(), -117.1875);
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedDrawdown(), 117.1875);
    
    // Fill buy 5@274175 for instrument 2
    //
    fill2._order = s2_b1;
    fill2._orderId = s2_b1->_orderId;
    fill2._side = s2_b1->_side;
    fill2._qty.set(5);
    fill2._price = s2_b1->_price;
    
    p_in.onFill(fill2);
    ASSERT_EQ(p_orders.getAll().size(), 0);
    
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedPnL(), 0.0);    
    ASSERT_DOUBLE_EQ(accountPnl.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedDrawdown(), 0.0);
    
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedPnL(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedDrawdown(), 0.0);
    
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL2.getUnrealizedDrawdown(), 0.0);
}


TEST(ChannelOrLibTestSuit, processor_pnl_unrealizedPnLBug_strat)
{
    // Init params used
    //
    tw::risk::Account accountParams;
    
    accountParams._id = 1;
    accountParams._name = "acc1";
    accountParams._maxRealizedLoss = 200.0;
    accountParams._maxUnrealizedLoss = 350.0;
    accountParams._maxRealizedDrawdown = 406.25;
    accountParams._maxUnrealizedDrawdown = 593.75;
    
    tw::risk::Strategy stratParams1;
    
    stratParams1._id = 11;
    stratParams1._name = "acc1_strat1";
    stratParams1._maxRealizedLoss = 150.0;
    stratParams1._maxUnrealizedLoss = 250.0;
    stratParams1._maxRealizedDrawdown = 300;
    stratParams1._maxUnrealizedDrawdown = 350;
    
    std::vector<tw::risk::Strategy> stratsParams;
    stratsParams.push_back(stratParams1);
    
    // Init processors
    //
    TProcessorOrders& p_orders = TProcessorOrders::instance();
    TProcessorPnL& p_pnl = TProcessorPnL::instance();
    
    p_orders.clear();
    p_pnl.clear();
    
    TProcessorOutPnL p_out_pnl(p_pnl);
    TProcessorOut p_out(p_orders, p_out_pnl);
    
    TProcessorInOrders p_in_orders(p_orders);
    TProcessorIn p_in(p_pnl, p_in_orders);
    
    // Structs for passing to processor
    //
    tw::instr::InstrumentPtr instrNQM2 = InstrHelper::getNQM2();
    tw::instr::InstrumentPtr instrZNM2 = InstrHelper::getZNM2();
    
    TOrderPtr s1_b1;    
    TOrderPtr s1_a1;
    
    TOrderPtr s2_b1;
    TOrderPtr s2_a1;
    
    Fill fill1;
    Fill fill2;
    Reject rej;
    
    double price = 0;
    uint32_t qty = 0;
    tw::price::Ticks ticks(0);
    
    // Fill1
    //
    fill1._type = tw::channel_or::eFillType::kNormal;
    fill1._subType = tw::channel_or::eFillSubType::kOutright;
    fill1._accountId = accountParams._id;
    fill1._strategyId = stratParams1._id;
    fill1._instrumentId = instrNQM2->_keyId;
    fill1._liqInd = tw::channel_or::eLiqInd::kRem;
    
    // Fill2
    //
    fill2._type = tw::channel_or::eFillType::kNormal;
    fill2._subType = tw::channel_or::eFillSubType::kOutright;
    fill2._accountId = accountParams._id;
    fill2._strategyId = stratParams1._id;
    fill2._instrumentId = instrZNM2->_keyId;
    fill2._liqInd = tw::channel_or::eLiqInd::kRem;    
    
    // Get tvs
    //
    tw::common_trade::WboManager::TTv& tv1 = tw::common_trade::WboManager::instance().getOrCreateTv(instrNQM2->_keyId);
    tw::common_trade::WboManager::TTv& tv2 = tw::common_trade::WboManager::instance().getOrCreateTv(instrZNM2->_keyId);
              
    // Initialize ProcessorPnL
    //
    ASSERT_TRUE(p_pnl.init(accountParams, stratsParams));
    
    // Get account and strats pnls
    //
    const tw::channel_or::TPnLComposite& accountPnl = p_pnl.getAccountPnL();
    const tw::channel_or::ProcessorPnL::TStratsPnL& stratsPnl = p_pnl.getStratsPnL();
    
    // Set TVs
    //
    
    // TV 1.5 tick above avgPrice for instrument 1
    //
    price = 274212.5;
    tv1.setTv(price);    
    
    // TV 1.5 tick above avgPrice for instrument 2
    //
    price = 131.0078125;
    tv2.setTv(price);
    
    // Test orders/fills
    //
    
    // Buy 5@274175 for instrument 1
    //
    price = 274175;
    ticks = instrNQM2->_tc->fromExchangePrice(price);    
    qty = 5;
    s1_b1 = OrderHelper::getBuyLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams1._id, instrNQM2);
    
    ASSERT_TRUE(p_out.sendNew(s1_b1, rej));    
    ASSERT_EQ(p_orders.getAll().size(), 1);
    
    p_in.onNewAck(s1_b1);
    ASSERT_EQ(p_orders.getAll().size(), 1);
    
    
    // Sell 5@130.984375 for instrument 2
    //
    price = 130.984375;
    ticks = instrZNM2->_tc->fromExchangePrice(price);    
    qty = 5;
    s2_a1 = OrderHelper::getSellLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams1._id, instrZNM2);
    
    ASSERT_TRUE(p_out.sendNew(s2_a1, rej));    
    ASSERT_EQ(p_orders.getAll().size(), 2);
    
    p_in.onNewAck(s2_a1);
    ASSERT_EQ(p_orders.getAll().size(), 2);
    
    // Get strategies' pnls
    //
    tw::channel_or::ProcessorPnL::TStratsPnL::const_iterator iter;
    
    iter = stratsPnl.find(stratParams1._id);
    ASSERT_TRUE(iter != stratsPnl.end());    
    const tw::channel_or::TPnLComposite& stratPnL1 = iter->second._stratPnL;
    
    
    // Fill buy 5@274175 for instrument 1
    //
    fill1._order = s1_b1;
    fill1._orderId = s1_b1->_orderId;
    fill1._side = s1_b1->_side;
    fill1._qty.set(5);
    fill1._price = s1_b1->_price;
    
    p_in.onFill(fill1);
    ASSERT_EQ(p_orders.getAll().size(), 1);
    
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedPnL(), 37.5);    
    ASSERT_DOUBLE_EQ(accountPnl.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getMaxUnrealizedPnL(), 37.5);
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedDrawdown(), 0.0);
    
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedPnL(), 37.5);    
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxUnrealizedPnL(), 37.5);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedDrawdown(), 0.0);
    
    // Fill sell 5@274175 for instrument 2
    //
    fill2._order = s2_a1;
    fill2._orderId = s2_a1->_orderId;
    fill2._side = s2_a1->_side;
    fill2._qty.set(5);
    fill2._price = s2_a1->_price;
    
    p_in.onFill(fill2);
    ASSERT_EQ(p_orders.getAll().size(), 0);
    
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedPnL(), -79.6875);    
    ASSERT_DOUBLE_EQ(accountPnl.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getMaxUnrealizedPnL(), 37.5);
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedDrawdown(), 117.1875);
    
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedPnL(), -79.6875);    
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxUnrealizedPnL(), 37.5);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedDrawdown(), 117.1875);
    
    // Reverse positions
    //
    
    // Sell 5@274175 for instrument 1
    //
    price = 274175;
    ticks = instrNQM2->_tc->fromExchangePrice(price);    
    qty = 5;
    s1_a1 = OrderHelper::getSellLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams1._id, instrNQM2);
    
    ASSERT_TRUE(p_out.sendNew(s1_a1, rej));    
    ASSERT_EQ(p_orders.getAll().size(), 1);
    
    p_in.onNewAck(s1_a1);
    ASSERT_EQ(p_orders.getAll().size(), 1);
    
    
    // Buy 5@130.984375 for instrument 2
    //
    price = 130.984375;
    ticks = instrZNM2->_tc->fromExchangePrice(price);    
    qty = 5;
    s2_b1 = OrderHelper::getBuyLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams1._id, instrZNM2);
    
    ASSERT_TRUE(p_out.sendNew(s2_b1, rej));    
    ASSERT_EQ(p_orders.getAll().size(), 2);
    
    p_in.onNewAck(s2_b1);
    ASSERT_EQ(p_orders.getAll().size(), 2);
    
    
    
    // Fill sell 5@274175 for instrument 1
    //
    fill1._order = s1_a1;
    fill1._orderId = s1_a1->_orderId;
    fill1._side = s1_a1->_side;
    fill1._qty.set(5);
    fill1._price = s1_a1->_price;
    
    p_in.onFill(fill1);
    ASSERT_EQ(p_orders.getAll().size(), 1);
    
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedPnL(), -117.1875);    
    ASSERT_DOUBLE_EQ(accountPnl.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getMaxUnrealizedPnL(), 37.5);
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedDrawdown(), 154.6875);
    
    
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedPnL(), -117.1875);    
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxUnrealizedPnL(), 37.5);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedDrawdown(), 154.6875);
    
    // Fill buy 5@274175 for instrument 2
    //
    fill2._order = s2_b1;
    fill2._orderId = s2_b1->_orderId;
    fill2._side = s2_b1->_side;
    fill2._qty.set(5);
    fill2._price = s2_b1->_price;
    
    p_in.onFill(fill2);
    ASSERT_EQ(p_orders.getAll().size(), 0);
    
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedPnL(), 0.0);    
    ASSERT_DOUBLE_EQ(accountPnl.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(accountPnl.getUnrealizedDrawdown(), 0.0);
    
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedPnL(), 0.0);    
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getUnrealizedDrawdown(), 0.0);
}

TEST(ChannelOrLibTestSuit, processor_pnl_printToStringBug)
{
    // Init params used
    //
    tw::risk::Account accountParams;
    
    accountParams._id = 1;
    accountParams._name = "acc1";
    accountParams._maxRealizedLoss = 200.0;
    accountParams._maxUnrealizedLoss = 350.0;
    accountParams._maxRealizedDrawdown = 406.25;
    accountParams._maxUnrealizedDrawdown = 593.75;
    
    tw::risk::Strategy stratParams1;
    
    stratParams1._id = 11;
    stratParams1._name = "acc1_strat1";
    stratParams1._maxRealizedLoss = 150.0;
    stratParams1._maxUnrealizedLoss = 250.0;
    stratParams1._maxRealizedDrawdown = 300;
    stratParams1._maxUnrealizedDrawdown = 350;
    stratParams1._tradeEnabled = true;
    
    std::vector<tw::risk::Strategy> stratsParams;
    stratsParams.push_back(stratParams1);    
   
    // Init processors
    //
    TProcessorOrders& p_orders = TProcessorOrders::instance();
    TProcessorPnL& p_pnl = TProcessorPnL::instance();
    
    p_orders.clear();
    p_pnl.clear();
    
    TProcessorOutPnL p_out_pnl(p_pnl);
    TProcessorOut p_out(p_orders, p_out_pnl);
    
    TProcessorInOrders p_in_orders(p_orders);
    TProcessorIn p_in(p_pnl, p_in_orders);
    
    // Structs for passing to processor
    //
    tw::instr::InstrumentPtr instrNQM2 = InstrHelper::getNQM2(); 
    
    TOrderPtr b;        
    Reject rej;
    
    double price = 0;
    uint32_t qty = 0;
    tw::price::Ticks ticks(0);
    std::string temp;
              
    // Initialize ProcessorPnL
    //
    ASSERT_TRUE(p_pnl.init(accountParams, stratsParams));
    
    // Set tv to invalid
    //
    tw::common_trade::WboManager::TTv& tv = tw::common_trade::WboManager::instance().getOrCreateTv(instrNQM2->_keyId);
    tv.clear();
    ASSERT_TRUE(!tv.isValid());
    
    // Test printToString() before and after placing an order
    //
    ASSERT_TRUE(p_pnl.printToString(0, temp));
    ASSERT_TRUE(p_pnl.printToString(stratParams1._id, temp));
    
    // Buy 5@274175 for instrument 1
    //
    price = 274175;
    ticks = instrNQM2->_tc->fromExchangePrice(price);    
    qty = 5;
    b = OrderHelper::getBuyLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams1._id, instrNQM2);
    
    ASSERT_TRUE(p_out.sendNew(b, rej));    
    ASSERT_EQ(p_orders.getAll().size(), 1);
    
    ASSERT_TRUE(p_pnl.printToString(0, temp));
    ASSERT_TRUE(p_pnl.printToString(stratParams1._id, temp));
    
    p_in.onNewRej(b, rej);
    ASSERT_EQ(p_orders.getAll().size(), 0);
    
    ASSERT_TRUE(p_pnl.printToString(0, temp));
    ASSERT_TRUE(p_pnl.printToString(stratParams1._id, temp));
    
    // Initialize tv
    //
    tv.setInstrument(instrNQM2);
    tv.setTv(price);
    
    // Buy 5@274175 for instrument 1
    //
    price = 274175;
    ticks = instrNQM2->_tc->fromExchangePrice(price);    
    qty = 5;
    b = OrderHelper::getBuyLimitForAccountStrategy(ticks.get(), qty, accountParams._id, stratParams1._id, instrNQM2);
    
    ASSERT_TRUE(p_out.sendNew(b, rej));    
    ASSERT_EQ(p_orders.getAll().size(), 1);
    
    ASSERT_TRUE(p_pnl.printToString(0, temp));
    ASSERT_TRUE(p_pnl.printToString(stratParams1._id, temp));
    
    p_in.onNewRej(b, rej);
    ASSERT_EQ(p_orders.getAll().size(), 0);
    
    ASSERT_TRUE(p_pnl.printToString(0, temp));
    ASSERT_TRUE(p_pnl.printToString(stratParams1._id, temp));

}


TEST(ChannelOrLibTestSuit, processor_pnl_realizedPnLBug_strat)
{
    // Init params used
    //
    tw::risk::Account accountParams;
    
    accountParams._id = 15;
    accountParams._name = "tw_research";
    accountParams._maxRealizedLoss = 10000000.0;
    accountParams._maxUnrealizedLoss = 10000000.0;
    accountParams._maxRealizedDrawdown = 10000000.0;
    accountParams._maxUnrealizedDrawdown = 10000000.0;
    accountParams._tradeEnabled = true;
    
    tw::risk::Strategy stratParams1;
    
    stratParams1._id = 175;
    stratParams1._name = "6CU2_6SU2";
    stratParams1._accountId = 15;
    stratParams1._maxRealizedLoss = 301.0;
    stratParams1._maxUnrealizedLoss = 501.0;
    stratParams1._maxRealizedDrawdown = 301;
    stratParams1._maxUnrealizedDrawdown = 601;
    
    std::vector<tw::risk::Strategy> stratsParams;
    stratsParams.push_back(stratParams1);
    
    // Init processors
    //    
    TProcessorPnL& p_pnl = TProcessorPnL::instance();        
    p_pnl.clear();
    
    // Structs for passing to processor
    //
    tw::instr::InstrumentPtr instr6CU2 = InstrHelper::get6CU2();
    tw::instr::InstrumentPtr instr6SU2 = InstrHelper::get6SU2();
    
    Fill fill1;
    Fill fill2;
    Reject rej;        
    
    // Fill1
    //
    fill1._type = tw::channel_or::eFillType::kNormal;
    fill1._subType = tw::channel_or::eFillSubType::kOutright;
    fill1._accountId = accountParams._id;
    fill1._strategyId = stratParams1._id;
    fill1._instrumentId = instr6CU2->_keyId;
    fill1._liqInd = tw::channel_or::eLiqInd::kAdd;
    
    // Fill2
    //
    fill2._type = tw::channel_or::eFillType::kNormal;
    fill2._subType = tw::channel_or::eFillSubType::kOutright;
    fill2._accountId = accountParams._id;
    fill2._strategyId = stratParams1._id;
    fill2._instrumentId = instr6SU2->_keyId;
    fill2._liqInd = tw::channel_or::eLiqInd::kAdd;    
              
    // Initialize ProcessorPnL
    //
    ASSERT_TRUE(p_pnl.init(accountParams, stratsParams));
    
    
    // Fill sell 1@10248 for instrument 2
    //
    fill2._side = tw::channel_or::eOrderSide::kSell;
    fill2._qty.set(1);
    fill2._price.set(10248);
    
    p_pnl.onFill(fill2);
    
    // Get strategies' pnls
    //
    const tw::channel_or::ProcessorPnL::TStratsPnL& stratsPnl = p_pnl.getStratsPnL();
    tw::channel_or::ProcessorPnL::TStratsPnL::const_iterator iter;
    
    iter = stratsPnl.find(stratParams1._id);
    ASSERT_TRUE(iter != stratsPnl.end());    
    const tw::channel_or::TPnLComposite& stratPnL1 = iter->second._stratPnL;
    
    // Check PnL
    //
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 0.0);
    
    
    // Fill buy 1@10249 for instrument 2
    //
    fill2._side = tw::channel_or::eOrderSide::kBuy;
    fill2._qty.set(1);
    fill2._price.set(10249);    
    
    p_pnl.onFill(fill2);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -12.5);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 12.5);
    
    
    // Fill buy 1@10247 for instrument 2
    //
    fill2._side = tw::channel_or::eOrderSide::kBuy;
    fill2._qty.set(1);
    fill2._price.set(10247);    
    
    p_pnl.onFill(fill2);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -12.5);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 12.5);
    
    // Fill sell 1@9908 for instrument 1
    //
    fill1._side = tw::channel_or::eOrderSide::kSell;
    fill1._qty.set(1);
    fill1._price.set(9908);    
    
    p_pnl.onFill(fill1);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -12.5);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 12.5);
    
    
    // Fill buy 1@10242 for instrument 2
    //
    fill2._side = tw::channel_or::eOrderSide::kBuy;
    fill2._qty.set(1);
    fill2._price.set(10242);    
    
    p_pnl.onFill(fill2);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -12.5);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 12.5);
    
    // Fill buy 1@9907 for instrument 1
    //
    fill1._side = tw::channel_or::eOrderSide::kBuy;
    fill1._qty.set(1);
    fill1._price.set(9907);    
    
    p_pnl.onFill(fill1);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -2.5);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 2.5);
    
    
    // Fill sell 1@10249 for instrument 2
    //
    fill2._side = tw::channel_or::eOrderSide::kSell;
    fill2._qty.set(2);
    fill2._price.set(10241);    
    
    p_pnl.onFill(fill2);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -90.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 90.0);
    
    
    // Fill sell 1@9909 for instrument 1
    //
    fill1._side = tw::channel_or::eOrderSide::kSell;
    fill1._qty.set(1);
    fill1._price.set(9909);    
    
    p_pnl.onFill(fill1);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -90.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 90.0);
    
    // Fill sell 1@9916 for instrument 1
    //
    fill1._side = tw::channel_or::eOrderSide::kSell;
    fill1._qty.set(1);
    fill1._price.set(9916);    
    
    p_pnl.onFill(fill1);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -90.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 90.0);
    
    // Fill buy 1@9915 for instrument 1
    //
    fill1._side = tw::channel_or::eOrderSide::kBuy;
    fill1._qty.set(1);
    fill1._price.set(9915);
    
    p_pnl.onFill(fill1);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -115.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 115.0);
 
    // Fill buy 1@10242 for instrument 2
    //
    fill2._side = tw::channel_or::eOrderSide::kBuy;
    fill2._qty.set(1);
    fill2._price.set(10242);    
    
    p_pnl.onFill(fill2);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -115.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 115.0);
    
    // Fill sell 1@10244 for instrument 2
    //
    fill2._side = tw::channel_or::eOrderSide::kSell;
    fill2._qty.set(1);
    fill2._price.set(10244);    
    
    p_pnl.onFill(fill2);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -90.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 90.0);
    
    // Fill sell 1@10254 for instrument 2
    //
    fill2._side = tw::channel_or::eOrderSide::kSell;
    fill2._qty.set(1);
    fill2._price.set(10254);    
    
    p_pnl.onFill(fill2);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -90.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 90.0);
    
    
    // Fill buy 1@10252 for instrument 2
    //
    fill2._side = tw::channel_or::eOrderSide::kBuy;
    fill2._qty.set(1);
    fill2._price.set(10252);    
    
    p_pnl.onFill(fill2);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -65.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 65.0);
    
    
    // Fill sell 1@10251 for instrument 2
    //
    fill2._side = tw::channel_or::eOrderSide::kSell;
    fill2._qty.set(1);
    fill2._price.set(10251);    
    
    p_pnl.onFill(fill2);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -65.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 65.0);
 
    
    // Fill buy 1@9914 for instrument 1
    //
    fill1._side = tw::channel_or::eOrderSide::kBuy;
    fill1._qty.set(1);
    fill1._price.set(9914);
    
    p_pnl.onFill(fill1);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -80.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 80.0);
    
    
    // Fill sell 1@9914 for instrument 1
    //
    fill1._side = tw::channel_or::eOrderSide::kSell;
    fill1._qty.set(1);
    fill1._price.set(9914);
    
    p_pnl.onFill(fill1);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -80.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 80.0);
    
    
    // Fill buy 1@10251 for instrument 2
    //
    fill2._side = tw::channel_or::eOrderSide::kBuy;
    fill2._qty.set(1);
    fill2._price.set(10251);    
    
    p_pnl.onFill(fill2);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -80.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 80.0);
    
    
    // Fill buy 1@10246 for instrument 2
    //
    fill2._side = tw::channel_or::eOrderSide::kBuy;
    fill2._qty.set(1);
    fill2._price.set(10246);    
    
    p_pnl.onFill(fill2);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -80.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 80.0);
    
    
    // Fill sell 1@10246 for instrument 2
    //
    fill2._side = tw::channel_or::eOrderSide::kSell;
    fill2._qty.set(1);
    fill2._price.set(10246);    
    
    p_pnl.onFill(fill2);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -80.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 80.0);
    
    
    // Fill buy 1@10249 for instrument 2
    //
    fill2._side = tw::channel_or::eOrderSide::kBuy;
    fill2._qty.set(1);
    fill2._price.set(10249);    
    
    p_pnl.onFill(fill2);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -80.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 80.0);
    
    
    // Fill sell 1@10239 for instrument 2
    //
    fill2._side = tw::channel_or::eOrderSide::kSell;
    fill2._qty.set(1);
    fill2._price.set(10239);    
    
    p_pnl.onFill(fill2);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -205.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 205.0);
    
    
    // Fill buy 1@9916 for instrument 1
    //
    fill1._side = tw::channel_or::eOrderSide::kBuy;
    fill1._qty.set(1);
    fill1._price.set(9916);
    
    p_pnl.onFill(fill1);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -225.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 225.0);
    
    
    // Fill buy 1@10230 for instrument 2
    //
    fill2._side = tw::channel_or::eOrderSide::kBuy;
    fill2._qty.set(1);
    fill2._price.set(10230);    
    
    p_pnl.onFill(fill2);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -225.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 225.0);
    
    // Fill sell 1@9915 for instrument 1
    //
    fill1._side = tw::channel_or::eOrderSide::kSell;
    fill1._qty.set(1);
    fill1._price.set(9915);
    
    p_pnl.onFill(fill1);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -225.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 225.0);
    
    // Fill sell 1@10232 for instrument 2
    //
    fill2._side = tw::channel_or::eOrderSide::kSell;
    fill2._qty.set(1);
    fill2._price.set(10232);    
    
    p_pnl.onFill(fill2);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -200.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 200.0);
    
    // Fill buy 1@9915 for instrument 1
    //
    fill1._side = tw::channel_or::eOrderSide::kBuy;
    fill1._qty.set(1);
    fill1._price.set(9915);
    
    p_pnl.onFill(fill1);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -200.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 200.0);
    
    
    // Fill sell 1@10233 for instrument 2
    //
    fill2._side = tw::channel_or::eOrderSide::kSell;
    fill2._qty.set(1);
    fill2._price.set(10233);    
    
    p_pnl.onFill(fill2);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -200.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 200.0);
    
    // Fill buy 1@10231 for instrument 2
    //
    fill2._side = tw::channel_or::eOrderSide::kBuy;
    fill2._qty.set(1);
    fill2._price.set(10231);    
    
    p_pnl.onFill(fill2);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -175.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 175.0);
    
    
    // Fill buy 1@9912 for instrument 1
    //
    fill1._side = tw::channel_or::eOrderSide::kBuy;
    fill1._qty.set(1);
    fill1._price.set(9912);
    
    p_pnl.onFill(fill1);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -175.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 175.0);
    
    
    // Fill sell 1@10231 for instrument 2
    //
    fill2._side = tw::channel_or::eOrderSide::kSell;
    fill2._qty.set(1);
    fill2._price.set(10231);    
    
    p_pnl.onFill(fill2);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -175.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 175.0);
    
    // Fill buy 1@10230 for instrument 2
    //
    fill2._side = tw::channel_or::eOrderSide::kBuy;
    fill2._qty.set(1);
    fill2._price.set(10230);    
    
    p_pnl.onFill(fill2);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -162.5);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 162.5);
    
    // Fill sell 1@9910 for instrument 1
    //
    fill1._side = tw::channel_or::eOrderSide::kSell;
    fill1._qty.set(1);
    fill1._price.set(9910);
    
    p_pnl.onFill(fill1);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -182.5);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 182.5);
    
    
    // Fill buy 1@9908 for instrument 1
    //
    fill1._side = tw::channel_or::eOrderSide::kBuy;
    fill1._qty.set(1);
    fill1._price.set(9908);
    
    p_pnl.onFill(fill1);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -182.5);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 182.5);
    
    
    // Fill buy 1@10215 for instrument 2
    //
    fill2._side = tw::channel_or::eOrderSide::kBuy;
    fill2._qty.set(1);
    fill2._price.set(10215);    
    
    p_pnl.onFill(fill2);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -182.5);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 182.5);
    
    
    // Fill sell 1@10217 for instrument 2
    //
    fill2._side = tw::channel_or::eOrderSide::kSell;
    fill2._qty.set(1);
    fill2._price.set(10217);    
    
    p_pnl.onFill(fill2);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedPnL(), -157.5);
    ASSERT_DOUBLE_EQ(stratPnL1.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(stratPnL1.getRealizedDrawdown(), 157.5);
}
