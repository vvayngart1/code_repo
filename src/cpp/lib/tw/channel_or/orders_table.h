#pragma once

#include <tw/common/defs.h>
#include <tw/log/defs.h>

#include <tw/generated/channel_or_defs.h>

#include <map>
#include <vector>

namespace tw {
namespace channel_or {

typedef std::vector<TOrderPtr> TOrders;
    
template <typename TFilter>
struct OrdersFilter {
    typedef void result_type;
    
    OrdersFilter(const TFilter& filter,
                 TOrders& orders) : _filter(filter),
                                     _orders(orders) {        
    }
    
    void operator()(const TOrderPtr& order) const {
        if ( _filter(order) )
            _orders.push_back(order);
    } 
    
    const TOrders& get() const {
        return _orders;
    }
    
private:
    const TFilter& _filter;
    TOrders& _orders;
};

struct FilterNull {
    bool operator()(const TOrderPtr& order) const {
        return true;
    }
};

struct FilterAccountId {
    FilterAccountId(const TAccountId& value) : _value(value){        
    }
    
    bool operator()(const TOrderPtr& order) const {
        return ( (order != NULL) && (_value == order->_accountId) );
    }
    
    const TAccountId& _value;
};

struct FilterAccountIdStrategyId {
    FilterAccountIdStrategyId(const TAccountId& account,
                              const TStrategyId& value) : _value(value),
                                                          _f(account) {        
    }
    
    bool operator()(const TOrderPtr& order) const {
        return ( _f(order) && (_value == order->_strategyId) );
    }
    
    const TStrategyId& _value;
    FilterAccountId _f;
};

struct FilterAccountIdStrategyIdInstr {
    FilterAccountIdStrategyIdInstr(const TAccountId& account,
                                   const TStrategyId& strat,
                                   const tw::instr::Instrument::TKeyId& value) : _value(value),
                                                                                 _f(account, strat) {        
    }
    
    bool operator()(const TOrderPtr& order) const {
        return ( _f(order) && (_value == order->_instrumentId) );
    }
    
    const tw::instr::Instrument::TKeyId& _value;
    FilterAccountIdStrategyId _f;
};

struct FilterInstrumentId {
    FilterInstrumentId(const tw::instr::Instrument::TKeyId& value) : _value(value){        
    }
    
    bool operator()(const TOrderPtr& order) const {
        return ( (order != NULL) && ( _value == order->_instrumentId) );
    }
    
    const tw::instr::Instrument::TKeyId& _value;
};
    
template <typename TKey = tw::common::TUuidBuffer, typename TTable = std::map<TKey, TOrderPtr> >
class OrderTable {
public:
    OrderTable() {
        clear();
    }
    
    ~OrderTable() {        
    }
    
public:
    void clear() {
        _s.str("");
        _table.clear();
    }
    
public:
    bool add(const TKey& key, TOrderPtr order) {
        if ( exist(key) )
            return false;
        
        _table.insert(typename TTable::value_type(key, order));
        return true;
    }
    
    bool rem(const TKey& key) {
        typename TTable::iterator iter = _table.find(key);
        if ( iter == _table.end() )
            return false;
        
        _table.erase(iter);
        return true;
    }
    
    bool exist(const TKey& key) const {
        return (_table.find(key) != _table.end());
    }
    
    TOrderPtr get(const TKey& key) const {
        static TOrderPtr null;
        
        typename TTable::const_iterator iter = _table.find(key);
        if ( iter != _table.end() )
            return iter->second;
        
        return null;
    }
    
    template <typename TFilter>
    TOrders get(const TFilter& filter) const {
        TOrders orders;
        OrdersFilter<TFilter> ordersFilter(filter, orders);
        
        std::for_each(_table.begin(), _table.end(), stdext::compose1(ordersFilter, stdext::select2nd<typename TTable::value_type>()));
        return ordersFilter.get();
    }
    
    std::string toString() const {
        _s.str("");
        
        std::for_each(_table.begin(), _table.end(), std::bind2obj(std::mem_fun_ref(&OrderTable::print), *this));
        return _s.str();
    }
    
private:
    void print(typename TTable::value_type v) const {
        _s << v.first << " :: " << v.second->toString() << "\n";
    }
    
private:
    mutable std::stringstream _s;
    TTable _table;        
};
    
} // namespace common
} // namespace tw
