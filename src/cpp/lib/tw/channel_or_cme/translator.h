#pragma once

#include <tw/common/defs.h>
#include <tw/channel_or_cme/settings.h>
#include <tw/generated/channel_or_defs.h>

#include <OnixS/FIXEngine.h>

#include <boost/shared_ptr.hpp>

namespace tw {
namespace channel_or_cme { 
    
static const char FIX_FIELDS_DELIMITER = '|';
        
namespace CustomTags {
    const int ManualOrderIndicator = 1028;
    const int AggressorIndicator = 1057;
    const int ApplicationSystemName = 1603;
    const int TradingSystemVersion = 1604;
    const int ApplicationSystemVendor = 1605;

    const int CtiCode = 9702;
    const int CorrelationClOrdID = 9717;
    const int OFMOverride = 9768;
}


namespace CustomValues {
    const int SessionReject = 100001;
    const int BusinessReject = 100002;
}

using namespace OnixS::FIX;
using namespace OnixS::FIX::FIX42;

typedef boost::shared_ptr<Message> TMessagePtr;
    
class Translator {
public:
    static Version version() {
        return OnixS::FIX::FIX_42;
    }               
    
public:
    Translator();
    ~Translator();
    
    void clear();
    
public:
    bool init(const TSessionSettingsPtr& settings);
    
public:
    // Helper functions
    //
    bool shouldSend(const Message& msg);
    
    TSessionSettingsPtr getSettings() const {
        return _settings;
    }
    
public:
    // Preformatted messages
    //
    Message& getMsgLogon() {
        return _msgLogon;
    }
    
    Message& getMsgNew() {
        return _msgNew;
    }
    
    Message& getMsgMod() { 
        return _msgMod; 
    }
    
    Message& getMsgCxl() { 
        return _msgCxl; 
    }
    
public:
    // 'Out' (prop to FIX) translation
    //
    
    // NOTE: in methods below, attempt is made to reuse the message for performance
    // optimization, which requires to set the same fields in all new orders.  If
    // becomes too restrictive, need to clear the message and rebuild from start.
    //
    bool translateNew(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej);
    bool translateMod(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej);
    bool translateCxl(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej);
    
public:
    // 'In' (FIX to FIX) translation
    //
    bool translateMsgRej(const Message& rej, const Message& rejMsg, TMessagePtr& rejAckMsg);

    // 'In' (FIX to prop) translation
    //
    bool translateExecutionReport(const Message& msg, tw::channel_or::OrderResp& orderResp);
    bool translateOrderCancelReject(const Message& msg, tw::channel_or::OrderResp& orderResp);
    
    tw::channel_or::eOrderRespType getOrderRespTypeExecutionReport(const Message& msg);
    tw::channel_or::eOrderRespType getOrderRespTypeCancelReject(const Message& msg);
    
protected:
    void setConstantFields(Message& msg);
    bool translateResp(const Message& msg, tw::channel_or::OrderResp& orderResp);
    
protected:
    tw::channel_or::Reject getRej(tw::channel_or::eRejectReason reason) {
        tw::channel_or::Reject rej;
        
        rej._rejType = tw::channel_or::eRejectType::kInternal;
        rej._rejSubType = tw::channel_or::eRejectSubType::kTranslator;
        rej._rejReason = reason;
        
        return rej;
    }
    
    void copyField(const Message& source, Message& dest, uint32_t tag) {
        if ( source.contain(tag) )
            dest.set(tag, source.get(tag));
    }
    
    void copyField(const Message& source, std::string& dest, uint32_t tag) {
        if ( source.contain(tag) )
            dest = source.get(tag);
    }
    
    void getOrderIds(const Message& source, tw::channel_or::OrderResp& orderResp) {
        copyField(source, orderResp._clOrderId, Tags::ClOrdID);
        copyField(source, orderResp._origClOrderId, Tags::OrigClOrdID);
        copyField(source, orderResp._corrClOrderId, CustomTags::CorrelationClOrdID);
        
        // Get order id if newAck/newRej/modAck/cxlAck/orderStatus
        //
        switch ( orderResp._type ) {
            case tw::channel_or::eOrderRespType::kNewAck:
            case tw::channel_or::eOrderRespType::kNewRej:                
            case tw::channel_or::eOrderRespType::kModAck:
            case tw::channel_or::eOrderRespType::kCxlAck:
            case tw::channel_or::eOrderRespType::kOrderStatus:
                copyField(source, orderResp._exOrderId, Tags::OrderID);
                break;            
            default:
                break;
        }
    }
    
    void getOrderRejReason(const Message& source, tw::channel_or::OrderResp& orderResp) {
        copyField(source, orderResp._exRejReason, Tags::OrdRejReason);
        copyField(source, orderResp._exRejText, Tags::Text);
    }
    
    void getCxlRejReason(const Message& source, tw::channel_or::OrderResp& orderResp) {
        copyField(source, orderResp._exRejReason, Tags::CxlRejReason);
        copyField(source, orderResp._exRejText, Tags::Text);
    }
    
    void getFillFields(const Message& source, tw::channel_or::OrderResp& orderResp) {
        if ( source.contain(Tags::LastShares) )
            orderResp._lastShares = source.getInt32(Tags::LastShares);
        
        if ( source.contain(CustomTags::AggressorIndicator) ) {
            switch ( source.get(CustomTags::AggressorIndicator)[0]) {
                case 'Y':
                case 'y':
                    orderResp._liqInd = tw::channel_or::eLiqInd::kRem;
                    break;
                case 'N':
                case 'n':
                    orderResp._liqInd = tw::channel_or::eLiqInd::kAdd;
                    break;
                default:
                    orderResp._liqInd = tw::channel_or::eLiqInd::kUnknown;
                    break;
            }
        } else {
            orderResp._liqInd = tw::channel_or::eLiqInd::kUnknown;
        }
        
        copyField(source, orderResp._lastPrice, Tags::LastPx);
        copyField(source, orderResp._exFillId, Tags::ExecID);
        copyField(source, orderResp._exFillRefId, Tags::ExecRefID);
    }    
    
protected:
    TSessionSettingsPtr _settings;
    
    Message _msgLogon;
    Message _msgNew;
    Message _msgMod;
    Message _msgCxl;
};
    
} // namespace channel_or_cme
} // namespace tw
