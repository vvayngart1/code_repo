#include <tw/channel_or/processor_wtp.h>
#include <tw/price/quote_store.h>

#include "order_helper.h"

#include <gtest/gtest.h>

typedef tw::channel_or::TOrderPtr TOrderPtr;
typedef tw::channel_or::Reject Reject;

typedef tw::channel_or::ProcessorWTP TProcessorWTP;

TEST(ChannelOrLibTestSuit, processor_wtp)
{
    TProcessorWTP& p = TProcessorWTP::instance();
    p.clear();
    
    TOrderPtr s1_b1;
    TOrderPtr s1_b2;
    TOrderPtr s1_b3;
    TOrderPtr s1_b4;
    TOrderPtr s1_b5;
    
    TOrderPtr s1_a1;
    TOrderPtr s1_a2;
    TOrderPtr s1_a3;
    
    TOrderPtr s2_b1;

    Reject rej;
    tw::instr::InstrumentPtr instrNQM2 = InstrHelper::getNQM2();
    tw::instr::InstrumentPtr instrZNM2 = InstrHelper::getZNM2();
    
    // Add 3 bids
    //
    s1_b1 = OrderHelper::getBuyLimit(tw::price::Ticks(1078), 1, instrNQM2);
    s1_b2 = OrderHelper::getBuyLimit(tw::price::Ticks(1089), 1, instrNQM2);
    s1_b3 = OrderHelper::getBuyLimit(tw::price::Ticks(1067), 1, instrNQM2);
    
    ASSERT_TRUE(p.sendNew(s1_b1, rej));
    s1_b1->_state = tw::channel_or::eOrderState::kWorking;
    
    ASSERT_TRUE(p.sendNew(s1_b2, rej));
    s1_b2->_state = tw::channel_or::eOrderState::kWorking;
    
    ASSERT_TRUE(p.sendNew(s1_b3, rej));
    s1_b3->_state = tw::channel_or::eOrderState::kWorking;
    
    TProcessorWTP::TOrdersBook& s1_ob = p.getOrCreateOrdersBook(instrNQM2->_keyId);
    ASSERT_EQ(s1_ob.first.size(), 3U);
    ASSERT_EQ(s1_ob.second.size(), 0U);
    
    
    // Start adding asks
    //
    s1_a1 = OrderHelper::getSellLimit(tw::price::Ticks(1098), 1, instrNQM2);
    
    ASSERT_TRUE(p.sendNew(s1_a1, rej));
    s1_a1->_state = tw::channel_or::eOrderState::kWorking;
    
    ASSERT_EQ(s1_ob.first.size(), 3U);
    ASSERT_EQ(s1_ob.second.size(), 1U);
    
    // Add ask below best bid price
    //
    s1_a2 = OrderHelper::getSellLimit(tw::price::Ticks(1080), 1, instrNQM2);
    rej.clear();
    ASSERT_TRUE(!p.sendNew(s1_a2, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorWTP);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kWashTradePrevention);
    
    ASSERT_EQ(s1_ob.first.size(), 3U);
    ASSERT_EQ(s1_ob.second.size(), 1U);
    
    // Add ask below best bid price
    //
    s1_a2 = OrderHelper::getSellLimit(tw::price::Ticks(1089), 1, instrNQM2);
    rej.clear();
    ASSERT_TRUE(!p.sendNew(s1_a2, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorWTP);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kWashTradePrevention);
    
    ASSERT_EQ(s1_ob.first.size(), 3U);
    ASSERT_EQ(s1_ob.second.size(), 1U);
    
    // Add ask above best bid price
    //
    s1_a2 = OrderHelper::getSellLimit(tw::price::Ticks(1095), 1, instrNQM2);    
    ASSERT_TRUE(p.sendNew(s1_a2, rej));
    s1_a2->_state = tw::channel_or::eOrderState::kWorking;
    
    ASSERT_EQ(s1_ob.first.size(), 3U);
    ASSERT_EQ(s1_ob.second.size(), 2U);
    
    // Test ask below best bid price for the only remaining bid
    //
    s1_b1->_state = tw::channel_or::eOrderState::kRejected;
    s1_b2->_state = tw::channel_or::eOrderState::kFilled;
    
    
    s1_a3 = OrderHelper::getSellLimit(tw::price::Ticks(1066), 1, instrNQM2);
    rej.clear();
    ASSERT_TRUE(!p.sendNew(s1_a3, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorWTP);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kWashTradePrevention);
    
    ASSERT_EQ(s1_ob.first.size(), 1U);
    ASSERT_EQ(s1_ob.second.size(), 2U);
    
    // Add another ask above bid price
    //
    s1_a3 = OrderHelper::getSellLimit(tw::price::Ticks(1086), 1, instrNQM2);
    ASSERT_TRUE(p.sendNew(s1_a3, rej));
    s1_a3->_state = tw::channel_or::eOrderState::kWorking;
    
    ASSERT_EQ(s1_ob.first.size(), 1U);
    ASSERT_EQ(s1_ob.second.size(), 3U);
    
    // Now try to cross asks
    //
    s1_b4 = OrderHelper::getBuyLimit(tw::price::Ticks(1087), 1, instrNQM2);
    
    rej.clear();
    ASSERT_TRUE(!p.sendNew(s1_b4, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorWTP);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kWashTradePrevention);
    
    ASSERT_EQ(s1_ob.first.size(), 1U);
    ASSERT_EQ(s1_ob.second.size(), 3U);
    
    
    // Add another bid below best ask and then try to modify it above best ask
    //
    s1_b4 = OrderHelper::getBuyLimit(tw::price::Ticks(1075), 1, instrNQM2);
    ASSERT_TRUE(p.sendNew(s1_b4, rej));
    s1_b4->_state = tw::channel_or::eOrderState::kWorking;
    
    ASSERT_EQ(s1_ob.first.size(), 2U);
    ASSERT_EQ(s1_ob.second.size(), 3U);
    
    s1_b4->_newPrice = tw::price::Ticks(1087);
    rej.clear();
    ASSERT_TRUE(!p.sendMod(s1_b4, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorWTP);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kWashTradePrevention);
    
    ASSERT_EQ(s1_ob.first.size(), 2U);
    ASSERT_EQ(s1_ob.second.size(), 3U);
    
    
    // Modify it below best ask
    //
    s1_b4->_newPrice = tw::price::Ticks(1077);
    
    rej.clear();
    ASSERT_TRUE(p.sendMod(s1_b4, rej));
    s1_b4->_state = tw::channel_or::eOrderState::kModifying;
    
    ASSERT_EQ(s1_ob.first.size(), 2U);
    ASSERT_EQ(s1_ob.second.size(), 3U);
    
    // Try to modify ask below best bid's newPrice but above working price
    //
    s1_a2->_newPrice = tw::price::Ticks(1076);
    rej.clear();
    ASSERT_TRUE(!p.sendMod(s1_a2, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorWTP);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kWashTradePrevention);
    
    ASSERT_EQ(s1_ob.first.size(), 2U);
    ASSERT_EQ(s1_ob.second.size(), 3U);
    
    
    // Modify best bid below it's working price
    //
    s1_b4->_newPrice = tw::price::Ticks(1072);
    
    rej.clear();
    ASSERT_TRUE(p.sendMod(s1_b4, rej));
    s1_b4->_state = tw::channel_or::eOrderState::kModifying;
    
    ASSERT_EQ(s1_ob.first.size(), 2U);
    ASSERT_EQ(s1_ob.second.size(), 3U);
    
    
    // Try to modify ask again with the same price, but now should go through
    //
    s1_a2->_newPrice = tw::price::Ticks(1076);
    rej.clear();
    ASSERT_TRUE(p.sendMod(s1_a2, rej));
    
    ASSERT_EQ(s1_ob.first.size(), 2U);
    ASSERT_EQ(s1_ob.second.size(), 3U);
    
    // Try to modify ask below best bid's working but above newPrice price
    //
    s1_a2->_newPrice = tw::price::Ticks(1075);
    rej.clear();
    ASSERT_TRUE(!p.sendMod(s1_a2, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorWTP);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kWashTradePrevention);
    
    ASSERT_EQ(s1_ob.first.size(), 2U);
    ASSERT_EQ(s1_ob.second.size(), 3U);
    
    // Send an order for another instrument
    //
    s2_b1 = OrderHelper::getBuyLimit(tw::price::Ticks(1278), 1, instrZNM2);
    ASSERT_TRUE(p.sendNew(s2_b1, rej));
    
    TProcessorWTP::TOrdersBook& s2_ob = p.getOrCreateOrdersBook(instrZNM2->_keyId);
    
    ASSERT_EQ(s1_ob.first.size(), 2U);
    ASSERT_EQ(s1_ob.second.size(), 3U);
    
    ASSERT_EQ(s2_ob.first.size(), 1U);
    ASSERT_EQ(s2_ob.second.size(), 0U);
    
    
    // Test stopLoss being let through
    //
    
    // Try to cross asks with normal order
    //
    s1_b4 = OrderHelper::getBuyLimit(tw::price::Ticks(1087), 1, instrNQM2);
    
    rej.clear();
    ASSERT_TRUE(!p.sendNew(s1_b4, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorWTP);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kWashTradePrevention);
    
    ASSERT_EQ(s1_ob.first.size(), 2U);
    ASSERT_EQ(s1_ob.second.size(), 3U);
    
    ASSERT_EQ(s2_ob.first.size(), 1U);
    ASSERT_EQ(s2_ob.second.size(), 0U);
    
    // Now set it to being stopLoss order
    //
    s1_b4 = OrderHelper::getBuyLimit(tw::price::Ticks(1087), 1, instrNQM2);
    s1_b4->_stopLoss = true;
    
    rej.clear();
    ASSERT_TRUE(p.sendNew(s1_b4, rej));
    
    ASSERT_EQ(s1_ob.first.size(), 3U);
    ASSERT_EQ(s1_ob.second.size(), 3U);
    
    ASSERT_EQ(s2_ob.first.size(), 1U);
    ASSERT_EQ(s2_ob.second.size(), 0U);
    
    // Test mod with normal order
    //
    s1_b4->_stopLoss = false;    
    s1_b4->_newPrice = tw::price::Ticks(1089);
    
    rej.clear();
    ASSERT_TRUE(!p.sendMod(s1_b4, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorWTP);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kWashTradePrevention);
    
    ASSERT_EQ(s1_ob.first.size(), 3U);
    ASSERT_EQ(s1_ob.second.size(), 3U);
    
    ASSERT_EQ(s2_ob.first.size(), 1U);
    ASSERT_EQ(s2_ob.second.size(), 0U);
    
    // Now set it to being stopLoss order
    //
    s1_b4->_stopLoss = true;    
    s1_b4->_newPrice = tw::price::Ticks(1089);
    
    rej.clear();
    ASSERT_TRUE(p.sendMod(s1_b4, rej));
    
    ASSERT_EQ(s1_ob.first.size(), 3U);
    ASSERT_EQ(s1_ob.second.size(), 3U);
    
    ASSERT_EQ(s2_ob.first.size(), 1U);
    ASSERT_EQ(s2_ob.second.size(), 0U);
    
    p.stop();
}

