#include <tw/channel_or/processor_throttle.h>
#include <tw/risk/risk_storage.h>

namespace tw {
namespace channel_or {

ProcessorThrottle::ProcessorThrottle() {
    clear();    
}

ProcessorThrottle::~ProcessorThrottle() {
    stop();
}
    
void ProcessorThrottle::clear() {
    _timerId = tw::common::TTimerId();
    _enabled = false;
    _counters.clear();
    _account.clear();
}

bool ProcessorThrottle::init(const tw::risk::Account& account) {
    _account = account;
    return true;
}

// tw::common::TimerClient interface
//
bool ProcessorThrottle::onTimeout(const tw::common::TTimerId& id) {
    _counters.clear();
    return true;
}

// ProcessorOut interface
//
bool ProcessorThrottle::init(const tw::common::Settings& settings) {
    tw::risk::Account account;
    if ( !tw::risk::RiskStorage::instance().getAccount(account, settings._trading_account) ) {
        LOGGER_ERRO << "Failed to get configuration for trading account: " << settings._trading_account << "\n";
        return false;
    }
    
    return init(account);
}

bool ProcessorThrottle::start() {
    if ( !tw::common::TimerServer::instance().registerClient(this, 1000, false, _timerId) ) {
        LOGGER_ERRO << "Failed to register with timer server" << "\n";
        return false;
    }
    
    _enabled = true;
    return true;
}

void ProcessorThrottle::stop() {
    if ( !_enabled )
        return;
    
    clear();
}

bool ProcessorThrottle::sendNew(const TOrderPtr& order, Reject& rej) {
    if ( ++_counters._counterMPSNew > _account._maxMPSNew )
        _enabled = false;
    
    setCounters(order);
    if ( !_enabled ) {
        rej = getRej(tw::channel_or::eRejectReason::kExceededMaxMPS);
        return false;
    }
    
    return true;
}

bool ProcessorThrottle::sendMod(const TOrderPtr& order, Reject& rej) {
    if ( ++_counters._counterMPSMod > _account._maxMPSMod )
        _enabled = false;
    
    setCounters(order);
    if ( !_enabled ) {
        rej = getRej(tw::channel_or::eRejectReason::kExceededMaxMPS);
        return false;
    }
    
    return true;
}

bool ProcessorThrottle::sendCxl(const TOrderPtr& order, Reject& rej) {
    ++_counters._counterMPSCxl;
    setCounters(order);
    if ( _counters._counterMPSCxl > _account._maxMPSCxl ) {
        rej = getRej(tw::channel_or::eRejectReason::kExceededMaxMPS);
        return false;
    }
    
    return true;
}

void ProcessorThrottle::onCommand(const tw::common::Command& command) {
    // TODO: need to implement
    //
}

void ProcessorThrottle::setCounters(const TOrderPtr& order) {
    order->_newMPS = _counters._counterMPSNew;
    order->_modMPS = _counters._counterMPSMod;
    order->_cxlMPS = _counters._counterMPSCxl;
}
    
} // namespace channel_or
} // namespace tw
