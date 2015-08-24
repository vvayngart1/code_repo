#include <tw/channel_or/processor_orders.h>
#include <tw/instr/instrument_manager.h>
#include <tw/common_strat/strategy_container.h>
#include <tw/common_strat/consumer_proxy.h>
#include <tw/generated/commands_common.h>
#include <bits/stl_vector.h>

namespace tw {
namespace channel_or {
    
static const uint32_t MAX_MOD_COUNTER = 5;
static const uint32_t MAX_MOD_REJ_COUNTER = 5;
static const uint32_t MAX_CXL_REJ_COUNTER = 5;

ProcessorOrders::ProcessorOrders() {
    clear();    
}

ProcessorOrders::~ProcessorOrders() {
    stop();
}
    
void ProcessorOrders::clear() {
    _pool.clear();    
    _table.clear();
    
    _accountInstrOpenOrdersPos.clear();
    _strategyInstrOpenOrdersPos.clear();
}

TOrderPtr ProcessorOrders::createOrder(bool createOrderId) {
    TOrderPtr order(_pool.obtain(), _pool.getDeleter());
    if ( createOrderId )
        order->_orderId = tw::channel_or::UuidFactory::instance().get();    
    return order;
}

TOrderPtr ProcessorOrders::get(const TOrderId& id) {
    return _table.get(id);
}

TOrders ProcessorOrders::getAll() const {
    return _table.get(FilterNull());
}

TOrders ProcessorOrders::getAllForAccount(const TAccountId& x) const {
    return _table.get(FilterAccountId(x));
}

TOrders ProcessorOrders::getAllForAccountStrategy(const TAccountId& x, const TStrategyId& y) const {
    return _table.get(FilterAccountIdStrategyId(x, y));
}

TOrders ProcessorOrders::getAllForAccountStrategyInstr(const TAccountId& x, const TStrategyId& y, const tw::instr::Instrument::TKeyId& z) const {
    return _table.get(FilterAccountIdStrategyIdInstr(x, y, z));
}

TOrders ProcessorOrders::getAllForInstrument(const tw::instr::Instrument::TKeyId& x) const {
    return _table.get(FilterInstrumentId(x));
}

const ProcessorOrders::TInstrOpenOrdersPos& ProcessorOrders::getAllInstrOpenOrdersPosForAccount(const TAccountId& x) {
    TAccountInstrOpenOrdersPos::iterator iter = _accountInstrOpenOrdersPos.find(x);
    if ( iter == _accountInstrOpenOrdersPos.end() ) {
        iter = _accountInstrOpenOrdersPos.insert(TAccountInstrOpenOrdersPos::value_type(x, TInstrOpenOrdersPos())).first;
    }
    
    return iter->second;
}

const AccountInstrOpenOrdersPos& ProcessorOrders::getInstrOpenOrdersPosForAccountInstr(const TAccountId& x, const tw::instr::Instrument::TKeyId& y) {
    TInstrOpenOrdersPos& instrOpenOrdersPos = const_cast<TInstrOpenOrdersPos&>(getAllInstrOpenOrdersPosForAccount(x));
    
    TInstrOpenOrdersPos::iterator iter = instrOpenOrdersPos.find(y);
    if ( iter == instrOpenOrdersPos.end() ) {
        iter = instrOpenOrdersPos.insert(TInstrOpenOrdersPos::value_type(y, AccountInstrOpenOrdersPos())).first;
        iter->second._accountId = x;
        iter->second._instrumentId = y;        
        iter->second._bids.set(0);
        iter->second._asks.set(0);
        
        tw::instr::InstrumentPtr instrument = tw::instr::InstrumentManager::instance().getByKeyId(y);
        if ( instrument )
            iter->second._displayName = instrument->_displayName;
    }   
    
    return iter->second;
}

const ProcessorOrders::TInstrStratOpenOrdersPos& ProcessorOrders::getAllInstrOpenOrdersPosForStrategy(const TStrategyId& x) {
    TStrategyInstrOpenOrdersPos::iterator iter = _strategyInstrOpenOrdersPos.find(x);
    if ( iter == _strategyInstrOpenOrdersPos.end() ) {
        iter = _strategyInstrOpenOrdersPos.insert(TStrategyInstrOpenOrdersPos::value_type(x, TInstrStratOpenOrdersPos())).first;
    }
    
    return iter->second;
}

const StrategyInstrOpenOrdersPos& ProcessorOrders::getInstrOpenOrdersPosForStrategyInstr(const TStrategyId& x, const tw::instr::Instrument::TKeyId& y) {
    TInstrStratOpenOrdersPos& instrOpenOrdersPos = const_cast<TInstrStratOpenOrdersPos&>(getAllInstrOpenOrdersPosForStrategy(x));
    
    TInstrStratOpenOrdersPos::iterator iter = instrOpenOrdersPos.find(y);
    if ( iter == instrOpenOrdersPos.end() ) {
        iter = instrOpenOrdersPos.insert(TInstrStratOpenOrdersPos::value_type(y, StrategyInstrOpenOrdersPos())).first;
        iter->second._strategyId = x;
        iter->second._instrumentId = y;        
        iter->second._bids.set(0);
        iter->second._asks.set(0);
        
        tw::instr::InstrumentPtr instrument = tw::instr::InstrumentManager::instance().getByKeyId(y);
        if ( instrument )
            iter->second._displayName = instrument->_displayName;
    }   
    
    return iter->second;
}

bool ProcessorOrders::isOrderLive(const TOrderPtr& order) {
    if ( NULL == order )
        return false;
    
    switch ( order->_state ) {
        case eOrderState::kUnknown:
        case eOrderState::kRejected:
        case eOrderState::kCancelled:
        case eOrderState::kFilled:
            return false;
        default:
            break;
    }
    
    return true;
}

// ProcessorOut interface
//
bool ProcessorOrders::init(const tw::common::Settings& settings) {
    _settings = settings;
    
    return true;
}

bool ProcessorOrders::start() {
    if ( 0 != _settings._strategy_container_stuck_orders_timeout ) {    
        tw::common::TTimerId id;
        if ( !tw::common::TimerServer::instance().registerClient(this, _settings._strategy_container_stuck_orders_timeout, false, id) )
            return false;
        
        LOGGER_WARN << "Registering for checking stuck orders every: " << _settings._strategy_container_stuck_orders_timeout << " ms" << "\n";
    } else {
        LOGGER_WARN << "Not registering for checking stuck orders" << "\n";
    }
    
    return tw::channel_or::UuidFactory::instance().start();
}

void ProcessorOrders::stop() {
    tw::channel_or::UuidFactory::instance().stop();
    
    _table.clear();
    _accountInstrOpenOrdersPos.clear();
}

bool ProcessorOrders::sendNew(const TOrderPtr& order, Reject& rej) {
    order->_action = tw::common::eCommandSubType::kOrNew;
    
    if ( eOrderState::kUnknown != order->_state ) {
        rej = getRej(eRejectReason::kInvalidOrderState);
        return false;
    }
    order->_state = eOrderState::kPending;
    
    if ( 0 >= order->_qty ) {
        onNewRej(order, (rej = getRej(eRejectReason::kInvalidOrderQty)));
        return false;
    }
    
    if ( eOrderType::kLimit == order->_type && !order->_price.isValid() ) {
        onNewRej(order, (rej = getRej(eRejectReason::kInvalidOrderPrice)));
        return false;
    }    
    
    if ( !_table.add(order->_orderId, order) ) {
        onNewRej(order, (rej = getRej(eRejectReason::kDuplicateOrderId)));
        return false;
    }
    
    changeOpenOrdersCounts(order, 1);
    return true;
}

// NOTE: allows to modify modifies
//
bool ProcessorOrders::sendMod(const TOrderPtr& order, Reject& rej) {
    order->_action = tw::common::eCommandSubType::kOrMod;
    
    if ( !order->_newPrice.isValid() ) {
        rej = getRej(eRejectReason::kInvalidOrderPrice);
        return false;
    }
    
    if ( order->_newPrice == order->_price) {
        rej = getRej(eRejectReason::kSameOrderPrice);
        LOGGER_ERRO << "rej=" << rej.toString() << " :: " << order->toString() << "\n";
        return false;
    }
    
    if ( MAX_MOD_REJ_COUNTER < order->_modRejCounter ) {
        rej = getRej(eRejectReason::kMaxModRejCounter);
        return false;
    }
    
    bool status = true;
    switch ( order->_state ) {
        case eOrderState::kWorking:
            order->_state = eOrderState::kModifying;
        case eOrderState::kModifying:
            if ( MAX_MOD_COUNTER < ++order->_modCounter ) {
                rej = getRej(eRejectReason::kMaxModCounter);
                --order->_modCounter;
                return false;
            }
            break;
        default:
            rej = getRej(eRejectReason::kInvalidOrderState);
            status = false;
            break;
    }
    
    return status;
}

bool ProcessorOrders::sendCxl(const TOrderPtr& order, Reject& rej) {
    order->_action = tw::common::eCommandSubType::kOrCxl;
    if ( MAX_CXL_REJ_COUNTER < order->_cxlRejCounter ) {
        rej = getRej(eRejectReason::kMaxCxlRejCounter);
        return false;
    }
    
    switch ( order->_state ) {
        case eOrderState::kWorking:
        case eOrderState::kModifying:
            order->_state = eOrderState::kCancelling;        
            break;
        default:
            order->_cancelOnAck = true;
            break;
    }
    
    return true;
}

void ProcessorOrders::onCommand(const tw::common::Command& command) {
    if ( command._type != tw::common::eCommandType::kProcessorOrders )
        return;
    
    if ( command._subType != tw::common::eCommandSubType::kList )
        return;
    
    tw::common::Command c;
    if ( command.has("posUpdate") ) {
        if ( !command.has("accountId") ) {
            LOGGER_ERRO << "No accountId in command: " << command.toString() << "\n";
            return;
        }
        
        tw::risk::TAccountId accountId;
        if ( !command.get("accountId", accountId) ) {
            LOGGER_ERRO << "Failed to get accountId from command: " << command.toString() << "\n";
            return;
        }
        
        tw::common_str_util::FastStream<1024*2> buffer;
        const TInstrOpenOrdersPos& positions =  getAllInstrOpenOrdersPosForAccount(accountId);
        
        TInstrOpenOrdersPos::const_iterator iter = positions.begin();
        TInstrOpenOrdersPos::const_iterator end = positions.end();
        for ( ; iter != end; ++iter ) {
            buffer.clear();
            c.clear();
            
            c.addParams("accountId", iter->second._accountId);
            c.addParams("instrumentId", iter->second._instrumentId);
            c.addParams("displayName", iter->second._displayName);
            c.addParams("pos", iter->second._pos);
                    
            c._type = command._type;
            c._subType = command._subType;
            
            tw::common_strat::StrategyContainer::instance().sendToMsgBusConnection(command._connectionId, c.toString()+"\n");
        }
    } else {
        TOrders orders = getAll();
        if ( orders.empty() )
            return;
        
        TOrders::iterator iter = orders.begin();
        TOrders::iterator end = orders.end();
        for ( ; iter != end; ++iter ) {
            c = (*iter)->toCommand();
            c._type = tw::common::eCommandType::kProcessorOrders;
            c._subType = tw::common::eCommandSubType::kOpenOrders;
            
            tw::common_strat::StrategyContainer::instance().sendToMsgBusConnection(command._connectionId, c.toString()+"\n");
        }
    }
}

bool ProcessorOrders::rebuildOrder(const TOrderPtr& order, Reject& rej) {
    if ( !_table.add(order->_orderId, order) ) {
        rej = getRej(eRejectReason::kDuplicateOrderId);
        return false;
    }
    
    order->_instrument = tw::instr::InstrumentManager::instance().getByKeyId(order->_instrumentId);
    if ( NULL == order->_instrument ) {
        rej = getRej(eRejectReason::kSymbolNotConfigured);
        return false;
    }
    
    changeOpenOrdersCounts(order, 1);
    return true;
}

void ProcessorOrders::recordFill(const Fill& fill) {
    onFill(fill);
}

void ProcessorOrders::rebuildPos(const PosUpdate& update) {
    tw::instr::InstrumentConstPtr instr = tw::instr::InstrumentManager::instance().getByDisplayName(update._displayName);
    if ( instr == NULL ) {
        LOGGER_ERRO << "Failed to find instrument for pos update: " << update.toString() << "\n";
        return;
    }
    
    AccountInstrOpenOrdersPos& v = const_cast<AccountInstrOpenOrdersPos&>(getInstrOpenOrdersPosForAccountInstr(update._accountId, instr->_keyId));
    v._pos += update._pos;
    
    StrategyInstrOpenOrdersPos& s = const_cast<StrategyInstrOpenOrdersPos&>(getInstrOpenOrdersPosForStrategyInstr(update._strategyId, instr->_keyId));
    s._pos += update._pos;
}

// ProcessorIn interface
//
void ProcessorOrders::onNewAck(const TOrderPtr& order) {
    order->_action = tw::common::eCommandSubType::kOrNewAck;
    
    if ( eOrderState::kPending != order->_state ) {
        LOGGER_ERRO << "Incorrect state for order: " << order->toString() << "\n";
        return;
    }
    
    order->_state = eOrderState::kWorking;
}

void ProcessorOrders::onNewRej(const TOrderPtr& order, const Reject& rej) {
    order->_action = tw::common::eCommandSubType::kOrNewRej;
    
    if ( eOrderState::kPending != order->_state && eOrderState::kUnknown != order->_state ) {
        LOGGER_ERRO << "Incorrect state for order: " << order->toString() << "\n";
        return;
    }
    
    order->_state = eOrderState::kRejected;
    remove(order);
}

void ProcessorOrders::onModAck(const TOrderPtr& order) {
    order->_action = tw::common::eCommandSubType::kOrModAck;
    order->_modRejCounter = 0;
}

void ProcessorOrders::onModAckPost(const TOrderPtr& order) {
    doOnModPost(order, true);
}

void ProcessorOrders::onModRej(const TOrderPtr& order, const Reject& rej) {
    order->_action = tw::common::eCommandSubType::kOrModRej;
    ++order->_modRejCounter;
}

void ProcessorOrders::onModRejPost(const TOrderPtr& order, const Reject& rej) {
    if ( eRejectType::kInternal == rej._rejType && eRejectSubType::kProcessorOrders == rej._rejSubType ) {
        order->_newPrice.clear();
        order->_exNewPrice = 0.0;
        return;
    }
    
    doOnModPost(order, false);
}

void ProcessorOrders::doOnModPost(const TOrderPtr& order, bool updatePrice) {
    if ( eOrderState::kModifying != order->_state ) {
        LOGGER_ERRO << "Incorrect state for order: " << order->toString() << "\n";
        return;
    }
    
    if ( 0 == --order->_modCounter )
        order->_state = eOrderState::kWorking;
    
    if ( updatePrice ) {
        order->_price = order->_newPrice;
        order->_exPrice = order->_exNewPrice;
    }
    
    order->_newPrice.clear();
    order->_exNewPrice = 0.0;
    
    return;
}

void ProcessorOrders::onCxlAck(const TOrderPtr& order) {
    order->_action = tw::common::eCommandSubType::kOrCxlAck;
    order->_state = eOrderState::kCancelled;
    remove(order);
}

void ProcessorOrders::onCxlRej(const TOrderPtr& order, const Reject& rej) {
    order->_action = tw::common::eCommandSubType::kOrCxlRej;
    ++order->_cxlRejCounter;
    if ( eOrderState::kCancelling != order->_state ) {
        LOGGER_ERRO << "Incorrect state for order: " << order->toString() << "\n";
        return;
    }
    
    order->_state = eOrderState::kWorking;
}

void ProcessorOrders::onFill(const Fill& fill) {        
    TOrderPtr order = fill._order;
    if (  (order.get() == NULL) && (eFillType::kExternal != fill._type) ) {
        LOGGER_ERRO << "Failed to add fill to non_existent order: " << fill.toStringVerbose() << "\n";
        return;
    }
    
    switch ( fill._type ) {
        case eFillType::kNormal:        
            if ( eOrderState::kFilled == order->_state ) {
                LOGGER_ERRO << "Failed to add fill to already filled order: " << fill.toStringVerbose() << "\n";
                return;
            }
            order->_cumQty += fill._qty;
            if ( order->_qty <= order->_cumQty ) {
                order->_state = eOrderState::kFilled;
                remove(order);
            }
        
            break;
        case eFillType::kBusted:
            order->_cumQty -= fill._qty;
            if ( eOrderState::kFilled == order->_state )
                order->_state = eOrderState::kWorking;
            break;
        default:
            break;
    }
    
    changePosCounts(fill);
}

void ProcessorOrders::onAlert(const Alert& alert) {
    // Not implemented - nothing to do
    //
}

void ProcessorOrders::onRebuildOrderRej(const TOrderPtr& order, const Reject& rej) {
    remove(order);
}

bool ProcessorOrders::onTimeout(const tw::common::TTimerId& id) {
    tw::common::THighResTime now = tw::common::THighResTime::now();
    
    // Check all open orders
    //
    typedef std::map<TStrategyId, TOrders> TStrategyStuckOrders;
    
    TStrategyStuckOrders strats;
    {
        TOrders orders = getAll();
        TOrders::iterator iter = orders.begin();
        TOrders::iterator end = orders.end();
        for ( ; iter != end; ++iter ) {
            TOrderPtr& order = *iter;
            switch ( order->_state ) {
                case eOrderState::kPending:
                case eOrderState::kModifying:
                case eOrderState::kCancelling:
                {
                    uint64_t delta = now - order->_timestamp1;
                    if ( delta > _settings._strategy_container_stuck_orders_timeout*1000)
                        strats[order->_strategyId].push_back(order);
                }
                    break;
                default:
                    break;
            }
        }
    }
    
    if ( !strats.empty() ) {
        Alert alert;
        alert._type = eAlertType::kStuckOrders;
    
        TStrategyStuckOrders::iterator iter = strats.begin();
        TStrategyStuckOrders::iterator end = strats.end();
        
        for ( ; iter != end; ++iter ) {
            alert._strategyId = iter->first;
            alert._text.clear();
            TOrders& stuckOrders = iter->second;
            {
                TOrders::iterator iter = stuckOrders.begin();
                TOrders::iterator end = stuckOrders.end();
                for ( ; iter != end; ++iter ) {
                    if ( iter != stuckOrders.begin() )
                        alert._text += ",";
                    
                    alert._text += (*iter)->_orderId.toString();
                    LOGGER_INFO << "Stuck order: " << (*iter)->toString() << "\n";
                }
            }
            tw::common_strat::ConsumerProxy::instance().onAlert(alert);
        }        
    }
    
    return true;
}
 
void ProcessorOrders::remove(const TOrderPtr& order) {
    changeOpenOrdersCounts(order, -1);
    
    if ( !_table.rem(order->_orderId) ) {
        LOGGER_ERRO << "can't remove order: " << order->_orderId.toString() << " :: " << order->toString() << "\n";
    }
}

void ProcessorOrders::changeOpenOrdersCounts(const TOrderPtr& order, int8_t multiplier) {
    AccountInstrOpenOrdersPos& v = const_cast<AccountInstrOpenOrdersPos&>(getInstrOpenOrdersPosForAccountInstr(order->_accountId, order->_instrumentId));
    StrategyInstrOpenOrdersPos& s = const_cast<StrategyInstrOpenOrdersPos&>(getInstrOpenOrdersPosForStrategyInstr(order->_strategyId, order->_instrumentId));
    
    tw::price::Size delta = order->_qty - order->_cumQty;
    switch ( order->_side ) {
        case tw::channel_or::eOrderSide::kBuy:
            if ( 1 == multiplier ) {
                v._bids += delta;
                s._bids += delta;
            } else {
                v._bids -= delta;
                s._bids -= delta;
            }
            break;
        case tw::channel_or::eOrderSide::kSell:
            if ( 1 == multiplier ) {
                v._asks += delta;
                s._asks += delta;
            } else {
                v._asks -= delta;
                s._asks -= delta;
            }
            break;
        default:
            break;
    }
}

void ProcessorOrders::changePosCounts(const Fill& fill) {
    AccountInstrOpenOrdersPos& v = const_cast<AccountInstrOpenOrdersPos&>(getInstrOpenOrdersPosForAccountInstr(fill._accountId, fill._instrumentId));
    StrategyInstrOpenOrdersPos& s = const_cast<StrategyInstrOpenOrdersPos&>(getInstrOpenOrdersPosForStrategyInstr(fill._strategyId, fill._instrumentId));
    
    switch ( fill._side ) {
        case tw::channel_or::eOrderSide::kBuy:
            switch ( fill._type ) {
                case tw::channel_or::eFillType::kNormal:
                    v._bids -= fill._qty;
                    s._bids -= fill._qty;
                case tw::channel_or::eFillType::kExternal:
                    v._pos += fill._qty;
                    s._pos += fill._qty;
                    break;
                case tw::channel_or::eFillType::kBusted:
                    v._pos -= fill._qty;
                    s._pos -= fill._qty;
                    if ( fill._order != NULL ) {
                        v._bids += fill._qty;
                        s._bids += fill._qty;
                    }
                    break;
                default:
                    break;
            }            
            break;
        case tw::channel_or::eOrderSide::kSell:
            switch ( fill._type ) {
                case tw::channel_or::eFillType::kNormal:
                    v._asks -= fill._qty;
                    s._asks -= fill._qty;
                case tw::channel_or::eFillType::kExternal:
                    v._pos -= fill._qty;
                    s._pos -= fill._qty;
                    break;
                case tw::channel_or::eFillType::kBusted:
                    v._pos += fill._qty;
                    s._pos += fill._qty;
                    if ( fill._order != NULL ) {
                        v._asks += fill._qty;
                        s._asks += fill._qty;
                    }
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    
    const_cast<Fill&>(fill)._posAccount.set(v._pos);
}
    
} // namespace channel_or
} // namespace tw
