#include <tw/exchange_sim/matcher.h>
#include <tw/price/quote_store.h>

#include <algorithm>

namespace tw {
namespace exchange_sim {

static std::less_equal<tw::price::Ticks> le;
static std::greater_equal<tw::price::Ticks> ge;

static std::less<tw::price::Ticks> l;
static std::greater<tw::price::Ticks> g;

struct pricePred {
    pricePred(const tw::price::Ticks& price) : _price(price) {        
    }
    
    bool operator() (const Matcher::TBookLevel& level) const {
        return (level.first._price == _price);
    }
    
    const tw::price::Ticks& _price;
};

TOrderPtr Matcher::createOrder() {
    TOrderPtr order(_pool.obtain(), _pool.getDeleter());
    if ( _params._verbose )
        LOGGER_INFO << _instrument->_displayName << " - _pool: available :: total: " << _pool.size() << " :: " << _pool.capacity() << "\n";
    
    return order;
}

Matcher::Matcher(MatcherEventListener& eventListener,
                 const tw::instr::InstrumentPtr& instrument) : _eventListener(eventListener),
                                                               _instrument(instrument) {
    clear();
}

Matcher::~Matcher() {
}

void Matcher::clear() {
    _pool.clear();
    _bids.clear();
    _asks.clear();
    _ordersMap.clear();
    
    _params._percentCancelFront = 20;
}

void Matcher::toString(TBook book, TStream& stream) const {
    static const char SEP_ORDERS_BEGIN = '<';
    static const char SEP_ORDERS_END = '>';
    static const char SEP_COMMA = ',';
    static const char SEP_SEMICOLON = ';';
    
    static const size_t MAX_DEPTH = 5;
    
    size_t size = std::min(book.size(), MAX_DEPTH);
    for ( size_t i = 0; i < size; ++i ) {
        stream << book[i].first._size << SEP_COMMA;
        stream << _instrument->_tc->toExchangePrice(book[i].first._price);

        TOrders::iterator iter = book[i].second.begin();
        TOrders::iterator end = book[i].second.end();

        stream << SEP_ORDERS_BEGIN;

        for ( ; iter != end; ++iter ) {
            stream << (*iter)->_qty-(*iter)->_cumQty << SEP_COMMA;
            switch ( (*iter)->_type ) {
                case tw::channel_or::eOrderType::kSim:
                    stream << "S";
                    break;
                case tw::channel_or::eOrderType::kLimit:
                    stream << "L";
                    break;
                default:
                    stream << "U";
                    break;
            }
            stream << SEP_SEMICOLON;
        }

        stream << SEP_ORDERS_END;
    }
}

void Matcher::toString(TStream& stream) const {
    static const char SEP_BOOKS = '|';
    
    toString(getBids(), stream);    
    stream << SEP_BOOKS;
    toString(getAsks(), stream);
    
    stream << "\n";
}

std::string Matcher::toString() const {
    TStream stream;
    
    toString(stream);    
    return stream.str();
}

bool Matcher::init(const tw::channel_or::SimulatorMatcherParams& params) {
    // Lock for thread synchronization
    //
    tw::common_thread::LockGuard<tw::common_thread::Lock> lock(_lock);
    
    if ( params._percentCancelFront > 100 ) {
        LOGGER_ERRO << "Incorrect params: " << params.toStringVerbose() << "\n";
        return false;
    }
    
    _params = params;
    return true;
}

Matcher::TBookLevel Matcher::getBookLevel(const tw::price::Ticks& price, bool bids) {
    static TBookLevel nullLevel;

    // Lock for thread synchronization
    //
    tw::common_thread::LockGuard<tw::common_thread::Lock> lock(_lock);
    
    if ( bids ) {
        TBook::iterator iter = std::find_if(_bids.begin(), _bids.end(), pricePred(price));
        if ( iter != _bids.end() )
            return *iter;
    } else {
        TBook::iterator iter = std::find_if(_asks.begin(), _asks.end(), pricePred(price));
        if ( iter != _asks.end() )
            return *iter;
    }

    return nullLevel;
}

void Matcher::onQuote(const tw::price::Quote& quote) {
    // Lock for thread synchronization
    //
    tw::common_thread::LockGuard<tw::common_thread::Lock> lock(_lock);
    
    switch ( quote._status ) {
        case tw::price::Quote::kSuccess:
            break;
        case tw::price::Quote::kExchangeSlow:
            break;
        case tw::price::Quote::kExchangeDown:
        case tw::price::Quote::kDropRate:
        case tw::price::Quote::kTradingHaltedOrStopped:
        case tw::price::Quote::kTradingPause:
        case tw::price::Quote::kConnectionHandlerError:
        case tw::price::Quote::kPriceGap:
        default:
            print(quote, false);
            return;
    }
    
    if ( tw::instr::eInstrType::kFuture == quote._instrument->_instrType ) {
        tw::price::Ticks bid  = quote._book[0]._bid._price;
        tw::price::Ticks ask  = quote._book[0]._ask._price;
        if ( (bid.isValid() && 0 >= bid) || (ask.isValid() && 0 >= ask) ) {
            if ( _params._verbose )
                LOGGER_ERRO << "Severe error: 0 >= price in quote in bid or ask: " << quote.toString() << "\n";
            return;
        }
    }
    
    // Process trades if any
    //
    if ( quote.isNormalTrade() ) {
        processTrade(_bids, quote._trade, le);
        processTrade(_asks, quote._trade, ge);
    }
    
    // Process crosses if any
    //
    if ( quote.isAskUpdatePrice(0) )
        processCross(_bids, quote._book[0]._ask, le);
    
    if ( quote.isBidUpdatePrice(0) )
        processCross(_asks, quote._book[0]._bid, ge);
        
    // Check all levels
    //
    for ( size_t i = 0; i < tw::price::Quote::SIZE; ++i ) {
        if ( quote.isBidUpdate(i) ) {
            if ( 0 < quote._book[i]._bid._price || tw::instr::eInstrType::kFuture != quote._instrument->_instrType)
                processLevel(_bids, quote._book[i]._bid, g);
            else if ( _params._verbose )
                LOGGER_ERRO << "0 >= price in bid[" << i << "]: " << quote.toString() << "\n";
        }
        
        if ( quote.isAskUpdate(i) ) {
            if ( 0 < quote._book[i]._ask._price || tw::instr::eInstrType::kFuture != quote._instrument->_instrType)
                processLevel(_asks, quote._book[i]._ask, l);
            else if ( _params._verbose )
                LOGGER_ERRO << "0 >= price in bid[" << i << "]: " << quote.toString() << "\n";
        }
    }
    
    _eventListener.onEvent(this);
    
    if ( !_ordersMap.empty() )
        print(quote);
}

void Matcher::sendNew(TOrderPtr& order) {
    // Lock for thread synchronization
    //
    tw::common_thread::LockGuard<tw::common_thread::Lock> lock(_lock);
    
    order->_exOrderId = order->_clOrderId;
    _eventListener.onNewAck(order);
    
    processNewOrderAtLevel(order);
    
    _eventListener.onEvent(this);
    print(order);
}

void Matcher::sendMod(TOrderPtr& order) {
    // Lock for thread synchronization
    //
    tw::common_thread::LockGuard<tw::common_thread::Lock> lock(_lock);
    
    switch ( order->_side ) {
        case tw::channel_or::eOrderSide::kBuy:
            eraseOrder(_bids, order);
            break;
        case tw::channel_or::eOrderSide::kSell:
            eraseOrder(_asks, order);
            break;
        default:
            break;
    }
    
    order->_price = order->_newPrice;
    _eventListener.onModAck(order);
    
    processNewOrderAtLevel(order);   
    
    _eventListener.onEvent(this);
    print(order);
}

void Matcher::sendCxl(TOrderPtr& order) {
    // Lock for thread synchronization
    //
    tw::common_thread::LockGuard<tw::common_thread::Lock> lock(_lock);
    
    switch ( order->_side ) {
        case tw::channel_or::eOrderSide::kBuy:
            eraseOrder(_bids, order);
            break;
        case tw::channel_or::eOrderSide::kSell:
            eraseOrder(_asks, order);
            break;
        default:
            break;
    }
    
    _eventListener.onCxlAck(order);
    
    _eventListener.onEvent(this);
    print(order);
}

void Matcher::processNewOrderAtLevel(TOrderPtr& order) {
    switch ( order->_side ) {
        case tw::channel_or::eOrderSide::kBuy:
            checkMatches(_asks, order, ge);
            if ( tw::channel_or::eTimeInForce::kIOC == order->_timeInForce ) {
                order->_internalCxl = true;
                sendCxl(order);
                return;
            }
            
            processNew(_bids, order, g);
            break;
        case tw::channel_or::eOrderSide::kSell:
            checkMatches(_bids, order, le);
            if ( tw::channel_or::eTimeInForce::kIOC == order->_timeInForce ) {
                order->_internalCxl = true;
                sendCxl(order);
                return;
            }
            
            processNew(_asks, order, l);
            break;
        default:
            break;
    }
    
    if ( order->_qty > order->_cumQty )
        _ordersMap.insert(TOrdersMap::value_type(order->_exOrderId, order));
}

bool Matcher::processMatch(TOrderPtr& order, tw::price::Size& qty) {
    tw::price::Size openQty = order->_qty - order->_cumQty;
    
    if ( qty < openQty ) {        
        generateFill(order, order->_price, qty, tw::channel_or::eLiqInd::kAdd);
        qty.set(0);
        return false;
    }
    
    generateFill(order, order->_price, openQty, tw::channel_or::eLiqInd::kAdd);
    qty -= openQty;
    return true;
}

void Matcher::processMatches(TOrders& orders, tw::price::Trade& trade) {    
    for ( TOrders::iterator iter = orders.begin(); (iter != orders.end()) && (trade._size.get() > 0);  ) {
        if ( processMatch(*iter, trade._size) ) {
            eraseOrder(orders, iter);
            iter = orders.begin();
        } else {
            ++iter;
        }
    }
}

void Matcher::processCrosses(TOrders& orders, const tw::price::Ticks& price) {
    for ( TOrders::iterator iter = orders.begin(); iter != orders.end(); iter = orders.begin() ) {
        generateFill(*iter, price, (*iter)->_qty - (*iter)->_cumQty, tw::channel_or::eLiqInd::kAdd);
        eraseOrder(orders, iter);
    }
}

void Matcher::updateLevelOrders(TBookLevel& bookLevel, const tw::price::PriceSizeNumOfOrders& level) {
    bookLevel.first = level;
    
    tw::price::Size size = getOpenSimQty(bookLevel);
    if ( size == level._size )
        return;
    
    if ( level._size > size ) {
        tw::price::Size delta = level._size - size;
        
        TOrders::reverse_iterator iter = bookLevel.second.rbegin();
        if ( iter != bookLevel.second.rend() && tw::channel_or::eOrderType::kSim == (*iter)->_type )
            (*iter)->_qty += delta;
        else
            bookLevel.second.push_back(createSimOrder(delta));
        
        return;
    }
    
    tw::price::Size deltaQtyFront = ((size-level._size)*static_cast<tw::price::Size::type>(_params._percentCancelFront))/100;
    tw::price::Size deltaQtyBack = size-level._size-deltaQtyFront;

    {
        TOrders::iterator iter = bookLevel.second.begin();
        TOrders::iterator end = bookLevel.second.end();

        for ( ; (iter != end) && (deltaQtyFront.get() > 0);  ) {
            if ( tw::channel_or::eOrderType::kSim == (*iter)->_type ) {
                if ( processMatch(*iter, deltaQtyFront) ) {
                    eraseOrder(bookLevel.second, iter);
                    iter = bookLevel.second.begin();
                    end = bookLevel.second.end();
                } else {
                    ++iter;
                }
            } else {
                ++iter;
            }
        }
    }

    {
        TOrders::reverse_iterator iter = bookLevel.second.rbegin();
        TOrders::reverse_iterator end = bookLevel.second.rend();

        for ( ; (iter != end) && (deltaQtyBack.get() > 0);  ) {
            if ( tw::channel_or::eOrderType::kSim == (*iter)->_type ) {
                if ( processMatch(*iter, deltaQtyBack) ) {
                    eraseOrder(bookLevel.second, (++iter).base());
                    iter = bookLevel.second.rbegin();
                    end = bookLevel.second.rend();
                } else {
                    ++iter;
                }
            } else {
                ++iter;
            }
        }
    }
}

void Matcher::generateFill(TOrderPtr& order,
                           const tw::price::Ticks& price,
                           const tw::price::Size& qty,
                           const tw::channel_or::eLiqInd liqInd) {
    if ( tw::channel_or::eOrderType::kSim != order->_type ) {
        TFill fill;
        fill._order = order;
        fill._price = price;
        fill._qty = qty;
        fill._liqInd = liqInd;
        
        _eventListener.onFill(fill);
    }
    
    order->_cumQty += qty;
}

inline TOrderPtr Matcher::createSimOrder(const tw::price::Size& qty) {
    TOrderPtr order = createOrder();
    order->_type = tw::channel_or::eOrderType::kSim;
    order->_qty = qty;
    order->_cumQty.set(0);
    
    return order;
}

inline void Matcher::eraseOrder(Matcher::TOrders& orders, Matcher::TOrders::iterator iterOrder) {
    if ( iterOrder == orders.end() || (*iterOrder) == NULL )
        return;
    
    if ( tw::channel_or::eOrderType::kSim != (*iterOrder)->_type ) {
        TOrdersMap::iterator iter = _ordersMap.find((*iterOrder)->_exOrderId);
        _ordersMap.erase(iter);
    }
    
    orders.erase(iterOrder);
}

inline void Matcher::eraseOrder(TBook& book, TOrderPtr& order) {
    TBook::iterator iter = std::find_if(book.begin(), book.end(), pricePred(order->_price));
    if ( iter != book.end() ) {
        TOrders::iterator iterOrders = iter->second.begin();
        TOrders::iterator iterEnd = iter->second.end();

        for ( ; iterOrders != iterEnd; ++iterOrders ) {
            if ( order->_exOrderId == (*iterOrders)->_exOrderId ) {
                eraseOrder(iter->second, iterOrders);
                if ( iter->second.empty() )
                    book.erase(iter);
                break;
            }
        }
    }
}

void Matcher::print(const tw::price::Quote& quote, bool isInfo) const {
    if ( !_params._verbose )
        return;
    
    TStream stream;
    
    stream << quote.toString() << "\n" << "\n";
    toString(stream);
    
    if ( isInfo )
        LOGGER_INFO << stream.str() << "\n";
    else
        LOGGER_WARN << stream.str() << "\n";
}

void Matcher::print(const TOrderPtr& order) const {
    if ( !_params._verbose )
        return;
    
    TStream stream;
    if ( tw::channel_or::eOrderType::kSim != order->_type )
        stream << "::" << order->_corrClOrderId << "<-->" << order->_exOrderId << " " << order->_side.toString() << " ";
    
    stream << " ==> " << order->_qty-order->_cumQty;
    
    if ( tw::channel_or::eOrderType::kSim != order->_type )
        stream << "@" << order->_price;
    
    stream << "\n" << "\n";
    
    toString(stream);
    
    LOGGER_INFO << stream.str() << "\n";
}

} // tw::exchange_sim
} // tw
