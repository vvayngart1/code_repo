#include <tw/exchange_sim/matcher.h>
#include "../unit_test_channel_or_lib/order_helper.h"

#include <gtest/gtest.h>

#include <vector>

typedef tw::exchange_sim::TOrderPtr TOrderPtr;
typedef tw::channel_or::Reject Reject;
typedef tw::exchange_sim::Matcher TMatcher;
typedef tw::exchange_sim::Matcher::TBookLevel TBookLevel;
typedef tw::price::Quote TQuote;

typedef std::vector<TOrderPtr> TOrders;
typedef tw::exchange_sim::Matcher::TBook TBook;

typedef tw::exchange_sim::TFill TFill;
typedef std::vector<TFill> TFills;

TFill createFill(const tw::price::Ticks& price,
                 const tw::price::Size& size,
                 const TOrderPtr& order,
                 const tw::channel_or::eLiqInd liqInd) {
    TFill fill;
    
    fill._price = price;
    fill._qty = size;
    fill._order = order;
    fill._liqInd = liqInd;
    
    return fill;
}

struct pricePred {
    pricePred(const tw::price::Ticks& price) : _price(price) {        
    }
    
    bool operator() (const tw::exchange_sim::Matcher::TBookLevel& level) const {
        return (level.first._price == _price);
    }
    
    const tw::price::Ticks& _price;
};

tw::price::Size getOpenOrderQty(const TOrderPtr& order) {
    return order->_qty - order->_cumQty;
}

class TestListener : public tw::exchange_sim::MatcherEventListener {
public:
    void clear() {
        _newAcks.clear();
        _modAcks.clear();
        _cxlAcks.clear();
        _fills.clear();
    }
    
    virtual void onNewAck(const TOrderPtr& order) {
        _newAcks.push_back(order);
    }
    
    virtual void onModAck(const TOrderPtr& order) {
        _modAcks.push_back(order);
    }
    
    virtual void onCxlAck(const TOrderPtr& order) {
        _cxlAcks.push_back(order);
    }
    
    virtual void onFill(const TFill& fill) {
        _fills.push_back(fill);
    }
    
    virtual void onEvent(const TMatcher* matcher) {
        
    }
    
    TOrders _newAcks;
    TOrders _modAcks;
    TOrders _cxlAcks;
    TFills _fills;
};

bool checkOrder(const TOrderPtr& order,
                const TBook& book,
                const size_t expectedBookSizeAtPriceLevel,
                const TOrders& orders,
                const TMatcher& matcher) {
    EXPECT_EQ(order->_clOrderId, order->_exOrderId);
    if ( order->_clOrderId != order->_exOrderId )
        return false;
    
    EXPECT_TRUE(!book.empty());
    if ( book.empty() )
        return false;    
    
    TBook::const_iterator iter = std::find_if(book.begin(), book.end(), pricePred(order->_price));
    EXPECT_TRUE(iter != book.end());
    if ( iter == book.end() )
        return false;
    
    EXPECT_EQ(iter->second.size(), expectedBookSizeAtPriceLevel);
    if ( iter->second.size() != expectedBookSizeAtPriceLevel )
        return false;
    
    EXPECT_EQ(iter->second[expectedBookSizeAtPriceLevel-1]->_exOrderId, order->_exOrderId);
    if ( iter->second[expectedBookSizeAtPriceLevel-1]->_exOrderId != order->_exOrderId )
        return false;
    
    EXPECT_TRUE(!orders.empty());
    if ( orders.empty() )
        return false;
    
    EXPECT_EQ(orders[0]->_exOrderId, order->_exOrderId);
    if ( orders[0]->_exOrderId != order->_exOrderId )
        return false;
    
    TOrderPtr cachedOrder = matcher.getOrder(order->_exOrderId);
    EXPECT_TRUE(cachedOrder.get() != NULL);
    if ( cachedOrder.get() == NULL )
        return false;
    
    EXPECT_EQ(cachedOrder->_exOrderId, order->_exOrderId);
    if ( cachedOrder->_exOrderId != order->_exOrderId )
        return false;
    
    return true;
}

bool checkPriceLevel(const TBook& book,
                     const size_t expectedBookSizeAtPriceLevel,
                     const tw::price::Ticks& price) {
    TBook::const_iterator iter = std::find_if(book.begin(), book.end(), pricePred(price));
    if ( iter == book.end() ) {
        if ( expectedBookSizeAtPriceLevel == 0 )
            return true;
        else
            return false;
    }
    
    EXPECT_EQ(iter->second.size(), expectedBookSizeAtPriceLevel);
    if ( iter->second.size() != expectedBookSizeAtPriceLevel )
        return false;
    
    return true;    
}

bool checkFill(const TFill& fill,
               const TFill& fillToCompare) {
    EXPECT_EQ(fill._price, fillToCompare._price);
    if ( fill._price != fillToCompare._price )
        return false;
    
    EXPECT_EQ(fill._qty, fillToCompare._qty);
    if ( fill._qty != fillToCompare._qty )
        return false;
    
    EXPECT_EQ(fill._liqInd, fillToCompare._liqInd);
    if ( fill._liqInd != fillToCompare._liqInd )
        return false;
    
    EXPECT_EQ(fill._order->_exOrderId, fillToCompare._order->_exOrderId);
    if ( fill._order->_exOrderId != fillToCompare._order->_exOrderId )
        return false;
    
    return true;
}


