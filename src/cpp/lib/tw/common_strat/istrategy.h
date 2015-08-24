#pragma once

#include <tw/common_strat/defs.h>
#include <tw/common/command.h>
#include <tw/instr/instrument_manager.h>
#include <tw/common/settings.h>

#include <tw/generated/instrument.h>
#include <tw/generated/channel_or_defs.h>
#include <tw/generated/risk_defs.h>

#include <vector>

namespace tw {
namespace common_strat {
    
class IStrategy {
public:
    typedef std::vector<tw::instr::InstrumentPtr> TQuoteSubscriptions;
    
public:
    IStrategy() {
        clear();
    }
    
    virtual ~IStrategy() {
    }
    
public:
    virtual bool init(const tw::common::Settings& settings, const tw::risk::Strategy& strategyParams) {
        if ( strategyParams._name !=  _params._name )
            return false;
        
        _params = strategyParams;        
        return true;
    }
    
    virtual bool start() = 0;
    virtual bool stop() = 0;
    
    // Crash recovery methods
    //
    virtual void recordExternalFill(const tw::channel_or::Fill& fill) = 0;
    virtual bool rebuildOrder(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej) = 0;
    virtual void rebuildPos(const tw::channel_or::PosUpdate& update) = 0;
    
    // TMsgBusCallback interface
    //
    virtual void onConnectionUp(TMsgBusConnection::native_type id, TMsgBusConnection::pointer connection) = 0;
    virtual void onConnectionDown(TMsgBusConnection::native_type id) = 0;    
    virtual void onConnectionData(TMsgBusConnection::native_type id, const tw::common::Command& cmnd) = 0;
    
    // Command communication interface
    //
    virtual void onCommand(const tw::common::Command& cmnd) {        
    }
    
public:
    void clear() {
        _params.clear();
        _quoteSubscriptions.clear();
    }
    
    const tw::risk::Strategy& getParams() const {
        return _params;
    }
    
    const std::string& getName() const {
        return _params._name;
    }
    
    std::string getFullName() const {
        tw::common_str_util::TFastStream s;
        s << getStrategyId() << "::" << getName();
        
        return s.str();
    }
    
    const tw::channel_or::TAccountId& getAccountId() const {
        return _params._accountId;
    }
    
    const tw::channel_or::TStrategyId& getStrategyId() const {
        return _params._id;
    }
    
    const TQuoteSubscriptions& getQuoteSubscriptions() const {
        return _quoteSubscriptions;
    }

    bool addQuoteSubscription(const std::string& displayName) {
        if ( !addQuoteSubscription(tw::instr::InstrumentManager::instance().getByDisplayName(displayName)) ) {        
            LOGGER_ERRO << "Key: " << displayName << " :: can't find instrument" << "\n";
            return false;
        }
        
        return true;
    }
    
    bool addQuoteSubscription(tw::instr::InstrumentPtr instrument) {
        if ( instrument == NULL ) {
            LOGGER_ERRO << "Instrument is NULL" << "\n";
            return false;
        }
        
        _quoteSubscriptions.push_back(instrument);               
        LOGGER_INFO << "Added subscription for: " << instrument->_displayName << "\n";
        return true;
    }
    
    tw::instr::InstrumentManager::TInstruments getByExchange(tw::instr::eExchange exchange) const {
        tw::instr::InstrumentManager::TInstruments instruments;
        
        TQuoteSubscriptions::const_iterator iter = _quoteSubscriptions.begin();
        TQuoteSubscriptions::const_iterator end = _quoteSubscriptions.end();
        
        for ( ; iter != end; ++iter ) {
            if ( (exchange = (*iter)->_exchange) ) {
                tw::instr::InstrumentPtr instrument = *iter;
                instruments.push_back(instrument);
            }
        }
        
        return instruments;
    }
    
    virtual void onAlert(const tw::channel_or::Alert& alert) = 0;
    
protected:
    tw::risk::Strategy _params;    
    TQuoteSubscriptions _quoteSubscriptions;    
};

} // common_strat
} // tw

