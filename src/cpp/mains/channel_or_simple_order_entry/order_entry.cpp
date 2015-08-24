#include "order_entry.h"

#include <tw/price/ticks_converter.h>
#include <tw/channel_or/processor_orders.h>
#include <tw/generated/commands_common.h>

#include <iomanip>

OrderEntry::OrderEntry() { 
    _params._name = "channel_or_simple_order_entry";
}

OrderEntry::~OrderEntry() {
    stop();
}

void OrderEntry::clear() {
}

bool OrderEntry::init(const tw::common::Settings& settings, const tw::risk::Strategy& strategyParams) {
    if ( !IStrategy::init(settings, strategyParams) )
        return false;
    
    return true;
}  

bool OrderEntry::start() {
    return true;
}

bool OrderEntry::stop() {    
    return true;
}

void OrderEntry::recordExternalFill(const tw::channel_or::Fill& fill) {
    LOGGER_INFO << "External fill: "  << fill.toString() << "\n";
}

bool OrderEntry::rebuildOrder(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej) {
    try {
       if ( _params._accountId != order->_accountId ) {
           rej = getReject("Order for another account");
           return false;
       }
       
       if ( _params._id != order->_strategyId ) {
           rej = getReject("Order for another strategy");
           return false;
       }
       
       const_cast<tw::channel_or::TOrderPtr&>(order)->_instrument = tw::instr::InstrumentManager::instance().getByKeyId(order->_instrumentId);
       if ( NULL == order->_instrument ) {
           rej = getReject("Invalid instrument");
           return false;
       }
       
        const_cast<tw::channel_or::TOrderPtr&>(order)->_client.registerClient(this);
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    
    return true;
}

void OrderEntry::rebuildPos(const tw::channel_or::PosUpdate& update) {
    // Not implemented - nothing to do
    //
}

void OrderEntry::onConnectionUp(tw::common_strat::TMsgBusConnection::native_type id, tw::common_strat::TMsgBusConnection::pointer connection) {
    // Not implemented - nothing to do
    //
}
    
void OrderEntry::onConnectionDown(tw::common_strat::TMsgBusConnection::native_type id) {
    // Not implemented - nothing to do
    //
}

void OrderEntry::onConnectionData(tw::common_strat::TMsgBusConnection::native_type id, const tw::common::Command& cmnd) {
    try {
        tw::common::THighResTime now = tw::common::THighResTime::now();
        
        if ( tw::common::eCommandType::kStrat != cmnd._type )
            return;
        
        LOGGER_INFO << id << " :: " << cmnd.toString() << "\n";
        tw::channel_or::TOrderPtr order;
        tw::channel_or::Reject rej;
        std::string text;
        
        switch ( cmnd._subType ) {
            case tw::common::eCommandSubType::kOrNew:
            {
                tw::channel_or::OrderWireOrderEntry orderWire = tw::channel_or::OrderWireOrderEntry::fromCommand(cmnd);
                if ( orderWire._orderId.isValid() ) {
                    order = tw::channel_or::ProcessorOrders::instance().createOrder(false);
                    order->_orderId = orderWire._orderId;
                } else {                    
                    order = tw::channel_or::ProcessorOrders::instance().createOrder();
                    orderWire._orderId = order->_orderId;
                }
                reinterpret_cast<tw::channel_or::OrderWire&>(*order) = orderWire;
                order->_client.registerClient(this);
                order->_accountId = _params._accountId;
                order->_strategyId = _params._id;
                order->_instrument = tw::instr::InstrumentManager::instance().getByDisplayName(orderWire._displayName);
                order->_manual = true;
                order->_trTimestamp = now;

                if ( NULL == order->_instrument ) {
                    text = "Can't find instrument for order: " + orderWire.toString();
                    sendAlert(text);
                    return;
                }

                orderWire._instrumentId = order->_instrumentId = order->_instrument->_keyId;
                order->_price = order->_instrument->_tc->fromExchangePrice(orderWire._exPrice);
                order->_stratReason = "order_entry :: manual_new";

                tw::common_strat::StrategyContainer::instance().sendNew(order, rej);
                
                tw::channel_or::TOrders orders = tw::channel_or::ProcessorOrders::instance().getAllForAccountStrategyInstr(_params._accountId, _params._id, order->_instrument->_keyId);
                for ( uint32_t i = 0; i < orders.size(); ++i ) {
                    LOGGER_INFO << "Open order: " << orders[i]->toString() << "\n";
                }
            }
                break;
            case tw::common::eCommandSubType::kOrMod:
            {
                tw::channel_or::OrderWireOrderEntry orderWire = tw::channel_or::OrderWireOrderEntry::fromCommand(cmnd);
                order = tw::channel_or::ProcessorOrders::instance().get(orderWire._orderId);
                if ( !order ) {
                    text = "Can't find order for id: " + orderWire._orderId.toString();
                    sendAlert(text);
                } else {
                    tw::price::Ticks newPrice = order->_instrument->_tc->fromExchangePrice(orderWire._exPrice);

                    if ( newPrice.isValid() )
                        order->_newPrice = newPrice;
                    else
                        order->_newPrice = order->_price;

                    order->_manual = true;
                    order->_trTimestamp = now;
                    order->_stratReason = "order_entry :: manual_mod";

                    tw::common_strat::StrategyContainer::instance().sendMod(order, rej);
                }
            }
                break;
            case tw::common::eCommandSubType::kOrCxl:
            {
                tw::channel_or::OrderWire orderWire = tw::channel_or::OrderWire::fromCommand(cmnd);
                order = tw::channel_or::ProcessorOrders::instance().get(orderWire._orderId);
                if ( !order ) {
                    text = "Can't find order for id: " + orderWire._orderId.toString();
                    sendAlert(text);
                } else {
                    order->_manual = true;
                    order->_trTimestamp = now;
                    order->_stratReason = "order_entry :: manual_cxl";
                    tw::common_strat::StrategyContainer::instance().sendCxl(order, rej);
                }
            }
                break;
            case tw::common::eCommandSubType::kParams:
            {
                tw::common_commands::StratParams params = tw::common_commands::StratParams::fromCommand(cmnd);
                if ( getName() == params._name ) {
                    // Cancel all open orders
                    //
                    if ( tw::common_commands::eStratAction::kNoOrders == params._action )
                        tw::common_strat::StrategyContainer::instance().sendCxlForAccountStrategy(getAccountId(), getStrategyId());
                }
            }
                break;
            default:
                break;                    
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void OrderEntry::onNewAck(const tw::channel_or::TOrderPtr& order) {    
}

void OrderEntry::onNewRej(const tw::channel_or::TOrderPtr& order, const tw::channel_or::Reject& rej) {
}

void OrderEntry::onModAck(const tw::channel_or::TOrderPtr& order) {
}

void OrderEntry::onModRej(const tw::channel_or::TOrderPtr& order, const tw::channel_or::Reject& rej) {
}

void OrderEntry::onCxlAck(const tw::channel_or::TOrderPtr& order) {
}

void OrderEntry::onCxlRej(const tw::channel_or::TOrderPtr& order, const tw::channel_or::Reject& rej) {
}

void OrderEntry::onFill(const tw::channel_or::Fill& fill) {
}    

void OrderEntry::onAlert(const tw::channel_or::Alert& alert) {
}

void OrderEntry::onRebuildOrderRej(const tw::channel_or::TOrderPtr& order, const tw::channel_or::Reject& rej) {
}

void OrderEntry::sendAlert(const std::string& text) {
    try {
        tw::channel_or::Alert alert;
        alert._text = text;
        
        tw::common_strat::StrategyContainer::instance().onAlert(alert);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}
