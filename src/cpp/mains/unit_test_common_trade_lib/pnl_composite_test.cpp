#include <tw/common_trade/pnl.h>
#include <tw/common_trade/pnlComposite.h>
#include <tw/generated/channel_or_defs.h>
#include "../unit_test_price_lib/instr_helper.h"

#include <gtest/gtest.h>

static const double PRICE_EPSILON = 0.005;

class TestTv : public tw::common_trade::ITv {
    typedef tw::common_trade::ITv Parent;
public:
    TestTv() {
        clear();
    }
    
    void clear() {
        Parent::clear();
    }
    
    virtual bool setInstrument(tw::instr::InstrumentConstPtr instrument) {
        _instrument = instrument;
	return true;
    }
    
    void operator=(double v) {
        _tv = v;
        _isTvSet = true;
    }
};

TEST(CommonTradeLibTestSuit, PnLComposite_Leafs)
{   
    tw::instr::InstrumentPtr instrument1 = InstrHelper::getNQM2();
    tw::instr::InstrumentPtr instrument2 = InstrHelper::getZNM2();
    
    tw::channel_or::Fill fill1;
    fill1._instrumentId = instrument1->_keyId;
    
    tw::channel_or::Fill fill2;
    fill2._instrumentId = instrument2->_keyId;
    
    TestTv tv1;
    tv1.setInstrument(instrument1);
    
    TestTv tv2;
    tv2.setInstrument(instrument2);
    
    tw::common_trade::PnL pnl1;
    tw::common_trade::PnL pnl2;
    
    pnl1.setInstrument(instrument1->_keyId);
    pnl2.setInstrument(instrument2->_keyId);
    
    tw::common_trade::PnLComposite pnl12;
    const tw::instr::Fees& fees = pnl12.getFeesPaid();
    
    double price = 0.0;
    
    // Check initial values in composite
    //    
    ASSERT_TRUE(!pnl12.isValid());
    
    pnl12.addComponent(&pnl1);
    pnl12.addComponent(&pnl2);
    
    ASSERT_TRUE(pnl12.isValid());
    
    ASSERT_DOUBLE_EQ(pnl12.getPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getUnrealizedPnL(), 0.0);
    
    ASSERT_DOUBLE_EQ(pnl12.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getUnrealizedDrawdown(), 0.0);
    ASSERT_EQ(pnl12.getPosition(), 0);
    
    ASSERT_DOUBLE_EQ(fees._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(fees._feeExLiqRem, 0.0);
    ASSERT_DOUBLE_EQ(fees._feeExClearing, 0.0);
    ASSERT_DOUBLE_EQ(fees._feeBrokerage, 0.0);
    ASSERT_DOUBLE_EQ(fees._feePerTrade, 0.0);
  
    // Buy 5@274175 for instrument 1 - add liq - pos: 5
    //
    price = 274175;
    fill1._side = tw::channel_or::eOrderSide::kBuy;
    fill1._liqInd = tw::channel_or::eLiqInd::kAdd;
    fill1._qty.set(5);
    fill1._price = instrument1->_tc->fromExchangePrice(price);
    pnl12.onFill(fill1);
    
    ASSERT_TRUE(pnl12.isValid());
    
    ASSERT_DOUBLE_EQ(pnl12.getPnL(), 0.0);    
    ASSERT_DOUBLE_EQ(pnl12.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getUnrealizedPnL(), 0.0);
    
    ASSERT_DOUBLE_EQ(pnl12.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getUnrealizedDrawdown(), 0.0);
    ASSERT_EQ(pnl12.getPosition(), 5);
    
    ASSERT_DOUBLE_EQ(fees._feeExLiqAdd, -0.25);
    ASSERT_DOUBLE_EQ(fees._feeExLiqRem, 0.0);
    ASSERT_DOUBLE_EQ(fees._feeExClearing, 0.35);
    ASSERT_DOUBLE_EQ(fees._feeBrokerage, 0.40);
    ASSERT_DOUBLE_EQ(fees._feePerTrade, 0.03);
    
    
    // TV 1.5 tick above avgPrice for instrument 1
    //
    tv1 = 274212.5;    
    pnl12.onTv(tv1);
    
    ASSERT_DOUBLE_EQ(pnl12.getPnL(), 37.5);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getUnrealizedPnL(), 37.5);
    
    ASSERT_DOUBLE_EQ(pnl12.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getMaxUnrealizedPnL(), 37.5);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getUnrealizedDrawdown(), 0.0);
    ASSERT_EQ(pnl12.getPosition(), 5);
    
    // TV 2.3 tick below avgPrice for instrument 1
    //
    tv1 = 274117.5;    
    pnl12.onTv(tv1);    
    
    ASSERT_DOUBLE_EQ(pnl12.getPnL(), -57.5);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getUnrealizedPnL(), -57.5);
    
    ASSERT_DOUBLE_EQ(pnl12.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getMaxUnrealizedPnL(), 37.5);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getUnrealizedDrawdown(), 95.0);
    ASSERT_EQ(pnl12.getPosition(), 5);
    
    // Buy 10@274150 for instrument 1 - rem liq - pos: 15
    //
    price = 274150;
    fill1._side = tw::channel_or::eOrderSide::kBuy;
    fill1._liqInd = tw::channel_or::eLiqInd::kRem;
    fill1._qty.set(10);
    fill1._price = instrument1->_tc->fromExchangePrice(price);
    pnl12.onFill(fill1);
    
    ASSERT_NEAR(pnl12.getPnL(), -122.5, PRICE_EPSILON);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedPnL(), 0.0);
    ASSERT_NEAR(pnl12.getUnrealizedPnL(), -122.5, PRICE_EPSILON);
    
    ASSERT_DOUBLE_EQ(pnl12.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getMaxUnrealizedPnL(), 37.5);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedDrawdown(), 0.0);
    ASSERT_NEAR(pnl12.getUnrealizedDrawdown(), 160.0, PRICE_EPSILON);
    ASSERT_EQ(pnl12.getPosition(), 15);
    
    ASSERT_DOUBLE_EQ(fees._feeExLiqAdd, -0.25);
    ASSERT_DOUBLE_EQ(fees._feeExLiqRem, 0.60);
    ASSERT_DOUBLE_EQ(fees._feeExClearing, 1.05);
    ASSERT_DOUBLE_EQ(fees._feeBrokerage, 1.20);
    ASSERT_DOUBLE_EQ(fees._feePerTrade, 0.06);
    
    // Buy 5@130.984375 for instrument 2 - add liq - pos: 5
    //
    price = 130.984375;
    fill2._side = tw::channel_or::eOrderSide::kBuy;
    fill2._liqInd = tw::channel_or::eLiqInd::kAdd;
    fill2._qty.set(5);
    fill2._price = instrument2->_tc->fromExchangePrice(price);
    pnl12.onFill(fill2);
    
    ASSERT_NEAR(pnl12.getPnL(), -122.5, PRICE_EPSILON);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedPnL(), 0.0);
    ASSERT_NEAR(pnl12.getUnrealizedPnL(), -122.5, PRICE_EPSILON);
    
    ASSERT_DOUBLE_EQ(pnl12.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getMaxUnrealizedPnL(), 37.5);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedDrawdown(), 0.0);
    ASSERT_NEAR(pnl12.getUnrealizedDrawdown(), 160.0, PRICE_EPSILON);
    ASSERT_EQ(pnl12.getPosition(), 20);
    
    ASSERT_DOUBLE_EQ(fees._feeExLiqAdd, -1.00);
    ASSERT_DOUBLE_EQ(fees._feeExLiqRem, 0.60);
    ASSERT_DOUBLE_EQ(fees._feeExClearing, 1.90);
    ASSERT_DOUBLE_EQ(fees._feeBrokerage, 2.10);
    ASSERT_DOUBLE_EQ(fees._feePerTrade, 0.19);
    
    // TV 1.5 tick above avgPrice
    //
    tv2 = 131.0078125;
    pnl12.onTv(tv2);
    
    ASSERT_NEAR(pnl12.getPnL(), -5.3125, PRICE_EPSILON);    
    ASSERT_NEAR(pnl12.getRealizedPnL(), 0.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl12.getUnrealizedPnL(), -5.3125, PRICE_EPSILON);
    
    ASSERT_DOUBLE_EQ(pnl12.getMaxRealizedPnL(), 0.0);
    ASSERT_NEAR(pnl12.getMaxUnrealizedPnL(), 37.5, PRICE_EPSILON);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedDrawdown(), 0.0);
    ASSERT_NEAR(pnl12.getUnrealizedDrawdown(), 42.8125, PRICE_EPSILON);
    ASSERT_EQ(pnl12.getPosition(), 20);
    
    // TV 2.3 tick below avgPrice
    //
    tv2 = 130.9484375;
    pnl12.onTv(tv2);
    
    ASSERT_NEAR(pnl12.getPnL(), -302.1875, PRICE_EPSILON);
    ASSERT_NEAR(pnl12.getRealizedPnL(), 0.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl12.getUnrealizedPnL(), -302.1875, PRICE_EPSILON);
    
    ASSERT_DOUBLE_EQ(pnl12.getMaxRealizedPnL(), 0.0);
    ASSERT_NEAR(pnl12.getMaxUnrealizedPnL(), 37.5, PRICE_EPSILON);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedDrawdown(), 0.0);
    ASSERT_NEAR(pnl12.getUnrealizedDrawdown(), 339.6875, PRICE_EPSILON);
    ASSERT_EQ(pnl12.getPosition(), 20);
    
    // Buy 10@130.96875 for instrument 2 - rem liq - pos: 15
    //
    price = 130.96875;
    fill2._side = tw::channel_or::eOrderSide::kBuy;
    fill2._liqInd = tw::channel_or::eLiqInd::kRem;
    fill2._qty.set(10);
    fill2._price = instrument2->_tc->fromExchangePrice(price);
    pnl12.onFill(fill2);
    
    ASSERT_NEAR(pnl12.getPnL(), -505.3125, PRICE_EPSILON);    
    ASSERT_NEAR(pnl12.getRealizedPnL(), 0.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl12.getUnrealizedPnL(), -505.3125, PRICE_EPSILON);
    
    ASSERT_DOUBLE_EQ(pnl12.getMaxRealizedPnL(), 0.0);
    ASSERT_NEAR(pnl12.getMaxUnrealizedPnL(), 37.5, PRICE_EPSILON);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedDrawdown(), 0.0);
    ASSERT_NEAR(pnl12.getUnrealizedDrawdown(), 542.8125, PRICE_EPSILON);
    ASSERT_EQ(pnl12.getPosition(), 30);
    
    ASSERT_DOUBLE_EQ(fees._feeExLiqAdd, -1.00);
    ASSERT_DOUBLE_EQ(fees._feeExLiqRem, 2.20);
    ASSERT_DOUBLE_EQ(fees._feeExClearing, 3.60);
    ASSERT_DOUBLE_EQ(fees._feeBrokerage, 3.90);
    ASSERT_DOUBLE_EQ(fees._feePerTrade, 0.32);
}

