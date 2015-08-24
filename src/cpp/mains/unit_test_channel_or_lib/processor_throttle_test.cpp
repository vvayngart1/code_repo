#include <tw/channel_or/processor_throttle.h>

#include "order_helper.h"

#include <gtest/gtest.h>

typedef tw::channel_or::TOrderPtr TOrderPtr;
typedef tw::channel_or::Reject Reject;

typedef tw::channel_or::ProcessorThrottle TProcessorThrottle;

TEST(ChannelOrLibTestSuit, processor_throttle)
{
    TOrderPtr o;
    Reject rej;
    tw::risk::Account account;
    
    o = OrderHelper::getEmpty();
    account._maxMPSNew = 2;
    account._maxMPSMod = 3;
    account._maxMPSCxl = 5;
    
    TProcessorThrottle& p = TProcessorThrottle::instance();    
    
    ASSERT_TRUE(p.init(account));
    ASSERT_TRUE(p.start());
    
    ASSERT_TRUE(p.sendNew(o, rej));
    ASSERT_EQ(o->_newMPS, 1U);
    ASSERT_EQ(o->_modMPS, 0U);
    ASSERT_EQ(o->_cxlMPS, 0U);
    
    ASSERT_TRUE(p.sendNew(o, rej));
    ASSERT_EQ(o->_newMPS, 2U);
    ASSERT_EQ(o->_modMPS, 0U);
    ASSERT_EQ(o->_cxlMPS, 0U);
    
    ASSERT_TRUE(p.sendMod(o, rej));
    ASSERT_EQ(o->_newMPS, 2U);
    ASSERT_EQ(o->_modMPS, 1U);
    ASSERT_EQ(o->_cxlMPS, 0U);
    
    ASSERT_TRUE(p.sendMod(o, rej));
    ASSERT_EQ(o->_newMPS, 2U);
    ASSERT_EQ(o->_modMPS, 2U);
    ASSERT_EQ(o->_cxlMPS, 0U);
    
    ASSERT_TRUE(p.sendMod(o, rej));
    ASSERT_EQ(o->_newMPS, 2U);
    ASSERT_EQ(o->_modMPS, 3U);
    ASSERT_EQ(o->_cxlMPS, 0U);
    
    ASSERT_TRUE(p.sendCxl(o, rej));
    ASSERT_EQ(o->_newMPS, 2U);
    ASSERT_EQ(o->_modMPS, 3U);
    ASSERT_EQ(o->_cxlMPS, 1U);
    
    ASSERT_TRUE(p.sendCxl(o, rej));
    ASSERT_EQ(o->_newMPS, 2U);
    ASSERT_EQ(o->_modMPS, 3U);
    ASSERT_EQ(o->_cxlMPS, 2U);
    
    ASSERT_TRUE(p.sendCxl(o, rej));
    ASSERT_EQ(o->_newMPS, 2U);
    ASSERT_EQ(o->_modMPS, 3U);
    ASSERT_EQ(o->_cxlMPS, 3U);
    
    ASSERT_TRUE(p.sendCxl(o, rej));
    ASSERT_EQ(o->_newMPS, 2U);
    ASSERT_EQ(o->_modMPS, 3U);
    ASSERT_EQ(o->_cxlMPS, 4U);
    
    ASSERT_TRUE(p.sendCxl(o, rej));
    ASSERT_EQ(o->_newMPS, 2U);
    ASSERT_EQ(o->_modMPS, 3U);
    ASSERT_EQ(o->_cxlMPS, 5U);
    
    ASSERT_TRUE(!p.sendCxl(o, rej));
    ASSERT_EQ(o->_newMPS, 2U);
    ASSERT_EQ(o->_modMPS, 3U);
    ASSERT_EQ(o->_cxlMPS, 6U);
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorThrottle);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kExceededMaxMPS);
    
    p.onTimeout(1);
    
    ASSERT_TRUE(p.sendNew(o, rej));
    ASSERT_EQ(o->_newMPS, 1U);
    ASSERT_EQ(o->_modMPS, 0U);
    ASSERT_EQ(o->_cxlMPS, 0U);
    
    ASSERT_TRUE(p.sendNew(o, rej));
    ASSERT_EQ(o->_newMPS, 2U);
    ASSERT_EQ(o->_modMPS, 0U);
    ASSERT_EQ(o->_cxlMPS, 0U);
    
    ASSERT_TRUE(p.sendMod(o, rej));
    ASSERT_EQ(o->_newMPS, 2U);
    ASSERT_EQ(o->_modMPS, 1U);
    ASSERT_EQ(o->_cxlMPS, 0U);
    
    ASSERT_TRUE(p.sendMod(o, rej));
    ASSERT_EQ(o->_newMPS, 2U);
    ASSERT_EQ(o->_modMPS, 2U);
    ASSERT_EQ(o->_cxlMPS, 0U);
    
    ASSERT_TRUE(p.sendMod(o, rej));
    ASSERT_EQ(o->_newMPS, 2U);
    ASSERT_EQ(o->_modMPS, 3U);
    ASSERT_EQ(o->_cxlMPS, 0U);
    
    ASSERT_TRUE(!p.sendNew(o, rej));
    ASSERT_EQ(o->_newMPS, 3U);
    ASSERT_EQ(o->_modMPS, 3U);
    ASSERT_EQ(o->_cxlMPS, 0U);
    ASSERT_TRUE(!p.isEnabled());
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorThrottle);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kExceededMaxMPS);
    
    ASSERT_TRUE(!p.sendMod(o, rej));
    ASSERT_EQ(o->_newMPS, 3U);
    ASSERT_EQ(o->_modMPS, 4U);
    ASSERT_EQ(o->_cxlMPS, 0U);
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorThrottle);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kExceededMaxMPS);
    
    ASSERT_TRUE(p.sendCxl(o, rej));
    ASSERT_EQ(o->_newMPS, 3U);
    ASSERT_EQ(o->_modMPS, 4U);
    ASSERT_EQ(o->_cxlMPS, 1U);
    
    ASSERT_TRUE(p.sendCxl(o, rej));
    ASSERT_EQ(o->_newMPS, 3U);
    ASSERT_EQ(o->_modMPS, 4U);
    ASSERT_EQ(o->_cxlMPS, 2U);
    
    ASSERT_TRUE(p.sendCxl(o, rej));
    ASSERT_EQ(o->_newMPS, 3U);
    ASSERT_EQ(o->_modMPS, 4U);
    ASSERT_EQ(o->_cxlMPS, 3U);
    
    ASSERT_TRUE(p.sendCxl(o, rej));
    ASSERT_EQ(o->_newMPS, 3U);
    ASSERT_EQ(o->_modMPS, 4U);
    ASSERT_EQ(o->_cxlMPS, 4U);
    
    ASSERT_TRUE(p.sendCxl(o, rej));
    ASSERT_EQ(o->_newMPS, 3U);
    ASSERT_EQ(o->_modMPS, 4U);
    ASSERT_EQ(o->_cxlMPS, 5U);
    
    ASSERT_TRUE(!p.sendCxl(o, rej));
    ASSERT_EQ(o->_newMPS, 3U);
    ASSERT_EQ(o->_modMPS, 4U);
    ASSERT_EQ(o->_cxlMPS, 6U);
    
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorThrottle);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kExceededMaxMPS);
    
    p.onTimeout(1);
    
    ASSERT_TRUE(!p.isEnabled());
    
    ASSERT_TRUE(!p.sendNew(o, rej));
    ASSERT_EQ(o->_newMPS, 1U);
    ASSERT_EQ(o->_modMPS, 0U);
    ASSERT_EQ(o->_cxlMPS, 0U);
    
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorThrottle);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kExceededMaxMPS);
    
    ASSERT_TRUE(!p.sendMod(o, rej));
    ASSERT_EQ(o->_newMPS, 1U);
    ASSERT_EQ(o->_modMPS, 1U);
    ASSERT_EQ(o->_cxlMPS, 0U);
    
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorThrottle);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kExceededMaxMPS);
    
    ASSERT_TRUE(p.sendCxl(o, rej));
    ASSERT_EQ(o->_newMPS, 1U);
    ASSERT_EQ(o->_modMPS, 1U);
    ASSERT_EQ(o->_cxlMPS, 1U);
    
    ASSERT_TRUE(p.sendCxl(o, rej));
    ASSERT_EQ(o->_newMPS, 1U);
    ASSERT_EQ(o->_modMPS, 1U);
    ASSERT_EQ(o->_cxlMPS, 2U);
    
    ASSERT_TRUE(p.sendCxl(o, rej));
    ASSERT_EQ(o->_newMPS, 1U);
    ASSERT_EQ(o->_modMPS, 1U);
    ASSERT_EQ(o->_cxlMPS, 3U);
    
    ASSERT_TRUE(p.sendCxl(o, rej));
    ASSERT_EQ(o->_newMPS, 1U);
    ASSERT_EQ(o->_modMPS, 1U);
    ASSERT_EQ(o->_cxlMPS, 4U);
    
    ASSERT_TRUE(p.sendCxl(o, rej));
    ASSERT_EQ(o->_newMPS, 1U);
    ASSERT_EQ(o->_modMPS, 1U);
    ASSERT_EQ(o->_cxlMPS, 5U);
    
    ASSERT_TRUE(!p.sendCxl(o, rej));
    ASSERT_EQ(o->_newMPS, 1U);
    ASSERT_EQ(o->_modMPS, 1U);
    ASSERT_EQ(o->_cxlMPS, 6U);
    
    ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
    ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kProcessorThrottle);
    ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kExceededMaxMPS);
    
    p.stop();
}
