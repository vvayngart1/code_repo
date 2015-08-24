#pragma once

#include <tw/common/defs.h>
#include <tw/price/quote_store.h>
#include <tw/channel_pf/ichannel_impl.h>

#include <map>

namespace tw {
namespace channel_pf {

class Channel {
public:
    typedef tw::price::QuoteStore TQuoteStore;    
    
public:
    Channel() {
        clear();
    }

    void clear() {
        _pimpl = NULL;
        _subscriptionsCount.clear();
        _isActive = false;
    }
    
public:
    bool init(const tw::common::Settings& settings, IChannelImpl* pimpl) {
        _pimpl = pimpl;    
        if ( !_pimpl )
            return false;
        
        return _pimpl->init(settings);
    }
    
    bool start() {
        if ( !_pimpl )
            return false;
        
        _isActive = _pimpl->start();
        return _isActive;
    }
    
    bool isActive() {
        return _isActive;
    }
    
    void stop() {
        if ( _pimpl ) {
            if ( _isActive ) {
                _pimpl->stop();
                _isActive = false;
            } else {
                LOGGER_WARN << "stop() called without a successfull start()!\n";
            }
        }
    }
    
    template <typename TClient>
    bool subscribe(tw::instr::InstrumentConstPtr instrument, TClient* client) {        
        if ( _isActive ) {
            LOGGER_ERRO << "you can not change subscriptions while start() is active!\n";
            return false;
        }
        
        if ( !TQuoteStore::instance().subscribe(instrument, client) )
            return false;
        
        TSubscriptionsCount::iterator iter = _subscriptionsCount.find(instrument->_keyId);        
        if ( iter !=  _subscriptionsCount.end() ) {
            ++iter->second;
            return true;
        }
            
        if ( !_pimpl || !_pimpl->subscribe(instrument) ) {
            TQuoteStore::instance().unsubscribe(instrument, client);
            return false;
        }
        
        _subscriptionsCount.insert(TSubscriptionsCount::value_type(instrument->_keyId, 1UL));
        
        return true;
    }
    
    template <typename TClient>
    bool unsubscribe(tw::instr::InstrumentConstPtr instrument, TClient* client) {
        if ( _isActive ) {
            LOGGER_ERRO << "you can not change subscriptions while start() is active!\n";
            return false;
        }

        if ( !TQuoteStore::instance().unsubscribe(instrument, client) )
            return false;
        
        TSubscriptionsCount::iterator iter = _subscriptionsCount.find(instrument->_keyId);        
        if ( iter ==  _subscriptionsCount.end() )
            return false;
        
        if ( 0 == (--(iter->second)) ) {        
            if ( !_pimpl || !_pimpl->unsubscribe(instrument) )
                return false;
            
            _subscriptionsCount.erase(iter);
        }
        
        return true;
    }
    
private:
    typedef std::map<tw::instr::Instrument::TKeyId, uint32_t> TSubscriptionsCount;
    
private:
    IChannelImpl* _pimpl;
    TSubscriptionsCount _subscriptionsCount;
    bool _isActive;
};
	
} // channel_pf
} // tw
