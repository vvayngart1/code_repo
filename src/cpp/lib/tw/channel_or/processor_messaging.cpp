#include <tw/channel_or/processor_messaging.h>

#include "tw/instr/instrument_manager.h"
#include "channel_or_storage.h"

namespace tw {
namespace channel_or {
    
ProcessorMessaging::ProcessorMessaging() {
    clear();    
}

ProcessorMessaging::~ProcessorMessaging() {
    stop();
}
    
void ProcessorMessaging::clear() {
    _infos.clear();
    _index.clear();
}

tw::channel_or::MessagingForExchange* ProcessorMessaging::getInfo(const tw::instr::Instrument::TKeyId& v, const TAccountId& w) {
    TKey key(v, w);
    TIndexToMessagingInfos::iterator iter = _index.find(key);
    if ( iter == _index.end() ) {
        tw::instr::InstrumentPtr instrument = tw::instr::InstrumentManager::instance().getByKeyId(v);
        
        if ( !instrument )
            return NULL;
        
        tw::channel_or::MessagingForExchange* info = NULL;
        {
            TMessagingInfos::iterator iter = _infos.begin();
            TMessagingInfos::iterator end = _infos.end();
            for ( ; iter != end && !info; ++iter ) {
                if ( (instrument->_symbol == iter->_symbol) && (instrument->_exchange == iter->_exchange) && (w == iter->_accountId) )
                    info = &(*iter);
            }
        }
        
        if ( !info ) {
            tw::channel_or::MessagingForExchange m;
            m._symbol = instrument->_symbol;
            m._exchange = instrument->_exchange;
            m._accountId = w;
            _infos.push_back(m);
            info = &(*(_infos.rbegin()));
        }
        
        iter = _index.insert(TIndexToMessagingInfos::value_type(key, info)).first;
    }
    
    return iter->second;
}


bool ProcessorMessaging::init(const std::vector<tw::channel_or::MessagingForExchange>& v) {
    try {
        std::string temp;
        for ( size_t i = 0; i < v.size(); ++i ) {
            _infos.push_back(v[i]);
            temp += "\n\t" + v[i].toString();
        }
        
        LOGGER_INFO << temp << "\n";
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    
    return true;
}

// ProcessorOut interface
//
bool ProcessorMessaging::init(const tw::common::Settings& settings) {
    try {
        std::vector<tw::channel_or::MessagingForExchange> infos;
        if ( !tw::channel_or::ChannelOrStorage::instance().getMessagingForExchanges(infos) )
            return false;
        
        return init(infos);
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    
    return true;
}

bool ProcessorMessaging::start() {
    return true;
}

void ProcessorMessaging::stop() {
    clear();
}

bool ProcessorMessaging::sendNew(const TOrderPtr& order, Reject& rej) {
    tw::channel_or::MessagingForExchange* info = getInfo(order->_instrumentId, order->_accountId);
    if ( info ) {
        ++info->_newMsgs;
        tw::channel_or::TOrderPtr& o = const_cast<tw::channel_or::TOrderPtr&>(order);
        static_cast<tw::channel_or::Messaging&>(*o) = static_cast<tw::channel_or::Messaging&>(*info);
    }    
    
    return true;
}

bool ProcessorMessaging::sendMod(const TOrderPtr& order, Reject& rej) {
    tw::channel_or::MessagingForExchange* info = getInfo(order->_instrumentId, order->_accountId);
    if ( info ) {
        ++info->_modMsgs;
        tw::channel_or::TOrderPtr& o = const_cast<tw::channel_or::TOrderPtr&>(order);
        static_cast<tw::channel_or::Messaging&>(*o) = static_cast<tw::channel_or::Messaging&>(*info);
    }    
    
    return true;
}

bool ProcessorMessaging::sendCxl(const TOrderPtr& order, Reject& rej) {
    tw::channel_or::MessagingForExchange* info = getInfo(order->_instrumentId, order->_accountId);
    if ( info ) {
        ++info->_cxlMsgs;
        tw::channel_or::TOrderPtr& o = const_cast<tw::channel_or::TOrderPtr&>(order);
        static_cast<tw::channel_or::Messaging&>(*o) = static_cast<tw::channel_or::Messaging&>(*info);
    }    
    
    return true;
}

void ProcessorMessaging::onCommand(const tw::common::Command& command) {
}

bool ProcessorMessaging::rebuildOrder(const TOrderPtr& order, Reject& rej) {
    return true;
}

void ProcessorMessaging::recordFill(const Fill& fill) {
    onFill(fill);
}

void ProcessorMessaging::rebuildPos(const PosUpdate& update) {
}

// ProcessorIn interface
//
void ProcessorMessaging::onNewAck(const TOrderPtr& order) {
}

void ProcessorMessaging::onNewRej(const TOrderPtr& order, const Reject& rej) {
    if ( tw::channel_or::eRejectType::kInternal != rej._rejType)
        return;
    
    tw::channel_or::MessagingForExchange* info = getInfo(order->_instrumentId, order->_accountId);
    if ( info ) {
        --info->_newMsgs;
        tw::channel_or::TOrderPtr& o = const_cast<tw::channel_or::TOrderPtr&>(order);
        static_cast<tw::channel_or::Messaging&>(*o) = static_cast<tw::channel_or::Messaging&>(*info);
    }
}

void ProcessorMessaging::onModAck(const TOrderPtr& order) {
}

void ProcessorMessaging::onModRej(const TOrderPtr& order, const Reject& rej) {
    if ( tw::channel_or::eRejectType::kInternal != rej._rejType)
        return;
    
    tw::channel_or::MessagingForExchange* info = getInfo(order->_instrumentId, order->_accountId);
    if ( info ) {
        --info->_modMsgs;
        tw::channel_or::TOrderPtr& o = const_cast<tw::channel_or::TOrderPtr&>(order);
        static_cast<tw::channel_or::Messaging&>(*o) = static_cast<tw::channel_or::Messaging&>(*info);
    }
}

void ProcessorMessaging::onCxlAck(const TOrderPtr& order) {
}

void ProcessorMessaging::onCxlRej(const TOrderPtr& order, const Reject& rej) {
    if ( tw::channel_or::eRejectType::kInternal != rej._rejType)
        return;
    
    tw::channel_or::MessagingForExchange* info = getInfo(order->_instrumentId, order->_accountId);
    if ( info ) {
        --info->_cxlMsgs;
        tw::channel_or::TOrderPtr& o = const_cast<tw::channel_or::TOrderPtr&>(order);
        static_cast<tw::channel_or::Messaging&>(*o) = static_cast<tw::channel_or::Messaging&>(*info);
    }
}

void ProcessorMessaging::onFill(const Fill& fill) {
    tw::channel_or::MessagingForExchange* info = getInfo(fill._instrumentId, fill._accountId);
    if ( info ) {
        info->_totalVolume += fill._qty.get();
        Fill& f = const_cast<Fill&>(fill);
        static_cast<tw::channel_or::Messaging&>(f) = static_cast<tw::channel_or::Messaging&>(*info);
    }
}

void ProcessorMessaging::onAlert(const Alert& alert) {
}

void ProcessorMessaging::onRebuildOrderRej(const TOrderPtr& order, const Reject& rej) {
}
    
} // namespace channel_or
} // namespace tw