TEST(ExchangeSimLibTestSuit, matcher_wo_quotes_bids)
{
    TestListener listener;
    
    tw::instr::InstrumentPtr instrument = InstrHelper::getNQH2();
    TMatcher matcher(listener, instrument);    
    tw::price::Ticks price(9241);
    
    // Test bids
    //
    TOrderPtr b1, b2;
    TOrderPtr a1, a2, a3;

    matcher.clear();

    // b1 - price = 9241, qty = 3
    //
    listener.clear();

    b1 = OrderHelper::getSimBuyLimitWithClOrderIds(price.get(), 3);
    ASSERT_TRUE(!b1->_clOrderId.empty());
    ASSERT_TRUE(b1->_exOrderId.empty());

    matcher.sendNew(b1);
    ASSERT_TRUE(checkOrder(b1, matcher.getBids(), 1U, listener._newAcks, matcher));
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());

    // a1 - price = 9241+1, qty = 2
    //
    listener.clear();

    a1 = OrderHelper::getSimSellLimitWithClOrderIds(price.get()+1, 2);
    ASSERT_TRUE(!a1->_clOrderId.empty());
    ASSERT_TRUE(a1->_exOrderId.empty());

    matcher.sendNew(a1);
    ASSERT_TRUE(checkOrder(a1, matcher.getAsks(), 1U, listener._newAcks, matcher));
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());

    // a2 - price = 9241+1, qty = 4
    //
    listener.clear();

    a2 = OrderHelper::getSimSellLimitWithClOrderIds(price.get()+1, 4);
    ASSERT_TRUE(!a2->_clOrderId.empty());
    ASSERT_TRUE(a2->_exOrderId.empty());

    matcher.sendNew(a2);
    ASSERT_TRUE(checkOrder(a2, matcher.getAsks(), 2U, listener._newAcks, matcher));
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());

    // b2 - price = 9241+1, qty = 7 - crosses with 2 asks - 4 fills
    //
    listener.clear();

    b2 = OrderHelper::getSimBuyLimitWithClOrderIds(price.get()+1, 7);
    ASSERT_TRUE(!b2->_clOrderId.empty());
    ASSERT_TRUE(b2->_exOrderId.empty());

    matcher.sendNew(b2);
    ASSERT_TRUE(checkOrder(b2, matcher.getBids(), 1U, listener._newAcks, matcher));
    ASSERT_EQ(b2->_qty.get()-b2->_cumQty.get(), 1U);

    ASSERT_TRUE(matcher.getOrder(a1->_exOrderId).get() == NULL);
    ASSERT_TRUE(matcher.getOrder(a2->_exOrderId).get() == NULL);

    ASSERT_EQ(listener._fills.size(), 4U);    
    ASSERT_TRUE(checkFill(listener._fills[0], createFill(price+1, tw::price::Size(2), a1, tw::channel_or::eLiqInd::kAdd)));
    ASSERT_TRUE(checkFill(listener._fills[1], createFill(price+1, tw::price::Size(2), b2, tw::channel_or::eLiqInd::kRem)));    
    ASSERT_TRUE(checkFill(listener._fills[2], createFill(price+1, tw::price::Size(4), a2, tw::channel_or::eLiqInd::kAdd)));
    ASSERT_TRUE(checkFill(listener._fills[3], createFill(price+1, tw::price::Size(4), b2, tw::channel_or::eLiqInd::kRem)));
    
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());

    // Mod b1 - price = 9241+2
    //
    listener.clear();

    b1->_newPrice = price+2;

    matcher.sendMod(b1);
    ASSERT_TRUE(checkOrder(b1, matcher.getBids(), 1U, listener._modAcks, matcher));
    ASSERT_EQ(b1->_qty.get()-b1->_cumQty.get(), 3U);

    ASSERT_TRUE(checkPriceLevel(matcher.getBids(), 0U, price));
    ASSERT_TRUE(checkPriceLevel(matcher.getBids(), 1U, price+1));
    ASSERT_TRUE(checkPriceLevel(matcher.getBids(), 1U, price+2));
    ASSERT_TRUE(checkPriceLevel(matcher.getAsks(), 0U, price));
    ASSERT_TRUE(checkPriceLevel(matcher.getAsks(), 0U, price+1));
    ASSERT_TRUE(checkPriceLevel(matcher.getAsks(), 0U, price+2));

    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());

    // a3 - price = 9241-1, qty = 8 - crosses with 2 bids - 4 fills
    //
    listener.clear();

    a3 = OrderHelper::getSimSellLimitWithClOrderIds(price.get()-1, 8);
    ASSERT_TRUE(!a3->_clOrderId.empty());
    ASSERT_TRUE(a3->_exOrderId.empty());

    matcher.sendNew(a3);
    ASSERT_TRUE(checkOrder(a3, matcher.getAsks(), 1U, listener._newAcks, matcher));
    ASSERT_EQ(a3->_qty.get()-a3->_cumQty.get(), 4U);

    ASSERT_TRUE(matcher.getOrder(b1->_exOrderId).get() == NULL);
    ASSERT_TRUE(matcher.getOrder(b2->_exOrderId).get() == NULL);

    ASSERT_EQ(listener._fills.size(), 4U);    
    ASSERT_TRUE(checkFill(listener._fills[0], createFill(price+2, tw::price::Size(3), b1, tw::channel_or::eLiqInd::kAdd)));
    ASSERT_TRUE(checkFill(listener._fills[1], createFill(price+2, tw::price::Size(3), a3, tw::channel_or::eLiqInd::kRem)));
    ASSERT_TRUE(checkFill(listener._fills[2], createFill(price+1, tw::price::Size(1), b2, tw::channel_or::eLiqInd::kAdd)));
    ASSERT_TRUE(checkFill(listener._fills[3], createFill(price+1, tw::price::Size(1), a3, tw::channel_or::eLiqInd::kRem)));


    ASSERT_TRUE(checkPriceLevel(matcher.getBids(), 0U, price-1));
    ASSERT_TRUE(checkPriceLevel(matcher.getBids(), 0U, price));
    ASSERT_TRUE(checkPriceLevel(matcher.getBids(), 0U, price+1));
    ASSERT_TRUE(checkPriceLevel(matcher.getBids(), 0U, price+2));
    ASSERT_TRUE(checkPriceLevel(matcher.getAsks(), 1U, price-1));
    ASSERT_TRUE(checkPriceLevel(matcher.getAsks(), 0U, price));
    ASSERT_TRUE(checkPriceLevel(matcher.getAsks(), 0U, price+1));
    ASSERT_TRUE(checkPriceLevel(matcher.getAsks(), 0U, price+2));

    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());

    // Cxl a3
    //
    listener.clear();

    matcher.sendCxl(a3);
    ASSERT_TRUE(matcher.getOrder(a3->_exOrderId).get() == NULL);
    ASSERT_EQ(listener._cxlAcks.size(), 1U);
    ASSERT_EQ(listener._cxlAcks[0]->_exOrderId, a3->_exOrderId);

    ASSERT_TRUE(checkPriceLevel(matcher.getBids(), 0U, price-1));
    ASSERT_TRUE(checkPriceLevel(matcher.getBids(), 0U, price));
    ASSERT_TRUE(checkPriceLevel(matcher.getBids(), 0U, price+1));
    ASSERT_TRUE(checkPriceLevel(matcher.getBids(), 0U, price+2));
    ASSERT_TRUE(checkPriceLevel(matcher.getAsks(), 0U, price-1));
    ASSERT_TRUE(checkPriceLevel(matcher.getAsks(), 0U, price));
    ASSERT_TRUE(checkPriceLevel(matcher.getAsks(), 0U, price+1));
    ASSERT_TRUE(checkPriceLevel(matcher.getAsks(), 0U, price+2));

    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
}


