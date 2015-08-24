#include <tw/channel_pf_cme/channel_pf_onix_upgrade.h>
#include <tw/log/defs.h>
#include <tw/common_strat/consumer_proxy.h>
#include <tw/price/ticks_converter.h>
#include <tw/common/high_res_time.h>

#include <stdio.h>
#include <tw/common_str_util/fast_numtoa.h>

static uint32_t MAX_GAP_FROM_PREVIOUS = 5;
static uint32_t MAX_DELAY_IN_MICROSEC = 250 * 1000; // 250 ms
static uint32_t MAX_DELAY_IN_MICROSEC_GAP = 60 * 1000 * 1000; // 1 min

namespace tw {
namespace channel_pf_cme {
  
static void checkExchangeSlow(const OnixS::CME::MarketData::ChannelId& channelId,
                              tw::price::Quote& quote,
                              uint64_t maxDelta,
                              const std::string& source) {
    uint64_t delta = quote._timestamp1-quote._exTimestamp;
    if ( maxDelta < delta ) {
        // Long delay over 1 minute is indicative of snapshot reception on start up? TODO: investigate
        //
        if ( delta > MAX_DELAY_IN_MICROSEC_GAP )
            quote._exTimestamp.setToNow();
        else
            quote._status = tw::price::Quote::kExchangeSlow;
    }
}

static bool getMessageSendingTime(tw::common::THighResTime& timestamp, const OnixS::CME::MarketData::Message& message) {
    static const uint32_t tag_SendingTime = 52;
    return message.get(tag_SendingTime).toTimestamp(timestamp.getImpl());
}

static bool getMessageStats(tw::price::Quote& quote, const OnixS::CME::MarketData::Message& message) {
    quote._seqNum = message.seqNum();
    quote._timestamp1 = message.receiveTime();
    if ( !quote._timestamp1.isValid() )
        quote._timestamp1.setToNow();
    
    if ( quote._timestamp2 < quote._timestamp1 )
        quote._timestamp2.setToNow();

    return getMessageSendingTime(quote._exTimestamp, message);
}

static void printQuote(tw::price::QuoteStore::TQuote& quote,
                       const OnixS::CME::MarketData::Message& message,
                       const OnixS::CME::MarketData::ChannelId& channelId,
                       const std::string& at,
                       const std::string& function,
                       bool verbose) {
    if ( !verbose )
        return;
    
    LOGGER_INFO << at << "::" << function <<  " -- " << (quote.isNormalTrade() ? "TRADES" : "BOOKS") << ": __________________________" << "\n";
    LOGGER_INFO << "FROM: \n" << channelId.data() << " :: " << message.toString() << "\n";
    LOGGER_INFO << "TO: \n" << quote.toString() << "\n";
}
    
static void printQuote(tw::price::QuoteStore::TQuote& quote,
                       const std::string& at,
                       const std::string& function,
                       bool verbose) {
    if ( !verbose )
        return;
    
    LOGGER_INFO << at << "::" << function <<  " -- " << (quote.isNormalTrade() ? "TRADES" : "BOOKS") << ": __________________________" << "\n";
    LOGGER_INFO << "QUOTE: \n" << quote.toString() << "\n";
}

static void updateSubscribers(tw::price::QuoteStore::TQuote& quote, const OnixS::CME::MarketData::Message& message, const OnixS::CME::MarketData::ChannelId& channelId, bool verbose, tw::price::QuoteStoreManager& quoteStoreManager) {
    if ( !quote.isChanged() )
        return;
    
    if ( getMessageStats(quote, message) )
        checkExchangeSlow(channelId, quote, MAX_DELAY_IN_MICROSEC, (quote.isNormalTrade() ? "Trade" : "BookUpdate"));
    
    quoteStoreManager.onQuote(quote);
    printQuote(quote, message, channelId, TW_AT, TW_FUNCTION, verbose);
    
    quote.clearRuntime(false);
}

class QuoteChannelFunctor {
public:
    QuoteChannelFunctor(tw::price::Quote::eStatus status,
                        const OnixS::CME::MarketData::ChannelId& channelId,
                        bool verbose,
                        tw::price::QuoteStoreManager& quoteStoreManager) : _status(status),
                                                                           _channelId(channelId),
                                                                           _verbose(verbose),
                                                                           _quoteStoreManager(quoteStoreManager) {
    }
                        
    QuoteChannelFunctor(const QuoteChannelFunctor& rhs) : _status(rhs._status),
                                                          _channelId(rhs._channelId),
                                                          _verbose(rhs._verbose),
                                                          _quoteStoreManager(rhs._quoteStoreManager) {
    }
                 
