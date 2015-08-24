#include <tw/channel_or_cme_bridge/translator.h>
#include <tw/channel_or_cme/id_factory.h>

namespace tw {
namespace channel_or_cme_bridge {
    
Translator::Translator() : TParent(),
                           _msgExRpt(Values::MsgType::Execution_Report, TParent::version()),
                           _msgOrderCxlRej(Values::MsgType::Order_Cancel_Reject, TParent::version()),
                           _msgRej(Values::MsgType::Reject, TParent::version()),
                           _msgBusRej(Values::MsgType::Business_Message_Reject, TParent::version()) {
}

Translator::~Translator() {
}
    
void Translator::clear() {
    TParent::clear();
    
    _msgExRpt.reset();    
    _msgOrderCxlRej.reset();
    _msgRej.reset();
}

bool Translator::translateNew(const Message& source, TOrderCmeBridgeInfoPtr& orderCmeBridgeInfoPtr) {
    bool status = true;
    try {
        Message& msg = _msgNew;
        
        copyField(source, msg, Tags::OrdType);
        copyField(source, msg, Tags::Price);
        copyField(source, msg, Tags::Side);
        
        if ( source.contain(Tags::TimeInForce) ) {
            copyField(source, msg, Tags::TimeInForce);
            copyField(source, msg, Tags::MinQty);
        } else {
            removeField(msg, Tags::TimeInForce);
            removeField(msg, Tags::MinQty);
        }
        
        orderCmeBridgeInfoPtr->_side = msg.get(Tags::Side);
        orderCmeBridgeInfoPtr->_symbol = msg.get(Tags::Symbol);
        orderCmeBridgeInfoPtr->_securityDesc = msg.get(Tags::SecurityDesc);
        orderCmeBridgeInfoPtr->_manualOrderIndicator = msg.get(tw::channel_or_cme::CustomTags::ManualOrderIndicator);
        
        orderCmeBridgeInfoPtr->_correlationClOrdID_bridge = orderCmeBridgeInfoPtr->_origClOrdID_bridge = tw::channel_or_cme::IdFactory::instance().get().c_str();
        orderCmeBridgeInfoPtr->_correlationClOrdID = orderCmeBridgeInfoPtr->_origClOrdID = source.get(Tags::ClOrdID);
        
        copyField(source, msg, Tags::OrderQty);
        copyField(source, msg, Tags::Symbol);
        copyField(source, msg, Tags::SecurityDesc);
        copyField(source, msg, tw::channel_or_cme::CustomTags::ManualOrderIndicator);
        copyField(source, msg, Tags::TransactTime);
        
        msg.set(Tags::ClOrdID, orderCmeBridgeInfoPtr->_origClOrdID_bridge);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    return status;
}

bool Translator::translateMod(const Message& source, TOrderCmeBridgeInfoPtr& orderCmeBridgeInfoPtr) {
    bool status = true;
    try {
        Message& msg = _msgMod;
        
        copyField(source, msg, Tags::OrdType);
        copyField(source, msg, Tags::Price);
        copyField(source, msg, Tags::Side);
        
        if ( source.contain(Tags::TimeInForce) ) {
            copyField(source, msg, Tags::TimeInForce);
            copyField(source, msg, Tags::MinQty);
        } else {
            removeField(msg, Tags::TimeInForce);
            removeField(msg, Tags::MinQty);
        }
        
        copyField(source, msg, Tags::OrderQty);
        copyField(source, msg, Tags::Symbol);
        copyField(source, msg, Tags::SecurityDesc);
        copyField(source, msg, tw::channel_or_cme::CustomTags::ManualOrderIndicator);
        copyField(source, msg, Tags::TransactTime);
        copyField(source, msg, Tags::OrderID);
        
        std::string clOrdID_bridge = tw::channel_or_cme::IdFactory::instance().get().c_str();
        orderCmeBridgeInfoPtr->_orderIds[clOrdID_bridge] = source.get(Tags::ClOrdID);
        
        msg.set(Tags::ClOrdID, clOrdID_bridge);
        msg.set(Tags::OrigClOrdID, orderCmeBridgeInfoPtr->_origClOrdID_bridge);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    return status;
}

bool Translator::translateCxl(const Message& source, TOrderCmeBridgeInfoPtr& orderCmeBridgeInfoPtr) {
    bool status = true;
    try {
        Message& msg = _msgCxl;
        
        copyField(source, msg, Tags::Side);
        copyField(source, msg, Tags::Symbol);
        copyField(source, msg, Tags::SecurityDesc);
        copyField(source, msg, tw::channel_or_cme::CustomTags::ManualOrderIndicator);
        copyField(source, msg, Tags::TransactTime);
        copyField(source, msg, Tags::OrderID);

        std::string clOrdID_bridge = tw::channel_or_cme::IdFactory::instance().get().c_str();
        orderCmeBridgeInfoPtr->_orderIds[clOrdID_bridge] = source.get(Tags::ClOrdID);
        
        msg.set(Tags::ClOrdID, clOrdID_bridge);
        msg.set(Tags::OrigClOrdID, orderCmeBridgeInfoPtr->_origClOrdID_bridge);
        msg.set(tw::channel_or_cme::CustomTags::CorrelationClOrdID, orderCmeBridgeInfoPtr->_correlationClOrdID_bridge);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    return status;
}

bool Translator::translateCxl(TOrderCmeBridgeInfoPtr& orderCmeBridgeInfoPtr) {
    bool status = true;
    try {
        Message& msg = _msgCxl;
        
        msg.set(Tags::Side, orderCmeBridgeInfoPtr->_side);
        msg.set(Tags::Symbol, orderCmeBridgeInfoPtr->_symbol);
        msg.set(Tags::SecurityDesc, orderCmeBridgeInfoPtr->_securityDesc);
        msg.set(tw::channel_or_cme::CustomTags::ManualOrderIndicator, orderCmeBridgeInfoPtr->_manualOrderIndicator);
        msg.set(Tags::TransactTime, Timestamp::getUtcTimestampWithMilliseconds());
        msg.set(Tags::OrderID, orderCmeBridgeInfoPtr->_orderID);

        std::string clOrdID_bridge = tw::channel_or_cme::IdFactory::instance().get().c_str();
        orderCmeBridgeInfoPtr->_orderIds[clOrdID_bridge] = clOrdID_bridge;
        
        msg.set(Tags::ClOrdID, clOrdID_bridge);
        msg.set(Tags::OrigClOrdID, orderCmeBridgeInfoPtr->_origClOrdID_bridge);
        msg.set(tw::channel_or_cme::CustomTags::CorrelationClOrdID, orderCmeBridgeInfoPtr->_correlationClOrdID_bridge);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    return status;
}

tw::common::eCMEOrBridgeMsgType Translator::getInboundMessageType(const Message& source) {
    try {
        char msgType = source.getType()[0];
        switch ( msgType ) {
            case '8':   // Execution Report
                return tw::common::eCMEOrBridgeMsgType::kExRpt;
            case '9':   // OrderCancel Reject
                return tw::common::eCMEOrBridgeMsgType::kOrderCxlRej;
            case '3':   // Reject
                return tw::common::eCMEOrBridgeMsgType::kRej;
            case 'j':   // Business Reject
                return tw::common::eCMEOrBridgeMsgType::kRej;
            case '5':   // Logout
                return tw::common::eCMEOrBridgeMsgType::kLogout;
            default:
                break;
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    return tw::common::eCMEOrBridgeMsgType::kUnknown;
}

bool Translator::translateInboundApplicationMsg(const Message& source,  TOrderCmeBridgeInfoPtr& orderCmeBridgeInfoPtr, const tw::common::eCMEOrBridgeMsgType& msgType,  const tw::channel_or::eOrderRespType& orderRespType) {
    bool status = true;
    try {
        switch ( msgType ) {
            case tw::common::eCMEOrBridgeMsgType::kExRpt:
            {
                Message& msg = _msgExRpt;
                msg.reset();
                
                copyField(source, msg, Tags::ExecID);
                copyField(source, msg, Tags::ExecRefID);
                copyField(source, msg, Tags::ExecTransType);
                copyField(source, msg, Tags::ExecType);
                copyField(source, msg, Tags::LastPx);
                copyField(source, msg, Tags::LastShares);
                copyField(source, msg, Tags::OrdRejReason);
                copyField(source, msg, Tags::OrdStatus);
                copyField(source, msg, Tags::OrderID);
                copyField(source, msg, Tags::Price);
                copyField(source, msg, Tags::SendingTime);
                copyField(source, msg, Tags::Side);
                copyField(source, msg, Tags::Symbol);
                copyField(source, msg, Tags::Text);
                copyField(source, msg, Tags::TransactTime);
                copyField(source, msg, tw::channel_or_cme::CustomTags::AggressorIndicator);
                
            }
                break;
            case tw::common::eCMEOrBridgeMsgType::kOrderCxlRej:
            {
                Message& msg = _msgOrderCxlRej;
                msg.reset();
                
                copyField(source, msg, Tags::CxlRejReason);
                copyField(source, msg, Tags::CxlRejResponseTo);
                copyField(source, msg, Tags::OrderID);
                copyField(source, msg, Tags::Text);
                copyField(source, msg, Tags::SendingTime);
                copyField(source, msg, Tags::TransactTime);
                
            }
                break;
            default:
                return false;
        }
        
        std::string clOrdID;
        std::string clOrdID_Bridge;
        std::map<std::string, std::string>::iterator iter = orderCmeBridgeInfoPtr->_orderIds.find(source.get(Tags::ClOrdID));
        if ( iter != orderCmeBridgeInfoPtr->_orderIds.end() ) {
            clOrdID_Bridge = iter->first;
            clOrdID = iter->second;
        } else {
            clOrdID_Bridge = orderCmeBridgeInfoPtr->_correlationClOrdID_bridge;
            clOrdID = orderCmeBridgeInfoPtr->_correlationClOrdID;
        }
        
        switch ( orderRespType ) {
            case tw::channel_or::eOrderRespType::kNewAck:
                orderCmeBridgeInfoPtr->_orderID = source.get(Tags::OrderID);
                break;
            case tw::channel_or::eOrderRespType::kNewRej:
            case tw::channel_or::eOrderRespType::kPartFill:
            case tw::channel_or::eOrderRespType::kTradeBreak:
            case tw::channel_or::eOrderRespType::kFill:
            case tw::channel_or::eOrderRespType::kCxlAck:
                break;
            case tw::channel_or::eOrderRespType::kModRej:
            case tw::channel_or::eOrderRespType::kCxlRej:
                break;
            case tw::channel_or::eOrderRespType::kModAck:
                orderCmeBridgeInfoPtr->_origClOrdID_bridge = clOrdID_Bridge;
                orderCmeBridgeInfoPtr->_origClOrdID = clOrdID;
                break;        
            default:
                break;
        }
        
        Message& msg = tw::common::eCMEOrBridgeMsgType::kExRpt == msgType ? _msgExRpt : _msgOrderCxlRej;
        msg.set(Tags::ClOrdID, clOrdID);
        msg.set(Tags::OrigClOrdID, orderCmeBridgeInfoPtr->_origClOrdID);
        msg.set(tw::channel_or_cme::CustomTags::CorrelationClOrdID, orderCmeBridgeInfoPtr->_correlationClOrdID);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    return status;
}

tw::common::eCMEOrBridgeMsgType Translator::translateInboundSessionMsg(const Message& source) {
    tw::common::eCMEOrBridgeMsgType result = tw::common::eCMEOrBridgeMsgType::kUnknown;
    try {
        char msgType = source.getType()[0];
        switch ( msgType ) {
            case '3':   // Reject
            {
                Message& msg = _msgRej;
                msg.reset();
                
                copyField(source, msg, Tags::Text);
                
                result = tw::common::eCMEOrBridgeMsgType::kRej;
            }
                break;
            case 'j':   // Business Reject
            {
                Message& msg = _msgBusRej;
                msg.reset();
                
                copyField(source, msg, Tags::Text);
                copyField(source, msg, Tags::BusinessRejectReason);
                
                result = tw::common::eCMEOrBridgeMsgType::kBusRej;
            }    
                break;
            case '0':   // Heartbeat
                result = tw::common::eCMEOrBridgeMsgType::kHeartbeat;
                break;
            case '5':   // Logout
                result = tw::common::eCMEOrBridgeMsgType::kLogout;
                break;
            default:
                return result;
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    return result;
}

} // namespace channel_or_cme_bridge
} // namespace tw