TEST(ExchangeSimLibTestSuit, matcher_wo_quotes_asks)
{
    TestListener listener;
    
    tw::instr::InstrumentPtr instrument = InstrHelper::getNQH2();
    TMatcher matcher(listener, instrument);    
    tw::price::Ticks price(9241);
    
    // Test asks
    //
    TOrderPtr a1, a2;
    TOrderPtr b1, b2, b3;

    matcher.clear();

    // a1 - price = 9241, qty = 3
    //
    listener.clear();

    a1 = OrderHelper::getSimSellLimitWithClOrderIds(price.get(), 3);
    ASSERT_TRUE(!a1->_clOrderId.empty());
    ASSERT_TRUE(a1->_exOrderId.empty());

    matcher.sendNew(a1);
    ASSERT_TRUE(checkOrder(a1, matcher.getAsks(), 1U, listener._newAcks, matcher));
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());

    // b1 - price = 9241-1, qty = 2
    //
    listener.clear();

    b1 = OrderHelper::getSimBuyLimitWithClOrderIds(price.get()-1, 2);
    ASSERT_TRUE(!b1->_clOrderId.empty());
    ASSERT_TRUE(b1->_exOrderId.empty());

    matcher.sendNew(b1);
    ASSERT_TRUE(checkOrder(b1, matcher.getBids(), 1U, listener._newAcks, matcher));
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());

    // b2 - price = 9241+1, qty = 4
    //
    listener.clear();

    b2 = OrderHelper::getSimBuyLimitWithClOrderIds(price.get()-1, 4);
    ASSERT_TRUE(!b2->_clOrderId.empty());
    ASSERT_TRUE(b2->_exOrderId.empty());

    matcher.sendNew(b2);
    ASSERT_TRUE(checkOrder(b2, matcher.getBids(), 2U, listener._newAcks, matcher));
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());

    // a2 - price = 9241-1, qty = 7 - crosses with 2 bids - 4 fills
    //
    listener.clear();

    a2 = OrderHelper::getSimSellLimitWithClOrderIds(price.get()-1, 7);
    ASSERT_TRUE(!a2->_clOrderId.empty());
    ASSERT_TRUE(a2->_exOrderId.empty());

    matcher.sendNew(a2);
    ASSERT_TRUE(checkOrder(a2, matcher.getAsks(), 1U, listener._newAcks, matcher));
    ASSERT_EQ(a2->_qty.get()-a2->_cumQty.get(), 1U);

    ASSERT_TRUE(matcher.getOrder(b1->_exOrderId).get() == NULL);
    ASSERT_TRUE(matcher.getOrder(b2->_exOrderId).get() == NULL);

    ASSERT_EQ(listener._fills.size(), 4U);    
    ASSERT_TRUE(checkFill(listener._fills[0], createFill(price-1, tw::price::Size(2), b1, tw::channel_or::eLiqInd::kAdd)));
    ASSERT_TRUE(checkFill(listener._fills[1], createFill(price-1, tw::price::Size(2), a2, tw::channel_or::eLiqInd::kRem)));
    ASSERT_TRUE(checkFill(listener._fills[2], createFill(price-1, tw::price::Size(4), b2, tw::channel_or::eLiqInd::kAdd)));
    ASSERT_TRUE(checkFill(listener._fills[3], createFill(price-1, tw::price::Size(4), a2, tw::channel_or::eLiqInd::kRem)));
    
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());

    // Mod a1 - price = 9241-2
    //
    listener.clear();

    a1->_newPrice = price-2;

    matcher.sendMod(a1);
    ASSERT_TRUE(checkOrder(a1, matcher.getAsks(), 1U, listener._modAcks, matcher));
    ASSERT_EQ(a1->_qty.get()-a1->_cumQty.get(), 3U);

    ASSERT_TRUE(checkPriceLevel(matcher.getAsks(), 0U, price));
    ASSERT_TRUE(checkPriceLevel(matcher.getAsks(), 1U, price-1));
    ASSERT_TRUE(checkPriceLevel(matcher.getAsks(), 1U, price-2));
    ASSERT_TRUE(checkPriceLevel(matcher.getBids(), 0U, price));
    ASSERT_TRUE(checkPriceLevel(matcher.getBids(), 0U, price-1));
    ASSERT_TRUE(checkPriceLevel(matcher.getBids(), 0U, price-2));

    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());

    // b3 - price = 9241+1, qty = 8 - crosses with 2 asks - 4 fills
    //
    listener.clear();

    b3 = OrderHelper::getSimBuyLimitWithClOrderIds(price.get()+1, 8);
    ASSERT_TRUE(!b3->_clOrderId.empty());
    ASSERT_TRUE(b3->_exOrderId.empty());

    matcher.sendNew(b3);
    ASSERT_TRUE(checkOrder(b3, matcher.getBids(), 1U, listener._newAcks, matcher));
    ASSERT_EQ(b3->_qty.get()-b3->_cumQty.get(), 4U);

    ASSERT_TRUE(matcher.getOrder(b1->_exOrderId).get() == NULL);
    ASSERT_TRUE(matcher.getOrder(b2->_exOrderId).get() == NULL);

    ASSERT_EQ(listener._fills.size(), 4U);
    
    ASSERT_TRUE(checkFill(listener._fills[0], createFill(price-2, tw::price::Size(3), a1, tw::channel_or::eLiqInd::kAdd)));
    ASSERT_TRUE(checkFill(listener._fills[1], createFill(price-2, tw::price::Size(3), b3, tw::channel_or::eLiqInd::kRem)));
    ASSERT_TRUE(checkFill(listener._fills[2], createFill(price-1, tw::price::Size(1), a2, tw::channel_or::eLiqInd::kAdd)));
    ASSERT_TRUE(checkFill(listener._fills[3], createFill(price-1, tw::price::Size(1), b3, tw::channel_or::eLiqInd::kRem)));


    ASSERT_TRUE(checkPriceLevel(matcher.getAsks(), 0U, price+1));
    ASSERT_TRUE(checkPriceLevel(matcher.getAsks(), 0U, price));
    ASSERT_TRUE(checkPriceLevel(matcher.getAsks(), 0U, price-1));
    ASSERT_TRUE(checkPriceLevel(matcher.getAsks(), 0U, price-2));
    ASSERT_TRUE(checkPriceLevel(matcher.getBids(), 1U, price+1));
    ASSERT_TRUE(checkPriceLevel(matcher.getBids(), 0U, price));
    ASSERT_TRUE(checkPriceLevel(matcher.getBids(), 0U, price-1));
    ASSERT_TRUE(checkPriceLevel(matcher.getBids(), 0U, price-2));

    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());

    // Cxl b3
    //
    listener.clear();

    matcher.sendCxl(b3);
    ASSERT_TRUE(matcher.getOrder(b3->_exOrderId).get() == NULL);
    ASSERT_EQ(listener._cxlAcks.size(), 1U);
    ASSERT_EQ(listener._cxlAcks[0]->_exOrderId, b3->_exOrderId);

    ASSERT_TRUE(checkPriceLevel(matcher.getBids(), 0U, price-1));
    ASSERT_TRUE(checkPriceLevel(matcher.getBids(), 0U, price));
    ASSERT_TRUE(checkPriceLevel(matcher.getBids(), 0U, price+1));
    ASSERT_TRUE(checkPriceLevel(matcher.getBids(), 0U, price+2));
    ASSERT_TRUE(checkPriceLevel(matcher.getAsks(), 0U, price-1));
    ASSERT_TRUE(checkPriceLevel(matcher.getAsks(), 0U, price));
    ASSERT_TRUE(checkPriceLevel(matcher.getAsks(), 0U, price+1));
    ASSERT_TRUE(checkPriceLevel(matcher.getAsks(), 0U, price+2));

    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._fills.empty());    
}


