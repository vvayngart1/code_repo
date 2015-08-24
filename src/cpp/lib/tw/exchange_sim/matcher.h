#pragma once

#include <tw/log/defs.h>
#include <tw/instr/instrument_manager.h>
#include <tw/channel_pf_cme/settings.h>
#include <tw/generated/channel_or_defs.h>
#include <tw/price/quote.h>
#include <tw/common/pool.h>
#include <tw/common_thread/locks.h>

#include <string>
#include <map>
#include <deque>

namespace tw {
namespace exchange_sim {
    
typedef std::stringstream TStream;

typedef tw::channel_or::OrderSim TOrder;
typedef tw::channel_or::TOrderSimPtr TOrderPtr;

class Matcher;
typedef boost::shared_ptr<Matcher> TMatcherPtr;

typedef tw::channel_or::FillSim TFill;
    
class MatcherEventListener {
public:
    virtual ~MatcherEventListener() {        
    }
    
    virtual void onNewAck(const TOrderPtr& order) = 0;    
    virtual void onModAck(const TOrderPtr& order) = 0;    
    virtual void onCxlAck(const TOrderPtr& order) = 0;
    virtual void onFill(const TFill& fill) = 0;
    virtual void onEvent(const Matcher* matcher) = 0;
};
    
class Matcher {
public:    
    typedef tw::common::Pool<TOrder, tw::common_thread::Lock> TPool;
    
    typedef std::vector<TOrderPtr> TOrders;
    typedef std::pair<tw::price::PriceSizeNumOfOrders, TOrders> TBookLevel;
    typedef std::vector<TBookLevel> TBook;
    
    typedef std::map<std::string, TOrderPtr> TOrdersMap;
    
public:
    TOrderPtr createOrder();
    
public:
    Matcher(MatcherEventListener& eventListener, const tw::instr::InstrumentPtr& instrument);
    ~Matcher();
    
    void clear();    
    std::string toString() const;
    
public:    
    bool init(const tw::channel_or::SimulatorMatcherParams& params);
    
    const tw::instr::InstrumentPtr& getInstrument() const {
        return _instrument;
    }
    
    tw::common_thread::Lock& getLock() {
        return _lock;
    }
    
public:
    void onQuote(const tw::price::Quote& quote);
    
    void sendNew(TOrderPtr& order);
    void sendMod(TOrderPtr& order);
    void sendCxl(TOrderPtr& order);
    
public:
    
    TBook getBids() const {
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<tw::common_thread::Lock> lock(_lock);
    
        return _bids;
    }
    
    TBook getAsks() const {
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<tw::common_thread::Lock> lock(_lock);
    
        return _asks;
    }
    
    TOrderPtr getOrder(const std::string& exOrderId) const {
        static TOrderPtr nullOrder;
        
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<tw::common_thread::Lock> lock(_lock);
        
        TOrdersMap::const_iterator iter = _ordersMap.find(exOrderId);
        if ( iter != _ordersMap.end() )
            return iter->second;
        
        return nullOrder;
    }
    
    TOrdersMap getAllOpenOrders() const {
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<tw::common_thread::Lock> lock(_lock);
        
        return _ordersMap;
    }
    
    TBookLevel getBookLevel(const tw::price::Ticks& price, bool bids);
    
    tw::price::Size getOpenSimQty(const TBookLevel& level) {
        TOrders::const_iterator iter = level.second.begin();
        TOrders::const_iterator end = level.second.end();

        tw::price::Size openSimQty(0);
        for ( ; iter != end; ++iter ) {
            if ( tw::channel_or::eOrderType::kSim == (*iter)->_type ) {
                openSimQty += (*iter)->_qty-(*iter)->_cumQty;
            }
        }

        return openSimQty;
    }
    
private:
    template <typename TOp>
    void processTrade(TBook& book, tw::price::Trade trade, const TOp& op) {
        for ( TBook::iterator iter = book.begin(); (iter != book.end()) && (trade._size.get() > 0);  ) {
            if ( !op(trade._price, iter->first._price) )
                return;
            
            processMatches(iter->second, trade);
            if ( iter->second.empty() ) {
                book.erase(iter);
                iter = book.begin();
            } else {
                ++iter;
            }
        }
    }
    
