#include <tw/channel_or/processor_risk.h>
#include <tw/channel_or/processor_orders.h>
#include <tw/instr/instrument_manager.h>
#include <tw/risk/risk_storage.h>

namespace tw {
namespace channel_or {

ProcessorRisk::ProcessorRisk() {
    clear();    
}

ProcessorRisk::~ProcessorRisk() {
    stop();
}
    
void ProcessorRisk::clear() {
    _account.clear();
    _params.clear();
}

bool ProcessorRisk::init(const tw::risk::Account& account, const std::vector<tw::risk::AccountRiskParams>& params) {
    _account = account;
    tw::instr::InstrumentPtr instrument;
    for ( size_t i = 0; i < params.size(); ++i ) {    
        instrument = tw::instr::InstrumentManager::instance().getByDisplayName(params[i]._displayName);
        if ( instrument )
            _params[instrument->_keyId] = params[i];
        else
            LOGGER_WARN << "Can't find instrument, trading is disabled for: " << params[i].toString() << "\n";
    }
    
    return true;
}

// ProcessorOut interface
//
bool ProcessorRisk::init(const tw::common::Settings& settings) {
    tw::risk::Account account;
    if ( !tw::risk::RiskStorage::instance().getAccount(account, settings._trading_account) ) {
        LOGGER_ERRO << "Failed to get configuration for trading account: " << settings._trading_account << "\n";
        return false;
    }
    
    std::vector<tw::risk::AccountRiskParams> params;
    if ( !tw::risk::RiskStorage::instance().getAccountRiskParams(params, account) ) {
        LOGGER_ERRO << "Failed to get configuration for trading account risk paramas: " << account.toString() << "\n";
        return false;
    }
    
    return init(account, params);
}

bool ProcessorRisk::start() {
    return true;
}

void ProcessorRisk::stop() {
    clear();
}

bool ProcessorRisk::sendNew(const TOrderPtr& order, Reject& rej) {
    if ( order->_accountId != _account._id ) {
        rej = getRej(eRejectReason::kAccountNotConfigured);
        rej._text = "Accounts don't match: " + _account.toString() + " :: " + boost::lexical_cast<std::string>(order->_accountId);
        return false;
    }
    
    if ( !_account._tradeEnabled ) {        
        rej = getRej(eRejectReason::kAccountDisabled);
        rej._text = "Trading disabled for account: " + _account.toString();
        return false;
    }
    
    // If instrument is not configured to be tradable buy this account,
    // reject the order
    //
    TInstrAccountRiskParams::iterator iter = _params.find(order->_instrumentId);
    if ( iter == _params.end() ) {
        rej = getRej(eRejectReason::kSymbolNotConfigured);
        rej._text = _account.toString() + " :: " + order->_instrument->toString();
        return false;
    }
    
    tw::risk::AccountRiskParams& param = iter->second;
    if ( !param._tradeEnabled ) {
        rej = getRej(eRejectReason::kSymbolDisabled);
        rej._text = _account.toString() + " :: " + param.toString();
        return false;
    }
    
    // Check max clip size
    //
    if ( order->_qty > static_cast<int32_t>(param._clipSize) ) {
        rej = getRej(eRejectReason::kSymbolClipSize);
        rej._text = _account.toString() + " :: " + param.toString();
        return false;
    }
    
    // Assumptions is made that ProcessorOrders has been called already
    //
    int32_t size = 0;
    const AccountInstrOpenOrdersPos& openOrdersPos = ProcessorOrders::instance().getInstrOpenOrdersPosForAccountInstr(order->_accountId, order->_instrumentId);
    switch ( order->_side ) {
        case eOrderSide::kBuy:
            size = openOrdersPos._pos + openOrdersPos._bids.get();
            break;
        case eOrderSide::kSell:
            size = -openOrdersPos._pos + openOrdersPos._asks.get();
            break;
        default:
            rej = getRej(eRejectReason::kInvalidOrderSide);
            rej._text = order->toString();
            return false;
    }
    
    if ( size > static_cast<int32_t>(param._maxPos) ) {
        rej = getRej(eRejectReason::kSymbolMaxPos);
        rej._text = boost::lexical_cast<std::string>(size) + " :: " + param.toString();
        return false;
    }    
    
    return true;
}

bool ProcessorRisk::sendMod(const TOrderPtr& order, Reject& rej) {
    // Nothing to do, since modifying only only price
    //
    return true;
}

bool ProcessorRisk::sendCxl(const TOrderPtr& order, Reject& rej) {
    return true;
}

void ProcessorRisk::onCommand(const tw::common::Command& command) {
    if ( tw::common::eCommandType::kProcessorRisk != command._type )
        return;
    
    switch ( command._subType ) {
        case tw::common::eCommandSubType::kUpdateAccount:
        {
            tw::risk::Account update;
            update = tw::risk::Account::fromCommand(command);
            if ( update._id != _account._id ) {
                LOGGER_ERRO << command._subType.toString() << " for different accountIds \n==> Old: \n" << "\n" << _account.toStringVerbose() << "==> New: \n" << update.toStringVerbose() << "\n";
                return;
            }
            
            LOGGER_WARN << "'UpdateAccount' \n==> Old: \n" << "\n" << _account.toStringVerbose() << "==> New: \n" << update.toStringVerbose() << "\n";
            _account = update;
        }
            break;
        case tw::common::eCommandSubType::kUpdateAccountInstrument:
        {
            tw::risk::AccountRiskParams update;
            update = tw::risk::AccountRiskParams::fromCommand(command);
            if ( update._accountId != _account._id ) {
                LOGGER_ERRO << command._subType.toString() << " for different accountIds \n==> Old: \n" << "\n" << _account.toStringVerbose() << "==> New: \n" << update.toStringVerbose() << "\n";
                return;
            }
            
            TInstrAccountRiskParams::iterator iter = _params.begin();
            TInstrAccountRiskParams::iterator end = _params.end();
            for ( ; iter != end; ++iter ) {
                if ( iter->second._displayName == update._displayName ) {
                    LOGGER_WARN << "'UpdateAccountInstrument' \n==> Old: \n" << "\n" << iter->second.toStringVerbose() << "==> New: \n" << update.toStringVerbose() << "\n";
                    iter->second = update;
                }
            }
            
        }
            break;
        default:
            LOGGER_INFO << "Unsupported command: " << command.toString() << "\n";
            break;
    }
}

bool ProcessorRisk::rebuildOrder(const TOrderPtr& order, Reject& rej) {    
    // Nothing to do - not implemented
    //
    return true;
}

void ProcessorRisk::recordFill(const Fill& fill) {
    // Nothing to do - not implemented
    //
}

void ProcessorRisk::rebuildPos(const PosUpdate& update) {
    // Not implemented - nothing to do
    //
}

bool ProcessorRisk::canSend(const TOrderPtr& order) {
    return canSend(order->_accountId, order->_instrumentId, order->_qty, order->_side);
}

bool ProcessorRisk::canSend(const TAccountId& accountId, tw::instr::Instrument::TKeyId instrumentId, tw::price::Size qty, eOrderSide side) {
    if ( accountId != _account._id )
        return false;
    
    if ( !_account._tradeEnabled )
        return false;
    
    // If instrument is not configured to be tradable buy this account,
    // reject the order
    //
    TInstrAccountRiskParams::iterator iter = _params.find(instrumentId);
    if ( iter == _params.end() )
        return false;
    
    tw::risk::AccountRiskParams& param = iter->second;
    if ( !param._tradeEnabled )
        return false;
    
    // Check max clip size
    //
    if ( qty > static_cast<int32_t>(param._clipSize) )
        return false;
    
    int32_t size = 0;
    const AccountInstrOpenOrdersPos& openOrdersPos = ProcessorOrders::instance().getInstrOpenOrdersPosForAccountInstr(accountId, instrumentId);
    switch ( side ) {
        case eOrderSide::kBuy:
            size = openOrdersPos._pos + openOrdersPos._bids.get();
            break;
        case eOrderSide::kSell:
            size = -openOrdersPos._pos + openOrdersPos._asks.get();
            break;
        default:
            return false;
    }
    
    if ( size + qty > static_cast<int32_t>(param._maxPos) )
        return false;
    
    return true;
}

bool ProcessorRisk::isTradeable(tw::instr::Instrument::TKeyId id) {
    TInstrAccountRiskParams::iterator iter = _params.find(id);
    if ( iter == _params.end() )
        return false;
    
    tw::risk::AccountRiskParams& param = iter->second;
    if ( !param._tradeEnabled )
        return false;
    
    return true;
}
    
} // namespace channel_or
} // namespace tw