TEST(ExchangeSimLibTestSuit, matcher_w_quotes_bids)
{
    TestListener listener;    
    tw::instr::InstrumentPtr instrument = InstrHelper::getNQH2();
    
    TMatcher matcher(listener, instrument);
    
    tw::price::Ticks price(9241);
    tw::price::Size size;
    TBookLevel level;
    
    TQuote quote;
    
    quote._instrument = instrument.get();
    quote._instrumentId = quote._instrument->_keyId;
    
    TOrderPtr b1, b2, b3, b4;
    
    
    // Test bids
    //
    
    // Add quote with b1 = 10@9241
    //
    size.set(10);
    quote.setBid(price, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 0U);
    
    level = matcher.getBookLevel(price, true);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 10U);
    ASSERT_TRUE(matcher.getAllOpenOrders().empty());
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty()); 

    // b1 - price = 9241, qty = 3
    //
    listener.clear();

    b1 = OrderHelper::getSimBuyLimitWithClOrderIds(price.get(), 3);
    matcher.sendNew(b1);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 1U);
    ASSERT_TRUE(checkOrder(b1, matcher.getBids(), 2U, listener._newAcks, matcher));
    
    level = matcher.getBookLevel(price, true);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 2U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 10U);
    ASSERT_EQ(level.second[1]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[1]).get(), 3U);
    
    ASSERT_EQ(listener._newAcks.size(), 1U);
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // Change quote with b1 = 30@9241
    //
    listener.clear();    
    
    size.set(30);
    quote.clearFlag();
    quote.setBid(price, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 1U);
    
    level = matcher.getBookLevel(price, true);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 3U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 10U);
    ASSERT_EQ(level.second[1]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[1]).get(), 3U);
    ASSERT_EQ(level.second[2]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[2]).get(), 20U);
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    // b2 - price = 9241, qty = 2
    //
    listener.clear();

    b2 = OrderHelper::getSimBuyLimitWithClOrderIds(price.get(), 2);
    matcher.sendNew(b2);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 2U);
    ASSERT_TRUE(checkOrder(b2, matcher.getBids(), 4U, listener._newAcks, matcher));
    
    level = matcher.getBookLevel(price, true);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 4U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 10U);
    ASSERT_EQ(level.second[1]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[1]).get(), 3U);
    ASSERT_EQ(level.second[2]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[2]).get(), 20U);
    ASSERT_EQ(level.second[3]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[3]).get(), 2U);
    
    ASSERT_EQ(listener._newAcks.size(), 1U);
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // Change quote with b1 = 5@9241
    //
    listener.clear();
    
    size.set(5);
    quote.clearFlag();
    quote.setBid(price, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 2U);
    
    level = matcher.getBookLevel(price, true);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 3U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 5U);
    ASSERT_EQ(level.second[1]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[1]).get(), 3U);
    ASSERT_EQ(level.second[2]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[2]).get(), 2U);
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // Change quote with b1 = 40@9241
    //
    listener.clear();
    
    size.set(40);
    quote.clearFlag();
    quote.setBid(price, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 2U);
    
    level = matcher.getBookLevel(price, true);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 4U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 5U);
    ASSERT_EQ(level.second[1]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[1]).get(), 3U);
    ASSERT_EQ(level.second[2]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[2]).get(), 2U);
    ASSERT_EQ(level.second[3]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[3]).get(), 35U);
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // Change quote with b1 = 9@9241
    //
    listener.clear();
    
    size.set(9);
    quote.clearFlag();
    quote.setBid(price, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 2U);
    
    level = matcher.getBookLevel(price, true);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 3U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 3U);
    ASSERT_EQ(level.second[1]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[1]).get(), 2U);
    ASSERT_EQ(level.second[2]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[2]).get(), 9U);
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // b3 - price = 9241-1, qty = 4
    //
    listener.clear();

    b3 = OrderHelper::getSimBuyLimitWithClOrderIds(price.get()-1, 4);
    matcher.sendNew(b3);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 3U);
    ASSERT_TRUE(checkOrder(b3, matcher.getBids(), 1U, listener._newAcks, matcher));
    
    level = matcher.getBookLevel(price, true);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 3U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 3U);
    ASSERT_EQ(level.second[1]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[1]).get(), 2U);
    ASSERT_EQ(level.second[2]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[2]).get(), 9U);
    
    ASSERT_EQ(listener._newAcks.size(), 1U);
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // Change quote with b1 = 15@9241
    //
    listener.clear();
    
    size.set(15);
    quote.clearFlag();
    quote.setBid(price, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 3U);
    
    level = matcher.getBookLevel(price, true);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 3U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 3U);
    ASSERT_EQ(level.second[1]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[1]).get(), 2U);
    ASSERT_EQ(level.second[2]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[2]).get(), 15U);
    
    
    level = matcher.getBookLevel(price-1, true);
    ASSERT_EQ(level.first._price, price-1);
    ASSERT_EQ(level.first._size, 0U);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 4U);
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // Change quote with b1 = 23@9241
    //
    listener.clear();
    
    size.set(23);
    quote.clearFlag();
    quote.setBid(price, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 3U);
    
    level = matcher.getBookLevel(price, true);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 3U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 3U);
    ASSERT_EQ(level.second[1]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[1]).get(), 2U);
    ASSERT_EQ(level.second[2]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[2]).get(), 23U);
    
    
    level = matcher.getBookLevel(price-1, true);
    ASSERT_EQ(level.first._price, price-1);
    ASSERT_EQ(level.first._size, 0U);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 4U);
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // Change quote with b1 = 34@9241
    //
    listener.clear();
    
    size.set(34);
    quote.clearFlag();
    quote.setBid(price, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 3U);
    
    level = matcher.getBookLevel(price, true);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 3U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 3U);
    ASSERT_EQ(level.second[1]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[1]).get(), 2U);
    ASSERT_EQ(level.second[2]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[2]).get(), 34U);
    
    
    level = matcher.getBookLevel(price-1, true);
    ASSERT_EQ(level.first._price, price-1);
    ASSERT_EQ(level.first._size, 0U);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 4U);
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // Change quote with b1 = 9@9241
    //
    listener.clear();
    
    size.set(9);
    quote.clearFlag();
    quote.setBid(price, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 3U);
    
    level = matcher.getBookLevel(price, true);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 3U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 3U);
    ASSERT_EQ(level.second[1]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[1]).get(), 2U);
    ASSERT_EQ(level.second[2]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[2]).get(), 9U);
    
    level = matcher.getBookLevel(price-1, true);
    ASSERT_EQ(level.first._price, price-1);
    ASSERT_EQ(level.first._size, 0U);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 4U);
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    // Change quote with trade = 16@9241-1 - will cross with 5 orders:
    // 2 limit and 2 sim @9241 and 1 limit @9240
    //
    listener.clear();
    
    size.set(16);
    quote.clearFlag();
    quote.setTrade(price-1, size);
    
    matcher.onQuote(quote);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 1U);
    
    level = matcher.getBookLevel(price, true);
    ASSERT_EQ(level.first._price, tw::price::Ticks::INVALID_VALUE());
    ASSERT_EQ(level.first._size, 0U);
    
    ASSERT_EQ(level.second.size(), 0U);
    
    level = matcher.getBookLevel(price-1, true);
    ASSERT_EQ(level.first._price, price-1);
    ASSERT_EQ(level.first._size, 0U);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 2U);
    
    ASSERT_EQ(listener._fills.size(), 3U);
    ASSERT_TRUE(checkFill(listener._fills[0], createFill(price, tw::price::Size(3), b1, tw::channel_or::eLiqInd::kAdd)));
    ASSERT_TRUE(checkFill(listener._fills[1], createFill(price, tw::price::Size(2), b2, tw::channel_or::eLiqInd::kAdd)));
    ASSERT_TRUE(checkFill(listener._fills[2], createFill(price-1, tw::price::Size(2), b3, tw::channel_or::eLiqInd::kAdd)));
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    
    
    // b4 - price = 9241-2, qty = 4
    //
    listener.clear();

    b4 = OrderHelper::getSimBuyLimitWithClOrderIds(price.get()-2, 4);
    matcher.sendNew(b4);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 2U);
    ASSERT_TRUE(checkOrder(b4, matcher.getBids(), 1U, listener._newAcks, matcher));
    
    level = matcher.getBookLevel(price-1, true);
    ASSERT_EQ(level.first._price, price-1);
    ASSERT_EQ(level.first._size, 0U);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 2U);
    
    level = matcher.getBookLevel(price-2, true);
    ASSERT_EQ(level.first._price, price-2);
    ASSERT_EQ(level.first._size, 0U);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 4U);    
    
    ASSERT_EQ(listener._newAcks.size(), 1U);
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    // Check quote crossing (may never happen with CME or other exchange prices, but nevertheless)
    //
    listener.clear();
    
    size.set(20);
    quote.clearFlag();
    quote.setAsk(price-2, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 0U);
    
    level = matcher.getBookLevel(price-1, true);
    ASSERT_EQ(level.first._price, tw::price::Ticks::INVALID_VALUE());
    ASSERT_EQ(level.first._size, 0U);
    
    ASSERT_EQ(level.second.size(), 0U);
    
    level = matcher.getBookLevel(price-2, true);
    ASSERT_EQ(level.first._price, tw::price::Ticks::INVALID_VALUE());
    ASSERT_EQ(level.first._size, 0U);
    
    ASSERT_EQ(level.second.size(), 0U);
    
    level = matcher.getBookLevel(price-2, false);
    ASSERT_EQ(level.first._price, price-2);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 20U);
    
    ASSERT_EQ(listener._fills.size(), 2U); 
    ASSERT_TRUE(checkFill(listener._fills[0], createFill(price-1, tw::price::Size(2), b3, tw::channel_or::eLiqInd::kAdd))); 
    ASSERT_TRUE(checkFill(listener._fills[1], createFill(price-2, tw::price::Size(4), b4, tw::channel_or::eLiqInd::kAdd)));
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
}

