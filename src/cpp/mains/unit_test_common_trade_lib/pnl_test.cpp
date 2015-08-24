#include <tw/common_trade/pnl.h>
#include <tw/common_trade/wbo.h>
#include <tw/price/quote_store.h>
#include <tw/generated/channel_or_defs.h>
#include "../unit_test_price_lib/instr_helper.h"

#include <gtest/gtest.h>

typedef tw::price::QuoteStore::TQuote TQuote;
typedef tw::price::Ticks TTicks;
typedef tw::price::Size TSize;

typedef tw::common_trade::Wbo<tw::common_trade::Wbo_calc1> TWbo1;

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
    
    bool setInstrument(tw::instr::InstrumentConstPtr instrument) {
        _instrument = instrument;
        return true;
    }
    
    void operator=(double v) {
        _tv = v;
        _isTvSet = true;
    }
};

TEST(CommonTradeLibTestSuit, PnL_NQ)
{   
    tw::instr::InstrumentPtr instrument = InstrHelper::getNQM2();
    
    tw::channel_or::Fill fill;
    fill._instrumentId = instrument->_keyId;
    
    TestTv tv;
    double price = 0.0;
    double avgPrice = 0.0;
    
    tv.setInstrument(instrument);
    
    // Fees: -0.05,0.06,0.07,0.08,0.03
    // TickValue: $5.0
    //
    tw::common_trade::PnL pnl;
    const tw::instr::Fees& fees = pnl.getFeesPaid();    
    
    ASSERT_EQ(pnl.getPosition().get(), 0);
    ASSERT_DOUBLE_EQ(pnl.getAvgPrice(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getUnrealizedPnL(), 0.0);
    
    ASSERT_DOUBLE_EQ(pnl.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getUnrealizedDrawdown(), 0.0);    
    
    ASSERT_DOUBLE_EQ(fees._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(fees._feeExLiqRem, 0.0);
    ASSERT_DOUBLE_EQ(fees._feeExClearing, 0.0);
    ASSERT_DOUBLE_EQ(fees._feeBrokerage, 0.0);
    ASSERT_DOUBLE_EQ(fees._feePerTrade, 0.0);
    
    // Buy 5@274175 - add liq - pos: 5
    //
    price = 274175;
    avgPrice = 274175;
    fill._side = tw::channel_or::eOrderSide::kBuy;
    fill._liqInd = tw::channel_or::eLiqInd::kAdd;
    fill._qty.set(5);
    fill._price = instrument->_tc->fromExchangePrice(price);
    pnl.onFill(fill);
    
    ASSERT_NEAR(fill._avgPrice, avgPrice, PRICE_EPSILON);
    
    ASSERT_EQ(pnl.getPosition().get(), 5);
    ASSERT_DOUBLE_EQ(pnl.getAvgPrice(), avgPrice);
    ASSERT_DOUBLE_EQ(pnl.getPnL(), 0.0);    
    ASSERT_DOUBLE_EQ(pnl.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getUnrealizedPnL(), 0.0);
    
    ASSERT_DOUBLE_EQ(pnl.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getUnrealizedDrawdown(), 0.0);
    
    ASSERT_DOUBLE_EQ(fees._feeExLiqAdd, -0.25);
    ASSERT_DOUBLE_EQ(fees._feeExLiqRem, 0.0);
    ASSERT_DOUBLE_EQ(fees._feeExClearing, 0.35);
    ASSERT_DOUBLE_EQ(fees._feeBrokerage, 0.40);
    ASSERT_DOUBLE_EQ(fees._feePerTrade, 0.03);
    
    // Test different marks
    //
    
    // TV 1.5 tick above avgPrice
    //
    tv = 274212.5;
    avgPrice = 274175;
    pnl.onTv(tv);
    
    ASSERT_EQ(pnl.getPosition().get(), 5);
    ASSERT_DOUBLE_EQ(pnl.getAvgPrice(), avgPrice);
    ASSERT_DOUBLE_EQ(pnl.getPnL(), 37.5);
    ASSERT_DOUBLE_EQ(pnl.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getUnrealizedPnL(), 37.5);
    
    ASSERT_DOUBLE_EQ(pnl.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getMaxUnrealizedPnL(), 37.5);
    ASSERT_DOUBLE_EQ(pnl.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getUnrealizedDrawdown(), 0.0);
    
    // TV 2.3 tick below avgPrice
    //
    tv = 274117.5;
    avgPrice = 274175;
    pnl.onTv(tv);
    
    ASSERT_EQ(pnl.getPosition().get(), 5);
    ASSERT_DOUBLE_EQ(pnl.getAvgPrice(), avgPrice);
    ASSERT_DOUBLE_EQ(pnl.getPnL(), -57.5);
    ASSERT_DOUBLE_EQ(pnl.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getUnrealizedPnL(), -57.5);
    
    ASSERT_DOUBLE_EQ(pnl.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getMaxUnrealizedPnL(), 37.5);
    ASSERT_DOUBLE_EQ(pnl.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getUnrealizedDrawdown(), 95.0);
    
    // Buy 10@274150 - rem liq - pos: 15
    //
    price = 274150;
    fill._side = tw::channel_or::eOrderSide::kBuy;
    fill._liqInd = tw::channel_or::eLiqInd::kRem;
    fill._qty.set(10);
    fill._price = instrument->_tc->fromExchangePrice(price);
    pnl.onFill(fill);
    
    avgPrice = 274158.33;
    ASSERT_NEAR(fill._avgPrice, avgPrice, PRICE_EPSILON);
    
    ASSERT_EQ(pnl.getPosition().get(), 15);
    ASSERT_NEAR(pnl.getAvgPrice(), avgPrice, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getPnL(), -122.5, PRICE_EPSILON);
    ASSERT_DOUBLE_EQ(pnl.getRealizedPnL(), 0.0);
    ASSERT_NEAR(pnl.getUnrealizedPnL(), -122.5, PRICE_EPSILON);
    
    ASSERT_DOUBLE_EQ(pnl.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getMaxUnrealizedPnL(), 37.5);
    ASSERT_DOUBLE_EQ(pnl.getRealizedDrawdown(), 0.0);
    ASSERT_NEAR(pnl.getUnrealizedDrawdown(), 160.0, PRICE_EPSILON);
    
    ASSERT_DOUBLE_EQ(fees._feeExLiqAdd, -0.25);
    ASSERT_DOUBLE_EQ(fees._feeExLiqRem, 0.60);
    ASSERT_DOUBLE_EQ(fees._feeExClearing, 1.05);
    ASSERT_DOUBLE_EQ(fees._feeBrokerage, 1.20);
    ASSERT_DOUBLE_EQ(fees._feePerTrade, 0.06);
    
    // Sell 5@274175 - add liq - pos: 10
    //
    price = 274175;
    fill._side = tw::channel_or::eOrderSide::kSell;
    fill._liqInd = tw::channel_or::eLiqInd::kAdd;
    fill._qty.set(5);
    fill._price = instrument->_tc->fromExchangePrice(price);
    pnl.onFill(fill);
    
    avgPrice = 274158.33;
    ASSERT_NEAR(fill._avgPrice, avgPrice, PRICE_EPSILON);
    
    ASSERT_EQ(pnl.getPosition().get(), 10);
    ASSERT_NEAR(pnl.getAvgPrice(), avgPrice, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getPnL(), -65.00, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getRealizedPnL(), 16.67, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getUnrealizedPnL(), -81.67, PRICE_EPSILON);
    
    ASSERT_NEAR(pnl.getMaxRealizedPnL(), 16.67, PRICE_EPSILON);
    ASSERT_DOUBLE_EQ(pnl.getMaxUnrealizedPnL(), 37.5);
    ASSERT_DOUBLE_EQ(pnl.getRealizedDrawdown(), 0.0);
    ASSERT_NEAR(pnl.getUnrealizedDrawdown(), 119.17, PRICE_EPSILON);
    
    ASSERT_DOUBLE_EQ(fees._feeExLiqAdd, -0.50);
    ASSERT_DOUBLE_EQ(fees._feeExLiqRem, 0.60);
    ASSERT_DOUBLE_EQ(fees._feeExClearing, 1.40);
    ASSERT_DOUBLE_EQ(fees._feeBrokerage, 1.60);
    ASSERT_DOUBLE_EQ(fees._feePerTrade, 0.09);
    
    // Sell 15@274200 - rem liq - pos: -5
    //
    price = 274200;
    fill._side = tw::channel_or::eOrderSide::kSell;
    fill._liqInd = tw::channel_or::eLiqInd::kRem;
    fill._qty.set(15);
    fill._price = instrument->_tc->fromExchangePrice(price);
    pnl.onFill(fill);
    
    avgPrice = price;
    ASSERT_NEAR(fill._avgPrice, avgPrice, PRICE_EPSILON);
    
    ASSERT_EQ(pnl.getPosition().get(), -5);
    ASSERT_NEAR(pnl.getAvgPrice(), avgPrice, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getPnL(), 182.5, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getRealizedPnL(), 100.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getUnrealizedPnL(), 82.5, PRICE_EPSILON);
    
    ASSERT_NEAR(pnl.getMaxRealizedPnL(), 100.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getMaxUnrealizedPnL(), 82.5, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getRealizedDrawdown(), 0.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getUnrealizedDrawdown(), 0.0, PRICE_EPSILON);
    
    ASSERT_DOUBLE_EQ(fees._feeExLiqAdd, -0.50);
    ASSERT_DOUBLE_EQ(fees._feeExLiqRem, 1.50);
    ASSERT_DOUBLE_EQ(fees._feeExClearing, 2.45);
    ASSERT_DOUBLE_EQ(fees._feeBrokerage, 2.80);
    ASSERT_DOUBLE_EQ(fees._feePerTrade, 0.12);
    
    // Sell 5@274200 - add liq - pos: -10
    //
    price = 274200;
    fill._side = tw::channel_or::eOrderSide::kSell;
    fill._liqInd = tw::channel_or::eLiqInd::kAdd;
    fill._qty.set(5);
    fill._price = instrument->_tc->fromExchangePrice(price);
    pnl.onFill(fill);
    
    avgPrice = price;
    ASSERT_NEAR(fill._avgPrice, avgPrice, PRICE_EPSILON);
    
    ASSERT_EQ(pnl.getPosition().get(), -10);
    ASSERT_NEAR(pnl.getAvgPrice(), avgPrice, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getPnL(), 265.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getRealizedPnL(), 100.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getUnrealizedPnL(), 165.0, PRICE_EPSILON);
    
    ASSERT_NEAR(pnl.getMaxRealizedPnL(), 100.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getMaxUnrealizedPnL(), 165.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getRealizedDrawdown(), 0.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getUnrealizedDrawdown(), 0.0, PRICE_EPSILON);
    
    ASSERT_DOUBLE_EQ(fees._feeExLiqAdd, -0.75);
    ASSERT_DOUBLE_EQ(fees._feeExLiqRem, 1.50);
    ASSERT_DOUBLE_EQ(fees._feeExClearing, 2.80);
    ASSERT_DOUBLE_EQ(fees._feeBrokerage, 3.20);
    ASSERT_DOUBLE_EQ(fees._feePerTrade, 0.15);
    
    
    // Buy 16@274100 - rem liq - pos: 6
    //
    price = 274100;
    fill._side = tw::channel_or::eOrderSide::kBuy;
    fill._liqInd = tw::channel_or::eLiqInd::kRem;
    fill._qty.set(16);
    fill._price = instrument->_tc->fromExchangePrice(price);
    pnl.onFill(fill);
    
    avgPrice = price;
    ASSERT_NEAR(fill._avgPrice, avgPrice, PRICE_EPSILON);
    
    ASSERT_EQ(pnl.getPosition().get(), 6);
    ASSERT_NEAR(pnl.getAvgPrice(), avgPrice, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getPnL(), 321.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getRealizedPnL(), 300.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getUnrealizedPnL(), 21.0, PRICE_EPSILON);
    
    ASSERT_NEAR(pnl.getMaxRealizedPnL(), 300.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getMaxUnrealizedPnL(), 165.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getRealizedDrawdown(), 0.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getUnrealizedDrawdown(), 144.0, PRICE_EPSILON);
    
    ASSERT_DOUBLE_EQ(fees._feeExLiqAdd, -0.75);
    ASSERT_DOUBLE_EQ(fees._feeExLiqRem, 2.46);
    ASSERT_DOUBLE_EQ(fees._feeExClearing, 3.92);
    ASSERT_DOUBLE_EQ(fees._feeBrokerage, 4.48);
    ASSERT_DOUBLE_EQ(fees._feePerTrade, 0.18);
    
    
    // Sell 6@274150 - add liq - pos: 0
    //
    price = 274150;
    fill._side = tw::channel_or::eOrderSide::kSell;
    fill._liqInd = tw::channel_or::eLiqInd::kAdd;
    fill._qty.set(6);
    fill._price = instrument->_tc->fromExchangePrice(price);
    pnl.onFill(fill);
    
    avgPrice = 0.0;
    ASSERT_NEAR(fill._avgPrice, avgPrice, PRICE_EPSILON);
    
    ASSERT_EQ(pnl.getPosition().get(), 0);
    ASSERT_NEAR(pnl.getAvgPrice(), avgPrice, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getPnL(), 360.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getRealizedPnL(), 360.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getUnrealizedPnL(), 0.0, PRICE_EPSILON);
    
    ASSERT_NEAR(pnl.getMaxRealizedPnL(), 360.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getMaxUnrealizedPnL(), 0.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getRealizedDrawdown(), 0.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getUnrealizedDrawdown(), 0.0, PRICE_EPSILON);
    
    ASSERT_DOUBLE_EQ(fees._feeExLiqAdd, -1.05);
    ASSERT_DOUBLE_EQ(fees._feeExLiqRem, 2.46);
    ASSERT_DOUBLE_EQ(fees._feeExClearing, 4.34);
    ASSERT_DOUBLE_EQ(fees._feeBrokerage, 4.96);
    ASSERT_DOUBLE_EQ(fees._feePerTrade, 0.21);
}

TEST(CommonTradeLibTestSuit, PnL_ZN)
{   
    tw::instr::InstrumentPtr instrument = InstrHelper::getZNM2();
    
    tw::channel_or::Fill fill;
    fill._instrumentId = instrument->_keyId;
    
    TestTv tv;
    double price = 0.0;
    double avgPrice = 0.0;
    
    tv.setInstrument(instrument);
    
    // Fees: --0.15,0.16,0.17,0.18,0.13
    // TickValue: $15.625
    //
    tw::common_trade::PnL pnl;
    const tw::instr::Fees& fees = pnl.getFeesPaid();    
    
    ASSERT_EQ(pnl.getPosition().get(), 0);
    ASSERT_DOUBLE_EQ(pnl.getAvgPrice(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getUnrealizedPnL(), 0.0);
    
    ASSERT_DOUBLE_EQ(pnl.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getUnrealizedDrawdown(), 0.0);
    
    ASSERT_DOUBLE_EQ(fees._feeExLiqAdd, 0.0);
    ASSERT_DOUBLE_EQ(fees._feeExLiqRem, 0.0);
    ASSERT_DOUBLE_EQ(fees._feeExClearing, 0.0);
    ASSERT_DOUBLE_EQ(fees._feeBrokerage, 0.0);
    ASSERT_DOUBLE_EQ(fees._feePerTrade, 0.0);
    
    // Buy 5@130.984375 - add liq - pos: 5
    //
    price = 130.984375;
    avgPrice = 130.984375;
    fill._side = tw::channel_or::eOrderSide::kBuy;
    fill._liqInd = tw::channel_or::eLiqInd::kAdd;
    fill._qty.set(5);
    fill._price = instrument->_tc->fromExchangePrice(price);
    pnl.onFill(fill);
    
    ASSERT_EQ(pnl.getPosition().get(), 5);
    ASSERT_DOUBLE_EQ(pnl.getAvgPrice(), avgPrice);
    ASSERT_DOUBLE_EQ(pnl.getPnL(), 0.0);    
    ASSERT_DOUBLE_EQ(pnl.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getUnrealizedPnL(), 0.0);
    
    ASSERT_DOUBLE_EQ(pnl.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getUnrealizedDrawdown(), 0.0);
    
    ASSERT_DOUBLE_EQ(fees._feeExLiqAdd, -0.75);
    ASSERT_DOUBLE_EQ(fees._feeExLiqRem, 0.0);
    ASSERT_DOUBLE_EQ(fees._feeExClearing, 0.85);
    ASSERT_DOUBLE_EQ(fees._feeBrokerage, 0.90);
    ASSERT_DOUBLE_EQ(fees._feePerTrade, 0.13);
    
 
    // Test different marks
    //
    
    // TV 1.5 tick above avgPrice
    //
    tv = 131.0078125;
    avgPrice = 130.984375;
    pnl.onTv(tv);
    
    ASSERT_EQ(pnl.getPosition().get(), 5);
    ASSERT_NEAR(pnl.getAvgPrice(), avgPrice, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getPnL(), 117.1875, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getRealizedPnL(), 0.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getUnrealizedPnL(), 117.1875, PRICE_EPSILON);
    
    ASSERT_DOUBLE_EQ(pnl.getMaxRealizedPnL(), 0.0);
    ASSERT_NEAR(pnl.getMaxUnrealizedPnL(), 117.1875, PRICE_EPSILON);
    ASSERT_DOUBLE_EQ(pnl.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getUnrealizedDrawdown(), 0.0);
    
    // TV 2.3 tick below avgPrice
    //
    tv = 130.9484375;
    avgPrice = 130.984375;
    pnl.onTv(tv);
    
    ASSERT_EQ(pnl.getPosition().get(), 5);
    ASSERT_NEAR(pnl.getAvgPrice(), avgPrice, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getPnL(), -179.6875, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getRealizedPnL(), 0.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getUnrealizedPnL(), -179.6875, PRICE_EPSILON);
    
    ASSERT_DOUBLE_EQ(pnl.getMaxRealizedPnL(), 0.0);
    ASSERT_NEAR(pnl.getMaxUnrealizedPnL(), 117.1875, PRICE_EPSILON);
    ASSERT_DOUBLE_EQ(pnl.getRealizedDrawdown(), 0.0);
    ASSERT_NEAR(pnl.getUnrealizedDrawdown(), 296.875, PRICE_EPSILON);
    
    // Buy 10@130.96875 - rem liq - pos: 15
    //
    price = 130.96875;
    fill._side = tw::channel_or::eOrderSide::kBuy;
    fill._liqInd = tw::channel_or::eLiqInd::kRem;
    fill._qty.set(10);
    fill._price = instrument->_tc->fromExchangePrice(price);
    pnl.onFill(fill);
    
    avgPrice = 130.97395833;
    ASSERT_NEAR(fill._avgPrice, avgPrice, PRICE_EPSILON);
    
    ASSERT_EQ(pnl.getPosition().get(), 15);
    ASSERT_NEAR(pnl.getAvgPrice(), avgPrice, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getPnL(), -382.8125, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getRealizedPnL(), 0.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getUnrealizedPnL(), -382.8125, PRICE_EPSILON);
    
    ASSERT_DOUBLE_EQ(pnl.getMaxRealizedPnL(), 0.0);
    ASSERT_NEAR(pnl.getMaxUnrealizedPnL(), 117.1875, PRICE_EPSILON);
    ASSERT_DOUBLE_EQ(pnl.getRealizedDrawdown(), 0.0);
    ASSERT_NEAR(pnl.getUnrealizedDrawdown(), 500.0, PRICE_EPSILON);
    
    ASSERT_DOUBLE_EQ(fees._feeExLiqAdd, -0.75);
    ASSERT_DOUBLE_EQ(fees._feeExLiqRem, 1.60);
    ASSERT_DOUBLE_EQ(fees._feeExClearing, 2.55);
    ASSERT_DOUBLE_EQ(fees._feeBrokerage, 2.70);
    ASSERT_DOUBLE_EQ(fees._feePerTrade, 0.26);
    
    // Sell 5@130.984375 - add liq - pos: 10
    //
    price = 130.984375;
    fill._side = tw::channel_or::eOrderSide::kSell;
    fill._liqInd = tw::channel_or::eLiqInd::kAdd;
    fill._qty.set(5);
    fill._price = instrument->_tc->fromExchangePrice(price);
    pnl.onFill(fill);
    
    avgPrice = 130.97395833;
    ASSERT_NEAR(fill._avgPrice, avgPrice, PRICE_EPSILON);
    
    ASSERT_EQ(pnl.getPosition().get(), 10);
    ASSERT_NEAR(pnl.getAvgPrice(), avgPrice, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getPnL(), -203.125, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getRealizedPnL(), 52.08333333, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getUnrealizedPnL(), -255.2083333, PRICE_EPSILON);
    
    ASSERT_NEAR(pnl.getMaxRealizedPnL(), 52.08333333, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getMaxUnrealizedPnL(), 117.1875, PRICE_EPSILON);
    ASSERT_DOUBLE_EQ(pnl.getRealizedDrawdown(), 0.0);
    ASSERT_NEAR(pnl.getUnrealizedDrawdown(), 372.395833, PRICE_EPSILON);
    
    ASSERT_DOUBLE_EQ(fees._feeExLiqAdd, -1.50);
    ASSERT_DOUBLE_EQ(fees._feeExLiqRem, 1.60);
    ASSERT_DOUBLE_EQ(fees._feeExClearing, 3.40);
    ASSERT_DOUBLE_EQ(fees._feeBrokerage, 3.60);
    ASSERT_DOUBLE_EQ(fees._feePerTrade, 0.39);
    
    // Sell 15@131.0 - rem liq - pos: -5
    //
    price = 131.0;
    fill._side = tw::channel_or::eOrderSide::kSell;
    fill._liqInd = tw::channel_or::eLiqInd::kRem;
    fill._qty.set(15);
    fill._price = instrument->_tc->fromExchangePrice(price);
    pnl.onFill(fill);
    
    avgPrice = price;
    ASSERT_NEAR(fill._avgPrice, avgPrice, PRICE_EPSILON);
    
    ASSERT_EQ(pnl.getPosition().get(), -5);
    ASSERT_NEAR(pnl.getAvgPrice(), avgPrice, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getPnL(), 570.3125, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getRealizedPnL(), 312.5, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getUnrealizedPnL(), 257.8125, PRICE_EPSILON);
    
    ASSERT_NEAR(pnl.getMaxRealizedPnL(), 312.5, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getMaxUnrealizedPnL(), 257.8125, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getRealizedDrawdown(), 0.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getUnrealizedDrawdown(), 0.0, PRICE_EPSILON);
    
    ASSERT_NEAR(fees._feeExLiqAdd, -1.50, PRICE_EPSILON);
    ASSERT_NEAR(fees._feeExLiqRem, 4.00, PRICE_EPSILON);
    ASSERT_NEAR(fees._feeExClearing, 5.95, PRICE_EPSILON);
    ASSERT_NEAR(fees._feeBrokerage, 6.30, PRICE_EPSILON);
    ASSERT_NEAR(fees._feePerTrade, 0.52, PRICE_EPSILON);
    
    // Sell 5@131.0 - add liq - pos: -10
    //
    price = 131.0;
    fill._side = tw::channel_or::eOrderSide::kSell;
    fill._liqInd = tw::channel_or::eLiqInd::kAdd;
    fill._qty.set(5);
    fill._price = instrument->_tc->fromExchangePrice(price);
    pnl.onFill(fill);
    
    avgPrice = price;
    ASSERT_NEAR(fill._avgPrice, avgPrice, PRICE_EPSILON);
    
    ASSERT_EQ(pnl.getPosition().get(), -10);
    ASSERT_NEAR(pnl.getAvgPrice(), avgPrice, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getPnL(), 828.125, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getRealizedPnL(), 312.5, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getUnrealizedPnL(), 515.625, PRICE_EPSILON);
    
    ASSERT_NEAR(pnl.getMaxRealizedPnL(), 312.5, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getMaxUnrealizedPnL(), 515.625, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getRealizedDrawdown(), 0.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getUnrealizedDrawdown(), 0.0, PRICE_EPSILON);
    
    ASSERT_NEAR(fees._feeExLiqAdd, -2.25, PRICE_EPSILON);
    ASSERT_NEAR(fees._feeExLiqRem, 4.00, PRICE_EPSILON);
    ASSERT_NEAR(fees._feeExClearing, 6.80, PRICE_EPSILON);
    ASSERT_NEAR(fees._feeBrokerage, 7.20, PRICE_EPSILON);
    ASSERT_NEAR(fees._feePerTrade, 0.65, PRICE_EPSILON);
    
    // Buy 16@130.9375 - rem liq - pos: 6
    //
    price = 130.9375;
    fill._side = tw::channel_or::eOrderSide::kBuy;
    fill._liqInd = tw::channel_or::eLiqInd::kRem;
    fill._qty.set(16);
    fill._price = instrument->_tc->fromExchangePrice(price);
    pnl.onFill(fill);
    
    avgPrice = price;
    ASSERT_NEAR(fill._avgPrice, avgPrice, PRICE_EPSILON);
    
    ASSERT_EQ(pnl.getPosition().get(), 6);
    ASSERT_NEAR(pnl.getAvgPrice(), avgPrice, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getPnL(), 1003.125, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getRealizedPnL(), 937.5, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getUnrealizedPnL(), 65.625, PRICE_EPSILON);
    
    ASSERT_NEAR(pnl.getMaxRealizedPnL(), 937.5, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getMaxUnrealizedPnL(), 515.625, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getRealizedDrawdown(), 0.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getUnrealizedDrawdown(), 450.0, PRICE_EPSILON);
    
    ASSERT_NEAR(fees._feeExLiqAdd, -2.25, PRICE_EPSILON);
    ASSERT_NEAR(fees._feeExLiqRem, 6.56, PRICE_EPSILON);
    ASSERT_NEAR(fees._feeExClearing, 9.52, PRICE_EPSILON);
    ASSERT_NEAR(fees._feeBrokerage, 10.08, PRICE_EPSILON);
    ASSERT_NEAR(fees._feePerTrade, 0.78, PRICE_EPSILON);    
    
    // Sell 6@130.96875 - add liq - pos: 0
    //
    price = 130.96875;
    fill._side = tw::channel_or::eOrderSide::kSell;
    fill._liqInd = tw::channel_or::eLiqInd::kAdd;
    fill._qty.set(6);
    fill._price = instrument->_tc->fromExchangePrice(price);
    pnl.onFill(fill);
    
    avgPrice = 0.0;
    ASSERT_NEAR(fill._avgPrice, avgPrice, PRICE_EPSILON);
    
    ASSERT_EQ(pnl.getPosition().get(), 0);
    ASSERT_NEAR(pnl.getAvgPrice(), avgPrice, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getPnL(), 1125.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getRealizedPnL(), 1125.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getUnrealizedPnL(), 0.0, PRICE_EPSILON);
    
    ASSERT_NEAR(pnl.getMaxRealizedPnL(), 1125.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getMaxUnrealizedPnL(), 0.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getRealizedDrawdown(), 0.0, PRICE_EPSILON);
    ASSERT_NEAR(pnl.getUnrealizedDrawdown(), 0.0, PRICE_EPSILON);
    
    ASSERT_NEAR(fees._feeExLiqAdd, -3.15, PRICE_EPSILON);
    ASSERT_NEAR(fees._feeExLiqRem, 6.56, PRICE_EPSILON);
    ASSERT_NEAR(fees._feeExClearing, 10.54, PRICE_EPSILON);
    ASSERT_NEAR(fees._feeBrokerage, 11.16, PRICE_EPSILON);
    ASSERT_NEAR(fees._feePerTrade, 0.91, PRICE_EPSILON);
}

TEST(CommonTradeLibTestSuit, PnL_NQ_RebuildPos)
{   
    tw::instr::InstrumentPtr instrument = InstrHelper::getNQM2();
    double price = 0.0;
    
    // 'Long' position
    {
        tw::channel_or::PosUpdate update;
        
        tw::common_trade::PnL pnl;
        const tw::instr::Fees& fees = pnl.getFeesPaid();
        
        price = 274158.33;
        
        update._displayName = instrument->_displayName;
        update._pos = 5;
        update._avgPrice = price;
        
        pnl.rebuildPos(update);
        
        ASSERT_EQ(pnl.getPosition().get(), 5);
        ASSERT_DOUBLE_EQ(pnl.getAvgPrice(), price);
        ASSERT_DOUBLE_EQ(pnl.getPnL(), 0.0);
        ASSERT_DOUBLE_EQ(pnl.getRealizedPnL(), 0.0);
        ASSERT_DOUBLE_EQ(pnl.getUnrealizedPnL(), 0.0);

        ASSERT_DOUBLE_EQ(pnl.getMaxRealizedPnL(), 0.0);
        ASSERT_DOUBLE_EQ(pnl.getMaxUnrealizedPnL(), 0.0);
        ASSERT_DOUBLE_EQ(pnl.getRealizedDrawdown(), 0.0);
        ASSERT_DOUBLE_EQ(pnl.getUnrealizedDrawdown(), 0.0);    

        ASSERT_DOUBLE_EQ(fees._feeExLiqAdd, 0.0);
        ASSERT_DOUBLE_EQ(fees._feeExLiqRem, 0.0);
        ASSERT_DOUBLE_EQ(fees._feeExClearing, 0.0);
        ASSERT_DOUBLE_EQ(fees._feeBrokerage, 0.0);
        ASSERT_DOUBLE_EQ(fees._feePerTrade, 0.0);
    }
    
    
    // 'Short' position
    {
        tw::channel_or::PosUpdate update;
        
        tw::common_trade::PnL pnl;
        const tw::instr::Fees& fees = pnl.getFeesPaid();
        
        price = 274158.33;
        
        update._displayName = instrument->_displayName;
        update._pos = -5;
        update._avgPrice = price;
        
        pnl.rebuildPos(update);
        
        ASSERT_EQ(pnl.getPosition().get(), -5);
        ASSERT_DOUBLE_EQ(pnl.getAvgPrice(), price);
        ASSERT_DOUBLE_EQ(pnl.getPnL(), 0.0);
        ASSERT_DOUBLE_EQ(pnl.getRealizedPnL(), 0.0);
        ASSERT_DOUBLE_EQ(pnl.getUnrealizedPnL(), 0.0);

        ASSERT_DOUBLE_EQ(pnl.getMaxRealizedPnL(), 0.0);
        ASSERT_DOUBLE_EQ(pnl.getMaxUnrealizedPnL(), 0.0);
        ASSERT_DOUBLE_EQ(pnl.getRealizedDrawdown(), 0.0);
        ASSERT_DOUBLE_EQ(pnl.getUnrealizedDrawdown(), 0.0);    

        ASSERT_DOUBLE_EQ(fees._feeExLiqAdd, 0.0);
        ASSERT_DOUBLE_EQ(fees._feeExLiqRem, 0.0);
        ASSERT_DOUBLE_EQ(fees._feeExClearing, 0.0);
        ASSERT_DOUBLE_EQ(fees._feeBrokerage, 0.0);
        ASSERT_DOUBLE_EQ(fees._feePerTrade, 0.0);
    }
}


TEST(CommonTradeLibTestSuit, PnL_AvgPriceCheck)
{   
    tw::instr::InstrumentPtr instrument = InstrHelper::getNQM2();
    
    tw::channel_or::Fill fill;
    fill._instrumentId = instrument->_keyId;
    
    TestTv tv;
    double price = 0.0;
    double avgPrice = 0.0;
    
    tv.setInstrument(instrument);
    
    // Fees: -0.05,0.06,0.07,0.08,0.03
    // TickValue: $5.0
    //
    tw::common_trade::PnL pnl;
    
    // Buy 5@274175 - pos: 5
    //
    price = 274175;
    avgPrice = 274175;
    fill._side = tw::channel_or::eOrderSide::kBuy;
    fill._qty.set(5);
    fill._price = instrument->_tc->fromExchangePrice(price);
    pnl.onFill(fill);
    
    ASSERT_NEAR(fill._avgPrice, avgPrice, PRICE_EPSILON);
    
    ASSERT_EQ(pnl.getPosition().get(), 5);
    ASSERT_DOUBLE_EQ(pnl.getAvgPrice(), avgPrice);
    ASSERT_DOUBLE_EQ(pnl.getPnL(), 0.0);    
    ASSERT_DOUBLE_EQ(pnl.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getUnrealizedPnL(), 0.0);
    
    ASSERT_DOUBLE_EQ(pnl.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getUnrealizedDrawdown(), 0.0);
    
    // Set tv = 274175 - 11 ticks    
    //
    price = 274175;
    tv = instrument->_tc->toExchangePrice(instrument->_tc->fromExchangePrice(price)-11);
    pnl.onTv(tv);    
    
    ASSERT_EQ(pnl.getPosition().get(), 5);
    ASSERT_DOUBLE_EQ(pnl.getAvgPrice(), avgPrice);
    ASSERT_DOUBLE_EQ(pnl.getPnL(), -275.0);    
    ASSERT_DOUBLE_EQ(pnl.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getUnrealizedPnL(), -275.0);
    
    ASSERT_DOUBLE_EQ(pnl.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getRealizedDrawdown(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getUnrealizedDrawdown(), 275.0);
    
    // Sell 4@274175 - 10 ticks - pos: 1
    //
    price = 274175;
    avgPrice = 274175;
    fill._side = tw::channel_or::eOrderSide::kSell;
    fill._qty.set(4);
    fill._price = instrument->_tc->fromExchangePrice(price)-10;
    pnl.onFill(fill);
    
    ASSERT_NEAR(fill._avgPrice, avgPrice, PRICE_EPSILON);
    
    ASSERT_EQ(pnl.getPosition().get(), 1);
    ASSERT_DOUBLE_EQ(pnl.getAvgPrice(), avgPrice);
    ASSERT_DOUBLE_EQ(pnl.getPnL(), -255.0);    
    ASSERT_DOUBLE_EQ(pnl.getRealizedPnL(), -200.0);
    ASSERT_DOUBLE_EQ(pnl.getUnrealizedPnL(), -55.0);
    
    ASSERT_DOUBLE_EQ(pnl.getMaxRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getMaxUnrealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getRealizedDrawdown(), 200.0);
    ASSERT_DOUBLE_EQ(pnl.getUnrealizedDrawdown(), 55.0);    
}

TEST(CommonTradeLibTestSuit, PnL_ZBM3_openPnL_bug)
{   
    tw::instr::InstrumentPtr instrument = InstrHelper::getZBM3();
    
    TWbo1 wbo;
    TQuote quote;
    
    quote.setInstrument(instrument);
    
    tw::channel_or::Fill fill;
    fill._instrumentId = instrument->_keyId;
    
    // TickValue: $31.25
    //
    tw::common_trade::PnL pnl;
    
    ASSERT_EQ(pnl.getPosition().get(), 0);
    ASSERT_DOUBLE_EQ(pnl.getAvgPrice(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getRealizedPnL(), 0.0);
    ASSERT_DOUBLE_EQ(pnl.getUnrealizedPnL(), 0.0);
    
    // Buy 9@148.125(4740 ticks) - add liq - pos: 9 - no tv set yet
    //
    fill._side = tw::channel_or::eOrderSide::kBuy;
    fill._liqInd = tw::channel_or::eLiqInd::kAdd;
    fill._qty.set(9);
    fill._price = instrument->_tc->fromExchangePrice(148.125);
    pnl.onFill(fill);
    
    ASSERT_EQ(pnl.getPosition().get(), 9);
    ASSERT_DOUBLE_EQ(pnl.getUnrealizedPnL(), 0.0);
    
    // Set tv to slightly positive pnl
    //
    quote.setBid(TTicks(4740), TSize(44), 0, 7);
    quote.setAsk(TTicks(4741), TSize(1100), 0, 51);
    
    wbo.onQuote(quote);
    ASSERT_TRUE(wbo.isValid());        
    ASSERT_NEAR(wbo.getTv(), 148.1262, PRICE_EPSILON);
    
    pnl.onTv(wbo);
    ASSERT_EQ(pnl.getPosition().get(), 9);
    ASSERT_NEAR(pnl.getUnrealizedPnL(), 10.8173, PRICE_EPSILON);
    
    // Set tv to more positive pnl
    //
    quote.setBid(TTicks(4741), TSize(500), 0, 7);
    quote.setAsk(TTicks(4742), TSize(100), 0, 51);
        
    wbo.onQuote(quote);
    ASSERT_TRUE(wbo.isValid());        
    ASSERT_NEAR(wbo.getTv(), 148.182, PRICE_EPSILON);
    
    pnl.onTv(wbo);
    ASSERT_EQ(pnl.getPosition().get(), 9);
    ASSERT_NEAR(pnl.getUnrealizedPnL(), 515.625, PRICE_EPSILON);
    
    // Change tv to negative pnl
    //
    quote.setBid(TTicks(4739), TSize(711), 0, 123);
    quote.setAsk(TTicks(4740), TSize(45), 0, 8);
        
    wbo.onQuote(quote);
    ASSERT_TRUE(wbo.isValid());        
    ASSERT_NEAR(wbo.getTv(), 148.1231, PRICE_EPSILON);
    
    pnl.onTv(wbo);
    ASSERT_EQ(pnl.getPosition().get(), 9);
    ASSERT_NEAR(pnl.getUnrealizedPnL(), -16.7411, PRICE_EPSILON);
 
    
}

