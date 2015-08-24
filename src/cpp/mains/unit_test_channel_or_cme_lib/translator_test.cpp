#include <tw/channel_or_cme/translator.h>
#include <tw/channel_or_cme/channel_or_onix.h>

#include "../unit_test_channel_or_lib/order_helper.h"

#include <gtest/gtest.h>

tw::channel_or_cme::TSessionSettingsPtr getSessionSettings() {
    tw::channel_or_cme::TSessionSettingsPtr s;
    s.reset(new tw::channel_or_cme::SessionSettings("7E59Z1"));
    
    s->_host="10.135.70.92";
    s->_port=10400;
    s->_heartBeatInt=30;
    s->_password="7E5";
    s->_senderCompId="7E59Z1N";
    s->_senderSubId="AMR_TW";
    s->_senderLocationId="US,IL";
    s->_applicationSystemName="RosenthalSPOC";
    s->_applicationSystemVersion="1.0";
    s->_applicationSystemVendor="AMRSPOC";
    s->_account="82409802";
    s->_customerOrFirm="1";
    s->_ctiCode="2";
    s->_handlInst="1";
    s->_enabled=true;    
    
    return s;
}

tw::channel_or_cme::Settings getGlobalSettings() {
    tw::channel_or_cme::Settings s;
    
    s._global_fixDialectDescriptionFile = "/opt/tradework/onix/config/or/dialect_or.xml";
    s._global_schedulerSettingsFile = "/opt/tradework/onix/config/or/scheduler_or.xml";
    s._global_licenseStore="/opt/tradework/onix/license/";
    s._global_logDirectory="./";
    
    return s;
}

template <typename T>
bool checkField(const OnixS::FIX::Message& m, int32_t tag, const T& value) {
    EXPECT_TRUE(m.contain(tag));    
    if ( !m.contain(tag) )
        return false;
    
    EXPECT_EQ(m.get(tag), boost::lexical_cast<std::string>(value));
    return (m.get(tag) == boost::lexical_cast<std::string>(value));
}

void setOrderIds(OnixS::FIX::Message& m, const std::string& orderId) {
    m.set(OnixS::FIX::FIX42::Tags::ClOrdID, orderId);
    m.set(OnixS::FIX::FIX42::Tags::OrderID, orderId);
    m.set(OnixS::FIX::FIX42::Tags::OrigClOrdID, orderId);
    m.set(tw::channel_or_cme::CustomTags::CorrelationClOrdID, orderId);
}

bool checkOrderIds(const tw::channel_or::OrderResp& resp, const std::string& orderId, bool checkExOrderId = true) {
    EXPECT_EQ(resp._clOrderId, orderId);
    if ( resp._clOrderId != orderId )
        return false;
    
    EXPECT_EQ(resp._origClOrderId, orderId);
    if ( resp._origClOrderId != orderId )
        return false;
    
    EXPECT_EQ(resp._corrClOrderId, orderId);
    if ( resp._corrClOrderId != orderId )
        return false;
    
    if ( checkExOrderId ) {
        EXPECT_EQ(resp._exOrderId, orderId);
        if ( resp._exOrderId != orderId )
            return false;
    }
    
    return true;
}