TEST(ExchangeSimLibTestSuit, matcher_w_quotes_asks)
{
    TestListener listener;    
    
    tw::instr::InstrumentPtr instrument = InstrHelper::getNQH2();
    TMatcher matcher(listener, instrument);
    
    tw::price::Ticks price(9241);
    tw::price::Size size;
    TBookLevel level;
    
    TQuote quote;
    
    quote._instrument = instrument.get();
    quote._instrumentId = quote._instrument->_keyId;
    
    TOrderPtr a1, a2, a3, a4;
    
    
    // Test asks
    //
    
    // Add quote with a1 = 10@9241
    //
    size.set(10);
    quote.setAsk(price, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 0U);
    
    level = matcher.getBookLevel(price, false);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 10U);
    ASSERT_TRUE(matcher.getAllOpenOrders().empty());
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty()); 

    // a1 - price = 9241, qty = 3
    //
    listener.clear();

    a1 = OrderHelper::getSimSellLimitWithClOrderIds(price.get(), 3);
    matcher.sendNew(a1);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 1U);
    ASSERT_TRUE(checkOrder(a1, matcher.getAsks(), 2U, listener._newAcks, matcher));
    
    level = matcher.getBookLevel(price, false);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 2U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 10U);
    ASSERT_EQ(level.second[1]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[1]).get(), 3U);
    
    ASSERT_EQ(listener._newAcks.size(), 1U);
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // Change quote with a1 = 30@9241
    //
    listener.clear();    
    
    size.set(30);
    quote.clearFlag();
    quote.setAsk(price, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 1U);
    
    level = matcher.getBookLevel(price, false);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 3U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 10U);
    ASSERT_EQ(level.second[1]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[1]).get(), 3U);
    ASSERT_EQ(level.second[2]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[2]).get(), 20U);
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    // a2 - price = 9241, qty = 2
    //
    listener.clear();

    a2 = OrderHelper::getSimSellLimitWithClOrderIds(price.get(), 2);
    matcher.sendNew(a2);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 2U);
    ASSERT_TRUE(checkOrder(a2, matcher.getAsks(), 4U, listener._newAcks, matcher));
    
    level = matcher.getBookLevel(price, false);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 4U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 10U);
    ASSERT_EQ(level.second[1]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[1]).get(), 3U);
    ASSERT_EQ(level.second[2]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[2]).get(), 20U);
    ASSERT_EQ(level.second[3]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[3]).get(), 2U);
    
    ASSERT_EQ(listener._newAcks.size(), 1U);
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // Change quote with a1 = 5@9241
    //
    listener.clear();
    
    size.set(5);
    quote.clearFlag();
    quote.setAsk(price, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 2U);
    
    level = matcher.getBookLevel(price, false);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 3U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 5U);
    ASSERT_EQ(level.second[1]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[1]).get(), 3U);
    ASSERT_EQ(level.second[2]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[2]).get(), 2U);
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // Change quote with a1 = 40@9241
    //
    listener.clear();
    
    size.set(40);
    quote.clearFlag();
    quote.setAsk(price, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 2U);
    
    level = matcher.getBookLevel(price, false);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 4U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 5U);
    ASSERT_EQ(level.second[1]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[1]).get(), 3U);
    ASSERT_EQ(level.second[2]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[2]).get(), 2U);
    ASSERT_EQ(level.second[3]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[3]).get(), 35U);
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // Change quote with a1 = 9@9241
    //
    listener.clear();
    
    size.set(9);
    quote.clearFlag();
    quote.setAsk(price, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 2U);
    
    level = matcher.getBookLevel(price, false);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 3U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 3U);
    ASSERT_EQ(level.second[1]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[1]).get(), 2U);
    ASSERT_EQ(level.second[2]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[2]).get(), 9U);
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // a3 - price = 9241+1, qty = 4
    //
    listener.clear();

    a3 = OrderHelper::getSimSellLimitWithClOrderIds(price.get()+1, 4);
    matcher.sendNew(a3);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 3U);
    ASSERT_TRUE(checkOrder(a3, matcher.getAsks(), 1U, listener._newAcks, matcher));
    
    level = matcher.getBookLevel(price, false);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 3U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 3U);
    ASSERT_EQ(level.second[1]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[1]).get(), 2U);
    ASSERT_EQ(level.second[2]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[2]).get(), 9U);
    
    ASSERT_EQ(listener._newAcks.size(), 1U);
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // Change quote with a1 = 15@9241
    //
    listener.clear();
    
    size.set(15);
    quote.clearFlag();
    quote.setAsk(price, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 3U);
    
    level = matcher.getBookLevel(price, false);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 3U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 3U);
    ASSERT_EQ(level.second[1]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[1]).get(), 2U);
    ASSERT_EQ(level.second[2]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[2]).get(), 15U);
    
    
    level = matcher.getBookLevel(price+1, false);
    ASSERT_EQ(level.first._price, price+1);
    ASSERT_EQ(level.first._size, 0U);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 4U);
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // Change quote with a1 = 23@9241
    //
    listener.clear();
    
    size.set(23);
    quote.clearFlag();
    quote.setAsk(price, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 3U);
    
    level = matcher.getBookLevel(price, false);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 3U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 3U);
    ASSERT_EQ(level.second[1]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[1]).get(), 2U);
    ASSERT_EQ(level.second[2]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[2]).get(), 23U);
    
    level = matcher.getBookLevel(price+1, false);
    ASSERT_EQ(level.first._price, price+1);
    ASSERT_EQ(level.first._size, 0U);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 4U);
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // Change quote with a1 = 34@9241
    //
    listener.clear();
    
    size.set(34);
    quote.clearFlag();
    quote.setAsk(price, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 3U);
    
    level = matcher.getBookLevel(price, false);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 3U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 3U);
    ASSERT_EQ(level.second[1]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[1]).get(), 2U);
    ASSERT_EQ(level.second[2]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[2]).get(), 34U);    
    
    level = matcher.getBookLevel(price+1, false);
    ASSERT_EQ(level.first._price, price+1);
    ASSERT_EQ(level.first._size, 0U);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 4U);
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // Change quote with a1 = 9@9241
    //
    listener.clear();
    
    size.set(9);
    quote.clearFlag();
    quote.setAsk(price, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 3U);
    
    level = matcher.getBookLevel(price, false);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 3U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 3U);
    ASSERT_EQ(level.second[1]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[1]).get(), 2U);
    ASSERT_EQ(level.second[2]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[2]).get(), 9U);
    
    level = matcher.getBookLevel(price+1, false);
    ASSERT_EQ(level.first._price, price+1);
    ASSERT_EQ(level.first._size, 0U);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 4U);
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    // Change quote with trade = 16@9241+1 - will cross with 5 orders:
    // 2 limit and 2 sim @9241 and 1 limit @9240
    //
    listener.clear();
    
    size.set(16);
    quote.clearFlag();
    quote.setTrade(price+1, size);
    
    matcher.onQuote(quote);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 1U);
    
    level = matcher.getBookLevel(price, false);
    ASSERT_EQ(level.first._price, tw::price::Ticks::INVALID_VALUE());
    ASSERT_EQ(level.first._size, 0U);
    
    ASSERT_EQ(level.second.size(), 0U);
    
    level = matcher.getBookLevel(price+1, false);
    ASSERT_EQ(level.first._price, price+1);
    ASSERT_EQ(level.first._size, 0U);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 2U);
    
    ASSERT_EQ(listener._fills.size(), 3U);
    ASSERT_TRUE(checkFill(listener._fills[0], createFill(price, tw::price::Size(3), a1, tw::channel_or::eLiqInd::kAdd)));     
    ASSERT_TRUE(checkFill(listener._fills[1], createFill(price, tw::price::Size(2), a2, tw::channel_or::eLiqInd::kAdd)));     
    ASSERT_TRUE(checkFill(listener._fills[2], createFill(price+1, tw::price::Size(2), a3, tw::channel_or::eLiqInd::kAdd))); 
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    
    
    // a4 - price = 9241+2, qty = 4
    //
    listener.clear();

    a4 = OrderHelper::getSimSellLimitWithClOrderIds(price.get()+2, 4);
    matcher.sendNew(a4);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 2U);
    ASSERT_TRUE(checkOrder(a4, matcher.getAsks(), 1U, listener._newAcks, matcher));
    
    level = matcher.getBookLevel(price+1, false);
    ASSERT_EQ(level.first._price, price+1);
    ASSERT_EQ(level.first._size, 0U);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 2U);
    
    level = matcher.getBookLevel(price+2, false);
    ASSERT_EQ(level.first._price, price+2);
    ASSERT_EQ(level.first._size, 0U);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 4U);    
    
    ASSERT_EQ(listener._newAcks.size(), 1U);
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    // Check quote crossing (may never happen with CME or other exchange prices, but nevertheless)
    //
    listener.clear();
    
    size.set(20);
    quote.clearFlag();
    quote.setBid(price+2, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 0U);
    
    level = matcher.getBookLevel(price+1, false);
    ASSERT_EQ(level.first._price, tw::price::Ticks::INVALID_VALUE());
    ASSERT_EQ(level.first._size, 0U);
    
    ASSERT_EQ(level.second.size(), 0U);
    
    level = matcher.getBookLevel(price+2, false);
    ASSERT_EQ(level.first._price, tw::price::Ticks::INVALID_VALUE());
    ASSERT_EQ(level.first._size, 0U);
    
    ASSERT_EQ(level.second.size(), 0U);
    
    level = matcher.getBookLevel(price+2, true);
    ASSERT_EQ(level.first._price, price+2);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 20U);
    
    ASSERT_EQ(listener._fills.size(), 2U);    
    ASSERT_TRUE(checkFill(listener._fills[0], createFill(price+1, tw::price::Size(2), a3, tw::channel_or::eLiqInd::kAdd)));         
    ASSERT_TRUE(checkFill(listener._fills[1], createFill(price+2, tw::price::Size(4), a4, tw::channel_or::eLiqInd::kAdd)));     
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
}

