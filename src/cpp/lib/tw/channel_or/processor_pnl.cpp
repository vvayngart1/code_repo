#include <tw/channel_or/processor_pnl.h>
#include <tw/channel_or/processor_orders.h>
#include <tw/risk/risk_storage.h>
#include <tw/common_strat/strategy_container.h>
#include <tw/common_trade/wbo_manager.h>
#include <tw/generated/commands_common.h>

namespace tw {
namespace channel_or {

ProcessorPnL::ProcessorPnL() {
    clear();    
}

ProcessorPnL::~ProcessorPnL() {
    stop();
}
    
void ProcessorPnL::clear() {
    _printPnL = false;
    _account.clear();
    _accountPnL.clear();
    _stratsPnL.clear();
}

bool ProcessorPnL::init(const tw::risk::Account& account, const std::vector<tw::risk::Strategy>& strats) {
    _account = account;
    
    LOGGER_INFO << "ACCOUNT_LIMITS: " << _account.toString() << "\n";
    
    std::vector<tw::risk::Strategy>::const_iterator iter = strats.begin();
    std::vector<tw::risk::Strategy>::const_iterator end = strats.end();
    
    for ( ; iter != end; ++iter ) {
        if ( iter->_tradeEnabled ) {
            StratPnL& stratPnl = getOrCreateStrat((*iter)._id);
            stratPnl._limits = (*iter);
            LOGGER_INFO << "STRATEGY_LIMITS: " << stratPnl._limits.toString() << "\n";
        }
    }
    
    return true;
}

// ProcessorOut interface
//
bool ProcessorPnL::init(const tw::common::Settings& settings) {
    _printPnL = settings._trading_print_pnl;
    _account._name = settings._trading_account;
    
    return true;
}

bool ProcessorPnL::start() {
    if ( !_stratsPnL.empty() )
        return true;
    
    tw::risk::Account account;
    if ( !tw::risk::RiskStorage::instance().getAccount(account, _account._name) ) {
        LOGGER_ERRO << "Failed to get configuration for trading account: " << _account._name << "\n";
        return false;
    }
    
    std::vector<tw::risk::Strategy> strats;
    if ( !tw::risk::RiskStorage::instance().getStrategies(strats, account) ) {
        LOGGER_ERRO << "Failed to get configuration for trading account risk paramas: " << account.toString() << "\n";
        return false;
    }
    
    return init(account, strats);
}

void ProcessorPnL::stop() {
    clear();
}

bool ProcessorPnL::isAccountTotalLossReached(double& pnl_amount, std::string& reason) {
    double fees_amount = _accountPnL.getFeesPaid().total();
    pnl_amount = -(_accountPnL.getRealizedPnL()+ _accountPnL.getUnrealizedPnL());
    if ( (pnl_amount+fees_amount) > _account._maxTotalLoss ) {
        char buf[256];
        fast_dtoa(pnl_amount+fees_amount, &buf[0], 2);
        reason = std::string(&buf[0]) + " :: " + boost::lexical_cast<std::string>(_account._maxTotalLoss);
        return true;
    }
    
    return false;
}

bool ProcessorPnL::sendNew(const TOrderPtr& order, Reject& rej) {
    // Update pnl
    //
    tw::common_trade::WboManager::TTv& tv = tw::common_trade::WboManager::instance().getOrCreateTv(order->_instrumentId);
    TPnL& pnl = getOrCreatePnl(order->_strategyId, order->_instrumentId);
    pnl.onTv(tv);
    
    if ( _printPnL )
        logPnl();
    
    // If any of limits exceeded, allow only orders toward 'close' of the position
    //
    double pnl_amount = 0.0;
    double fees_amount = 0.0;    
    
    // Check account limits
    //
    fees_amount = _accountPnL.getFeesPaid().total();
    
    pnl_amount = -1 * _accountPnL.getRealizedPnL();
    if ( (pnl_amount+fees_amount) > _account._maxRealizedLoss ) {
        const AccountInstrOpenOrdersPos& openOrdersPos = ProcessorOrders::instance().getInstrOpenOrdersPosForAccountInstr(order->_accountId, order->_instrumentId);
        if ( isToReject(order, openOrdersPos._pos) ) {
            rej = getRej(eRejectReason::kMaxRealizedLossAccount);
            
            char buf[256];
            fast_dtoa(pnl_amount+fees_amount, &buf[0], 2);
            rej._text = std::string(&buf[0]) + " :: " + boost::lexical_cast<std::string>(_account._maxRealizedLoss);
            return false;
        }
        
        return true;
    }
    
    pnl_amount = -1 * _accountPnL.getUnrealizedPnL();
    if ( (pnl_amount+fees_amount) > _account._maxUnrealizedLoss ) {
        const AccountInstrOpenOrdersPos& openOrdersPos = ProcessorOrders::instance().getInstrOpenOrdersPosForAccountInstr(order->_accountId, order->_instrumentId);
        if ( isToReject(order, openOrdersPos._pos) ) {
            rej = getRej(eRejectReason::kMaxUnrealizedLossAccount);
            
            char buf[256];
            fast_dtoa(pnl_amount+fees_amount, &buf[0], 2);
            rej._text = std::string(&buf[0]) + " :: " + boost::lexical_cast<std::string>(_account._maxUnrealizedLoss);
            return false;
        }
        
        return true;
    }

    std::string reason;
    if ( isAccountTotalLossReached(pnl_amount, reason) ) {
        const AccountInstrOpenOrdersPos& openOrdersPos = ProcessorOrders::instance().getInstrOpenOrdersPosForAccountInstr(order->_accountId, order->_instrumentId);
        if ( isToReject(order, openOrdersPos._pos) ) {
            rej = getRej(eRejectReason::kMaxTotalLossAccount);
            rej._text = reason;
            return false;
        }
        
        return true;
    }
    
    if ( (_accountPnL.getRealizedDrawdown()+fees_amount) > _account._maxRealizedDrawdown ) {
        const AccountInstrOpenOrdersPos& openOrdersPos = ProcessorOrders::instance().getInstrOpenOrdersPosForAccountInstr(order->_accountId, order->_instrumentId);
        if ( isToReject(order, openOrdersPos._pos) ) {
            rej = getRej(eRejectReason::kMaxRealizedDrawdownAccount);
            
            char buf[256];
            fast_dtoa(_accountPnL.getRealizedDrawdown()+fees_amount, &buf[0], 2);
            rej._text = std::string(&buf[0]) + " :: " + boost::lexical_cast<std::string>(_account._maxRealizedDrawdown);
            return false;
        }
        
        return true;
    }
    
    if ( (_accountPnL.getUnrealizedDrawdown()+fees_amount) > _account._maxUnrealizedDrawdown ) {
        const AccountInstrOpenOrdersPos& openOrdersPos = ProcessorOrders::instance().getInstrOpenOrdersPosForAccountInstr(order->_accountId, order->_instrumentId);
        if ( isToReject(order, openOrdersPos._pos) ) {
            rej = getRej(eRejectReason::kMaxUnrealizedDrawdownAccount);
            
            char buf[256];
            fast_dtoa(_accountPnL.getUnrealizedDrawdown()+fees_amount, &buf[0], 2);
            rej._text = std::string(&buf[0]) + " :: " + boost::lexical_cast<std::string>(_account._maxUnrealizedDrawdown);
            return false;
        }
        
        return true;
    }
    
    // Check strategy limits
    //
    StratPnL& strat = getOrCreateStrat(order->_strategyId);
    TPnLComposite& stratPnl = strat._stratPnL;
    fees_amount = stratPnl.getFeesPaid().total();
    
    pnl_amount = -1 * stratPnl.getRealizedPnL();
    if ( (pnl_amount+fees_amount) > strat._limits._maxRealizedLoss ) {
        if ( isToReject(order, pnl.getPosition().get() ) ) {
            rej = getRej(eRejectReason::kMaxRealizedLossStrategy);
            
            char buf[256];
            fast_dtoa(pnl_amount+fees_amount, &buf[0], 2);
            rej._text = std::string(&buf[0]) + " :: " + boost::lexical_cast<std::string>(strat._limits._maxRealizedLoss);
            return false;
        }
    }
    
    pnl_amount = -1 * stratPnl.getUnrealizedPnL();
    if ( (pnl_amount+fees_amount) > strat._limits._maxUnrealizedLoss ) {
        if ( isToReject(order, pnl.getPosition().get() ) ) {
            rej = getRej(eRejectReason::kMaxUnrealizedLossStrategy);
            
            char buf[256];
            fast_dtoa(pnl_amount+fees_amount, &buf[0], 2);
            rej._text = std::string(&buf[0]) + " :: " + boost::lexical_cast<std::string>(strat._limits._maxUnrealizedLoss);
            return false;
        }
    }
    
    if ( (stratPnl.getRealizedDrawdown()+fees_amount) > strat._limits._maxRealizedDrawdown ) {
        if ( isToReject(order, pnl.getPosition().get() ) ) {
            rej = getRej(eRejectReason::kMaxRealizedDrawdownStrategy);
            
            char buf[256];
            fast_dtoa(stratPnl.getRealizedDrawdown()+fees_amount, &buf[0], 2);
            rej._text = std::string(&buf[0]) + " :: " + boost::lexical_cast<std::string>(strat._limits._maxRealizedDrawdown);
            return false;
        }
    }
    
    if ( (stratPnl.getUnrealizedDrawdown()+fees_amount) > strat._limits._maxUnrealizedDrawdown ) {
        if ( isToReject(order, pnl.getPosition().get() ) ) {
            rej = getRej(eRejectReason::kMaxUnrealizedDrawdownStrategy);
            
            char buf[256];
            fast_dtoa(stratPnl.getUnrealizedDrawdown()+fees_amount, &buf[0], 2);
            rej._text = std::string(&buf[0]) + " :: " + boost::lexical_cast<std::string>(strat._limits._maxUnrealizedDrawdown);
            return false;
        }
    }
    
    return true;
}

void ProcessorPnL::onCommand(const tw::common::Command& command) {
    if ( command._type != tw::common::eCommandType::kProcessorPnL )
        return;
    
    if ( command._subType != tw::common::eCommandSubType::kList )
        return;
    
    tw::common_commands::ProcessorPnL c = tw::common_commands::ProcessorPnL::fromCommand(command);
    
    if ( c._accountId != _account._id )
        return;
    
    if ( !printToString(c._strategyId, c._results) )
        return;
    
    tw::common_strat::StrategyContainer::instance().sendToMsgBusConnection(command._connectionId, c.toString());
}

void ProcessorPnL::recordFill(const Fill& fill) {
    onFill(fill);
}

void ProcessorPnL::rebuildPos(const PosUpdate& update) {
    if ( update._accountId != _account._id )
        return;
    
    tw::instr::InstrumentConstPtr instrument = tw::instr::InstrumentManager::instance().getByDisplayName(update._displayName);
    if ( !instrument || !instrument->isValid() ) {
        LOGGER_ERRO << "Can't rebuild position for unknown or invalid instrument: " << update._displayName << "\n";            
        return;
    }
    
    TPnL& pnl = getOrCreatePnl(update._strategyId, instrument->_keyId);
    pnl.rebuildPos(update);
}

void ProcessorPnL::onFill(const Fill& fill) {
    if ( fill._accountId != _account._id )
        return;
    
    TPnL& pnl = getOrCreatePnl(fill._strategyId, fill._instrumentId);
    pnl.onFill(fill);
    const_cast<Fill&>(fill)._posStrategy = pnl.getPosition();
    
    if ( _printPnL )
        logPnl();
        
}

Reject ProcessorPnL::getRej(eRejectReason reason) {
    tw::channel_or::Reject rej;

    rej._rejType = eRejectType::kInternal;
    rej._rejSubType = eRejectSubType::kProcessorPnL;
    rej._rejReason = reason;

    return rej;
}

TPnL& ProcessorPnL::getOrCreatePnl(const tw::risk::TStrategyId& x,  const tw::instr::Instrument::TKeyId& y) {
    StratPnL& stratPnl = getOrCreateStrat(x);
    StratPnL::TInstrumentsPnL& instrumentsPnl = stratPnl._instrumentsPnL;
    StratPnL::TInstrumentsPnL::iterator iterPnl = instrumentsPnl.find(y);
    if ( iterPnl == instrumentsPnl.end() ) {
        iterPnl = instrumentsPnl.insert(StratPnL::TInstrumentsPnL::value_type(y, TPnL())).first;
        iterPnl->second.setObserver(&(stratPnl._stratPnL));
        stratPnl._stratPnL.addComponent(&iterPnl->second);
    }

    return iterPnl->second;
}

StratPnL& ProcessorPnL::getOrCreateStrat(const tw::risk::TStrategyId& x) {
    TStratsPnL::iterator iterStratsPnl = _stratsPnL.find(x);
    if ( iterStratsPnl == _stratsPnL.end() ) {
        iterStratsPnl = _stratsPnL.insert(TStratsPnL::value_type(x, StratPnL())).first;
        iterStratsPnl->second._stratPnL.setObserver(&_accountPnL);
        _accountPnL.addComponent(&iterStratsPnl->second._stratPnL);
    }

    return iterStratsPnl->second;
}

bool ProcessorPnL::isToReject(const TOrderPtr& order, int32_t pos) {
    if ( pos == 0 )
        return true;

    if ( pos > 0 && order->_side == eOrderSide::kBuy)
        return true;

    if ( pos < 0 && order->_side == eOrderSide::kSell )
        return true;

    return false;
}

void ProcessorPnL::logPnl() const {
    LOGGER_INFO << "\n";
    LOGGER << "Header: " << _accountPnL.getHeader() << "\n";
    LOGGER << "PnL for: " << _account._id << "(" << _account._name << ")" << " :: ACCOUNT_PNL ==> " << _accountPnL.toString();

    TStratsPnL::const_iterator iter = _stratsPnL.begin();
    TStratsPnL::const_iterator end = _stratsPnL.end();
    for ( ; iter != end; ++iter ) {            
        LOGGER << "PnL for: " << _account._id << "(" << _account._name << ")" << " :: " << iter->first << "(" << iter->second._limits._name << ")" << " ==> " << iter->second._stratPnL.toString();
    }

    LOGGER << "\n";
}

void ProcessorPnL::updatePnLForTvForStratInstrument(const tw::instr::Instrument::TKeyId& instrId) {
    TStratsPnL::iterator iter = _stratsPnL.begin();
    TStratsPnL::iterator end = _stratsPnL.end();
    for ( ; iter!=end; ++iter ) {
        StratPnL::TInstrumentsPnL::iterator iter2 = iter->second._instrumentsPnL.begin();
        StratPnL::TInstrumentsPnL::iterator end2 = iter->second._instrumentsPnL.end();
        for ( ; iter2 != end2; ++iter2 ) {
            if ( iter2->second.getInstrument() && instrId == iter2->second.getInstrument()->_keyId ) {
                tw::common_trade::WboManager::TTv& tv = tw::common_trade::WboManager::instance().getOrCreateTv(iter2->second.getInstrument()->_keyId);    
                iter2->second.onTv(tv);
            }
        }
    }
}

bool ProcessorPnL::updatePnLForTv(const StratPnL& stratPnL) const {
    StratPnL& v = const_cast<StratPnL&>(stratPnL);

    StratPnL::TInstrumentsPnL::iterator iter = v._instrumentsPnL.begin();
    StratPnL::TInstrumentsPnL::iterator end = v._instrumentsPnL.end();
    for ( ; iter != end; ++iter ) {
        if ( iter->second.getInstrument() ) {
            tw::common_trade::WboManager::TTv& tv = tw::common_trade::WboManager::instance().getOrCreateTv(iter->second.getInstrument()->_keyId);    
            iter->second.onTv(tv);
        } else {
            return false;
        }
    }
    
    return true;
}

bool ProcessorPnL::printToString(tw::channel_or::TStrategyId strategyId, std::string& results) const {
    tw::common_str_util::TFastStream s;
    results.clear();

    if ( strategyId != 0 ) {
        TStratsPnL::const_iterator iter = _stratsPnL.find(strategyId);
        if ( iter == _stratsPnL.end() ) {
            LOGGER_ERRO << "Can't find: " <<  _account._id << " :: " << strategyId << "\n";
            return false;
        }
        
        updatePnLForTv(iter->second);

        s << "Header: " << _accountPnL.getHeader() << "\n";
        s << "PnL for: " << _account._id << "(" << _account._name << ")" << " :: " << strategyId << "(" << iter->second._limits._name << ")" << " ==> " << iter->second._stratPnL.toString();
        results = s.str();
    } else {
        {
            TStratsPnL::const_iterator iter = _stratsPnL.begin();
            TStratsPnL::const_iterator end = _stratsPnL.end();
            for ( ; iter != end; ++iter ) {
                updatePnLForTv(iter->second);
            }
        }

        {
            s << "Header: " << _accountPnL.getHeader() << "\n";
            s << "PnL for: " << _account._id << "(" << _account._name << ")" << " :: ACCOUNT_PNL ==> " << _accountPnL.toString();
            results = s.str();

            TStratsPnL::const_iterator iter = _stratsPnL.begin();
            TStratsPnL::const_iterator end = _stratsPnL.end();
            for ( ; iter != end; ++iter ) {
                s.clear();
                s << "PnL for: " << _account._id << "(" << _account._name << ")" << " :: " << iter->first << "(" << iter->second._limits._name << ")" << " ==> " << iter->second._stratPnL.toString();
                results += s.str();
            }
        }
    }

    return true;
}
    
} // namespace channel_or
} // namespace tw