TEST(ChannelOrCmeLibTestSuit, translator)
{
    ASSERT_TRUE(tw::channel_or_cme::ChannelOrOnix::global_init(getGlobalSettings()));
    
    tw::channel_or_cme::Translator t;
    tw::channel_or_cme::TSessionSettingsPtr settings = getSessionSettings();
    ASSERT_TRUE(t.init(settings));
    
    std::string rejText = "unit test reject";
    std::string testId = "12345678";
    
    // Test initialization
    //
    {
        OnixS::FIX::Message& m = t.getMsgNew();
        
        ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::MsgType, "D"));
        ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::HandlInst, "1"));
        ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::CustomerOrFirm, "1"));
        ASSERT_TRUE(checkField(m, tw::channel_or_cme::CustomTags::CtiCode, "2"));
        ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Account, "82409802"));
        ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::SecurityType, "FUT"));
        
        ASSERT_TRUE(t.shouldSend(m));
        m.set(OnixS::FIX::FIX42::Tags::PossDupFlag, "Y");
        ASSERT_TRUE(!t.shouldSend(m));
        m.remove(OnixS::FIX::FIX42::Tags::PossDupFlag);
    }
    
    {
        OnixS::FIX::Message& m = t.getMsgMod();
        
        ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::MsgType, "G"));
        ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::HandlInst, "1"));
        ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::CustomerOrFirm, "1"));
        ASSERT_TRUE(checkField(m, tw::channel_or_cme::CustomTags::CtiCode, "2"));
        ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Account, "82409802"));
        ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::SecurityType, "FUT"));
        ASSERT_TRUE(checkField(m, tw::channel_or_cme::CustomTags::OFMOverride, "Y"));
        
        ASSERT_TRUE(t.shouldSend(m));
        m.set(OnixS::FIX::FIX42::Tags::PossDupFlag, "Y");
        ASSERT_TRUE(!t.shouldSend(m));
        m.remove(OnixS::FIX::FIX42::Tags::PossDupFlag);
    }
    
    {
        OnixS::FIX::Message& m = t.getMsgCxl();
        
        ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::MsgType, "F"));
        ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Account, "82409802"));
        ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::SecurityType, "FUT"));
        
        ASSERT_TRUE(t.shouldSend(m));
        m.set(OnixS::FIX::FIX42::Tags::PossDupFlag, "Y");
        ASSERT_TRUE(t.shouldSend(m));
        m.remove(OnixS::FIX::FIX42::Tags::PossDupFlag);
    }
    
    // Test outbound limit/market buy
    //
    {
        tw::channel_or::TOrderPtr order = OrderHelper::getBuyLimit(9241, 2);
        order->_manual = true;
        ASSERT_TRUE(order->_origClOrderId.empty());
        
        tw::channel_or::Reject rej;
        
        ASSERT_TRUE(t.translateNew(order, rej));        
        {
            OnixS::FIX::Message& m = t.getMsgNew();
            ASSERT_TRUE(!order->_origClOrderId.empty());
            ASSERT_TRUE(order->_origClOrderId == order->_clOrderId);
            ASSERT_TRUE(order->_origClOrderId == order->_corrClOrderId);
            ASSERT_EQ(order->_exSessionName, settings->_senderCompId);
            ASSERT_NEAR(order->_exPrice, 2310.25, 0.000001);
            ASSERT_NEAR(order->_exNewPrice, 0.0, 0.000001);

            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrdType, "2"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Price, "2310.25"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrderQty, "2"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Side, "1"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Symbol, "NQ"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::SecurityDesc, "NQH2"));
            ASSERT_TRUE(checkField(m, tw::channel_or_cme::CustomTags::ManualOrderIndicator, "Y"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::ClOrdID, order->_clOrderId));
            
            ASSERT_TRUE(!m.contain(OnixS::FIX::FIX42::Tags::TimeInForce));
            ASSERT_TRUE(!m.contain(OnixS::FIX::FIX42::Tags::MinQty));
        }
        
        order->_newPrice.set(9242);
        order->_exOrderId = "1234567";
        
        ASSERT_TRUE(t.translateMod(order, rej));        
        {
            OnixS::FIX::Message& m = t.getMsgMod();
            ASSERT_TRUE(!order->_origClOrderId.empty());
            ASSERT_TRUE(!order->_clOrderId.empty());
            ASSERT_TRUE(order->_origClOrderId != order->_clOrderId);
            ASSERT_NEAR(order->_exPrice, 2310.25, 0.000001);
            ASSERT_NEAR(order->_exNewPrice, 2310.50, 0.000001);

            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrdType, "2"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Price, "2310.5"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrderQty, "2"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Side, "1"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Symbol, "NQ"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::SecurityDesc, "NQH2"));
            ASSERT_TRUE(checkField(m, tw::channel_or_cme::CustomTags::ManualOrderIndicator, "Y"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::ClOrdID, order->_clOrderId));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrderID, order->_exOrderId));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrigClOrdID, order->_origClOrderId));
            
            ASSERT_TRUE(!m.contain(OnixS::FIX::FIX42::Tags::TimeInForce));
            ASSERT_TRUE(!m.contain(OnixS::FIX::FIX42::Tags::MinQty));
        }
        
        order->_exOrderId = "12345678";
        ASSERT_TRUE(t.translateCxl(order, rej));        
        {
            OnixS::FIX::Message& m = t.getMsgCxl();
            ASSERT_TRUE(!order->_origClOrderId.empty());
            ASSERT_TRUE(!order->_clOrderId.empty());
            ASSERT_TRUE(order->_origClOrderId != order->_clOrderId);
            
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Side, "1"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Symbol, "NQ"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::SecurityDesc, "NQH2"));
            ASSERT_TRUE(checkField(m, tw::channel_or_cme::CustomTags::ManualOrderIndicator, "Y"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::ClOrdID, order->_clOrderId));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrderID, order->_exOrderId));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrigClOrdID, order->_origClOrderId));
        }
        
        order = OrderHelper::getBuyMarket(2);
        ASSERT_TRUE(!t.translateNew(order, rej));
        ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
        ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kTranslator);
        ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kType);
    }
    
    // Test outbound limit/market sell
    //
    {
        tw::channel_or::TOrderPtr order = OrderHelper::getSellLimit(9243, 4);
        order->_manual = true;
        ASSERT_TRUE(order->_origClOrderId.empty());
        
        tw::channel_or::Reject rej;
        
        ASSERT_TRUE(t.translateNew(order, rej));        
        {
            OnixS::FIX::Message& m = t.getMsgNew();
            ASSERT_TRUE(!order->_origClOrderId.empty());
            ASSERT_TRUE(order->_origClOrderId == order->_clOrderId);
            ASSERT_TRUE(order->_origClOrderId == order->_corrClOrderId);
            ASSERT_EQ(order->_exSessionName, settings->_senderCompId);
            ASSERT_NEAR(order->_exPrice, 2310.75, 0.000001);
            ASSERT_NEAR(order->_exNewPrice, 0.0, 0.000001);

            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrdType, "2"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Price, "2310.75"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrderQty, "4"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Side, "2"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Symbol, "NQ"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::SecurityDesc, "NQH2"));
            ASSERT_TRUE(checkField(m, tw::channel_or_cme::CustomTags::ManualOrderIndicator, "Y"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::ClOrdID, order->_clOrderId));
            
            ASSERT_TRUE(!m.contain(OnixS::FIX::FIX42::Tags::TimeInForce));
            ASSERT_TRUE(!m.contain(OnixS::FIX::FIX42::Tags::MinQty));
        }
        
        order->_newPrice.set(9244);
        order->_exOrderId = "1234567";
        
        ASSERT_TRUE(t.translateMod(order, rej));        
        {
            OnixS::FIX::Message& m = t.getMsgMod();
            ASSERT_TRUE(!order->_origClOrderId.empty());
            ASSERT_TRUE(!order->_clOrderId.empty());
            ASSERT_TRUE(order->_origClOrderId != order->_clOrderId);
            ASSERT_NEAR(order->_exPrice, 2310.75, 0.000001);
            ASSERT_NEAR(order->_exNewPrice, 2311.0, 0.000001);

            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrdType, "2"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Price, "2311"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrderQty, "4"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Side, "2"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Symbol, "NQ"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::SecurityDesc, "NQH2"));
            ASSERT_TRUE(checkField(m, tw::channel_or_cme::CustomTags::ManualOrderIndicator, "Y"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::ClOrdID, order->_clOrderId));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrderID, order->_exOrderId));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrigClOrdID, order->_origClOrderId));
            
            ASSERT_TRUE(!m.contain(OnixS::FIX::FIX42::Tags::TimeInForce));
            ASSERT_TRUE(!m.contain(OnixS::FIX::FIX42::Tags::MinQty));
        }
        
        order->_exOrderId = "12345678";
        ASSERT_TRUE(t.translateCxl(order, rej));        
        {
            OnixS::FIX::Message& m = t.getMsgCxl();
            ASSERT_TRUE(!order->_origClOrderId.empty());
            ASSERT_TRUE(!order->_clOrderId.empty());
            ASSERT_TRUE(order->_origClOrderId != order->_clOrderId);
            
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Side, "2"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Symbol, "NQ"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::SecurityDesc, "NQH2"));
            ASSERT_TRUE(checkField(m, tw::channel_or_cme::CustomTags::ManualOrderIndicator, "Y"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::ClOrdID, order->_clOrderId));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrderID, order->_exOrderId));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrigClOrdID, order->_origClOrderId));
        }
        
        order = OrderHelper::getSellMarket(2);
        ASSERT_TRUE(!t.translateNew(order, rej));
        ASSERT_EQ(rej._rejType, tw::channel_or::eRejectType::kInternal);
        ASSERT_EQ(rej._rejSubType, tw::channel_or::eRejectSubType::kTranslator);
        ASSERT_EQ(rej._rejReason, tw::channel_or::eRejectReason::kType);
    }
    
    // Test msg reject logic
    //
    {
        tw::channel_or_cme::TMessagePtr rejAckMsg;
        
        OnixS::FIX::Message rej("3", t.version());
        rej.set(OnixS::FIX::FIX42::Tags::Text, rejText);
        
        {
            OnixS::FIX::Message rejMsg("D", t.version());
            setOrderIds(rejMsg, testId);

            ASSERT_TRUE(t.translateMsgRej(rej, rejMsg, rejAckMsg));
            ASSERT_TRUE(checkField(*rejAckMsg, OnixS::FIX::FIX42::Tags::MsgType, "8"));
            ASSERT_TRUE(checkField(*rejAckMsg, OnixS::FIX::FIX42::Tags::ExecTransType, "0"));
            ASSERT_TRUE(checkField(*rejAckMsg, OnixS::FIX::FIX42::Tags::OrdStatus, "8"));
            ASSERT_TRUE(checkField(*rejAckMsg, OnixS::FIX::FIX42::Tags::ExecType, "8"));
            ASSERT_TRUE(checkField(*rejAckMsg, OnixS::FIX::FIX42::Tags::OrdRejReason, tw::channel_or_cme::CustomValues::SessionReject));
            ASSERT_TRUE(checkField(*rejAckMsg, OnixS::FIX::FIX42::Tags::ClOrdID, testId));
            ASSERT_TRUE(checkField(*rejAckMsg, OnixS::FIX::FIX42::Tags::OrderID, testId));
            ASSERT_TRUE(checkField(*rejAckMsg, OnixS::FIX::FIX42::Tags::OrigClOrdID, testId));
            ASSERT_TRUE(checkField(*rejAckMsg, tw::channel_or_cme::CustomTags::CorrelationClOrdID, testId));   
            ASSERT_TRUE(checkField(*rejAckMsg, OnixS::FIX::FIX42::Tags::Text, rejText));   
        }
        
        {
            OnixS::FIX::Message rejMsg("G", t.version());
            setOrderIds(rejMsg, testId);

            ASSERT_TRUE(t.translateMsgRej(rej, rejMsg, rejAckMsg));
            ASSERT_TRUE(checkField(*rejAckMsg, OnixS::FIX::FIX42::Tags::MsgType, "9"));
            ASSERT_TRUE(checkField(*rejAckMsg, OnixS::FIX::FIX42::Tags::CxlRejResponseTo, "2"));
            ASSERT_TRUE(checkField(*rejAckMsg, OnixS::FIX::FIX42::Tags::CxlRejReason, tw::channel_or_cme::CustomValues::SessionReject));
            ASSERT_TRUE(checkField(*rejAckMsg, OnixS::FIX::FIX42::Tags::ClOrdID, testId));
            ASSERT_TRUE(checkField(*rejAckMsg, OnixS::FIX::FIX42::Tags::OrderID, testId));
            ASSERT_TRUE(checkField(*rejAckMsg, OnixS::FIX::FIX42::Tags::OrigClOrdID, testId));
            ASSERT_TRUE(checkField(*rejAckMsg, tw::channel_or_cme::CustomTags::CorrelationClOrdID, testId));   
            ASSERT_TRUE(checkField(*rejAckMsg, OnixS::FIX::FIX42::Tags::Text, rejText));   
        }
        
        {
            OnixS::FIX::Message rejMsg("F", t.version());
            setOrderIds(rejMsg, testId);

            ASSERT_TRUE(t.translateMsgRej(rej, rejMsg, rejAckMsg));
            ASSERT_TRUE(checkField(*rejAckMsg, OnixS::FIX::FIX42::Tags::MsgType, "9"));
            ASSERT_TRUE(checkField(*rejAckMsg, OnixS::FIX::FIX42::Tags::CxlRejResponseTo, "1"));
            ASSERT_TRUE(checkField(*rejAckMsg, OnixS::FIX::FIX42::Tags::CxlRejReason, tw::channel_or_cme::CustomValues::SessionReject));
            ASSERT_TRUE(checkField(*rejAckMsg, OnixS::FIX::FIX42::Tags::ClOrdID, testId));
            ASSERT_TRUE(checkField(*rejAckMsg, OnixS::FIX::FIX42::Tags::OrderID, testId));
            ASSERT_TRUE(checkField(*rejAckMsg, OnixS::FIX::FIX42::Tags::OrigClOrdID, testId));
            ASSERT_TRUE(checkField(*rejAckMsg, tw::channel_or_cme::CustomTags::CorrelationClOrdID, testId));   
            ASSERT_TRUE(checkField(*rejAckMsg, OnixS::FIX::FIX42::Tags::Text, rejText));   
        }
    }
    
    // Test execution report
    //
    {
        OnixS::FIX::Message execReport("8", t.version());
        setOrderIds(execReport, testId);
        execReport.set(OnixS::FIX::FIX42::Tags::OrdRejReason, "1");
        execReport.set(OnixS::FIX::FIX42::Tags::CxlRejReason, "2");
        execReport.set(OnixS::FIX::FIX42::Tags::Text, rejText);
        execReport.set(OnixS::FIX::FIX42::Tags::MsgSeqNum, "3");
        
        {
            // New ack
            //
            execReport.set(OnixS::FIX::FIX42::Tags::ExecType, '0');
            execReport.set(OnixS::FIX::FIX42::Tags::SendingTime, "20120423-20:40:01.285");
            tw::channel_or::OrderResp resp;
            
            ASSERT_TRUE(t.translateExecutionReport(execReport, resp));
            ASSERT_EQ(resp._seqNum, 3U);
            ASSERT_EQ(resp._type, tw::channel_or::eOrderRespType::kNewAck);
            ASSERT_TRUE(checkOrderIds(resp, testId));
            ASSERT_EQ(resp._exTimestamp.toString(), "20120423-20:40:01.285000");
        }
        
        {
            // New rej
            //
            execReport.set(OnixS::FIX::FIX42::Tags::ExecType, '8');
            execReport.set(OnixS::FIX::FIX42::Tags::TransactTime, "20120423-20:40:01.284");
            tw::channel_or::OrderResp resp;
            
            ASSERT_TRUE(t.translateExecutionReport(execReport, resp));
            ASSERT_EQ(resp._seqNum, 3U);
            ASSERT_EQ(resp._type, tw::channel_or::eOrderRespType::kNewRej);
            ASSERT_TRUE(checkOrderIds(resp, testId));
            ASSERT_EQ(resp._exTimestamp.toString(), "20120423-20:40:01.284000");
        }
        
        {
            // Mod ack
            //
            execReport.set(OnixS::FIX::FIX42::Tags::ExecType, '5');
            execReport.set(OnixS::FIX::FIX42::Tags::Price, "2272.50");
            tw::channel_or::OrderResp resp;
            
            ASSERT_TRUE(t.translateExecutionReport(execReport, resp));
            ASSERT_EQ(resp._seqNum, 3U);
            ASSERT_EQ(resp._type, tw::channel_or::eOrderRespType::kModAck);
            ASSERT_EQ(resp._price, "2272.50");
            ASSERT_TRUE(checkOrderIds(resp, testId));
        }
        
        {
            // Cxl ack - 4
            //
            execReport.set(OnixS::FIX::FIX42::Tags::ExecType, '4');
            tw::channel_or::OrderResp resp;
            
            ASSERT_TRUE(t.translateExecutionReport(execReport, resp));
            ASSERT_EQ(resp._seqNum, 3U);
            ASSERT_EQ(resp._type, tw::channel_or::eOrderRespType::kCxlAck);
            ASSERT_TRUE(checkOrderIds(resp, testId));
        }
        
        {
            // Cxl ack - C
            //
            execReport.set(OnixS::FIX::FIX42::Tags::ExecType, 'C');
            tw::channel_or::OrderResp resp;
            
            ASSERT_TRUE(t.translateExecutionReport(execReport, resp));
            ASSERT_EQ(resp._seqNum, 3U);
            ASSERT_EQ(resp._type, tw::channel_or::eOrderRespType::kCxlAck);
            ASSERT_TRUE(checkOrderIds(resp, testId));
        }
        
        {
            // OrderStatus - I
            //
            execReport.set(OnixS::FIX::FIX42::Tags::ExecType, 'I');
            tw::channel_or::OrderResp resp;
            
            ASSERT_TRUE(t.translateExecutionReport(execReport, resp));
            ASSERT_EQ(resp._seqNum, 3U);
            ASSERT_EQ(resp._type, tw::channel_or::eOrderRespType::kOrderStatus);
            ASSERT_TRUE(checkOrderIds(resp, testId));
        }
        
        
        // Test fills
        //
        execReport.set(OnixS::FIX::FIX42::Tags::LastShares, "5");
        execReport.set(OnixS::FIX::FIX42::Tags::LastPx, "225057");
        execReport.set(OnixS::FIX::FIX42::Tags::ExecID, "2222");
        execReport.set(OnixS::FIX::FIX42::Tags::ExecRefID, "3333");
        
        {
            // Partial fill
            //
            execReport.set(OnixS::FIX::FIX42::Tags::ExecType, '1');
            tw::channel_or::OrderResp resp;
            
            ASSERT_TRUE(t.translateExecutionReport(execReport, resp));
            ASSERT_EQ(resp._seqNum, 3U);
            ASSERT_EQ(resp._type, tw::channel_or::eOrderRespType::kPartFill);
            ASSERT_TRUE(checkOrderIds(resp, testId, false));
            
            ASSERT_EQ(resp._lastShares, 5U);
            ASSERT_EQ(resp._lastPrice, "225057");
            ASSERT_EQ(resp._exFillId, "2222");
            ASSERT_EQ(resp._exFillRefId, "3333");
        }
        
        
        {
            // Fill - no AggressorIndicator (tag 1057 - default:
            // not present or 'N' - resting at match)
            //
            execReport.set(OnixS::FIX::FIX42::Tags::ExecType, '2');            
            tw::channel_or::OrderResp resp;
            
            ASSERT_TRUE(t.translateExecutionReport(execReport, resp));
            ASSERT_EQ(resp._seqNum, 3U);
            ASSERT_EQ(resp._type, tw::channel_or::eOrderRespType::kFill);
            ASSERT_TRUE(checkOrderIds(resp, testId, false));
            
            ASSERT_EQ(resp._lastShares, 5U);
            ASSERT_EQ(resp._lastPrice, "225057");
            ASSERT_EQ(resp._exFillId, "2222");
            ASSERT_EQ(resp._exFillRefId, "3333");
            ASSERT_EQ(resp._liqInd, tw::channel_or::eLiqInd::kUnknown);
        }
        
        {
            // Fill - AggressorIndicator (tag 1057 - default:
            // not present or 'N' - resting at match) set to 'N'
            //
            execReport.set(OnixS::FIX::FIX42::Tags::ExecType, '2');
            execReport.set(tw::channel_or_cme::CustomTags::AggressorIndicator, 'N');
            tw::channel_or::OrderResp resp;
            
            ASSERT_TRUE(t.translateExecutionReport(execReport, resp));
            ASSERT_EQ(resp._seqNum, 3U);
            ASSERT_EQ(resp._type, tw::channel_or::eOrderRespType::kFill);
            ASSERT_TRUE(checkOrderIds(resp, testId, false));
            
            ASSERT_EQ(resp._lastShares, 5U);
            ASSERT_EQ(resp._lastPrice, "225057");
            ASSERT_EQ(resp._exFillId, "2222");
            ASSERT_EQ(resp._exFillRefId, "3333");
            ASSERT_EQ(resp._liqInd, tw::channel_or::eLiqInd::kAdd);
        }
        
        {
            // Fill - AggressorIndicator (tag 1057 - default:
            // not present or 'N' - resting at match) set to 'Y'
            //
            execReport.set(OnixS::FIX::FIX42::Tags::ExecType, '2');
            execReport.set(tw::channel_or_cme::CustomTags::AggressorIndicator, 'Y');
            tw::channel_or::OrderResp resp;
            
            ASSERT_TRUE(t.translateExecutionReport(execReport, resp));
            ASSERT_EQ(resp._seqNum, 3U);
            ASSERT_EQ(resp._type, tw::channel_or::eOrderRespType::kFill);
            ASSERT_TRUE(checkOrderIds(resp, testId, false));
            
            ASSERT_EQ(resp._lastShares, 5U);
            ASSERT_EQ(resp._lastPrice, "225057");
            ASSERT_EQ(resp._exFillId, "2222");
            ASSERT_EQ(resp._exFillRefId, "3333");
            ASSERT_EQ(resp._liqInd, tw::channel_or::eLiqInd::kRem);
        }
        
        {
            // Trade break
            //
            execReport.set(OnixS::FIX::FIX42::Tags::ExecType, 'H');
            tw::channel_or::OrderResp resp;
            
            ASSERT_TRUE(t.translateExecutionReport(execReport, resp));
            ASSERT_EQ(resp._seqNum, 3U);
            ASSERT_EQ(resp._type, tw::channel_or::eOrderRespType::kTradeBreak);
            ASSERT_TRUE(checkOrderIds(resp, testId, false));
            
            ASSERT_EQ(resp._lastShares, 5U);
            ASSERT_EQ(resp._lastPrice, "225057");
            ASSERT_EQ(resp._exFillId, "2222");
            ASSERT_EQ(resp._exFillRefId, "3333");
        }
    }
    
    // Test mod/cxl reject
    //
    {
        OnixS::FIX::Message cxlReject("9", t.version());
        setOrderIds(cxlReject, testId);
        cxlReject.set(OnixS::FIX::FIX42::Tags::CxlRejReason, "4444");
        cxlReject.set(OnixS::FIX::FIX42::Tags::Text, rejText);
        cxlReject.set(OnixS::FIX::FIX42::Tags::MsgSeqNum, "4");

        {                
            // Mod rej
            //
            tw::channel_or::OrderResp resp;            
            cxlReject.set(OnixS::FIX::FIX42::Tags::CxlRejResponseTo, "2");

            ASSERT_TRUE(t.translateOrderCancelReject(cxlReject, resp));
            ASSERT_EQ(resp._seqNum, 4U);
            ASSERT_EQ(resp._type, tw::channel_or::eOrderRespType::kModRej);
            ASSERT_TRUE(checkOrderIds(resp, testId, false));

            ASSERT_EQ(resp._exRejReason, "4444");
            ASSERT_EQ(resp._exRejText, rejText);
        }
        
        {                
            // Cxl rej
            //
            tw::channel_or::OrderResp resp;            
            cxlReject.set(OnixS::FIX::FIX42::Tags::CxlRejResponseTo, "1");

            ASSERT_TRUE(t.translateOrderCancelReject(cxlReject, resp));
            ASSERT_EQ(resp._seqNum, 4U);
            ASSERT_EQ(resp._type, tw::channel_or::eOrderRespType::kCxlRej);
            ASSERT_TRUE(checkOrderIds(resp, testId, false));

            ASSERT_EQ(resp._exRejReason, "4444");
            ASSERT_EQ(resp._exRejText, rejText);
        }
    }
    
    
    tw::channel_or_cme::ChannelOrOnix::global_shutdown();
}