TEST(ExchangeSimLibTestSuit, matcher_bug_trade_quotes_bids)
{
    TestListener listener;    
    tw::instr::InstrumentPtr instrument = InstrHelper::getNQH2();
    
    TMatcher matcher(listener, instrument);
    
    tw::price::Ticks price(9241);
    tw::price::Size size;
    TBookLevel level;
    
    TQuote quote;
    
    quote._instrument = instrument.get();
    quote._instrumentId = quote._instrument->_keyId;
    
    
    // Test bids
    //
    
    // Add quote with b1 = 10@9241
    //
    size.set(10);
    quote.setBid(price, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_TRUE(matcher.getAllOpenOrders().empty());
    
    level = matcher.getBookLevel(price, true);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), size.get());
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // Change quote with trade = 4@9241 - will cross with 1 order:
    // 1 sim @9241
    //
    listener.clear();
    
    size.set(4);
    quote.clearFlag();
    quote.setTrade(price, size);
    
    matcher.onQuote(quote);
    ASSERT_TRUE(matcher.getAllOpenOrders().empty());
    
    level = matcher.getBookLevel(price, true);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, 10U);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 6U);
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // Change quote with b1 = 12@9241
    //
    size.set(12);
    quote.clearFlag();
    quote.setBid(price, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_TRUE(matcher.getAllOpenOrders().empty());
    
    level = matcher.getBookLevel(price, true);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), size.get());
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    // Change quote with trade = 1@9241 - will cross with 1 order:
    // 1 sim @9241
    //
    listener.clear();
    
    size.set(1);
    quote.clearFlag();
    quote.setTrade(price, size);
    
    matcher.onQuote(quote);
    ASSERT_TRUE(matcher.getAllOpenOrders().empty());
    
    level = matcher.getBookLevel(price, true);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, 12U);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 11U);
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // Change quote with b1 = 11@9241
    //
    size.set(11);
    quote.clearFlag();
    quote.setBid(price, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_TRUE(matcher.getAllOpenOrders().empty());
    
    level = matcher.getBookLevel(price, true);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, 11U);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 11U);
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());

}

