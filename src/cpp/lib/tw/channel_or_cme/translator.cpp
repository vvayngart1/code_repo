#include <tw/channel_or_cme/translator.h>
#include <tw/channel_or_cme/id_factory.h>
#include <tw/price/ticks_converter.h>

namespace tw {
namespace channel_or_cme {
    
Translator::Translator() : _msgLogon(Values::MsgType::Logon, version()),
                           _msgNew(Values::MsgType::Order_Single, version()),
                           _msgMod(Values::MsgType::Order_Cancel_Replace_Request, version()),
                           _msgCxl(Values::MsgType::Order_Cancel_Request, version()) {
}

Translator::~Translator() {
}
    
void Translator::clear() {
    _settings.reset();
    
    _msgNew.reset();
    _msgMod.reset();
    _msgCxl.reset();
}

void Translator::setConstantFields(Message& msg) {
    switch (msg.getType()[0]) {
        // Mod 
        //
        // NOTE: Mod case 'falls' thru to New, since
        // all fields are the same except OFMOverride
        //
        case 'G':
            // Set in-flight-fill-mitigation to be enabled.
            //
            msg.set(CustomTags::OFMOverride, "Y");
        // New
        //
        // NOTE: New case 'falls' thru to Cxl, since all three
        // types need 'account' and 'securityType' set
        //
        case 'D':
            msg.set(Tags::HandlInst, _settings->_handlInst);
            msg.set(Tags::CustomerOrFirm, _settings->_customerOrFirm);
            msg.set(CustomTags::CtiCode, _settings->_ctiCode);
        // Cxl
        //
        case 'F':
            msg.set(Tags::Account, _settings->_account);
            // NOTE: right now hard coded to 'FUT', change once need to
            // trade options as well
            //
            msg.set(Tags::SecurityType, "FUT");
            break;
        // Logon
        //
        case 'A':
            msg.set(Tags::RawData, _settings->_password);
            msg.set(Tags::RawDataLength, boost::lexical_cast<std::string>(_settings->_password.length()));
            msg.set(Tags::EncryptMethod, "0");
            msg.setFlag(Tags::ResetSeqNumFlag, _settings->_resetSeqNumFlag);
            break;
    }
    
    msg.set(CustomTags::ApplicationSystemName, _settings->_applicationSystemName);
    msg.set(CustomTags::TradingSystemVersion, _settings->_applicationSystemVersion);
    msg.set(CustomTags::ApplicationSystemVendor, _settings->_applicationSystemVendor);
    
    LOGGER_INFO << " msg :: " << msg.toString(FIX_FIELDS_DELIMITER) << "\n";
}
    
bool Translator::init(const TSessionSettingsPtr& settings) {
    bool status = true;
    try {
        _settings = settings;
        
        setConstantFields(_msgLogon);
        setConstantFields(_msgNew);
        setConstantFields(_msgMod);
        setConstantFields(_msgCxl);        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    if ( !status )
        clear();
    
    return status;
}

// If trying to resend, let through CXLs and reject NEWs/MODs
//
bool Translator::shouldSend(const Message& msg) {
    bool status = true;
    try {
        if ( !msg.contain(Tags::PossDupFlag) )
            return true;

        if ( msg.get(Tags::PossDupFlag) != "Y" )
            return true;

        if ( msg.contain(Tags::MsgType) ) {
            if ( msg.get(Tags::MsgType) != Values::MsgType::Order_Cancel_Request )
                return false;
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    return status;
}    

// NOTE: supporting 'Day' orders (which is default if msg.set(Tags::TimeInForce, ...) is not set
// and 'IOC' (immediate or cancel) orders only for now .  Once requirements change, need to revisit.
//
// NOTE: right now supporting only limit orders
//
bool Translator::translateNew(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej) {
    bool status = true;
    try {
        Message& msg = _msgNew;
        
        switch ( order->_type ) {
            case tw::channel_or::eOrderType::kLimit:
                msg.set(Tags::OrdType, Values::OrdType::Limit);
                order->_exPrice = order->_instrument->_tc->toExchangePrice(order->_price);
                msg.set(Tags::Price, order->_exPrice, order->_instrument->_precision);                
                break;
            default:
                rej = getRej(tw::channel_or::eRejectReason::kType);
                return false;
        }
        
        switch ( order->_side ) {
            case tw::channel_or::eOrderSide::kBuy:
                msg.set(Tags::Side, Values::Side::Buy);
                break;
            case tw::channel_or::eOrderSide::kSell:
                msg.set(Tags::Side, Values::Side::Sell);
                break;
            default:
                rej = getRej(tw::channel_or::eRejectReason::kSide);
                return false;
        }
        
        switch ( order->_timeInForce ) {
            case tw::channel_or::eTimeInForce::kIOC:
                msg.set(Tags::TimeInForce, Values::TimeInForce::Immediate_or_Cancel);
                msg.set(Tags::MinQty, 1);
                break;
            default:
                if ( msg.contain(Tags::TimeInForce) )
                    msg.remove(Tags::TimeInForce);
                
                if ( msg.contain(Tags::MinQty) )
                    msg.remove(Tags::MinQty);
                
                break;
        }
        
        msg.set(Tags::OrderQty, order->_qty.get());
        
        msg.set(Tags::Symbol, order->_instrument->_symbol);
        msg.set(Tags::SecurityDesc, order->_instrument->_displayName);
        
        msg.setFlag(CustomTags::ManualOrderIndicator, order->_manual);        
        msg.set(Tags::TransactTime, Timestamp::getUtcTimestampWithMilliseconds());
        
        order->_origClOrderId = order->_clOrderId = order->_corrClOrderId = IdFactory::instance().get().c_str();
        order->_exSessionName = _settings->_senderCompId;
        msg.set(Tags::ClOrdID, order->_clOrderId);
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    return status;
}

// NOTE: since right now supporting only limit and market orders, OrdType is set to 'Limit'
// since can't modify 'Market' order
//
bool Translator::translateMod(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej) {
    bool status = true;
    try {
        if ( tw::channel_or::eOrderType::kLimit != order->_type ) {
            rej = getRej(tw::channel_or::eRejectReason::kType);
            return false;
        }
        
        Message& msg = _msgMod;
        
        msg.set(Tags::OrdType, Values::OrdType::Limit);        
        order->_exNewPrice = order->_instrument->_tc->toExchangePrice(order->_newPrice);
        msg.set(Tags::Price, order->_exNewPrice, order->_instrument->_precision);
        
        switch ( order->_side ) {
            case tw::channel_or::eOrderSide::kBuy:
                msg.set(Tags::Side, Values::Side::Buy);
                break;
            case tw::channel_or::eOrderSide::kSell:
                msg.set(Tags::Side, Values::Side::Sell);
                break;
            default:
                rej = getRej(tw::channel_or::eRejectReason::kSide);
                return false;
        }
        
        switch ( order->_timeInForce ) {
            case tw::channel_or::eTimeInForce::kIOC:
                msg.set(Tags::TimeInForce, Values::TimeInForce::Immediate_or_Cancel);
                msg.set(Tags::MinQty, 1);
                break;
            default:
                if ( msg.contain(Tags::TimeInForce) )
                    msg.remove(Tags::TimeInForce);
                
                if ( msg.contain(Tags::MinQty) )
                    msg.remove(Tags::MinQty);
                break;
        }
        
        msg.set(Tags::OrderQty, order->_qty.get());
        
        msg.set(Tags::Symbol, order->_instrument->_symbol);
        msg.set(Tags::SecurityDesc, order->_instrument->_displayName);
        
        msg.setFlag(CustomTags::ManualOrderIndicator, order->_manual);        
        msg.set(Tags::TransactTime, Timestamp::getUtcTimestampWithMilliseconds());
        
        order->_clOrderId = IdFactory::instance().get().c_str();        
        msg.set(Tags::ClOrdID, order->_clOrderId);
        msg.set(Tags::OrderID, order->_exOrderId);
        msg.set(Tags::OrigClOrdID, order->_origClOrderId);        
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    return status;
}

bool Translator::translateCxl(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej) {
    bool status = true;
    try {
        Message& msg = _msgCxl;
        
        switch ( order->_side ) {
            case tw::channel_or::eOrderSide::kBuy:
                msg.set(Tags::Side, Values::Side::Buy);
                break;
            case tw::channel_or::eOrderSide::kSell:
                msg.set(Tags::Side, Values::Side::Sell);
                break;
            default:
                rej = getRej(tw::channel_or::eRejectReason::kSide);
                return false;
        }
        
        msg.set(Tags::Symbol, order->_instrument->_symbol);
        msg.set(Tags::SecurityDesc, order->_instrument->_displayName);
        
        msg.setFlag(CustomTags::ManualOrderIndicator, order->_manual);        
        msg.set(Tags::TransactTime, Timestamp::getUtcTimestampWithMilliseconds());
        
        order->_clOrderId = IdFactory::instance().get().c_str();        
        msg.set(Tags::ClOrdID, order->_clOrderId);
        msg.set(Tags::OrderID, order->_exOrderId);
        msg.set(Tags::OrigClOrdID, order->_origClOrderId);
        msg.set(CustomTags::CorrelationClOrdID, order->_corrClOrderId);
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    return status;
}

bool Translator::translateMsgRej(const Message& rej, const Message& rejMsg, TMessagePtr& rejAckMsg) {
    bool status = true;
    try {
        int32_t rejReason = 0;
        std::string text;
        
        if ( rej.get(Tags::MsgType)[0] == '3' ) {
            rejReason = CustomValues::SessionReject;
            if ( rej.contain(Tags::Text) )
                text = rej.get(Tags::Text);
        } else if ( rej.get(Tags::MsgType)[0] == 'j' ) {
            rejReason = CustomValues::BusinessReject;
            if ( rej.contain(Tags::BusinessRejectReason) )
                text = rej.get(Tags::BusinessRejectReason) + " :: ";
            
            if ( rej.contain(Tags::Text) )
                text += rej.get(Tags::Text);
        
        } else {
            LOGGER_WARN << "Unknown rej's msgType: "  << rej.toString(FIX_FIELDS_DELIMITER) << "\n";
        }
        
        const char msgType = rejMsg.get(Tags::MsgType)[0];
        switch ( msgType ) {
            case 'D':
            {
                rejAckMsg.reset(new Message(Values::MsgType::Execution_Report, version()));
                
                rejAckMsg->set(Tags::ExecTransType, Values::ExecTransType::New);
                rejAckMsg->set(Tags::OrdStatus, Values::OrdStatus::Rejected);
                rejAckMsg->set(Tags::ExecType, Values::ExecType::Rejected);
                rejAckMsg->set(Tags::OrdRejReason, rejReason);
            }
                break;
            case 'G':
            case 'F':
            {
                rejAckMsg.reset(new Message(Values::MsgType::Order_Cancel_Reject, version()));
                
                if ( 'G' == msgType )
                    rejAckMsg->set(Tags::CxlRejResponseTo, Values::CxlRejResponseTo::Order_Cancel_Replace_Request);
                else
                    rejAckMsg->set(Tags::CxlRejResponseTo, Values::CxlRejResponseTo::Order_Cancel_Request);
                
                rejAckMsg->set(Tags::CxlRejReason, rejReason);
            }
                break;
            default:
                LOGGER_ERRO << "Unknown rejMsg's msgType: "  << rejMsg.toString(FIX_FIELDS_DELIMITER) << "\n";        
                return false;                
        }        
        
        rejAckMsg->set(Tags::Text, text);
        
        copyField(rejMsg, *rejAckMsg, Tags::ClOrdID);
        copyField(rejMsg, *rejAckMsg, Tags::OrderID);
        copyField(rejMsg, *rejAckMsg, Tags::OrigClOrdID);
        copyField(rejMsg, *rejAckMsg, CustomTags::CorrelationClOrdID);
        
        LOGGER_INFO << " rej :: " << rej.toString(FIX_FIELDS_DELIMITER) << "\n";
        LOGGER_INFO << " rejMsg :: " << rejMsg.toString(FIX_FIELDS_DELIMITER) << "\n";
        LOGGER_INFO << " rejAckMsg :: " << rejAckMsg->toString(FIX_FIELDS_DELIMITER) << "\n";
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    return status;
}

tw::channel_or::eOrderRespType Translator::getOrderRespTypeExecutionReport(const Message& msg) {
    tw::channel_or::eOrderRespType type = tw::channel_or::eOrderRespType::kUnknown;
    try {
        const char execType = msg.get(Tags::ExecType)[0];
        switch ( execType ) {
            case '0':
                type = tw::channel_or::eOrderRespType::kNewAck;
                break;
            case '1':
                type = tw::channel_or::eOrderRespType::kPartFill;
                break;
            case '2':
                type = tw::channel_or::eOrderRespType::kFill;
                break;
            case '4':
            case 'C':
                type = tw::channel_or::eOrderRespType::kCxlAck;
                break;
            case '5':
                type = tw::channel_or::eOrderRespType::kModAck;
                break;
            case '8':
                type = tw::channel_or::eOrderRespType::kNewRej;
                break;
            case 'H':
                type = tw::channel_or::eOrderRespType::kTradeBreak;
                break;
            case 'I':
                type = tw::channel_or::eOrderRespType::kOrderStatus;
                break;
            default:
                LOGGER_ERRO << "Unknown execType: "  << msg.toString(FIX_FIELDS_DELIMITER) << "\n";        
                break;
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    return type;
}

tw::channel_or::eOrderRespType Translator::getOrderRespTypeCancelReject(const Message& msg) {
    tw::channel_or::eOrderRespType type = tw::channel_or::eOrderRespType::kUnknown;
    try {
        const char cxlRejResponseTo = msg.get(Tags::CxlRejResponseTo)[0];
        switch ( cxlRejResponseTo ) {
            case '1':
                type = tw::channel_or::eOrderRespType::kCxlRej;
                break;
            case '2':
                type = tw::channel_or::eOrderRespType::kModRej;
                break;
            default:
                LOGGER_ERRO << "Unknown cxlRejResponseTo: "  << msg.toString(FIX_FIELDS_DELIMITER) << "\n";        
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    return type;
}

// NOTE: no spread support yet, just outrights
//
bool Translator::translateExecutionReport(const Message& msg, tw::channel_or::OrderResp& orderResp) {
    bool status = true;
    try {
        orderResp._type = getOrderRespTypeExecutionReport(msg);
        return translateResp(msg, orderResp);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    return status;
}

bool Translator::translateOrderCancelReject(const Message& msg, tw::channel_or::OrderResp& orderResp) {
    bool status = true;
    try {
        orderResp._type = getOrderRespTypeCancelReject(msg);
        return translateResp(msg, orderResp);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    return status;
}

bool Translator::translateResp(const Message& msg, tw::channel_or::OrderResp& orderResp) {    
    switch ( orderResp._type ) {
        case tw::channel_or::eOrderRespType::kModAck:
            copyField(msg, orderResp._price, Tags::Price);
        case tw::channel_or::eOrderRespType::kNewAck:
        case tw::channel_or::eOrderRespType::kCxlAck:
        case tw::channel_or::eOrderRespType::kOrderStatus:
        case tw::channel_or::eOrderRespType::kMsgRej:
            break;
        case tw::channel_or::eOrderRespType::kNewRej:
            getOrderRejReason(msg, orderResp);
            break;
        case tw::channel_or::eOrderRespType::kModRej:
        case tw::channel_or::eOrderRespType::kCxlRej:
            getCxlRejReason(msg, orderResp);
            break;
        case tw::channel_or::eOrderRespType::kPartFill:
        case tw::channel_or::eOrderRespType::kFill:
        case tw::channel_or::eOrderRespType::kTradeBreak:
            getFillFields(msg, orderResp);
            break;        
        default:
            LOGGER_ERRO << "Unknown orderResp._type: "  << orderResp.toString() << "\n";        
            return false;
    }
    
    // Set common fields
    //
    orderResp._seqNum = msg.getSeqNum();
    getOrderIds(msg, orderResp);
    
    if ( msg.contain(OnixS::FIX::FIX42::Tags::TransactTime) )
        orderResp._exTimestamp = tw::common::THighResTime::parse(msg.get(OnixS::FIX::FIX42::Tags::TransactTime));
    else if ( msg.contain(OnixS::FIX::FIX42::Tags::SendingTime) )
        orderResp._exTimestamp = tw::common::THighResTime::parse(msg.get(OnixS::FIX::FIX42::Tags::SendingTime));
    
    return true;
}
    
} // namespace channel_or_cme
} // namespace tw