    void operator()(tw::price::QuoteStore::TQuote& quote) {
        printQuote(quote, TW_AT, TW_FUNCTION, _verbose);
        quote._timestamp2.setToNow();
        if ( !quote.isValid() || !quote.isSubscribed() )
            return;
        
        if ( quote.getInstrument()->_var1 != _channelId )
            return;
        
        quote._status = _status;
        _quoteStoreManager.onQuote(quote);        
    }
    
    tw::price::Quote::eStatus _status;
    const OnixS::CME::MarketData::ChannelId& _channelId;
    bool _verbose;
    tw::price::QuoteStoreManager& _quoteStoreManager;
};

class QuoteCoalesceFunctor {
public:
    QuoteCoalesceFunctor(const OnixS::CME::MarketData::Message& message,
                         const OnixS::CME::MarketData::ChannelId& channelId,
                         bool verbose,
                         tw::price::QuoteStoreManager& quoteStoreManager) : _message(message),
                                                                            _channelId(channelId),
                                                                            _verbose(verbose),
                                                                            _quoteStoreManager(quoteStoreManager) {
    }
                        
    QuoteCoalesceFunctor(const QuoteCoalesceFunctor& rhs) : _message(rhs._message),
                                                            _channelId(rhs._channelId),
                                                            _verbose(rhs._verbose),
                                                            _quoteStoreManager(rhs._quoteStoreManager) {                     
    }
    
    void operator()(tw::price::QuoteStore::TQuote* quote) {
        if ( !quote )
            return;
        
       updateSubscribers(*quote, _message, _channelId, _verbose, _quoteStoreManager);
    }
    