TEST(ExchangeSimLibTestSuit, matcher_bug_trade_quotes_asks)
{
    TestListener listener;    
    tw::instr::InstrumentPtr instrument = InstrHelper::getNQH2();
    
    TMatcher matcher(listener, instrument);
    
    tw::price::Ticks price(9241);
    tw::price::Size size;
    TBookLevel level;
    
    TQuote quote;
    
    quote._instrument = instrument.get();
    quote._instrumentId = quote._instrument->_keyId;
    
    
    // Test bids
    //
    
    // Add quote with a1 = 10@9241
    //
    size.set(10);
    quote.setAsk(price, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_TRUE(matcher.getAllOpenOrders().empty());
    
    level = matcher.getBookLevel(price, false);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), size.get());
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // Change quote with trade = 4@9241 - will cross with 1 order:
    // 1 sim @9241
    //
    listener.clear();
    
    size.set(4);
    quote.clearFlag();
    quote.setTrade(price, size);
    
    matcher.onQuote(quote);
    ASSERT_TRUE(matcher.getAllOpenOrders().empty());
    
    level = matcher.getBookLevel(price, false);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, 10U);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 6U);
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // Change quote with a1 = 12@9241
    //
    size.set(12);
    quote.clearFlag();
    quote.setAsk(price, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_TRUE(matcher.getAllOpenOrders().empty());
    
    level = matcher.getBookLevel(price, false);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), size.get());
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // Change quote with trade = 1@9241 - will cross with 1 order:
    // 1 sim @9241
    //
    listener.clear();
    
    size.set(1);
    quote.clearFlag();
    quote.setTrade(price, size);
    
    matcher.onQuote(quote);
    ASSERT_TRUE(matcher.getAllOpenOrders().empty());
    
    level = matcher.getBookLevel(price, false);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, 12U);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 11U);
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // Change quote with a1 = 11@9241
    //
    size.set(11);
    quote.clearFlag();
    quote.setAsk(price, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_TRUE(matcher.getAllOpenOrders().empty());
    
    level = matcher.getBookLevel(price, false);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, 11U);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 11U);
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());

}


TEST(ExchangeSimLibTestSuit, matcher_bug_delta_size_bids)
{
    TestListener listener;    
    tw::instr::InstrumentPtr instrument = InstrHelper::getNQH2();
    
    TMatcher matcher(listener, instrument);
    
    tw::price::Ticks price(9241);
    tw::price::Size size;
    TBookLevel level;
    
    TQuote quote;
    
    quote._instrument = instrument.get();
    quote._instrumentId = quote._instrument->_keyId;
    
    TOrderPtr b1;
    
    
    // Test bids
    //
    
    // Add quote with b1 = 10@9241
    //
    size.set(10);
    quote.setBid(price, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_TRUE(matcher.getAllOpenOrders().empty());
    
    level = matcher.getBookLevel(price, true);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), size.get());
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // b1 - price = 9241, qty = 3
    //
    listener.clear();

    b1 = OrderHelper::getSimBuyLimitWithClOrderIds(price.get(), 3);
    matcher.sendNew(b1);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 1U);
    ASSERT_TRUE(checkOrder(b1, matcher.getBids(), 2U, listener._newAcks, matcher));
    
    level = matcher.getBookLevel(price, true);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 2U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 10U);
    ASSERT_EQ(level.second[1]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[1]).get(), 3U);
    
    ASSERT_EQ(listener._newAcks.size(), 1U);
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // Set the quote with the same b1 = 10@9241
    //
    listener.clear();
    
    size.set(10);
    quote.clearFlag();
    quote.setBid(price, size, 0, 2);
    
    matcher.onQuote(quote);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 1U);
    
    level = matcher.getBookLevel(price, true);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 2U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 10U);
    ASSERT_EQ(level.second[1]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[1]).get(), 3U);
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());

}


