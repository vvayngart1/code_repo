#pragma once

#include <tw/common/defs.h>
#include <tw/price/quote_store.h>
#include <tw/functional/delegate.hpp>

#include <vector>

namespace tw {
namespace price {

// TClient interface:
//      void onQuote(const tw::price::Quote&);
//    
class ClientContainerBase {
public:
    typedef tw::functional::delegate1<void, const tw::price::Quote&> TClientCallback;
    
    template<typename TClient>
    static TClientCallback createClientCallback(TClient* client) {
        return TClientCallback::from_method<TClient, &TClient::onQuote>(client);
    }
};
    
class ClientContainerSingle : protected ClientContainerBase {
public:
    ClientContainerSingle() {
        clear();
    }

    void clear() {        
        _client.clear();
    }
    
    bool empty() const {
        return _client.empty();
    }
    
    template <typename TClient>
    bool add(TClient* client) {
        if ( !client )
            return false;
        
        TClientCallback clientToAdd = createClientCallback(client);
        if ( clientToAdd == _client )
            return false;
        
        _client = createClientCallback(client);
        return true;
    }
    
    template <typename TClient>
    bool addFront(TClient* client) {
        return add(client);
    }
    
    template <typename TClient>
    bool rem(TClient* client) {
        if ( !client )
            return false;
        
        if ( empty() )
            return false;
        
        TClientCallback clientToRem = createClientCallback(client);
        if ( clientToRem != _client )
            return false;
        
        clear();
        return true;
    }
    
    void onQuote(const tw::price::Quote& quote) {
        if ( !empty() )
            _client(quote);
    }
    
private:
    TClientCallback _client;
};


template < typename TContainer = std::list<ClientContainerBase::TClientCallback> >
class ClientContainerMultiple : protected ClientContainerBase {
public:
    ClientContainerMultiple() {
        clear();
    }

    void clear() {        
        _clients.clear();
    }
    
    bool empty() const {
        return _clients.empty();
    }
    
    template <typename TClient>
    bool add(TClient* client) {
        if ( !client )
            return false;
        
        TClientCallback clientToAdd = createClientCallback(client);        
        typename TContainer::iterator iter = findClient(clientToAdd);
        if ( iter != _clients.end() )
            return false;
        
        _clients.insert(_clients.end(), clientToAdd);
        return true;
    }
    
    template <typename TClient>
    bool addFront(TClient* client) {
        if ( !client )
            return false;
        
        TClientCallback clientToAdd = createClientCallback(client);        
        typename TContainer::iterator iter = findClient(clientToAdd);
        if ( iter != _clients.end() )
            return false;
        
        _clients.insert(_clients.begin(), clientToAdd);
        return true;
    }
    
    template <typename TClient>
    bool rem(TClient* client) {
        if ( !client )
            return false;
        
        if ( empty() )
            return false;
        
        typename TContainer::iterator iter = findClient(createClientCallback(client));
        if ( iter == _clients.end() )
            return false;
        
        _clients.erase(iter);
        return true;
    }
    
    void onQuote(const tw::price::Quote& quote) const {
        std::for_each(_clients.begin(), _clients.end(), quote);
    }
    
private:
    typename TContainer::iterator findClient(const TClientCallback& client) {
        typename TContainer::iterator iter = _clients.begin();
        typename TContainer::iterator end = _clients.end();
        for ( ; iter != end; ++iter ) {
            if ( client == (*iter) )
                return iter;
        }
        
        return iter;
    }
    
private:
    typedef std::pair<ClientContainerBase::TClientCallback, uint32_t> TClientSubscriptions;
    typedef std::vector<TClientSubscriptions> TClientsSubscriptions;
    
private:
    TContainer _clients;
    TClientsSubscriptions _clientsSubscriptions;
};
	
} // price
} // tw
