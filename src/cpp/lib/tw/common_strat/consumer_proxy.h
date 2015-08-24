#pragma once

#include <tw/common/singleton.h>
#include <tw/common/timer_server.h>

#include <tw/functional/delegate.hpp>
#include <tw/price/quote_store.h>
#include <tw/generated/channel_or_defs.h>

namespace tw {
namespace common_strat {

class ConsumerProxy : public tw::common::Singleton<ConsumerProxy> {
public:
    // TODO: create generic factory of create/register callback methods
    //
    typedef tw::functional::delegate1<void, const tw::price::QuoteStore::TQuote&> TClientCallbackQuote;
    
    template<typename TClient>
    static TClientCallbackQuote createClientCallbackQuote(TClient* client) {
        return TClientCallbackQuote::from_method<TClient, &TClient::onQuote>(client);
    }
    
    typedef tw::functional::delegate1<void, const tw::channel_or::OrderResp&> TClientCallbackOrderResp;
    
    template<typename TClient>
    static TClientCallbackOrderResp createClientCallbackOrderResp(TClient* client) {
        return TClientCallbackOrderResp::from_method<TClient, &TClient::onOrderResp>(client);
    }
    
    typedef tw::functional::delegate1<void, const tw::channel_or::Alert&> TClientCallbackAlert;
    
    template<typename TClient>
    static TClientCallbackAlert createClientCallbackAlert(TClient* client) {
        return TClientCallbackAlert::from_method<TClient, &TClient::onAlert>(client);
    }    
    
    typedef tw::functional::delegate2<bool, tw::common::TimerClient*, tw::common::TTimerId> TClientCallbackTimer;
    
    template<typename TClient>
    static TClientCallbackTimer createClientCallbackTimer(TClient* client) {
        return TClientCallbackTimer::from_method<TClient, &TClient::onTimeout>(client);
    }
    
public:
    ConsumerProxy() {
        clear();
    }
    
    ~ConsumerProxy() {
        clear();
    }
    
    void clear() {
        _clientQuote.clear();
        _clientOrderResp.clear();
        _clientAlert.clear();
        _clientTimer.clear();
    }
    
public:    
    template <typename TClient>
    bool registerCallbackQuote(TClient* client) {
        if ( !client )
            return false;
        
        TClientCallbackQuote clientToAdd = createClientCallbackQuote(client);
        if ( clientToAdd == _clientQuote )
            return false;
        
        _clientQuote = clientToAdd;
        return true;
    }
    
    template <typename TClient>
    bool registerCallbackOrderResp(TClient* client) {
        TClientCallbackOrderResp clientToAdd = createClientCallbackOrderResp(client);
        if ( clientToAdd == _clientOrderResp )
            return false;
        
        _clientOrderResp = clientToAdd;
        return true;
    }
    
    template <typename TClient>
    bool registerCallbackAlert(TClient* client) {
        TClientCallbackAlert clientToAdd = createClientCallbackAlert(client);
        if ( clientToAdd == _clientAlert )
            return false;
        
        _clientAlert = clientToAdd;
        return true;
    }
    
    template <typename TClient>
    bool registerCallbackTimer(TClient* client) {
        TClientCallbackTimer clientToAdd = createClientCallbackTimer(client);
        if ( clientToAdd == _clientTimer )
            return false;
        
        _clientTimer = clientToAdd;
        return true;
    }
    
public:
    void onQuote(const tw::price::QuoteStore::TQuote& value) {
        if ( !_clientQuote.empty() )
            _clientQuote(value);
    }
    
    void onOrderResp(const tw::channel_or::OrderResp& value) {
        if ( !_clientOrderResp.empty() )
            _clientOrderResp(value);        
    }
    
    void onAlert(const tw::channel_or::Alert& value) {
        if ( !_clientAlert.empty() )
            _clientAlert(value);        
    }
    
    bool onTimeout(tw::common::TimerClient* value, tw::common::TTimerId id) {
        if ( !_clientTimer.empty() )
            return _clientTimer(value, id);
        
        return false;
    }
    
private:
    TClientCallbackQuote _clientQuote;
    TClientCallbackOrderResp _clientOrderResp;
    TClientCallbackAlert _clientAlert;
    TClientCallbackTimer _clientTimer;
};
	
} // common_strat
} // tw