TEST(ExchangeSimLibTestSuit, matcher_bug_delta_size_asks)
{
    TestListener listener;    
    tw::instr::InstrumentPtr instrument = InstrHelper::getNQH2();
    
    TMatcher matcher(listener, instrument);
    
    tw::price::Ticks price(9241);
    tw::price::Size size;
    TBookLevel level;
    
    TQuote quote;
    
    quote._instrument = instrument.get();
    quote._instrumentId = quote._instrument->_keyId;
    
    TOrderPtr a1;
    
    
    // Test asks
    //
    
    // Add quote with a1 = 10@9241
    //
    size.set(10);
    quote.setAsk(price, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_TRUE(matcher.getAllOpenOrders().empty());
    
    level = matcher.getBookLevel(price, false);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), size.get());
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // a1 - price = 9241, qty = 3
    //
    listener.clear();

    a1 = OrderHelper::getSimSellLimitWithClOrderIds(price.get(), 3);
    matcher.sendNew(a1);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 1U);
    ASSERT_TRUE(checkOrder(a1, matcher.getAsks(), 2U, listener._newAcks, matcher));
    
    level = matcher.getBookLevel(price, false);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 2U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 10U);
    ASSERT_EQ(level.second[1]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[1]).get(), 3U);
    
    ASSERT_EQ(listener._newAcks.size(), 1U);
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // Set the quote with the same a1 = 10@9241
    //
    listener.clear();
    
    size.set(10);
    quote.clearFlag();
    quote.setAsk(price, size, 0, 2);
    
    matcher.onQuote(quote);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 1U);
    
    level = matcher.getBookLevel(price, false);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 2U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 10U);
    ASSERT_EQ(level.second[1]->_type, tw::channel_or::eOrderType::kLimit);
    ASSERT_EQ(getOpenOrderQty(level.second[1]).get(), 3U);
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());

}


TEST(ExchangeSimLibTestSuit, matcher_bug_limit_cross_bids)
{
    TestListener listener;    
    tw::instr::InstrumentPtr instrument = InstrHelper::getNQH2();
    
    TMatcher matcher(listener, instrument);
    
    tw::price::Ticks price(9241);
    tw::price::Size size;
    TBookLevel level;
    
    TQuote quote;
    
    quote._instrument = instrument.get();
    quote._instrumentId = quote._instrument->_keyId;
    
    TOrderPtr a1;
    
    
    // Test bids
    //
    
    // Add quote with b1 = 10@9241
    //
    size.set(10);
    quote.setBid(price, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_TRUE(matcher.getAllOpenOrders().empty());
    
    level = matcher.getBookLevel(price, true);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), size.get());
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    
    // Add quote with b2 = 12@9241 - 1
    //
    quote.clearFlag();
    quote.setBid(price-1, size+2, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_TRUE(matcher.getAllOpenOrders().empty());
    
    level = matcher.getBookLevel(price, true);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), size.get());
    
    level = matcher.getBookLevel(price-1, true);
    ASSERT_EQ(level.first._price, price-1);
    ASSERT_EQ(level.first._size, size+2);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), size.get()+2);
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // a1 - price = 9241-1, qty = 14 - takes bids out completely
    // on b1 and partially on b2
    //
    listener.clear();

    a1 = OrderHelper::getSimSellLimitWithClOrderIds(price.get()-1, 14);
    matcher.sendNew(a1);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 0U);
    
    level = matcher.getBookLevel(price, true);
    ASSERT_EQ(level.first._price, tw::price::Ticks::INVALID_VALUE());
    ASSERT_EQ(level.first._size, 0U);
    
    level = matcher.getBookLevel(price-1, true);
    ASSERT_EQ(level.first._price, price-1);
    ASSERT_EQ(level.first._size, 12U);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 8U);
    
    ASSERT_EQ(listener._newAcks.size(), 1U);
    ASSERT_EQ(listener._fills.size(), 2U);
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());

}

TEST(ExchangeSimLibTestSuit, matcher_bug_limit_cross_asks)
{
    TestListener listener;    
    tw::instr::InstrumentPtr instrument = InstrHelper::getNQH2();
    
    TMatcher matcher(listener, instrument);
    
    tw::price::Ticks price(9241);
    tw::price::Size size;
    TBookLevel level;
    
    TQuote quote;
    
    quote._instrument = instrument.get();
    quote._instrumentId = quote._instrument->_keyId;
    
    TOrderPtr b1;
    
    
    // Test asks
    //
    
    // Add quote with a1 = 10@9241
    //
    size.set(10);
    quote.setAsk(price, size, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_TRUE(matcher.getAllOpenOrders().empty());
    
    level = matcher.getBookLevel(price, false);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), size.get());
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // Add quote with a2 = 12@9241 + 1
    //
    quote.clearFlag();
    quote.setAsk(price+1, size+2, 0, 1);
    
    matcher.onQuote(quote);
    ASSERT_TRUE(matcher.getAllOpenOrders().empty());
    
    level = matcher.getBookLevel(price, false);
    ASSERT_EQ(level.first._price, price);
    ASSERT_EQ(level.first._size, size);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), size.get());
    
    level = matcher.getBookLevel(price+1, false);
    ASSERT_EQ(level.first._price, price+1);
    ASSERT_EQ(level.first._size, size+2);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), size.get()+2);
    
    ASSERT_TRUE(listener._newAcks.empty());
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());
    ASSERT_TRUE(listener._fills.empty());
    
    
    // b1 - price = 9241+1, qty = 14 - takes bids out completely
    //
    listener.clear();

    b1 = OrderHelper::getSimBuyLimitWithClOrderIds(price.get()+1, 14);
    matcher.sendNew(b1);
    ASSERT_EQ(matcher.getAllOpenOrders().size(), 0U);
    
    level = matcher.getBookLevel(price, false);
    ASSERT_EQ(level.first._price, tw::price::Ticks::INVALID_VALUE());
    ASSERT_EQ(level.first._size, 0U);
    
    level = matcher.getBookLevel(price+1, false);
    ASSERT_EQ(level.first._price, price+1);
    ASSERT_EQ(level.first._size, 12U);
    
    ASSERT_EQ(level.second.size(), 1U);
    ASSERT_EQ(level.second[0]->_type, tw::channel_or::eOrderType::kSim);
    ASSERT_EQ(getOpenOrderQty(level.second[0]).get(), 8U);
    
    ASSERT_EQ(listener._newAcks.size(), 1U);
    ASSERT_EQ(listener._fills.size(), 2U);
    ASSERT_TRUE(listener._modAcks.empty());
    ASSERT_TRUE(listener._cxlAcks.empty());

}