TEST(ChannelOrCmeLibTestSuit, translator_IOC)
{
    ASSERT_TRUE(tw::channel_or_cme::ChannelOrOnix::global_init(getGlobalSettings()));
    
    tw::channel_or_cme::Translator t;
    tw::channel_or_cme::TSessionSettingsPtr settings = getSessionSettings();
    ASSERT_TRUE(t.init(settings));
    
    // Test outbound limit buy IOC
    //
    {
        tw::channel_or::TOrderPtr order = OrderHelper::getBuyLimit(9241, 2);
        order->_manual = true;
        ASSERT_TRUE(order->_origClOrderId.empty());
        
        tw::channel_or::Reject rej;
        
        order->_timeInForce = tw::channel_or::eTimeInForce::kIOC;
        ASSERT_TRUE(t.translateNew(order, rej));        
        {
            OnixS::FIX::Message& m = t.getMsgNew();
            ASSERT_TRUE(!order->_origClOrderId.empty());
            ASSERT_TRUE(order->_origClOrderId == order->_clOrderId);
            ASSERT_TRUE(order->_origClOrderId == order->_corrClOrderId);
            ASSERT_EQ(order->_exSessionName, settings->_senderCompId);
            ASSERT_NEAR(order->_exPrice, 2310.25, 0.000001);
            ASSERT_NEAR(order->_exNewPrice, 0.0, 0.000001);

            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrdType, "2"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Price, "2310.25"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrderQty, "2"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Side, "1"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Symbol, "NQ"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::SecurityDesc, "NQH2"));
            ASSERT_TRUE(checkField(m, tw::channel_or_cme::CustomTags::ManualOrderIndicator, "Y"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::ClOrdID, order->_clOrderId));
            
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::TimeInForce, "3"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::MinQty, "1"));
        }
        
        order->_timeInForce = tw::channel_or::eTimeInForce::kUnknown;
        ASSERT_TRUE(t.translateNew(order, rej));        
        {
            OnixS::FIX::Message& m = t.getMsgNew();
            ASSERT_TRUE(!order->_origClOrderId.empty());
            ASSERT_TRUE(order->_origClOrderId == order->_clOrderId);
            ASSERT_TRUE(order->_origClOrderId == order->_corrClOrderId);
            ASSERT_EQ(order->_exSessionName, settings->_senderCompId);
            ASSERT_NEAR(order->_exPrice, 2310.25, 0.000001);
            ASSERT_NEAR(order->_exNewPrice, 0.0, 0.000001);

            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrdType, "2"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Price, "2310.25"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrderQty, "2"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Side, "1"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Symbol, "NQ"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::SecurityDesc, "NQH2"));
            ASSERT_TRUE(checkField(m, tw::channel_or_cme::CustomTags::ManualOrderIndicator, "Y"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::ClOrdID, order->_clOrderId));
            
            ASSERT_TRUE(!m.contain(OnixS::FIX::FIX42::Tags::TimeInForce));
            ASSERT_TRUE(!m.contain(OnixS::FIX::FIX42::Tags::MinQty));
        }
        
        order->_newPrice.set(9242);
        order->_exOrderId = "1234567";
        
        order->_timeInForce = tw::channel_or::eTimeInForce::kIOC;
        ASSERT_TRUE(t.translateMod(order, rej));        
        {
            OnixS::FIX::Message& m = t.getMsgMod();
            ASSERT_TRUE(!order->_origClOrderId.empty());
            ASSERT_TRUE(!order->_clOrderId.empty());
            ASSERT_TRUE(order->_origClOrderId != order->_clOrderId);
            ASSERT_NEAR(order->_exPrice, 2310.25, 0.000001);
            ASSERT_NEAR(order->_exNewPrice, 2310.50, 0.000001);

            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrdType, "2"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Price, "2310.5"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrderQty, "2"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Side, "1"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Symbol, "NQ"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::SecurityDesc, "NQH2"));
            ASSERT_TRUE(checkField(m, tw::channel_or_cme::CustomTags::ManualOrderIndicator, "Y"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::ClOrdID, order->_clOrderId));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrderID, order->_exOrderId));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrigClOrdID, order->_origClOrderId));
            
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::TimeInForce, "3"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::MinQty, "1"));
        }
        
        order->_timeInForce = tw::channel_or::eTimeInForce::kUnknown;
        ASSERT_TRUE(t.translateMod(order, rej));        
        {
            OnixS::FIX::Message& m = t.getMsgMod();
            ASSERT_TRUE(!order->_origClOrderId.empty());
            ASSERT_TRUE(!order->_clOrderId.empty());
            ASSERT_TRUE(order->_origClOrderId != order->_clOrderId);
            ASSERT_NEAR(order->_exPrice, 2310.25, 0.000001);
            ASSERT_NEAR(order->_exNewPrice, 2310.50, 0.000001);

            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrdType, "2"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Price, "2310.5"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrderQty, "2"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Side, "1"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Symbol, "NQ"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::SecurityDesc, "NQH2"));
            ASSERT_TRUE(checkField(m, tw::channel_or_cme::CustomTags::ManualOrderIndicator, "Y"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::ClOrdID, order->_clOrderId));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrderID, order->_exOrderId));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrigClOrdID, order->_origClOrderId));
            
            ASSERT_TRUE(!m.contain(OnixS::FIX::FIX42::Tags::TimeInForce));
            ASSERT_TRUE(!m.contain(OnixS::FIX::FIX42::Tags::MinQty));
        }
        
        order->_exOrderId = "12345678";
        ASSERT_TRUE(t.translateCxl(order, rej));        
        {
            OnixS::FIX::Message& m = t.getMsgCxl();
            ASSERT_TRUE(!order->_origClOrderId.empty());
            ASSERT_TRUE(!order->_clOrderId.empty());
            ASSERT_TRUE(order->_origClOrderId != order->_clOrderId);
            
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Side, "1"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Symbol, "NQ"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::SecurityDesc, "NQH2"));
            ASSERT_TRUE(checkField(m, tw::channel_or_cme::CustomTags::ManualOrderIndicator, "Y"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::ClOrdID, order->_clOrderId));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrderID, order->_exOrderId));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrigClOrdID, order->_origClOrderId));
        }
    }
    
    // Test outbound limit sell IOC
    //
    {
        tw::channel_or::TOrderPtr order = OrderHelper::getSellLimit(9243, 4);
        order->_manual = true;
        order->_timeInForce = tw::channel_or::eTimeInForce::kIOC;
        ASSERT_TRUE(order->_origClOrderId.empty());
        
        tw::channel_or::Reject rej;
        
        ASSERT_TRUE(t.translateNew(order, rej));        
        {
            OnixS::FIX::Message& m = t.getMsgNew();
            ASSERT_TRUE(!order->_origClOrderId.empty());
            ASSERT_TRUE(order->_origClOrderId == order->_clOrderId);
            ASSERT_TRUE(order->_origClOrderId == order->_corrClOrderId);
            ASSERT_EQ(order->_exSessionName, settings->_senderCompId);
            ASSERT_NEAR(order->_exPrice, 2310.75, 0.000001);
            ASSERT_NEAR(order->_exNewPrice, 0.0, 0.000001);

            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrdType, "2"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Price, "2310.75"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrderQty, "4"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Side, "2"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Symbol, "NQ"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::SecurityDesc, "NQH2"));
            ASSERT_TRUE(checkField(m, tw::channel_or_cme::CustomTags::ManualOrderIndicator, "Y"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::ClOrdID, order->_clOrderId));
            
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::TimeInForce, "3"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::MinQty, "1"));
        }
        
        order->_newPrice.set(9244);
        order->_exOrderId = "1234567";
        
        ASSERT_TRUE(t.translateMod(order, rej));        
        {
            OnixS::FIX::Message& m = t.getMsgMod();
            ASSERT_TRUE(!order->_origClOrderId.empty());
            ASSERT_TRUE(!order->_clOrderId.empty());
            ASSERT_TRUE(order->_origClOrderId != order->_clOrderId);
            ASSERT_NEAR(order->_exPrice, 2310.75, 0.000001);
            ASSERT_NEAR(order->_exNewPrice, 2311.0, 0.000001);

            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrdType, "2"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Price, "2311"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrderQty, "4"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Side, "2"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Symbol, "NQ"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::SecurityDesc, "NQH2"));
            ASSERT_TRUE(checkField(m, tw::channel_or_cme::CustomTags::ManualOrderIndicator, "Y"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::ClOrdID, order->_clOrderId));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrderID, order->_exOrderId));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrigClOrdID, order->_origClOrderId));
            
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::TimeInForce, "3"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::MinQty, "1"));
        }
        
        order->_exOrderId = "12345678";
        ASSERT_TRUE(t.translateCxl(order, rej));        
        {
            OnixS::FIX::Message& m = t.getMsgCxl();
            ASSERT_TRUE(!order->_origClOrderId.empty());
            ASSERT_TRUE(!order->_clOrderId.empty());
            ASSERT_TRUE(order->_origClOrderId != order->_clOrderId);
            
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Side, "2"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::Symbol, "NQ"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::SecurityDesc, "NQH2"));
            ASSERT_TRUE(checkField(m, tw::channel_or_cme::CustomTags::ManualOrderIndicator, "Y"));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::ClOrdID, order->_clOrderId));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrderID, order->_exOrderId));
            ASSERT_TRUE(checkField(m, OnixS::FIX::FIX42::Tags::OrigClOrdID, order->_origClOrderId));
        }
        
    }
    
    tw::channel_or_cme::ChannelOrOnix::global_shutdown();
}
