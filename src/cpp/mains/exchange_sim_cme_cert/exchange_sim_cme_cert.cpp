#include "exchange_sim_cme_cert.h"

#include <tw/price/ticks_converter.h>
#include <tw/channel_or/processor_orders.h>
#include <tw/generated/commands_common.h>
#include <tw/generated/risk_defs.h>
#include <tw/risk/risk_storage.h>

ExchangeSimCmeCert::ExchangeSimCmeCert() { 
    _params._name = "ex_sim_cme_cert";
}

ExchangeSimCmeCert::~ExchangeSimCmeCert() {
    stop();
}

void ExchangeSimCmeCert::clear() {
    _account.clear();
    _markets.clear();
}

bool ExchangeSimCmeCert::init(const tw::common::Settings& settings, const tw::risk::Strategy& strategyParams) {
    if ( !IStrategy::init(settings, strategyParams) )
        return false;
    
    // Get all instruments configured for the account
    //
    if ( !tw::risk::RiskStorage::instance().getAccount(_account, settings._trading_account) ) {
        LOGGER_ERRO << "Failed to get configuration for trading account: " << settings._trading_account << "\n";
        return false;
    }
    
    std::vector<tw::risk::AccountRiskParams> params;
    if ( !tw::risk::RiskStorage::instance().getAccountRiskParams(params, _account) || params.empty() ) {
        LOGGER_ERRO << "Failed to get configuration for trading account risk paramas: " << _account.toString() << "\n";
        return false;
    }
    
    // Subscribe to quotes for configured instruments
    //
    {
        std::vector<tw::risk::AccountRiskParams>::iterator iter = params.begin();
        std::vector<tw::risk::AccountRiskParams>::iterator end = params.end();

        for ( ; iter != end; ++iter ) {
            if ( (*iter)._tradeEnabled )
                addQuoteSubscription((*iter)._displayName);
        }
    }
    
    // Register itself for a timeout
    //
    tw::common::TTimerId id;
    //if ( !tw::common::TimerServer::instance().registerClient(this, settings._global_simTimeout, false, id) )
    if ( !tw::common::TimerServer::instance().registerClient(this, 1000, false, id) )
        return false;
    
    return true;
}  

bool ExchangeSimCmeCert::start() {
    return true;
}

bool ExchangeSimCmeCert::stop() {    
    return true;
}

void ExchangeSimCmeCert::recordExternalFill(const tw::channel_or::Fill& fill) {
    LOGGER_INFO << "External fill: "  << fill.toString() << "\n";
}

bool ExchangeSimCmeCert::rebuildOrder(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej) {
    // Not implemented - nothing to do
    //
    return true;
}

void ExchangeSimCmeCert::rebuildPos(const tw::channel_or::PosUpdate& update) {
    // Not implemented - nothing to do
    //
}

void ExchangeSimCmeCert::onConnectionUp(tw::common_strat::TMsgBusConnection::native_type id, tw::common_strat::TMsgBusConnection::pointer connection) {
    // Not implemented - nothing to do
    //
}
    
void ExchangeSimCmeCert::onConnectionDown(tw::common_strat::TMsgBusConnection::native_type id) {
    // Not implemented - nothing to do
    //
}