TEST(ChannelOrLibTestSuit, processor_wtp_w_quotes)
{
    TProcessorWTP& p = TProcessorWTP::instance();
    p.clear();
    
    tw::common::Settings settings;
    settings._trading_wtp_use_quotes = true;
    
    ASSERT_TRUE(p.init(settings));
    
    TOrderPtr s1_b1;
    TOrderPtr s1_b2;
    
    TOrderPtr s1_a1;
    TOrderPtr s1_a2;
    
    
    Reject rej;
    tw::instr::InstrumentPtr instrNQM2 = InstrHelper::getNQM2();
    
    tw::price::QuoteStore::TQuote& quote = tw::price::QuoteStore::instance().getQuote(instrNQM2);
    
    // Add bid
    //
    s1_b1 = OrderHelper::getBuyLimit(tw::price::Ticks(1078), 1, instrNQM2);
    
    ASSERT_TRUE(p.sendNew(s1_b1, rej));
    s1_b1->_state = tw::channel_or::eOrderState::kWorking;
    
    TProcessorWTP::TOrdersBook& s1_ob = p.getOrCreateOrdersBook(instrNQM2->_keyId);
    ASSERT_EQ(s1_ob.first.size(), 1U);
    ASSERT_EQ(s1_ob.second.size(), 0U);
    
    // Start adding asks
    //
    s1_a1 = OrderHelper::getSellLimit(tw::price::Ticks(1098), 1, instrNQM2);
    
    ASSERT_TRUE(p.sendNew(s1_a1, rej));
    s1_a1->_state = tw::channel_or::eOrderState::kWorking;
    
    ASSERT_EQ(s1_ob.first.size(), 1U);
    ASSERT_EQ(s1_ob.second.size(), 1U);
    
    // Add ask below best bid price
    //
    s1_a2 = OrderHelper::getSellLimit(tw::price::Ticks(1078), 1, instrNQM2);
    rej.clear();
    ASSERT_TRUE(!p.sendNew(s1_a2, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorWTP);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kWashTradePrevention);
    
    ASSERT_EQ(s1_ob.first.size(), 1U);
    ASSERT_EQ(s1_ob.second.size(), 1U);
    
    quote.setTrade(tw::price::Ticks(1079), tw::price::Size(1));
    
    s1_a2 = OrderHelper::getSellLimit(tw::price::Ticks(1078), 1, instrNQM2);
    rej.clear();
    ASSERT_TRUE(!p.sendNew(s1_a2, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorWTP);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kWashTradePrevention);
    
    ASSERT_EQ(s1_ob.first.size(), 1U);
    ASSERT_EQ(s1_ob.second.size(), 1U);
    
    quote.setBid(tw::price::Ticks(1078), tw::price::Size(1), 0);
    quote.setAsk(tw::price::Ticks(1080), tw::price::Size(1), 0);
    
    s1_a2 = OrderHelper::getSellLimit(tw::price::Ticks(1078), 1, instrNQM2);
    rej.clear();
    ASSERT_TRUE(!p.sendNew(s1_a2, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorWTP);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kWashTradePrevention);
    
    ASSERT_EQ(s1_ob.first.size(), 1U);
    ASSERT_EQ(s1_ob.second.size(), 1U);
    
    quote.setBid(tw::price::Ticks(1079), tw::price::Size(1), 0);
    
    s1_a2 = OrderHelper::getSellLimit(tw::price::Ticks(1078), 1, instrNQM2);
    rej.clear();
    ASSERT_TRUE(p.sendNew(s1_a2, rej));
    
    ASSERT_EQ(s1_ob.first.size(), 1U);
    ASSERT_EQ(s1_ob.second.size(), 2U);
    
    // Clear asks
    //
    s1_ob.second.clear();
    
    s1_a1 = OrderHelper::getSellLimit(tw::price::Ticks(1098), 1, instrNQM2);
    
    ASSERT_TRUE(p.sendNew(s1_a1, rej));
    s1_a1->_state = tw::channel_or::eOrderState::kWorking;
    
    ASSERT_EQ(s1_ob.first.size(), 1U);
    
    // Modify best bid above it's working price
    //
    s1_b1->_newPrice = tw::price::Ticks(1082);
    
    rej.clear();
    ASSERT_TRUE(p.sendMod(s1_b1, rej));
    s1_b1->_state = tw::channel_or::eOrderState::kModifying;
    
    ASSERT_EQ(s1_ob.first.size(), 1U);
    ASSERT_EQ(s1_ob.second.size(), 1U);
    
    s1_a2 = OrderHelper::getSellLimit(tw::price::Ticks(1082), 1, instrNQM2);
    rej.clear();
    ASSERT_TRUE(!p.sendNew(s1_a2, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorWTP);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kWashTradePrevention);
    
    ASSERT_EQ(s1_ob.first.size(), 1U);
    ASSERT_EQ(s1_ob.second.size(), 1U);
    
    quote.setTrade(tw::price::Ticks(1083), tw::price::Size(1));
    
    s1_a2 = OrderHelper::getSellLimit(tw::price::Ticks(1082), 1, instrNQM2);
    rej.clear();
    ASSERT_TRUE(!p.sendNew(s1_a2, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorWTP);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kWashTradePrevention);
    
    ASSERT_EQ(s1_ob.first.size(), 1U);
    ASSERT_EQ(s1_ob.second.size(), 1U);
    
    quote.setBid(tw::price::Ticks(1083), tw::price::Size(1), 0);
    quote.setAsk(tw::price::Ticks(1084), tw::price::Size(1), 0);
    
    s1_a2 = OrderHelper::getSellLimit(tw::price::Ticks(1082), 1, instrNQM2);
    rej.clear();
    ASSERT_TRUE(p.sendNew(s1_a2, rej));
    
    ASSERT_EQ(s1_ob.first.size(), 1U);
    ASSERT_EQ(s1_ob.second.size(), 2U);
    
    // Clear asks
    //
    s1_ob.second.clear();
    
    s1_a1 = OrderHelper::getSellLimit(tw::price::Ticks(1098), 1, instrNQM2);
    
    ASSERT_TRUE(p.sendNew(s1_a1, rej));
    s1_a1->_state = tw::channel_or::eOrderState::kWorking;
    
    ASSERT_EQ(s1_ob.first.size(), 1U);
    ASSERT_EQ(s1_ob.second.size(), 1U);
    
    // Clear quote
    //
    quote._trade.clear();
    quote._book[0].clear();
    
    // Now try to cross asks
    //
    s1_b2 = OrderHelper::getBuyLimit(tw::price::Ticks(1098), 1, instrNQM2);
    
    rej.clear();
    ASSERT_TRUE(!p.sendNew(s1_b2, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorWTP);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kWashTradePrevention);
    
    ASSERT_EQ(s1_ob.first.size(), 1U);
    ASSERT_EQ(s1_ob.second.size(), 1U);
    
    quote.setTrade(tw::price::Ticks(1097), tw::price::Size(1));
    
    s1_b2 = OrderHelper::getBuyLimit(tw::price::Ticks(1098), 1, instrNQM2);
    
    rej.clear();
    ASSERT_TRUE(!p.sendNew(s1_b2, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorWTP);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kWashTradePrevention);
    
    ASSERT_EQ(s1_ob.first.size(), 1U);
    ASSERT_EQ(s1_ob.second.size(), 1U);
    
    quote.setBid(tw::price::Ticks(1096), tw::price::Size(1), 0);
    quote.setAsk(tw::price::Ticks(1098), tw::price::Size(1), 0);
    
    s1_b2 = OrderHelper::getBuyLimit(tw::price::Ticks(1098), 1, instrNQM2);
    
    rej.clear();
    ASSERT_TRUE(!p.sendNew(s1_b2, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorWTP);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kWashTradePrevention);
    
    ASSERT_EQ(s1_ob.first.size(), 1U);
    ASSERT_EQ(s1_ob.second.size(), 1U);
    
    quote.setAsk(tw::price::Ticks(1098), tw::price::Size(1), 0);
    
    s1_b2 = OrderHelper::getBuyLimit(tw::price::Ticks(1097), 1, instrNQM2);
    
    rej.clear();
    ASSERT_TRUE(p.sendNew(s1_b2, rej));
    
    ASSERT_EQ(s1_ob.first.size(), 2U);
    ASSERT_EQ(s1_ob.second.size(), 1U);
    
    // Clear bids
    //
    s1_ob.first.clear();
    
    s1_b1 = OrderHelper::getBuyLimit(tw::price::Ticks(1078), 1, instrNQM2);
    
    ASSERT_TRUE(p.sendNew(s1_b1, rej));
    s1_b1->_state = tw::channel_or::eOrderState::kWorking;
    
    ASSERT_EQ(s1_ob.first.size(), 1U);
    ASSERT_EQ(s1_ob.second.size(), 1U);
    
    // Modify best ask below it's working price
    //
    s1_a1->_newPrice = tw::price::Ticks(1094);
    
    rej.clear();
    ASSERT_TRUE(p.sendMod(s1_a1, rej));
    s1_a1->_state = tw::channel_or::eOrderState::kModifying;
    
    ASSERT_EQ(s1_ob.first.size(), 1U);
    ASSERT_EQ(s1_ob.second.size(), 1U);
    
    s1_b2 = OrderHelper::getBuyLimit(tw::price::Ticks(1094), 1, instrNQM2);
    
    rej.clear();
    ASSERT_TRUE(!p.sendNew(s1_b2, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorWTP);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kWashTradePrevention);
    
    ASSERT_EQ(s1_ob.first.size(), 1U);
    ASSERT_EQ(s1_ob.second.size(), 1U);
    
    quote.setTrade(tw::price::Ticks(1093), tw::price::Size(1));
    
    s1_b2 = OrderHelper::getBuyLimit(tw::price::Ticks(1094), 1, instrNQM2);
    
    rej.clear();
    ASSERT_TRUE(!p.sendNew(s1_b2, rej));
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorWTP);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kWashTradePrevention);
    
    ASSERT_EQ(s1_ob.first.size(), 1U);
    ASSERT_EQ(s1_ob.second.size(), 1U);
    
    quote.setBid(tw::price::Ticks(1092), tw::price::Size(1), 0);
    quote.setAsk(tw::price::Ticks(1093), tw::price::Size(1), 0);
    
    
    s1_b2 = OrderHelper::getBuyLimit(tw::price::Ticks(1094), 1, instrNQM2);
    
    rej.clear();
    ASSERT_TRUE(p.sendNew(s1_b2, rej));
    
    ASSERT_EQ(s1_ob.first.size(), 2U);
    ASSERT_EQ(s1_ob.second.size(), 1U);
    
    
    p.stop();
}