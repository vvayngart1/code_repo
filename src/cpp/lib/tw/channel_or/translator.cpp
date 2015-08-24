#include <tw/channel_or/translator.h>
#include <tw/channel_or/uuid_factory.h>

#include "tw/price/ticks_converter.h"

namespace tw {
namespace channel_or {
    
bool Translator::translateNewAck(const tw::channel_or::OrderResp& orderResp, TOrderPtr& order) {
    copyCommonFields(orderResp, order);
    return true;
}

bool Translator::translateNewRej(const tw::channel_or::OrderResp& orderResp, TOrderPtr& order, Reject& rej) {
    copyCommonFields(orderResp, order);
    rej = getRej(orderResp);
    return true;
}

bool Translator::translateModAck(const tw::channel_or::OrderResp& orderResp, TOrderPtr& order) {
    if ( orderResp._price.empty() ) {
        LOGGER_INFO << "Invalid price in: " << orderResp.toString() << "\n";
        return false;
    }
    
    copyCommonFields(orderResp, order);
    order->_origClOrderId = order->_clOrderId = orderResp._clOrderId;    
    order->_newPrice = order->_instrument->_tc->fromExchangePrice(boost::lexical_cast<double>(orderResp._price));
    order->_exNewPrice = boost::lexical_cast<double>(orderResp._price);
    return true;
}

bool Translator::translateModRej(const tw::channel_or::OrderResp& orderResp, TOrderPtr& order, Reject& rej) {
    copyCommonFields(orderResp, order);
    rej = getRej(orderResp);
    return true;
}

bool Translator::translateCxlAck(const tw::channel_or::OrderResp& orderResp, TOrderPtr& order) {
    copyCommonFields(orderResp, order);
    return true;
}

bool Translator::translateCxlRej(const tw::channel_or::OrderResp& orderResp, TOrderPtr& order, Reject& rej) {
    copyCommonFields(orderResp, order);
    rej = getRej(orderResp);
    return true;
}

bool Translator::translateFill(const tw::channel_or::OrderResp& orderResp, Fill& fill) {
    bool status = true;
    try {
        switch (orderResp._type) {
            case tw::channel_or::eOrderRespType::kPartFill:
            case tw::channel_or::eOrderRespType::kFill:
                fill._type = tw::channel_or::eFillType::kNormal;
                fill._exchangeFillId = orderResp._exFillId;
                break;
            case tw::channel_or::eOrderRespType::kTradeBreak:
                fill._type = tw::channel_or::eFillType::kBusted;
                fill._exchangeFillId = orderResp._exFillRefId;
                break;
            default:
                LOGGER_ERRO << "Unknown orderResp._type: "  << orderResp.toString() << "\n";        
                return false;
        }
        
        // NOTE: right now support only outrights
        //
        fill._subType = tw::channel_or::eFillSubType::kOutright;
        fill._fillId = tw::channel_or::UuidFactory::instance().get();
        
        copyCommonHeaderFields(orderResp, &fill);
        copy(fill._order, fill);        
        
        fill._qty.set(orderResp._lastShares);
        fill._exPrice = boost::lexical_cast<double>(orderResp._lastPrice);
        fill._price = fill._order->_instrument->_tc->fromExchangePrice(fill._exPrice);
        fill._liqInd = orderResp._liqInd;
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    return status;
}

void Translator::copy(const TOrderPtr& source, Fill& dest) {
    dest._accountId = source->_accountId;
    dest._strategyId = source->_strategyId;
    dest._instrumentId = source->_instrumentId;
    dest._orderId = source->_orderId;
    dest._side = source->_side;
    if ( tw::channel_or::eOrderState::kCancelling == source->_state )
        dest._pickOff = true;
}

bool Translator::translateUntrackedResp(const tw::channel_or::OrderResp& orderResp, tw::channel_or::Alert& alert) {
    bool status = true;
    try {
        switch ( orderResp._type ) {
            case tw::channel_or::eOrderRespType::kNewAck:
            case tw::channel_or::eOrderRespType::kModAck:
            case tw::channel_or::eOrderRespType::kCxlAck:
            case tw::channel_or::eOrderRespType::kOrderStatus:
            case tw::channel_or::eOrderRespType::kMsgRej:
            case tw::channel_or::eOrderRespType::kNewRej:
            case tw::channel_or::eOrderRespType::kModRej:
            case tw::channel_or::eOrderRespType::kCxlRej:
                alert._type = tw::channel_or::eAlertType::kUntrackedOrder;
                break;
            case tw::channel_or::eOrderRespType::kPartFill:
            case tw::channel_or::eOrderRespType::kFill:
            case tw::channel_or::eOrderRespType::kTradeBreak:
                alert._type = tw::channel_or::eAlertType::kUntrackedFill;
                break;        
            default:
                LOGGER_ERRO << "Unknown orderResp._type: "  << orderResp.toString() << "\n";        
                return false;
        }
        
        alert._text = orderResp.toStringVerbose();
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    return status;
}    
    
} // namespace channel_or
} // namespace tw
