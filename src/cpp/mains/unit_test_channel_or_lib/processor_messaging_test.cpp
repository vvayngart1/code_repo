#include <tw/channel_or/processor_messaging.h>

#include "order_helper.h"

#include <gtest/gtest.h>

typedef tw::channel_or::TOrderPtr TOrderPtr;
typedef tw::channel_or::Reject Reject;
typedef tw::channel_or::Fill TFill;

typedef tw::channel_or::ProcessorMessaging TProcessorMessaging;

bool isEqual(const tw::channel_or::Messaging& rhs, const tw::channel_or::Messaging& lhs) {    
    EXPECT_EQ(rhs._newMsgs, lhs._newMsgs);
    if ( rhs._newMsgs != lhs._newMsgs)
        return false;
    
    EXPECT_EQ(rhs._modMsgs, lhs._modMsgs);
    if ( rhs._modMsgs != lhs._modMsgs)
        return false;
    
    EXPECT_EQ(rhs._cxlMsgs, lhs._cxlMsgs);
    if ( rhs._cxlMsgs != lhs._cxlMsgs)
        return false;
    
    EXPECT_EQ(rhs._totalVolume, lhs._totalVolume);
    if ( rhs._totalVolume != lhs._totalVolume)
        return false;
    
    return true;
}

TEST(ChannelOrLibTestSuit, processor_messaging)
{
    TProcessorMessaging& p = TProcessorMessaging::instance();
    p.clear();
    
    TOrderPtr s1_b1;
    TOrderPtr s2_b1;
    TOrderPtr s3_b1;
    
    TFill fill;

    Reject rej;
    tw::instr::InstrumentPtr instrNQM2 = InstrHelper::getNQM2();
    tw::instr::InstrumentPtr instrSIM2 = InstrHelper::getSIM2();
    tw::instr::InstrumentPtr instrZNM2 = InstrHelper::getZNM2();    
    
    s1_b1 = OrderHelper::getBuyLimit(tw::price::Ticks(1078), 1, instrNQM2);
    s2_b1 = OrderHelper::getBuyLimit(tw::price::Ticks(1089), 1, instrSIM2);
    s3_b1 = OrderHelper::getBuyLimit(tw::price::Ticks(1091), 1, instrZNM2);
    
    s1_b1->_accountId = s2_b1->_accountId = s3_b1->_accountId = 1;
    
    
    // Init messaging infos
    //
    tw::channel_or::MessagingForExchange msgNQ;    
    tw::channel_or::MessagingForExchange msgZN;
    
    tw::channel_or::MessagingForExchange* ptrMsgNQ = NULL;
    tw::channel_or::MessagingForExchange* ptrMsgSI = NULL;
    tw::channel_or::MessagingForExchange* ptrMsgZN = NULL;
    
    msgNQ._symbol = "NQ";
    msgNQ._exchange = tw::instr::eExchange::kCME;
    msgNQ._accountId = 1;
    msgNQ._newMsgs = 1;
    msgNQ._modMsgs = 2;
    msgNQ._cxlMsgs = 3;
    msgNQ._totalVolume = 4;
    
    msgZN._symbol = "ZB";
    msgZN._exchange = tw::instr::eExchange::kCME;
    msgZN._accountId = 1;
    msgZN._newMsgs = 11;
    msgZN._modMsgs = 12;
    msgZN._cxlMsgs = 13;
    msgZN._totalVolume = 14;
    
    // Check initialization
    //
    std::vector<tw::channel_or::MessagingForExchange> infos;
    infos.push_back(msgNQ);
    infos.push_back(msgZN);
    
    ASSERT_TRUE(p.init(infos));
    
    ptrMsgNQ = p.getInfo(instrNQM2->_keyId, 1);
    ptrMsgSI = p.getInfo(instrSIM2->_keyId, 1);
    ptrMsgZN = p.getInfo(instrZNM2->_keyId, 1);
    
    ASSERT_TRUE(ptrMsgNQ != NULL);
    ASSERT_EQ(ptrMsgNQ->_symbol, "NQ");
    ASSERT_EQ(ptrMsgNQ->_exchange, tw::instr::eExchange::kCME);
    ASSERT_EQ(ptrMsgNQ->_newMsgs, 1);
    ASSERT_EQ(ptrMsgNQ->_modMsgs, 2);
    ASSERT_EQ(ptrMsgNQ->_cxlMsgs, 3);
    ASSERT_EQ(ptrMsgNQ->_accountId, 1);
    ASSERT_EQ(ptrMsgNQ->_totalVolume, 4);
    
    ptrMsgSI = p.getInfo(instrSIM2->_keyId, 1);
    ASSERT_TRUE(ptrMsgSI != NULL);
    ASSERT_EQ(ptrMsgSI->_symbol, "SI");
    ASSERT_EQ(ptrMsgSI->_exchange, tw::instr::eExchange::kCME);
    ASSERT_EQ(ptrMsgSI->_newMsgs, 0);
    ASSERT_EQ(ptrMsgSI->_modMsgs, 0);
    ASSERT_EQ(ptrMsgSI->_cxlMsgs, 0);
    ASSERT_EQ(ptrMsgSI->_accountId, 1);
    ASSERT_EQ(ptrMsgSI->_totalVolume, 0);
    
    ASSERT_TRUE(ptrMsgZN != NULL);
    ASSERT_EQ(ptrMsgZN->_symbol, "ZB");
    ASSERT_EQ(ptrMsgZN->_exchange, tw::instr::eExchange::kCME);
    ASSERT_EQ(ptrMsgZN->_newMsgs, 11);
    ASSERT_EQ(ptrMsgZN->_modMsgs, 12);
    ASSERT_EQ(ptrMsgZN->_cxlMsgs, 13);
    ASSERT_EQ(ptrMsgNQ->_accountId, 1);
    ASSERT_EQ(ptrMsgZN->_totalVolume, 14);
    
    // Check new orders/rejects
    //
    ASSERT_TRUE(p.sendNew(s1_b1, rej));
    ASSERT_EQ(ptrMsgNQ->_newMsgs, 2);
    ASSERT_EQ(ptrMsgNQ->_modMsgs, 2);
    ASSERT_EQ(ptrMsgNQ->_cxlMsgs, 3);
    ASSERT_EQ(ptrMsgNQ->_accountId, 1);
    ASSERT_EQ(ptrMsgNQ->_totalVolume, 4);
    ASSERT_TRUE(isEqual(static_cast<tw::channel_or::Messaging&>(*s1_b1), static_cast<tw::channel_or::Messaging&>(*ptrMsgNQ)));
    
    rej._rejType = tw::channel_or::eRejectType::kExternal;
    p.onNewRej(s1_b1, rej);
    ASSERT_EQ(ptrMsgNQ->_newMsgs, 2);
    ASSERT_EQ(ptrMsgNQ->_modMsgs, 2);
    ASSERT_EQ(ptrMsgNQ->_cxlMsgs, 3);
    ASSERT_EQ(ptrMsgNQ->_accountId, 1);
    ASSERT_EQ(ptrMsgNQ->_totalVolume, 4);
    ASSERT_TRUE(isEqual(static_cast<tw::channel_or::Messaging&>(*s1_b1), static_cast<tw::channel_or::Messaging&>(*ptrMsgNQ)));
    
    rej._rejType = tw::channel_or::eRejectType::kInternal;
    p.onNewRej(s1_b1, rej);
    ASSERT_EQ(ptrMsgNQ->_newMsgs, 1);
    ASSERT_EQ(ptrMsgNQ->_modMsgs, 2);
    ASSERT_EQ(ptrMsgNQ->_cxlMsgs, 3);
    ASSERT_EQ(ptrMsgNQ->_accountId, 1);
    ASSERT_EQ(ptrMsgNQ->_totalVolume, 4);
    ASSERT_TRUE(isEqual(static_cast<tw::channel_or::Messaging&>(*s1_b1), static_cast<tw::channel_or::Messaging&>(*ptrMsgNQ)));
    
    // Check mod/rejects
    //
    ASSERT_TRUE(p.sendMod(s1_b1, rej));
    ASSERT_EQ(ptrMsgNQ->_newMsgs, 1);
    ASSERT_EQ(ptrMsgNQ->_modMsgs, 3);
    ASSERT_EQ(ptrMsgNQ->_cxlMsgs, 3);
    ASSERT_EQ(ptrMsgNQ->_accountId, 1);
    ASSERT_EQ(ptrMsgNQ->_totalVolume, 4);
    ASSERT_TRUE(isEqual(static_cast<tw::channel_or::Messaging&>(*s1_b1), static_cast<tw::channel_or::Messaging&>(*ptrMsgNQ)));
    
    rej._rejType = tw::channel_or::eRejectType::kExternal;
    p.onModRej(s1_b1, rej);
    ASSERT_EQ(ptrMsgNQ->_newMsgs, 1);
    ASSERT_EQ(ptrMsgNQ->_modMsgs, 3);
    ASSERT_EQ(ptrMsgNQ->_cxlMsgs, 3);
    ASSERT_EQ(ptrMsgNQ->_accountId, 1);
    ASSERT_EQ(ptrMsgNQ->_totalVolume, 4);
    ASSERT_TRUE(isEqual(static_cast<tw::channel_or::Messaging&>(*s1_b1), static_cast<tw::channel_or::Messaging&>(*ptrMsgNQ)));
    
    rej._rejType = tw::channel_or::eRejectType::kInternal;
    p.onModRej(s1_b1, rej);
    ASSERT_EQ(ptrMsgNQ->_newMsgs, 1);
    ASSERT_EQ(ptrMsgNQ->_modMsgs, 2);
    ASSERT_EQ(ptrMsgNQ->_cxlMsgs, 3);
    ASSERT_EQ(ptrMsgNQ->_accountId, 1);
    ASSERT_EQ(ptrMsgNQ->_totalVolume, 4);
    ASSERT_TRUE(isEqual(static_cast<tw::channel_or::Messaging&>(*s1_b1), static_cast<tw::channel_or::Messaging&>(*ptrMsgNQ)));
    
    // Check cxl/rejects
    //
    ASSERT_TRUE(p.sendCxl(s1_b1, rej));
    ASSERT_EQ(ptrMsgNQ->_newMsgs, 1);
    ASSERT_EQ(ptrMsgNQ->_modMsgs, 2);
    ASSERT_EQ(ptrMsgNQ->_cxlMsgs, 4);
    ASSERT_EQ(ptrMsgNQ->_accountId, 1);
    ASSERT_EQ(ptrMsgNQ->_totalVolume, 4);
    ASSERT_TRUE(isEqual(static_cast<tw::channel_or::Messaging&>(*s1_b1), static_cast<tw::channel_or::Messaging&>(*ptrMsgNQ)));
    
    rej._rejType = tw::channel_or::eRejectType::kExternal;
    p.onCxlRej(s1_b1, rej);
    ASSERT_EQ(ptrMsgNQ->_newMsgs, 1);
    ASSERT_EQ(ptrMsgNQ->_modMsgs, 2);
    ASSERT_EQ(ptrMsgNQ->_cxlMsgs, 4);
    ASSERT_EQ(ptrMsgNQ->_accountId, 1);
    ASSERT_EQ(ptrMsgNQ->_totalVolume, 4);
    ASSERT_TRUE(isEqual(static_cast<tw::channel_or::Messaging&>(*s1_b1), static_cast<tw::channel_or::Messaging&>(*ptrMsgNQ)));
    
    rej._rejType = tw::channel_or::eRejectType::kInternal;
    p.onCxlRej(s1_b1, rej);
    ASSERT_EQ(ptrMsgNQ->_newMsgs, 1);
    ASSERT_EQ(ptrMsgNQ->_modMsgs, 2);
    ASSERT_EQ(ptrMsgNQ->_cxlMsgs, 3);
    ASSERT_EQ(ptrMsgNQ->_accountId, 1);
    ASSERT_EQ(ptrMsgNQ->_totalVolume, 4);
    ASSERT_TRUE(isEqual(static_cast<tw::channel_or::Messaging&>(*s1_b1), static_cast<tw::channel_or::Messaging&>(*ptrMsgNQ)));
    
    // Fills
    //
    fill._instrumentId = instrNQM2->_keyId;
    fill._accountId = 1;
    fill._qty.set(2);
    p.onFill(fill);
    ASSERT_EQ(ptrMsgNQ->_newMsgs, 1);
    ASSERT_EQ(ptrMsgNQ->_modMsgs, 2);
    ASSERT_EQ(ptrMsgNQ->_cxlMsgs, 3);
    ASSERT_EQ(ptrMsgNQ->_accountId, 1);
    ASSERT_EQ(ptrMsgNQ->_totalVolume, 6);
    ASSERT_TRUE(isEqual(static_cast<tw::channel_or::Messaging&>(fill), static_cast<tw::channel_or::Messaging&>(*ptrMsgNQ)));
    
    
    // Check new order for another symbol w/o configured MessagingForExchange
    //
    ASSERT_TRUE(p.sendNew(s2_b1, rej));
    ASSERT_EQ(ptrMsgSI->_newMsgs, 1);
    ASSERT_EQ(ptrMsgSI->_modMsgs, 0);
    ASSERT_EQ(ptrMsgSI->_cxlMsgs, 0);
    ASSERT_EQ(ptrMsgSI->_accountId, 1);
    ASSERT_EQ(ptrMsgSI->_totalVolume, 0);
    ASSERT_TRUE(isEqual(static_cast<tw::channel_or::Messaging&>(*s2_b1), static_cast<tw::channel_or::Messaging&>(*ptrMsgSI)));

    
    // Check new order for another symbol w/ configured MessagingForExchange
    //
    ASSERT_TRUE(p.sendNew(s3_b1, rej));
    ASSERT_EQ(ptrMsgZN->_newMsgs, 12);
    ASSERT_EQ(ptrMsgZN->_modMsgs, 12);
    ASSERT_EQ(ptrMsgZN->_cxlMsgs, 13);
    ASSERT_EQ(ptrMsgZN->_accountId, 1);
    ASSERT_EQ(ptrMsgZN->_totalVolume, 14);
    ASSERT_TRUE(isEqual(static_cast<tw::channel_or::Messaging&>(*s3_b1), static_cast<tw::channel_or::Messaging&>(*ptrMsgZN)));

    
    
    p.stop();
}
