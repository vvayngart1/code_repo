#pragma once

#include <tw/common/defs.h>
#include <tw/generated/channel_or_defs.h>

namespace tw {
namespace channel_or {    
   
class Translator {
public:
    // 'In' (prop to prop) translation
    //
    static bool translateNewAck(const tw::channel_or::OrderResp& orderResp, TOrderPtr& order);
    static bool translateNewRej(const tw::channel_or::OrderResp& orderResp, TOrderPtr& order, Reject& rej);
    static bool translateModAck(const tw::channel_or::OrderResp& orderResp, TOrderPtr& order);
    static bool translateModRej(const tw::channel_or::OrderResp& orderResp, TOrderPtr& order, Reject& rej);
    static bool translateCxlAck(const tw::channel_or::OrderResp& orderResp, TOrderPtr& order);
    static bool translateCxlRej(const tw::channel_or::OrderResp& orderResp, TOrderPtr& order, Reject& rej);
    
    static bool translateFill(const tw::channel_or::OrderResp& orderResp, Fill& fill);
    static bool translateUntrackedResp(const tw::channel_or::OrderResp& orderResp, tw::channel_or::Alert& alert);
    
private:
    static void copy(const TOrderPtr& source, Fill& dest);
    
    static Reject getRej(const tw::channel_or::OrderResp& orderResp) {
        Reject rej;
        
        rej._rejType = tw::channel_or::eRejectType::kExternal;
        rej._rejSubType = tw::channel_or::eRejectSubType::kExchange;
        rej._text = orderResp._exRejReason + " :: " + orderResp._exRejText;
        
        return rej;
    }
    
    template <typename TType>
    static void copyCommonHeaderFields(const tw::channel_or::OrderResp& orderResp, TType* obj) {
        obj->_exTimestamp = orderResp._exTimestamp;
        obj->_timestamp1 = orderResp._timestamp1;
        obj->_timestamp2 = orderResp._timestamp2;
    }
       
    static void copyCommonFields(const tw::channel_or::OrderResp& orderResp, TOrderPtr& order) {
        copyCommonHeaderFields(orderResp, order.get());
        if ( !orderResp._exOrderId.empty() )
            order->_exOrderId = orderResp._exOrderId;
    }
};
    
} // namespace channel_or
} // namespace tw
