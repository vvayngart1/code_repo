#include <tw/channel_or/translator.h>

#include "order_helper.h"

#include <gtest/gtest.h>

TEST(ChannelOrLibTestSuit, translator)
{
    tw::channel_or::OrderResp resp;
    
    std::string orderId = "1111111";
    std::string clOrderId = "12345678";
    std::string rejReason = "test reject reason";
    std::string rejText = "test reject text";
    std::string exFillId = "2222222";
    std::string exFillRefId = "3333333";
    
    resp._exOrderId = orderId;
    resp._clOrderId = clOrderId;
    resp._exRejReason = rejReason;
    resp._exRejText = rejText;
    resp._lastPrice = "2522.50";
    resp._price = "2522.25";
    resp._lastShares = 5;
    resp._exFillId = exFillId;
    resp._exFillRefId = exFillRefId;
    
    // Test newAck
    //
    {
        tw::channel_or::TOrderPtr order = OrderHelper::getEmpty();
        ASSERT_TRUE(tw::channel_or::Translator::translateNewAck(resp, order));
        ASSERT_EQ(order->_exOrderId, orderId);        
    }
    
    // Test newRej
    //
    {
        tw::channel_or::TOrderPtr order = OrderHelper::getEmpty();
        tw::channel_or::Reject rej;
        ASSERT_TRUE(tw::channel_or::Translator::translateNewRej(resp, order, rej));
        ASSERT_EQ(order->_exOrderId, orderId);
        
        ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kExternal);
        ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kExchange);
        ASSERT_EQ(rej._text, rejReason+" :: "+rejText);
    }
    
    // Test modAck
    //
    {
        tw::channel_or::TOrderPtr order = OrderHelper::getEmpty();
        ASSERT_TRUE(tw::channel_or::Translator::translateModAck(resp, order));
        ASSERT_EQ(order->_exOrderId, orderId);
        ASSERT_EQ(order->_clOrderId, clOrderId);
        ASSERT_EQ(order->_origClOrderId, clOrderId);
        ASSERT_EQ(order->_newPrice.get(), 10089);
        ASSERT_EQ(order->_exNewPrice, 2522.25);
    }
    
    // Test modRej
    //
    {
        tw::channel_or::TOrderPtr order = OrderHelper::getEmpty();
        tw::channel_or::Reject rej;
        ASSERT_TRUE(tw::channel_or::Translator::translateModRej(resp, order, rej));
        ASSERT_EQ(order->_exOrderId, orderId);
        
        ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kExternal);
        ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kExchange);
        ASSERT_EQ(rej._text, rejReason+" :: "+rejText);
    }
    
    // Test cxlAck
    //
    {
        tw::channel_or::TOrderPtr order = OrderHelper::getEmpty();
        ASSERT_TRUE(tw::channel_or::Translator::translateCxlAck(resp, order));
        ASSERT_EQ(order->_exOrderId, orderId);        
    }
    
    // Test cxlRej
    //
    {
        tw::channel_or::TOrderPtr order = OrderHelper::getEmpty();
        tw::channel_or::Reject rej;
        ASSERT_TRUE(tw::channel_or::Translator::translateCxlRej(resp, order, rej));
        ASSERT_EQ(order->_exOrderId, orderId);
        
        ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kExternal);
        ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kExchange);
        ASSERT_EQ(rej._text, rejReason+" :: "+rejText);
    }
    
    // Test fill - added liquidity
    //
    {
        resp._type = tw::channel_or::eOrderRespType::kFill;
        resp._liqInd = tw::channel_or::eLiqInd::kAdd;
        tw::channel_or::TOrderPtr order = OrderHelper::getEmpty();
        tw::channel_or::Fill fill;
        fill._order = order;
        
        order->_accountId = 1;
        order->_strategyId = 2;
        order->_instrumentId = 6;
        order->_orderId = tw::common::generateUuid();
        order->_side = tw::channel_or::eOrderSide::kBuy;
        
        ASSERT_TRUE(tw::channel_or::Translator::translateFill(resp, fill));
        ASSERT_EQ(fill._type, tw::channel_or::eFillType::kNormal);
        ASSERT_EQ(fill._subType, tw::channel_or::eFillSubType::kOutright);
        ASSERT_EQ(fill._exchangeFillId, resp._exFillId);
        ASSERT_EQ(fill._price.get(), 10090);
        ASSERT_NEAR(fill._exPrice, 2522.50, 0.0000001);
        ASSERT_EQ(fill._qty.get(), 5U);
        
        ASSERT_EQ(fill._accountId, order->_accountId);
        ASSERT_EQ(fill._strategyId, order->_strategyId);
        ASSERT_EQ(fill._instrumentId, order->_instrumentId);
        ASSERT_EQ(fill._orderId, order->_orderId);
        ASSERT_EQ(fill._side, order->_side);
        ASSERT_EQ(fill._liqInd, tw::channel_or::eLiqInd::kAdd);
        ASSERT_EQ(fill._pickOff, false);
    }
    
    // Test fill - removed liquidity
    //
    {
        resp._type = tw::channel_or::eOrderRespType::kFill;
        resp._liqInd = tw::channel_or::eLiqInd::kRem;
        tw::channel_or::TOrderPtr order = OrderHelper::getEmpty();
        tw::channel_or::Fill fill;
        fill._order = order;
        
        order->_accountId = 1;
        order->_strategyId = 2;
        order->_instrumentId = 6;
        order->_orderId = tw::common::generateUuid();
        order->_side = tw::channel_or::eOrderSide::kBuy;
        order->_state = tw::channel_or::eOrderState::kCancelling;
        
        ASSERT_TRUE(tw::channel_or::Translator::translateFill(resp, fill));
        ASSERT_EQ(fill._type, tw::channel_or::eFillType::kNormal);
        ASSERT_EQ(fill._subType, tw::channel_or::eFillSubType::kOutright);
        ASSERT_EQ(fill._exchangeFillId, resp._exFillId);
        ASSERT_EQ(fill._price.get(), 10090);
        ASSERT_NEAR(fill._exPrice, 2522.50, 0.0000001);
        ASSERT_EQ(fill._qty.get(), 5U);
        
        ASSERT_EQ(fill._accountId, order->_accountId);
        ASSERT_EQ(fill._strategyId, order->_strategyId);
        ASSERT_EQ(fill._instrumentId, order->_instrumentId);
        ASSERT_EQ(fill._orderId, order->_orderId);
        ASSERT_EQ(fill._side, order->_side);
        ASSERT_EQ(fill._liqInd, tw::channel_or::eLiqInd::kRem);
        ASSERT_EQ(fill._pickOff, true);
    }
    
    // Test busted fill
    //
    {
        resp._type = tw::channel_or::eOrderRespType::kTradeBreak;
        tw::channel_or::TOrderPtr order = OrderHelper::getEmpty();
        tw::channel_or::Fill fill;
        fill._order = order;
        
        order->_accountId = 1;
        order->_strategyId = 2;
        order->_instrumentId = 6;
        order->_orderId = tw::common::generateUuid();
        order->_side = tw::channel_or::eOrderSide::kBuy;
        
        ASSERT_TRUE(tw::channel_or::Translator::translateFill(resp, fill));
        ASSERT_EQ(fill._type, tw::channel_or::eFillType::kBusted);
        ASSERT_EQ(fill._subType, tw::channel_or::eFillSubType::kOutright);
        ASSERT_EQ(fill._exchangeFillId, resp._exFillRefId);
        ASSERT_EQ(fill._price.get(), 10090);
        ASSERT_NEAR(fill._exPrice, 2522.50, 0.0000001);
        ASSERT_EQ(fill._qty.get(), 5U);
        
        ASSERT_EQ(fill._accountId, order->_accountId);
        ASSERT_EQ(fill._strategyId, order->_strategyId);
        ASSERT_EQ(fill._instrumentId, order->_instrumentId);
        ASSERT_EQ(fill._orderId, order->_orderId);
        ASSERT_EQ(fill._side, order->_side);
    }
    
    
    // Test untracked order
    //
    {
        resp._type = tw::channel_or::eOrderRespType::kNewAck;
        tw::channel_or::Alert alert;
        
        ASSERT_TRUE(tw::channel_or::Translator::translateUntrackedResp(resp, alert));
        ASSERT_EQ(alert._type, tw::channel_or::eAlertType::kUntrackedOrder);
        ASSERT_EQ(alert._text, resp.toStringVerbose());
    }
    
    // Test untracked fill
    //
    {
        resp._type = tw::channel_or::eOrderRespType::kFill;
        tw::channel_or::Alert alert;
        
        ASSERT_TRUE(tw::channel_or::Translator::translateUntrackedResp(resp, alert));
        ASSERT_EQ(alert._type, tw::channel_or::eAlertType::kUntrackedFill);
        ASSERT_EQ(alert._text, resp.toStringVerbose());
    }
    
}
