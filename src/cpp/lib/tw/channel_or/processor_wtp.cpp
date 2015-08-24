#include <tw/channel_or/processor_wtp.h>
#include <tw/price/quote_store.h>

namespace tw {
namespace channel_or {
    
bool isCrossingOrderAtBestPrice(const tw::price::Ticks& quotePrice, const tw::price::Ticks& orderPrice, const eOrderSide side) {
    if ( !quotePrice.isValid() || !orderPrice.isValid() || eOrderSide::kUnknown == side )
        return true;
    
    if ( (eOrderSide::kBuy == side && orderPrice < quotePrice) || (eOrderSide::kSell == side && orderPrice > quotePrice) ) 
        return false;
    
    return true;
}
    
ProcessorWTP::ProcessorWTP() {
    clear();    
}

ProcessorWTP::~ProcessorWTP() {
    stop();
}
    
void ProcessorWTP::clear() {
    _ordersBooks.clear();
}

bool ProcessorWTP::init(const tw::common::Settings& settings) {
    _settings = settings;
    
    LOGGER_INFO << "_settings._trading_wtp_use_quotes=" << (_settings._trading_wtp_use_quotes ? "true" : "false") << "\n";
    return true;
}

bool ProcessorWTP::start() {
    return true;
}

void ProcessorWTP::stop() {
    clear();
}

bool ProcessorWTP::canSend(const tw::instr::InstrumentConstPtr& instrument, eOrderSide side, const tw::price::Ticks& price, Reject& rej) {
    TOrdersBook& ordersBook = getOrCreateOrdersBook(instrument->_keyId);
    
    TOrderPtr crossingOrder;
    if ( eOrderSide::kBuy == side )
        crossingOrder = getCrossingOrder(ordersBook.second, price, std::greater_equal<tw::price::Ticks>());
    else
        crossingOrder = getCrossingOrder(ordersBook.first, price, std::less_equal<tw::price::Ticks>());
    
    if ( NULL != crossingOrder ) {
        if ( _settings._trading_wtp_use_quotes && instrument ) {
            const tw::price::QuoteStore::TQuote& quote = tw::price::QuoteStore::instance().getQuote(instrument);
            
            tw::price::Ticks orderPrice = crossingOrder->_price;
            if ( (eOrderState::kModifying == crossingOrder->_state) && ((eOrderSide::kBuy == crossingOrder->_side && crossingOrder->_newPrice > orderPrice) || (eOrderSide::kSell == crossingOrder->_side && crossingOrder->_newPrice < orderPrice)) )
                orderPrice = crossingOrder->_newPrice;
            
            if ( !isCrossingOrderAtBestPrice(quote._trade._price, orderPrice, crossingOrder->_side) && !isCrossingOrderAtBestPrice((eOrderSide::kBuy == crossingOrder->_side ? quote._book[0]._bid._price : quote._book[0]._ask._price), orderPrice,crossingOrder->_side) )
                return true;
        }
        rej = getRej(eRejectReason::kWashTradePrevention);
        rej._text = "crosses with: " + crossingOrder->_orderId.toString();
        return false;
    }
    
    return true;
}

bool ProcessorWTP::canSend(const TOrderPtr& order, const tw::price::Ticks& price, Reject& rej) {
    if ( order->_stopLoss )
        return true;
    
    return canSend(order->_instrument, order->_side, price, rej);
}

bool ProcessorWTP::sendNew(const TOrderPtr& order, Reject& rej) {
    if ( !canSend(order, order->_price, rej) )
        return false;
    
    if ( eOrderSide::kBuy == order->_side )
        getOrCreateOrdersBook(order->_instrumentId).first.push_back(order);
    else
        getOrCreateOrdersBook(order->_instrumentId).second.push_back(order);
    
    return true;
}

bool ProcessorWTP::sendMod(const TOrderPtr& order, Reject& rej) {
    return canSend(order, order->_newPrice, rej);
}

bool ProcessorWTP::sendCxl(const TOrderPtr& order, Reject& rej) {
    return true;
}

void ProcessorWTP::onCommand(const tw::common::Command& command) {
    // TODO: need to implement
    //
}

ProcessorWTP::TOrdersBook& ProcessorWTP::getOrCreateOrdersBook(const tw::instr::Instrument::TKeyId& v) {
    TOrdersBooks::iterator iter = _ordersBooks.find(v);
    if ( iter == _ordersBooks.end() )
        iter = _ordersBooks.insert(TOrdersBooks::value_type(v, TOrdersBook())).first;
    
    return iter->second;
}
    
} // namespace channel_or
} // namespace tw