void ExchangeSimCmeCert::onConnectionData(tw::common_strat::TMsgBusConnection::native_type id, const tw::common::Command& cmnd) {
    try {
        if ( tw::common::eCommandType::kStrat != cmnd._type )
            return;
        
        switch ( cmnd._subType ) {
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
                LOGGER_ERRO << "Unsupported command: "  << cmnd.toString() << "\n" << "\n";
                break;                    
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

bool ExchangeSimCmeCert::onTimeout(const tw::common::TTimerId& id) {
    try {
        tw::channel_or::TOrders orders;
        
        tw::channel_or::TOrderPtr order;
        tw::channel_or::Reject rej;
        
        TMarkets::iterator iter = _markets.begin();
        TMarkets::iterator end = _markets.end();
        
        for ( ; iter != end; ++iter ) {
            const tw::price::Quote& quote = iter->second;
            const tw::price::PriceSizeNumOfOrders bid = quote._book[0]._bid;
            const tw::price::PriceSizeNumOfOrders ask = quote._book[0]._ask;
            orders = tw::channel_or::ProcessorOrders::instance().getAllForInstrument(quote._instrumentId);
            tw::instr::InstrumentPtr instrument = tw::instr::InstrumentManager::instance().getByKeyId(quote._instrumentId);
            switch ( orders.size() ) {
                case 0:
                    order = createOrder(tw::channel_or::eOrderSide::kBuy, instrument, bid._size, bid._price, std::string("Empty book - new buy"));
                    tw::common_strat::StrategyContainer::instance().sendNew(order, rej);
                    
                    order = createOrder(tw::channel_or::eOrderSide::kSell, instrument, ask._size, ask._price, std::string("Empty book - new buy"));
                    tw::common_strat::StrategyContainer::instance().sendNew(order, rej);
                    break;
                case 1:
                    if ( needToCancel(orders[0], quote) )
                        tw::common_strat::StrategyContainer::instance().sendCxl(orders[0], rej);
                    
                    if ( tw::channel_or::eOrderSide::kBuy == orders[0]->_side ) {
                        order = createOrder(tw::channel_or::eOrderSide::kSell, instrument, ask._size, ask._price, std::string("Empty book - new buy"));
                        tw::common_strat::StrategyContainer::instance().sendNew(order, rej);
                    } else {
                        order = createOrder(tw::channel_or::eOrderSide::kBuy, instrument, bid._size, bid._price, std::string("Empty book - new buy"));
                        tw::common_strat::StrategyContainer::instance().sendNew(order, rej);
                    }
                        
                    break;
                case 2:
                    if ( needToCancel(orders[0], quote) )
                        tw::common_strat::StrategyContainer::instance().sendCxl(orders[0], rej);
                    
                    if ( needToCancel(orders[1], quote) )
                        tw::common_strat::StrategyContainer::instance().sendCxl(orders[0], rej);
                    break;
                default:
                    for ( size_t i = 0; i < orders.size(); ++i ) {
                        tw::common_strat::StrategyContainer::instance().sendCxl(orders[i], rej);
                    }
                    break;
            }
        }
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    return true;
}

void ExchangeSimCmeCert::onQuote(const tw::price::Quote& quote) {
    if ( quote.isValid() )
        _markets[quote._instrument->_keyId] = quote;
}

void ExchangeSimCmeCert::onNewAck(const tw::channel_or::TOrderPtr& order) {    
}

void ExchangeSimCmeCert::onNewRej(const tw::channel_or::TOrderPtr& order, const tw::channel_or::Reject& rej) {
}

void ExchangeSimCmeCert::onModAck(const tw::channel_or::TOrderPtr& order) {
}

void ExchangeSimCmeCert::onModRej(const tw::channel_or::TOrderPtr& order, const tw::channel_or::Reject& rej) {
}

void ExchangeSimCmeCert::onCxlAck(const tw::channel_or::TOrderPtr& order) {
}

void ExchangeSimCmeCert::onCxlRej(const tw::channel_or::TOrderPtr& order, const tw::channel_or::Reject& rej) {
}

void ExchangeSimCmeCert::onFill(const tw::channel_or::Fill& fill) {
}    

void ExchangeSimCmeCert::onAlert(const tw::channel_or::Alert& alert) {
}

void ExchangeSimCmeCert::onRebuildOrderRej(const tw::channel_or::TOrderPtr& order, const tw::channel_or::Reject& rej) {
}

tw::channel_or::TOrderPtr ExchangeSimCmeCert::createOrder(const tw::channel_or::eOrderSide side,
                                                          const tw::instr::InstrumentPtr instrument,
                                                          const tw::price::Size qty,
                                                          const tw::price::Ticks price,
                                                          const std::string& reason) {
    tw::common::THighResTime now = tw::common::THighResTime::now();
    
    tw::channel_or::TOrderPtr order = tw::channel_or::ProcessorOrders::instance().createOrder();
    order->_client.registerClient(this);
    
    order->_type = tw::channel_or::eOrderType::kLimit;
    order->_side = side;
    order->_accountId = _params._accountId;
    order->_strategyId = _params._id;
    order->_instrument = instrument;
    order->_instrumentId = instrument->_keyId;
    order->_qty = qty;
    order->_price = price;
    order->_manual = false;
    order->_stratReason = reason;
    
    order->_trTimestamp = now;
    
    return order;
}

bool ExchangeSimCmeCert::needToCancel(const tw::channel_or::TOrderPtr& order, const tw::price::Quote& quote) {
    if ( tw::channel_or::eOrderSide::kBuy == order->_side ) {
        const tw::price::PriceSizeNumOfOrders bid = quote._book[0]._bid;
        if ( bid._price != order->_price )
            return true;
    } else {
        const tw::price::PriceSizeNumOfOrders ask = quote._book[0]._ask;
        if ( ask._price != order->_price )
            return true;
    }
    
    return false;
}