    const OnixS::CME::MarketData::Message& _message;
    const OnixS::CME::MarketData::ChannelId& _channelId;
    bool _verbose;
    tw::price::QuoteStoreManager& _quoteStoreManager;
};
    
ChannelPfOnix::ChannelPfOnix() : _settings(),
                                 _handlerSecurityDefinitions(NULL),
                                 _handlers() {
}

ChannelPfOnix::~ChannelPfOnix() {
    stop();
}

// tw::channel_pf::IChannelImpl interface
//
bool ChannelPfOnix::init(const tw::common::Settings& settings) {
    bool status = true;
    try {
        if ( settings._channel_pf_cme_dataSourceType != "file" ) {
            LOGGER_ERRO << "Unsupported data source type: " << settings._channel_pf_cme_dataSourceType << "\n";
            return false;
        }
        
        tw::channel_pf_cme::Settings settingsCme;
        if ( !settingsCme.parse(settings._channel_pf_cme_dataSource) ) {
            LOGGER_ERRO << "failed to parse config file settings: " << settings._channel_pf_cme_dataSource << " :: " << settingsCme.toStringDescription() << "\n";
            return false;
        }
        
        status = doInit(settingsCme);
        if ( status )
            _globalSettings = settings;
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    if ( !status )
        stop();
    
    return status;
}

bool ChannelPfOnix::start() {
    bool status = true;
    try {
        LOGGER_INFO_EMPHASIS << "Starting channel price feed CME (onix)..." << "\n";
        
        if ( !_quoteStoreManager.start(_globalSettings._channel_pf_cme_isMultithreaded, _settings._verbose) ) {
            LOGGER_ERRO << "Failed to start quoteStoreManager" << "\n";
            return false;
        }
        
        if ( _handlers.empty() ) {
            LOGGER_ERRO << "No handlers for channel price feed CME (onix)" << "\n";
            return false;
        }
        
        THandlers::iterator iter = _handlers.begin();
        THandlers::iterator end = _handlers.end();
        for ( ; iter != end; ++ iter ) {
            if ( !_settings._replay ) {
                LOGGER_INFO_EMPHASIS << "Starting channel handler: " << iter->first << (_settings._naturalRefresh ? "(naturalRefresh=true)" : "(naturalRefresh=false)") << "..." << "\n";
                if ( _settings._naturalRefresh ) {
                    OnixS::CME::MarketData::NaturalRefreshOptions options;
                    options.definitionsFile = _settings._definitionsFile;
                    iter->second.first->start(options);
                } else {
                    iter->second.first->start();
                }
                LOGGER_INFO_EMPHASIS << "Started channel handler: " << iter->first << "\n";
            } else {
                LOGGER_INFO_EMPHASIS << "Starting channel handler(REPLAY _MODE): " << iter->first << "..." << "\n";
                char buffer[4] = { '\0', '\0', '\0', '\0' };
                fast_itoa10(iter->first, buffer);
                OnixS::CME::MarketData::ChannelId channelId((std::string(buffer)));
                OnixS::CME::MarketData::ReplayOptions replayOptions(channelId, _settings._replayFolder, _settings._settings.logFileNamePrefix);
                iter->second.first->start(replayOptions);
                LOGGER_INFO_EMPHASIS << "Started channel handler(REPLAY _MODE): " << iter->first << "\n";
            }
        }
        
        LOGGER_INFO_EMPHASIS << "Started channel price feed CME (onix)" << "\n";
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }

    if ( !status )
        stop();
    
    return status;
}

bool ChannelPfOnix::start(OnixS::CME::MarketData::SecurityDefinitionListener* handler) {
    _handlerSecurityDefinitions = handler;
    return start();    
}

void ChannelPfOnix::stop() {
    try {
        LOGGER_INFO_EMPHASIS << "Stopping channel price feed CME (onix)..." << "\n";
        
        THandlers::iterator iter = _handlers.begin();
        THandlers::iterator end = _handlers.end();
        for ( ; iter != end; ++ iter ) {
            try {
                LOGGER_INFO_EMPHASIS << "Stopping channel handler: " << iter->first << "..." << "\n";
                iter->second.first->stop();
                LOGGER_INFO_EMPHASIS << "Stopped channel handler: " << iter->first << "\n";
            }catch(const std::exception& e) {
                LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
            } catch(...) {
                LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
            }
        }
        
        _handlerSecurityDefinitions = NULL;
        _handlers.clear();
        
        _quoteStoreManager.stop();
        
        LOGGER_INFO_EMPHASIS << "Stopped channel price feed CME (onix)" << "\n";
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

bool ChannelPfOnix::subscribe(tw::instr::InstrumentConstPtr instrument) {
    bool status = true;
    try {
        LOGGER_INFO_EMPHASIS << "Subscribing: " << instrument->_displayName << " :: " << instrument->toString() << "\n";
        
        THandlerPtr handler;
        int channelIdAsInt = channelIdToInt(OnixS::CME::MarketData::ChannelId(instrument->_var1));
        THandlers::iterator iter = _handlers.find(channelIdAsInt);
        if ( iter == _handlers.end() ) {
            LOGGER_INFO << "Can't find handler for channelId: " << channelIdAsInt << "\n";
            handler = addHandler(OnixS::CME::MarketData::ChannelId(instrument->_var1));
        } else {
            handler = iter->second.first;
        }
        
        handler->addSecurityIdFilter(instrument->_keyNum1);        
        
        LOGGER_INFO_EMPHASIS << "Subscribed: " << instrument->_displayName << "\n";
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    return status;
}

bool ChannelPfOnix::unsubscribe(tw::instr::InstrumentConstPtr instrument) {
    // NOTE: ChannelPfOnix always stays subscribed to instrument until
    // re initialized/re started/re subscribed
    // 
    return true;
}

// OnixS::CME::MarketData::DirectBookUpdateListener interface
//

// TODO: fast to implement, but not the most efficient way to parse books
// need to revisit later
//
void ChannelPfOnix::onDirectBookUpdated(const OnixS::CME::MarketData::DirectBook& book, const OnixS::CME::MarketData::ChannelId& channelId) {
    tw::price::QuoteStore::TQuote& quote = _quoteStoreManager.getStore().getQuoteByKeyNum1(book.securityId());
    if ( !quote.isValid() || !quote.isSubscribed() )
        return;
    
    if ( _settings._verbose ) {
        LOGGER_INFO << "SeqNum=" << book.message().seqNum() << ",channelId=" << channelId.data() <<  " -- BOOK ==> " << book.toString() << "\n";
        printQuote(quote, TW_AT, TW_FUNCTION, _settings._verbose);
    }
    
    if ( !quote.isChanged() )
        quote._timestamp2.setToNow();
    
    uint32_t numOfLevels = std::min(book.depth(), static_cast<OnixS::CME::MarketData::MarketDepth>(tw::price::QuoteStore::TQuote::SIZE));
    for ( uint32_t count = 0; count < numOfLevels; ++count ) {
        {
            // Set bid
            //
            const OnixS::CME::MarketData::DirectPriceLevel& priceLevelFrom = book.bids()[count];
            tw::price::Size size(static_cast<tw::price::Size::type>(priceLevelFrom.quantity()));
            if ( size.get() > 0 ) {
                tw::price::PriceSizeNumOfOrders& priceLevelTo = quote._book[count]._bid;        

                tw::price::Ticks ticks = quote._convertFrom(priceLevelFrom.price());
                if ( ticks != priceLevelTo._price ) {
                    if ( 0 == count ) 
                        checkGappedQuote(ticks, priceLevelTo._price, quote, MAX_GAP_FROM_PREVIOUS);

                    quote.setBidPrice(ticks, count);
                }

                if ( size != priceLevelTo._size )
                    quote.setBidSize(size, count);

                priceLevelTo._numOrders = priceLevelFrom.numberOfOrders();
            }
        }
        
        {
            // Set ask
            //
            const OnixS::CME::MarketData::DirectPriceLevel& priceLevelFrom = book.asks()[count];
            tw::price::Size size(static_cast<tw::price::Size::type>(priceLevelFrom.quantity()));
            if ( size.get() > 0 ) {
                tw::price::PriceSizeNumOfOrders& priceLevelTo = quote._book[count]._ask;

                tw::price::Ticks ticks = quote._convertFrom(priceLevelFrom.price());
                if ( ticks != priceLevelTo._price ) {
                    if ( 0 == count ) 
                        checkGappedQuote(ticks, priceLevelTo._price, quote, MAX_GAP_FROM_PREVIOUS);

                    quote.setAskPrice(ticks, count);
                }

                if ( size != priceLevelTo._size )
                    quote.setAskSize(size, count);

                priceLevelTo._numOrders = priceLevelFrom.numberOfOrders();
            }
        }        
    }
    
    if ( _settings._coalesce ) {
        _handlers[channelIdToInt(channelId)].second.insert(&quote);
        return;
    }
    
    updateSubscribers(quote, book.message(), channelId, _settings._verbose, _quoteStoreManager);    
}

// OnixS::CME::MarketData::TradeListener interface
//
void ChannelPfOnix::onTrade(const OnixS::CME::MarketData::Trade& trade, const OnixS::CME::MarketData::ChannelId& channelId) {
    tw::price::QuoteStore::TQuote& quote = _quoteStoreManager.getStore().getQuoteByKeyNum1(trade.securityId());
    if ( !quote.isValid() || !quote.isSubscribed() )
        return;
    
    if ( _settings._verbose ) {
        LOGGER_INFO << "SeqNum=" << trade.marketDataEntry().message().seqNum()
                    << ",channelId=" << channelId.data()
                    <<  " -- TRADE ==> " << trade.toString() << "\n";
    }
    
    if ( !quote.isChanged() )
        quote._timestamp2.setToNow();
    
    OnixS::CME::MarketData::SignedQuantity qty = 0;
    if ( !trade.quantity(qty) || qty < 1 )
        return;
    
    switch ( trade.tradeCondition() ) {
        case OnixS::CME::MarketData::TradeConditions::OpeningTrade:
            if ( _settings._processNormalTradesOnly )
                return;
            quote._trade._condition = tw::price::Trade::kConditionOpening;
            break; 
        case OnixS::CME::MarketData::TradeConditions::PriceCalculatedByGlobex:
            if ( _settings._processNormalTradesOnly )
                return;
            quote._trade._condition = tw::price::Trade::kConditionPriceCalculatedByExchange;
            break; 
        default:
            quote._trade._condition = tw::price::Trade::kConditionUnknown;
            break; 
    }
    
    switch ( trade.action() ) {
        case OnixS::CME::MarketData::TradeUpdateActions::New:
            quote._trade._updateAction = tw::price::Trade::kUpdateActionNew;
            break; 
        case OnixS::CME::MarketData::TradeUpdateActions::Delete:
            quote._trade._updateAction = tw::price::Trade::kUpdateActionDelete;
            break; 
        default:
            quote._trade._updateAction = tw::price::Trade::kUpdateActionUnknown;
            break; 
    }
    
    tw::price::Trade::eTickDirection prevTickDirection = quote._trade._tickDirection;
    switch ( trade.tickDirection() ) {
        case OnixS::CME::MarketData::TickDirections::PlusTick:
            quote._trade._tickDirection = tw::price::Trade::kTickDirectionPlus;
            break; 
        case OnixS::CME::MarketData::TickDirections::MinusTick:
            quote._trade._tickDirection = tw::price::Trade::kTickDirectionMinus;
            break; 
        default:
            quote._trade._tickDirection = tw::price::Trade::kTickDirectionUnknown;
            break; 
    }
    
    switch ( trade.aggressorSide() ) {
        case OnixS::CME::MarketData::AggressorSides::Buy:
            quote._trade._aggressorSide = tw::price::Trade::kAggressorSideBuy;
            break; 
        case OnixS::CME::MarketData::AggressorSides::Sell:
            quote._trade._aggressorSide = tw::price::Trade::kAggressorSideSell;
            break; 
        default:
            quote._trade._aggressorSide = tw::price::Trade::kAggressorSideUnknown;
            break; 
    }
    
    
    const tw::price::Ticks ticks = quote._convertFrom(trade.price());
    checkGappedTrade(ticks, quote, MAX_GAP_FROM_PREVIOUS);
    
    if ( _settings._coalesce ) {    
        if ( !quote.isTrade() ) {
            quote._trade._price = ticks;
            quote._trade._size.set(qty);
            quote.setTrade();
            _handlers[channelIdToInt(channelId)].second.insert(&quote);
        } else if ( ticks == quote._trade._price ) {
            quote._trade._tickDirection = prevTickDirection;
            quote._trade._size += tw::price::Size(qty);
        } else {
            tw::price::Trade::eTickDirection currTickDirection = quote._trade._tickDirection;
            quote._trade._tickDirection = prevTickDirection;
            updateSubscribers(quote, trade.marketDataEntry().message(), channelId, _settings._verbose, _quoteStoreManager);
            quote._trade._tickDirection = currTickDirection;
            quote._trade._price = ticks;
            quote._trade._size.set(qty);
            quote.setTrade();
        }
        
        printQuote(quote, trade.marketDataEntry().message(), channelId, TW_AT, TW_FUNCTION, _settings._verbose);
        return;
    }
    
    quote._trade._price = ticks;
    quote._trade._size.set(qty);
    quote.setTrade();
        
    updateSubscribers(quote, trade.marketDataEntry().message(), channelId, _settings._verbose, _quoteStoreManager);
}

// OnixS::CME::MarketData::ErrorListener interface
//
void ChannelPfOnix::onError(const OnixS::CME::MarketData::Error& error, const OnixS::CME::MarketData::ChannelId& channelId) {
    tw::price::Quote::eStatus status = tw::price::Quote::kSuccess;
    switch ( error.code() ) {
        case OnixS::CME::MarketData::KnownErrors::Generic:
        case OnixS::CME::MarketData::KnownErrors::NotLicensed:
        case OnixS::CME::MarketData::KnownErrors::BadConfiguration:
        case OnixS::CME::MarketData::KnownErrors::BadPacket:
        case OnixS::CME::MarketData::KnownErrors::BadMessage:
            status = tw::price::Quote::kConnectionHandlerError;
            break;
        default:
            break;
    }
    
    if ( tw::price::Quote::kSuccess != status )
        _quoteStoreManager.getStore().for_each_quote(QuoteChannelFunctor(status, channelId, _settings._verbose, _quoteStoreManager));
    
    LOGGER_INFO << "ERROR: __________________________" << "\n";
    LOGGER_INFO << "FROM: \n" << channelId.data() << " :: " << error.toString() << "\n";
    LOGGER_INFO << "TO: \n" << tw::price::Quote::statusToString(status) << "\n";
}

// OnixS::CME::MarketData::HandlerStateChangeListener interface
//
void ChannelPfOnix::onHandlerStateChange(const OnixS::CME::MarketData::HandlerStateChange& change, const OnixS::CME::MarketData::ChannelId& channelId) {
    tw::price::Quote::eStatus status = tw::price::Quote::kSuccess;
    switch ( change.newState() ) {
        case OnixS::CME::MarketData::HandlerStates::Stopped:
            status = tw::price::Quote::kExchangeDown;
            break;
        case OnixS::CME::MarketData::HandlerStates::Started:
            status = tw::price::Quote::kExchangeUp;
            break;
        case OnixS::CME::MarketData::HandlerStates::BooksResynchronizationStarted:
            status = tw::price::Quote::kDataRecoveryStart;
            break;
        case OnixS::CME::MarketData::HandlerStates::BooksResynchronizationFinished:
            status = tw::price::Quote::kDataRecoveryFinish;
            break;
        default:
            break;
    }
    
    if ( tw::price::Quote::kSuccess != status ) {
        _quoteStoreManager.getStore().for_each_quote(QuoteChannelFunctor(status, channelId, _settings._verbose, _quoteStoreManager));
        if ( _settings._verbose ) {
            LOGGER_INFO << "HANDLER_STATE_CHANGE: __________________________" << "\n";
            LOGGER_INFO << "FROM: \n" << channelId.data() << " :: " << change.toString() << "\n";
            LOGGER_INFO << "TO: \n" << tw::price::Quote::statusToString(status) << "\n";
        }
    } else if ( _settings._verbose ) {
        LOGGER_INFO << "HANDLER_STATE_CHANGE: __________________________" << "\n";
        LOGGER_INFO << "FROM: \n" << channelId.data() << " :: " << change.toString() << "\n";
    }
}

// OnixS::CME::MarketData::MessageProcessingListener interface
//
void ChannelPfOnix::onProcessingBegin(const OnixS::CME::MarketData::Message& message, const OnixS::CME::MarketData::ChannelId& channelId) {
    // Nothing to do for now
    //
}

void ChannelPfOnix::onProcessingEnd(const OnixS::CME::MarketData::Message& message, const OnixS::CME::MarketData::ChannelId& channelId) {
    if ( !_settings._coalesce )
        return;
    
    THandlers::iterator iter = _handlers.find(channelIdToInt(channelId));
    if ( iter == _handlers.end() || iter->second.second.empty() )
        return;
    
    TUpdatedQuotes& updatedQuotes = iter->second.second;
    std::for_each(updatedQuotes.begin(), updatedQuotes.end(), QuoteCoalesceFunctor(message, channelId, _settings._verbose, _quoteStoreManager));
    updatedQuotes.clear();
}

void ChannelPfOnix::onProcessingSuspended(const OnixS::CME::MarketData::ChannelId& channelId) {
    _quoteStoreManager.getStore().for_each_quote(QuoteChannelFunctor(tw::price::Quote::kStaleQuote, channelId, _settings._verbose, _quoteStoreManager));
}

// OnixS::CME::MarketData::NewsListener interface
//
void ChannelPfOnix::onNews(const OnixS::CME::MarketData::News& news, const OnixS::CME::MarketData::ChannelId& channelId) {
    // TODO: need to implement news' subscriber
    //
    if ( _settings._verbose ) {
        LOGGER_INFO << "NEWS: __________________________" << "\n";
        LOGGER_INFO << "FROM: \n" << channelId.data() << " :: " << news.toString() << "\n";
    }
}

// OnixS::CME::MarketData::ReplayListener interface
//
void ChannelPfOnix::onReplayError(const OnixS::CME::MarketData::ChannelId& channelId, const std::string& errorDescription) {
    _quoteStoreManager.getStore().for_each_quote(QuoteChannelFunctor(tw::price::Quote::kReplayError, channelId, _settings._verbose, _quoteStoreManager));
    
    if ( _settings._verbose ) {
        LOGGER_INFO << "REPLAY_ERROR: __________________________" << "\n";
        LOGGER_INFO << "FROM: \n" << channelId.data() << " :: " << errorDescription << "\n";            
    }
}  

void ChannelPfOnix::onReplayFinished(const OnixS::CME::MarketData::ChannelId& channelId) {
    _quoteStoreManager.getStore().for_each_quote(QuoteChannelFunctor(tw::price::Quote::kReplayFinished, channelId, _settings._verbose, _quoteStoreManager));
    if ( _settings._verbose ) {
        LOGGER_INFO << "REPLAY_FINISHED: __________________________" << "\n";
        LOGGER_INFO << "FROM: \n" << channelId.data() << " :: " << "\n";
    }
}

// OnixS::CME::MarketData::SecurityDefinitionListener interface
//
void ChannelPfOnix::onSecurityDefinition(const OnixS::CME::MarketData::SecurityDefinition& securityDefinition, const OnixS::CME::MarketData::ChannelId& channelId) {
    try {
        if ( !_settings._instrumentLoading ) {        
            LOGGER_ERRO << "SHOULDN'T be called -- '_instrumentLoading'= false" << "\n";
            return;
        }

        if ( _handlerSecurityDefinitions )
            _handlerSecurityDefinitions->onSecurityDefinition(securityDefinition, channelId);

        if ( _settings._verbose ) {
            LOGGER_INFO << "SECURITY_DEFINITION: __________________________" << "\n";
            LOGGER_INFO << "FROM: \n" << channelId.data() << " :: " << securityDefinition.toString() << "\n";
        } 
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void ChannelPfOnix::onSecurityDefinitionDeleted(OnixS::CME::MarketData::SecurityId securityId, const OnixS::CME::MarketData::ChannelId& channelId) {
    try {
        if ( !_settings._instrumentLoading ) {        
            LOGGER_ERRO << "SHOULDN'T be called -- '_instrumentLoading'= false" << "\n";
            return;
        }
        
        if ( _handlerSecurityDefinitions )
            _handlerSecurityDefinitions->onSecurityDefinitionDeleted(securityId, channelId);

        LOGGER_WARN << "SecurityDefinitionDeleted started for: " << channelId.data() << " :: " << securityId << "\n";    
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void ChannelPfOnix::onSecurityDefinitionsRecoveryStarted(const OnixS::CME::MarketData::ChannelId& channelId) {
    try {
        if ( !_settings._instrumentLoading ) {        
            LOGGER_ERRO << "SHOULDN'T be called -- '_instrumentLoading'= false" << "\n";
            return;
        }
        
        if ( _handlerSecurityDefinitions )
            _handlerSecurityDefinitions->onSecurityDefinitionsRecoveryStarted(channelId);

        LOGGER_INFO_EMPHASIS << "SecurityDefinitionsRecoveryStarted started for: " << channelId.data() << "\n";
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void ChannelPfOnix::onSecurityDefinitionsRecoveryFinished(const OnixS::CME::MarketData::ChannelId& channelId) {
    try {
        if ( !_settings._instrumentLoading ) {        
            LOGGER_ERRO << "SHOULDN'T be called -- '_instrumentLoading'= false" << "\n";
            return;
        }
        
        if ( _handlerSecurityDefinitions )
            _handlerSecurityDefinitions->onSecurityDefinitionsRecoveryFinished(channelId);

        LOGGER_INFO_EMPHASIS << "SecurityDefinitionsRecoveryStarted finished for: " << channelId.data() << "\n";   
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

// OnixS::CME::MarketData::SecurityStatusListener interface
//
void ChannelPfOnix::onSecurityStatus(const OnixS::CME::MarketData::SecurityStatus& status, const OnixS::CME::MarketData::ChannelId& channelId) {
    OnixS::CME::MarketData::SecurityId securityId;
    if ( !status.securityId(securityId) ) {
        if ( _settings._verbose ) {
            LOGGER_INFO << "SECURITY_STATUS: __________________________" << "\n";
            LOGGER_INFO << "FROM: \n" << channelId.data() << " :: " << status.toString() << "\n";
        }        
        return;
    }
    
    tw::price::QuoteStore::TQuote& quote = _quoteStoreManager.getStore().getQuoteByKeyNum1(securityId);
    if ( !quote.isValid() || !quote.isSubscribed() )
        return;
    
    switch ( status.tradingEvent() ) {
        case OnixS::CME::MarketData::SecurityTradingEvents::TradingHaltOrStopSpike:
            quote._status = tw::price::Quote::kTradingHaltedOrStopped;
            break;
        case OnixS::CME::MarketData::SecurityTradingEvents::ResumeOrOpen:
            quote._status = tw::price::Quote::kTradingResumeOrOpen;
            break;
        case OnixS::CME::MarketData::SecurityTradingEvents::EndOfTradingSession:
            quote._status = tw::price::Quote::kTradingSessionEnd;
            break;
        default:
            break;
    }
    
    if ( quote.isError() ) {
        if ( _settings._coalesce )
            _handlers[channelIdToInt(channelId)].second.insert(&quote);
        else
            updateSubscribers(quote, status.message(), channelId, _settings._verbose, _quoteStoreManager);    
    }
}

// OnixS::CME::MarketData::StatisticsListener interface
//
void ChannelPfOnix::onStatisticsReset(OnixS::CME::MarketData::SecurityId securityId, const OnixS::CME::MarketData::ChannelId& channelId) {
    tw::price::QuoteStore::TQuote& quote = _quoteStoreManager.getStore().getQuoteByKeyNum1(securityId);
    if ( !quote.isValid() || !quote.isSubscribed() )
        return;
    
    quote._status = tw::price::Quote::kHLOReset;
    _quoteStoreManager.onQuote(quote);
    if ( _settings._verbose ) {
        LOGGER_INFO << "STATISTICS_RESET: __________________________" << "\n";
        LOGGER_INFO << "FROM: \n" << channelId.data() << " :: " << "\n";
    }
}

void ChannelPfOnix::onStatisticsReset(const OnixS::CME::MarketData::Symbol& symbol, const OnixS::CME::MarketData::ChannelId& channelId) {
    tw::instr::InstrumentPtr instrument = tw::instr::InstrumentManager::instance().getByDisplayName(symbol.data());
    if ( instrument == NULL ) {
        if ( _settings._verbose ) {
            LOGGER_INFO << "STATISTICS_RESET: __________________________" << "\n";
            LOGGER_INFO << "FROM: \n" << channelId.data() << " :: " << "\n";
        }
        
        return;
    }
    
    onStatisticsReset(instrument->_keyNum1, channelId);
}

void ChannelPfOnix::onStatistics(const OnixS::CME::MarketData::Statistics& statistics, const OnixS::CME::MarketData::ChannelId& channelId) {
    tw::price::QuoteStore::TQuote& quote = _quoteStoreManager.getStore().getQuoteByKeyNum1(statistics.securityId());
    if ( !quote.isValid() || !quote.isSubscribed() )
        return;    
    
    switch ( statistics.type() ) {
        case OnixS::CME::MarketData::StatisticsTypes::OpeningPrice:
            quote.setOpen(quote._convertFrom(statistics.valueAsPrice()));
            break;
        case OnixS::CME::MarketData::StatisticsTypes::TradingSessionHighPrice:
            quote.setHigh(quote._convertFrom(statistics.valueAsPrice()));
            break;
        case OnixS::CME::MarketData::StatisticsTypes::TradingSessionLowPrice:
            quote.setLow(quote._convertFrom(statistics.valueAsPrice()));
            break;
        default:
            break;
    }
    
    if ( quote.isChanged() ) {
        if ( _settings._coalesce )
            _handlers[channelIdToInt(channelId)].second.insert(&quote);
        else
            updateSubscribers(quote, statistics.marketDataEntry().message(), channelId, _settings._verbose, _quoteStoreManager);    
    }
}

// OnixS::CME::MarketData::WarningListener interface
//
void ChannelPfOnix::onWarning(const OnixS::CME::MarketData::Warning& warning, const OnixS::CME::MarketData::ChannelId& channelId) {
    tw::price::Quote::eStatus status = tw::price::Quote::kSuccess;
    switch ( warning.code() ) {
        case OnixS::CME::MarketData::KnownWarnings::NoNetworkActivity:
            _quoteStoreManager.getStore().for_each_quote(QuoteChannelFunctor(tw::price::Quote::kStaleQuote, channelId, _settings._verbose, _quoteStoreManager));
            break;
        case OnixS::CME::MarketData::KnownWarnings::PacketGap:
        //case OnixS::CME::MarketData::KnownWarnings::PacketCacheOverflow: // !!!! - TODO: need to investogate and resolve with Onix!
            //status = tw::price::Quote::kDropRate;
            break;
        case OnixS::CME::MarketData::KnownWarnings::ListenerFailure:
            status = tw::price::Quote::kConnectionHandlerError;
            break;
        default:
            break;
    }
    
    if ( tw::price::Quote::kSuccess != status )
        _quoteStoreManager.getStore().for_each_quote(QuoteChannelFunctor(status, channelId, _settings._verbose, _quoteStoreManager));
    
    LOGGER_INFO << "WARNING: __________________________" << "\n";
    LOGGER_INFO << "FROM: \n" << channelId.data() << " :: " << warning.toString() << "\n";
    if ( tw::price::Quote::kSuccess != status )
        LOGGER_INFO << "TO: \n" << tw::price::Quote::statusToString(status) << "\n";
}

// Utility functions
//

// Registering all interfaces' callbacks
//
bool ChannelPfOnix::doInit(const tw::channel_pf_cme::Settings& settings) {
    bool status = true;
    try {
        LOGGER_INFO_EMPHASIS << "Initializing channel price feed CME (onix)..." << "\n";
        LOGGER_INFO_EMPHASIS << "Settings: " << settings.fileName() << " :: " << settings.toString() << "\n";
        
        if ( !settings.validate() ) {
            LOGGER_ERRO << "error in validating settings: " << settings.fileName() << " :: " << settings.toStringDescription() << "\n";
            return false;
        }
        
        // Global initialization
        //
        OnixS::CME::MarketData::initialize(settings._globalSettings);
        _settings = settings;
        
        // Handlers' initialization (instrument loader only)
        //
        if ( _settings._instrumentLoading ) {
            for ( size_t count = 0; count < _settings._channelIds.size(); ++count ) {
                addHandler(OnixS::CME::MarketData::ChannelId(_settings._channelIds[count]));
            }        
        }

        // QuoteStore initialization
        //
        tw::price::QuoteStore::instance().setQuoteNotificationStatsInterval(_settings._quoteNotificationStatsInterval);
        
        LOGGER_INFO_EMPHASIS << "Initialized channel price feed CME (onix)" << "\n";        
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    return status;    
}

ChannelPfOnix::THandlerPtr ChannelPfOnix::addHandler(const OnixS::CME::MarketData::ChannelId& channelId) {
    LOGGER_INFO << "creating handler for channelId: " << channelId.data() << "\n";
    
    _settings._settings.channelId = channelId.toString();
    THandlerPtr handler = THandlerPtr(new THandler(_settings._settings));
    registerWithHandler(handler, _settings._topOfBookOnly);
    
    int channelIdAsInt = channelIdToInt(channelId);
    _handlers[channelIdAsInt] = THandlerInfo(handler, TUpdatedQuotes());
    
    return handler;
}

void ChannelPfOnix::registerWithHandler(THandlerPtr handler, bool topOfBookOnly) {    
    if ( _settings._instrumentLoading ) {
        handler->registerSecurityDefinitionListener(this);
        LOGGER_WARN << "Channel is used only for instrument loading security definitions - WILL NOT listen to quotes!"<< "\n";
        return;
    }
    
    OnixS::CME::MarketData::DirectBestBidAskTrackingOptions bbaTrackingOptions;
    
    if ( topOfBookOnly )
        bbaTrackingOptions.parameters = OnixS::CME::MarketData::BestBidAskParameters::Price | OnixS::CME::MarketData::BestBidAskParameters::Quantity;    
    
    handler->registerDirectBookUpdateListener(this, bbaTrackingOptions);
    
    handler->registerTradeListener(this);
    handler->registerNewsListener(this);
    handler->registerSecurityStatusListener(this);
    handler->registerStatisticsListener(this);
    handler->registerMessageProcessingListener(this);
    handler->registerStateChangeListener(this);
    handler->registerErrorListener(this);
    handler->registerWarningListener(this);
}

    
} // channel_pf_cme
} // tw

