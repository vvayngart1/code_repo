#pragma once

#include <tw/channel_or_cme/translator.h>
#include <tw/generated/enums_common.h>

namespace tw {
namespace channel_or_cme_bridge {
    
using namespace OnixS::FIX;
using namespace OnixS::FIX::FIX42;
    
typedef tw::channel_or_cme::Translator TParent;
typedef boost::shared_ptr<tw::channel_or::OrderCmeBridgeInfo> TOrderCmeBridgeInfoPtr;
    
class Translator : public TParent {
public:
    Translator();
    ~Translator();
    
    void clear();
    
public:
    // Preformatted messages
    //
    Message& getMsgExRpt() {
        return _msgExRpt;
    }
    
    Message& getMsgOrderCxlRej() {
        return _msgOrderCxlRej;
    }
    
    Message& getMsgRej() { 
        return _msgRej; 
    }    
    
    Message& getMsgBusRej() { 
        return _msgBusRej; 
    }
    
public:
    // CME bridge methods
    //
    
    // 'Out' (FIX to FIX) translation
    //
    
    // NOTE: in methods below, attempt is made to reuse the message for performance
    // optimization, which requires to set the same fields in all new orders.  If
    // becomes too restrictive, need to clear the message and rebuild from start.
    //
    bool translateNew(const Message& source, TOrderCmeBridgeInfoPtr& orderCmeBridgeInfoPtr);
    bool translateMod(const Message& source, TOrderCmeBridgeInfoPtr& orderCmeBridgeInfoPtr);
    bool translateCxl(const Message& source, TOrderCmeBridgeInfoPtr& orderCmeBridgeInfoPtr);
    
    bool translateCxl(TOrderCmeBridgeInfoPtr& orderCmeBridgeInfoPtr);
    
    // 'In' (FIX to FIX) translation
    //
    tw::common::eCMEOrBridgeMsgType getInboundMessageType(const Message& source);
    bool translateInboundApplicationMsg(const Message& source,  TOrderCmeBridgeInfoPtr& orderCmeBridgeInfoPtr, const tw::common::eCMEOrBridgeMsgType& msgType,  const tw::channel_or::eOrderRespType& orderRespType);
    tw::common::eCMEOrBridgeMsgType translateInboundSessionMsg(const Message& source);
    
private:
    void removeField(Message& dest, uint32_t tag) {
        if ( dest.contain(tag) )
            dest.remove(tag);
    }
    
private:
    Message _msgExRpt;
    Message _msgOrderCxlRej;
    Message _msgRej;
    Message _msgBusRej;
};
    
} // namespace channel_or_cme_bridge
} // namespace tw