TEST(CommonTradeLibTestSuit, PnLComposite_Composite)
{   
    tw::instr::InstrumentPtr instrument1 = InstrHelper::getNQM2();
    tw::instr::InstrumentPtr instrument2 = InstrHelper::getZNM2();
    
    tw::channel_or::Fill fill1;
    fill1._instrumentId = instrument1->_keyId;
    
    tw::channel_or::Fill fill2;
    fill2._instrumentId = instrument2->_keyId;
    
    TestTv tv1;
    tv1.setInstrument(instrument1);
    
    TestTv tv2;
    tv2.setInstrument(instrument2);
    
    tw::common_trade::PnL pnl1;
    tw::common_trade::PnL pnl2;
    
    pnl1.setInstrument(instrument1->_keyId);
    pnl2.setInstrument(instrument2->_keyId);
    
    tw::common_trade::PnLComposite pnl11;
    tw::common_trade::PnLComposite pnl21;
    
    pnl11.addComponent(&pnl1);
    pnl21.addComponent(&pnl2);
    
    tw::common_trade::PnLComposite pnl12;
    const tw::instr::Fees& fees = pnl12.getFeesPaid();
    
    double price = 0.0;
    
    // Check initial values in composite
    //    
    ASSERT_TRUE(!pnl12.isValid());
    
    // Do the setup components/observers
    //        
    pnl11.setObserver(&pnl12);
    pnl21.setObserver(&pnl12);
    
    // Check initial values
    //
    ASSERT_DOUBLE_EQ(pnl12.getPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getUnrealizedPnL(), 0.0);
    
    ASSERT_DOUBLE_EQ(pnl12.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getUnrealizedDrawdown(), 0.0);
    ASSERT_EQ(pnl12.getPosition(), 0);
    
    ASSERT_DOUBLE_EQ(fees._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(fees._feeExLiqRem, 0.0);
    ASSERT_DOUBLE_EQ(fees._feeExClearing, 0.0);
    ASSERT_DOUBLE_EQ(fees._feeBrokerage, 0.0);
    ASSERT_DOUBLE_EQ(fees._feePerTrade, 0.0);
  
    // Buy 5@274175 for instrument 1 - add liq - pos: 5
    //
    price = 274175;
    fill1._side = tw::channel_or::eOrderSide::kBuy;
    fill1._liqInd = tw::channel_or::eLiqInd::kAdd;
    fill1._qty.set(5);
    fill1._price = instrument1->_tc->fromExchangePrice(price);
    pnl11.onFill(fill1);    
    
    
    ASSERT_DOUBLE_EQ(pnl12.getPnL(), 0.0);    
    ASSERT_DOUBLE_EQ(pnl12.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getUnrealizedPnL(), 0.0);
    
    ASSERT_DOUBLE_EQ(pnl12.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getUnrealizedDrawdown(), 0.0);
    ASSERT_EQ(pnl12.getPosition(), 5);
    
    ASSERT_DOUBLE_EQ(fees._feeExLiqAdd, -0.25);
    ASSERT_DOUBLE_EQ(fees._feeExLiqRem, 0.0);
    ASSERT_DOUBLE_EQ(fees._feeExClearing, 0.35);
    ASSERT_DOUBLE_EQ(fees._feeBrokerage, 0.40);
    ASSERT_DOUBLE_EQ(fees._feePerTrade, 0.03);
    
    
    // TV 1.5 tick above avgPrice for instrument 1
    //
    tv1 = 274212.5;    
    pnl11.onTv(tv1);
    
    ASSERT_DOUBLE_EQ(pnl12.getPnL(), 37.5);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getUnrealizedPnL(), 37.5);
    
    ASSERT_DOUBLE_EQ(pnl12.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getMaxUnrealizedPnL(), 37.5);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getUnrealizedDrawdown(), 0.0);
    ASSERT_EQ(pnl12.getPosition(), 5);
    
    // TV 2.3 tick below avgPrice for instrument 1
    //
    tv1 = 274117.5;    
    pnl11.onTv(tv1);    
    
    ASSERT_DOUBLE_EQ(pnl12.getPnL(), -57.5);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getUnrealizedPnL(), -57.5);
    
    ASSERT_DOUBLE_EQ(pnl12.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getMaxUnrealizedPnL(), 37.5);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getUnrealizedDrawdown(), 95.0);
    ASSERT_EQ(pnl12.getPosition(), 5);
    
    // Buy 10@274150 for instrument 1 - rem liq - pos: 15
    //
    price = 274150;
    fill1._side = tw::channel_or::eOrderSide::kBuy;
    fill1._liqInd = tw::channel_or::eLiqInd::kRem;
    fill1._qty.set(10);
    fill1._price = instrument1->_tc->fromExchangePrice(price);
    pnl11.onFill(fill1);
    
    ASSERT_NEAR(pnl12.getPnL(), -122.5, PRICE_EPSILON);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedPnL(), 0.0);
    ASSERT_NEAR(pnl12.getUnrealizedPnL(), -122.5, PRICE_EPSILON);
    
    ASSERT_DOUBLE_EQ(pnl12.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getMaxUnrealizedPnL(), 37.5);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedDrawdown(), 0.0);
    ASSERT_NEAR(pnl12.getUnrealizedDrawdown(), 160.0, PRICE_EPSILON);
    ASSERT_EQ(pnl12.getPosition(), 15);
    
    ASSERT_DOUBLE_EQ(fees._feeExLiqAdd, -0.25);
    ASSERT_DOUBLE_EQ(fees._feeExLiqRem, 0.60);
    ASSERT_DOUBLE_EQ(fees._feeExClearing, 1.05);
    ASSERT_DOUBLE_EQ(fees._feeBrokerage, 1.20);
    ASSERT_DOUBLE_EQ(fees._feePerTrade, 0.06);
    
    // Buy 5@130.984375 for instrument 2 - add liq - pos: 5
    //
    price = 130.984375;
    fill2._side = tw::channel_or::eOrderSide::kBuy;
    fill2._liqInd = tw::channel_or::eLiqInd::kAdd;
    fill2._qty.set(5);
    fill2._price = instrument2->_tc->fromExchangePrice(price);
    pnl21.onFill(fill2);
    
    ASSERT_NEAR(pnl12.getPnL(), -122.5, PRICE_EPSILON);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedPnL(), 0.0);
    ASSERT_NEAR(pnl12.getUnrealizedPnL(), -122.5, PRICE_EPSILON);
    
    ASSERT_DOUBLE_EQ(pnl12.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getMaxUnrealizedPnL(), 37.5);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedDrawdown(), 0.0);
    ASSERT_NEAR(pnl12.getUnrealizedDrawdown(), 160.0, PRICE_EPSILON);
    ASSERT_EQ(pnl12.getPosition(), 20);
    
    ASSERT_DOUBLE_EQ(fees._feeExLiqAdd, -1.00);
    ASSERT_DOUBLE_EQ(fees._feeExLiqRem, 0.60);
    ASSERT_DOUBLE_EQ(fees._feeExClearing, 1.90);
    ASSERT_DOUBLE_EQ(fees._feeBrokerage, 2.10);
    ASSERT_DOUBLE_EQ(fees._feePerTrade, 0.19);
    
    // TV 1.5 tick above avgPrice
    //
    tv2 = 131.0078125;
    pnl21.onTv(tv2);
    
    ASSERT_NEAR(pnl12.getPnL(), -5.3125, PRICE_EPSILON);    
    ASSERT_NEAR(pnl12.getRealizedPnL(), 0.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl12.getUnrealizedPnL(), -5.3125, PRICE_EPSILON);
    
    ASSERT_DOUBLE_EQ(pnl12.getMaxRealizedPnL(), 0.0);
    ASSERT_NEAR(pnl12.getMaxUnrealizedPnL(), 37.5, PRICE_EPSILON);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedDrawdown(), 0.0);
    ASSERT_NEAR(pnl12.getUnrealizedDrawdown(), 42.8125, PRICE_EPSILON);
    ASSERT_EQ(pnl12.getPosition(), 20);
    
    // TV 2.3 tick below avgPrice
    //
    tv2 = 130.9484375;
    pnl21.onTv(tv2);
    
    ASSERT_NEAR(pnl12.getPnL(), -302.1875, PRICE_EPSILON);
    ASSERT_NEAR(pnl12.getRealizedPnL(), 0.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl12.getUnrealizedPnL(), -302.1875, PRICE_EPSILON);
    
    ASSERT_DOUBLE_EQ(pnl12.getMaxRealizedPnL(), 0.0);
    ASSERT_NEAR(pnl12.getMaxUnrealizedPnL(), 37.5, PRICE_EPSILON);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedDrawdown(), 0.0);
    ASSERT_NEAR(pnl12.getUnrealizedDrawdown(), 339.6875, PRICE_EPSILON);
    ASSERT_EQ(pnl12.getPosition(), 20);
    
    // Buy 10@130.96875 for instrument 2 - rem liq - pos: 15
    //
    price = 130.96875;
    fill2._side = tw::channel_or::eOrderSide::kBuy;
    fill2._liqInd = tw::channel_or::eLiqInd::kRem;
    fill2._qty.set(10);
    fill2._price = instrument2->_tc->fromExchangePrice(price);
    pnl21.onFill(fill2);
    
    ASSERT_NEAR(pnl12.getPnL(), -505.3125, PRICE_EPSILON);    
    ASSERT_NEAR(pnl12.getRealizedPnL(), 0.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl12.getUnrealizedPnL(), -505.3125, PRICE_EPSILON);
    
    ASSERT_DOUBLE_EQ(pnl12.getMaxRealizedPnL(), 0.0);
    ASSERT_NEAR(pnl12.getMaxUnrealizedPnL(), 37.5, PRICE_EPSILON);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedDrawdown(), 0.0);
    ASSERT_NEAR(pnl12.getUnrealizedDrawdown(), 542.8125, PRICE_EPSILON);
    ASSERT_EQ(pnl12.getPosition(), 30);
    
    ASSERT_DOUBLE_EQ(fees._feeExLiqAdd, -1.00);
    ASSERT_DOUBLE_EQ(fees._feeExLiqRem, 2.20);
    ASSERT_DOUBLE_EQ(fees._feeExClearing, 3.60);
    ASSERT_DOUBLE_EQ(fees._feeBrokerage, 3.90);
    ASSERT_DOUBLE_EQ(fees._feePerTrade, 0.32);
}