    template <typename TOp>
    void processCross(TBook& book, const tw::price::PriceSizeNumOfOrders& level, const TOp& op) {
        if ( level._size <= 0 )
            return;
        
        for ( TBook::iterator iter = book.begin(); iter != book.end(); ) {
            if ( !op(level._price, iter->first._price) )
                return;
            
            processCrosses(iter->second, iter->first._price);
            if ( iter->second.empty() ) {
                book.erase(iter);
                iter = book.begin();
            } else {
                ++iter;
            }
        }
    }
    
    template <typename TOp>
    void processLevel(TBook& book, const tw::price::PriceSizeNumOfOrders& level, const TOp& op) {
        if (0 >= level._size.get() )
            return;
        
        TBook::iterator iter = book.begin();
        TBook::iterator end = book.end();
        
        for ( ; (iter != end) && !(op(level._price, iter->first._price)); ++iter ) {
            if ( level._price == iter->first._price ) {
                updateLevelOrders(*iter, level);
                return;
            }
        }        
        
        iter = book.insert(iter, TBook::value_type(level, TOrders()));
        iter->second.push_back(createSimOrder(level._size));
    }
    
    template <typename TOp>
    void processNew(TBook& book, const TOrderPtr& order, const TOp& op) {
        if ( order->_qty <= order->_cumQty )
            return;
        
        TBook::iterator iter = book.begin();
        TBook::iterator end = book.end();
        
        for ( ; (iter != end) && !(op(order->_price, iter->first._price)); ++iter ) {
            if ( order->_price == iter->first._price ) {
                iter->second.push_back(order);
                return;
            }
        }        
        
        tw::price::PriceSizeNumOfOrders level;
        level._price = order->_price;        
        iter = book.insert(iter, TBook::value_type(level, TOrders()));
        iter->second.push_back(order);
    }
    
    template <typename TOp>
    void checkMatches(TBook& book, TOrderPtr& order, const TOp& op) {
        tw::price::Size openQty = order->_qty - order->_cumQty;
        for ( TBook::iterator iter = book.begin(); (iter != book.end()) && (op(order->_price, iter->first._price)) && (openQty > 0);  ) {
            TOrders& orders = iter->second;
            for ( TOrders::iterator iterOrders = orders.begin(); (iterOrders != orders.end()) && (openQty > 0);  ) {
                tw::price::Size matchOpenQty = (*iterOrders)->_qty - (*iterOrders)->_cumQty;
                if ( openQty >= matchOpenQty ) {
                    generateFill((*iterOrders), iter->first._price, matchOpenQty, tw::channel_or::eLiqInd::kAdd);
                    generateFill(order, iter->first._price, matchOpenQty, tw::channel_or::eLiqInd::kRem);
                    openQty -= matchOpenQty;
                    eraseOrder(orders, iterOrders);
                    iterOrders = orders.begin();
                } else {
                    generateFill((*iterOrders), iter->first._price, openQty, tw::channel_or::eLiqInd::kAdd);
                    generateFill(order, iter->first._price, openQty, tw::channel_or::eLiqInd::kRem);
                    openQty -= openQty;
                    ++iterOrders;
                }
            }
            
            if ( orders.empty() ) {
                book.erase(iter);
                iter = book.begin();
            } else {                
                ++iter;
            }
        }
    }
    
    void processNewOrderAtLevel(TOrderPtr& order);
    bool processMatch(TOrderPtr& order, tw::price::Size& qty);
    void processMatches(TOrders& orders, tw::price::Trade& trade);
    void processCrosses(TOrders& orders, const tw::price::Ticks& price);
    void updateLevelOrders(TBookLevel& bookLevel, const tw::price::PriceSizeNumOfOrders& level);
    void generateFill(TOrderPtr& order, const tw::price::Ticks& price, const tw::price::Size& qty, const tw::channel_or::eLiqInd liqInd);
    
    TOrderPtr createSimOrder(const tw::price::Size& qty);
    void eraseOrder(TOrders& orders, TOrders::iterator iterOrder);
    void eraseOrder(TBook& book, TOrderPtr& order);
    
private:
    void toString(TBook book, TStream& stream) const;
    void toString(TStream& stream) const;
    
    void print(const tw::price::Quote& quote, bool isInfo = true) const;
    void print(const TOrderPtr& order) const;
private:
    mutable tw::common_thread::Lock _lock;
    TPool _pool;
    
    MatcherEventListener& _eventListener;
    tw::instr::InstrumentPtr _instrument;
    tw::channel_or::SimulatorMatcherParams _params;
    TBook _bids;
    TBook _asks;
    
    TOrdersMap _ordersMap;
};

} // tw::exchange_sim
} // tw