TEST(CommonTradeLibTestSuit, PnLComposite_Composite_UnrealizedPnLBug)
{   
    tw::instr::InstrumentPtr instrument1 = InstrHelper::getNQM2();
    tw::instr::InstrumentPtr instrument2 = InstrHelper::getZNM2();
    
    tw::channel_or::Fill fill1;
    fill1._instrumentId = instrument1->_keyId;
    
    tw::channel_or::Fill fill2;
    fill2._instrumentId = instrument2->_keyId;
    
    TestTv tv1;
    tv1.setInstrument(instrument1);
    
    TestTv tv2;
    tv2.setInstrument(instrument2);
    
    tw::common_trade::PnL pnl1;
    tw::common_trade::PnL pnl2;
    
    pnl1.setInstrument(instrument1->_keyId);
    pnl2.setInstrument(instrument2->_keyId);
    
    tw::common_trade::PnLComposite pnl11;
    tw::common_trade::PnLComposite pnl21;
    
    pnl11.addComponent(&pnl1);
    pnl21.addComponent(&pnl2);
    
    tw::common_trade::PnLComposite pnl12;
    double price = 0.0;
    
    // Check initial values in composite
    //    
    ASSERT_TRUE(!pnl12.isValid());
    
    // Do the setup components/observers
    //        
    pnl11.setObserver(&pnl12);
    pnl21.setObserver(&pnl12);
    
    pnl12.addComponent(&pnl11);
    pnl12.addComponent(&pnl21);
    
    // Check initial values
    //
    ASSERT_DOUBLE_EQ(pnl12.getPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getUnrealizedPnL(), 0.0);
    
    ASSERT_DOUBLE_EQ(pnl12.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getUnrealizedDrawdown(), 0.0);
    ASSERT_EQ(pnl12.getPosition(), 0);
  
    // Buy 5@274175 for instrument 1 - pos: 5
    //
    price = 274175;
    fill1._side = tw::channel_or::eOrderSide::kBuy;
    fill1._liqInd = tw::channel_or::eLiqInd::kAdd;
    fill1._qty.set(5);
    fill1._price = instrument1->_tc->fromExchangePrice(price);
    pnl11.onFill(fill1);    
    
    
    ASSERT_DOUBLE_EQ(pnl12.getPnL(), 0.0);    
    ASSERT_DOUBLE_EQ(pnl12.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getUnrealizedPnL(), 0.0);
    
    ASSERT_DOUBLE_EQ(pnl12.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getUnrealizedDrawdown(), 0.0);
    ASSERT_EQ(pnl12.getPosition(), 5);
    
    // TV 1.5 tick above avgPrice for instrument 1
    //
    tv1 = 274212.5;    
    pnl11.onTv(tv1);
    
    ASSERT_DOUBLE_EQ(pnl12.getPnL(), 37.5);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getUnrealizedPnL(), 37.5);
    
    ASSERT_DOUBLE_EQ(pnl12.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getMaxUnrealizedPnL(), 37.5);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getUnrealizedDrawdown(), 0.0);
    ASSERT_EQ(pnl12.getPosition(), 5);    
    
    // Sell 5@130.984375 for instrument 2 - pos: 0
    //
    price = 130.984375;
    fill2._side = tw::channel_or::eOrderSide::kSell;
    fill2._qty.set(5);
    fill2._price = instrument2->_tc->fromExchangePrice(price);
    pnl21.onFill(fill2);
    
    ASSERT_NEAR(pnl12.getPnL(), 37.5, PRICE_EPSILON);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedPnL(), 0.0);
    ASSERT_NEAR(pnl12.getUnrealizedPnL(), 37.5, PRICE_EPSILON);
    
    ASSERT_DOUBLE_EQ(pnl12.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getMaxUnrealizedPnL(), 37.5);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedDrawdown(), 0.0);
    ASSERT_NEAR(pnl12.getUnrealizedDrawdown(), 0.0, PRICE_EPSILON);
    ASSERT_EQ(pnl12.getPosition(), 0);
    
    // TV 1.5 tick above avgPrice for instrument 2
    //
    tv2 = 131.0078125;
    pnl21.onTv(tv2);
    
    ASSERT_NEAR(pnl12.getPnL(), -79.6875, PRICE_EPSILON);    
    ASSERT_NEAR(pnl12.getRealizedPnL(), 0.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl12.getUnrealizedPnL(), -79.6875, PRICE_EPSILON);
    
    ASSERT_DOUBLE_EQ(pnl12.getMaxRealizedPnL(), 0.0);
    ASSERT_NEAR(pnl12.getMaxUnrealizedPnL(), 37.5, PRICE_EPSILON);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedDrawdown(), 0.0);
    ASSERT_NEAR(pnl12.getUnrealizedDrawdown(), 117.1875, PRICE_EPSILON);
    ASSERT_EQ(pnl12.getPosition(), 0);
    
    
    // Sell 5@274175 for instrument 1 - pos: -5
    //
    price = 274175;
    fill1._side = tw::channel_or::eOrderSide::kSell;
    fill1._qty.set(5);
    fill1._price = instrument1->_tc->fromExchangePrice(price);
    pnl11.onFill(fill1);    
    
    
    ASSERT_DOUBLE_EQ(pnl12.getPnL(), -117.1875);    
    ASSERT_DOUBLE_EQ(pnl12.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getUnrealizedPnL(), -117.1875);
    
    ASSERT_DOUBLE_EQ(pnl12.getMaxRealizedPnL(), 0.0);
    ASSERT_NEAR(pnl12.getMaxUnrealizedPnL(), 37.5, PRICE_EPSILON);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedDrawdown(), 0.0);
    ASSERT_NEAR(pnl12.getUnrealizedDrawdown(), 154.6875, PRICE_EPSILON);
    ASSERT_EQ(pnl12.getPosition(), -5);
    
    // Buy 5@130.984375 for instrument 2 - pos: 0
    //
    price = 130.984375;
    fill2._side = tw::channel_or::eOrderSide::kBuy;
    fill2._qty.set(5);
    fill2._price = instrument2->_tc->fromExchangePrice(price);
    pnl21.onFill(fill2);
    
    ASSERT_NEAR(pnl12.getPnL(), 0.0, PRICE_EPSILON);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedPnL(), 0.0);
    ASSERT_NEAR(pnl12.getUnrealizedPnL(), 0.0, PRICE_EPSILON);
    
    ASSERT_DOUBLE_EQ(pnl12.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl12.getRealizedDrawdown(), 0.0);
    ASSERT_NEAR(pnl12.getUnrealizedDrawdown(), 0.0, PRICE_EPSILON);
    ASSERT_EQ(pnl12.getPosition(), 0);
    
}

