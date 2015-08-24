#include <tw/channel_pf_cme/channel_pf_onix_legacy.h>
#include <tw/log/defs.h>
#include <tw/price/quote_store.h>
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
    
class QuoteChannelFunctor {
public:
    QuoteChannelFunctor(tw::price::Quote::eStatus status,
                        const OnixS::CME::MarketData::ChannelId& channelId) : _status(status),
                                                                              _channelId(channelId) {                     
    }
                        
    QuoteChannelFunctor(const QuoteChannelFunctor& rhs) : _status(rhs._status),
                                                          _channelId(rhs._channelId) {                     
    }
                 
    void operator()(tw::price::QuoteStore::TQuote& quote) {
        quote._timestamp2.setToNow();
        if ( !quote.isValid() || !quote.isSubscribed() )
            return;
        
        if ( quote.getInstrument()->_var1 != _channelId )
            return;
        
        quote._status = _status;
        tw::common_strat::ConsumerProxy::instance().onQuote(quote);        
    }
    
    tw::price::Quote::eStatus _status;
    const OnixS::CME::MarketData::ChannelId& _channelId;
};
   

ChannelPfOnix::ChannelPfOnix() : _settings(),
                                 _handlerSecurityDefinitions(NULL),
                                 _handlers(),
                                 _handlersHighResolutionTime() {
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
        
        return doInit(settingsCme);
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

ChannelPfOnix::THandlerPtr ChannelPfOnix::addHandler(OnixS::CME::MarketData::ChannelId channel_id) {
    LOGGER_INFO << "creating handler for channelId: " << channel_id << "\n";
    _settings._settings.channelId = channel_id;
    THandlerPtr handler = THandlerPtr(new THandler(_settings._settings));
    registerWithHandler(handler, _settings._topOfBookOnly);
    int channelIdAsInt = channelIdToInt(channel_id);
    _handlers[channelIdAsInt] = handler;
    _handlersHighResolutionTime[channelIdAsInt] = tw::common::THighResTime::now();    
    return handler;
}

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
                addHandler(_settings._channelIds[count]);
            }        
        }

        // QuoteStore initialization
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

bool ChannelPfOnix::start() {
    bool status = true;
    try {
        LOGGER_INFO_EMPHASIS << "Starting channel price feed CME (onix)..." << "\n";
        
        if ( _handlers.empty() ) {
            LOGGER_ERRO << "No handlers for channel price feed CME (onix)" << "\n";
            return false;
        }
        
        THandlers::iterator iter = _handlers.begin();
        THandlers::iterator end = _handlers.end();
        for ( ; iter != end; ++ iter ) {
            if ( !_settings._replay ) {
                LOGGER_INFO_EMPHASIS << "Starting channel handler: " << iter->first << "..." << "\n";
                iter->second->start();
                LOGGER_INFO_EMPHASIS << "Started channel handler: " << iter->first << "\n";
            } else {
                LOGGER_INFO_EMPHASIS << "Starting channel handler(REPLAY _MODE): " << iter->first << "..." << "\n";
                char buffer[4] = { '\0', '\0', '\0', '\0' };
                fast_itoa10(iter->first, buffer);
                std::string channelId(buffer);
                OnixS::CME::MarketData::ReplayOptions replayOptions(channelId, _settings._replayFolder);
                iter->second->start(replayOptions);
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
                iter->second->stop();
                LOGGER_INFO_EMPHASIS << "Stopped channel handler: " << iter->first << "\n";
            }catch(const std::exception& e) {
                LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
            } catch(...) {
                LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
            }
        }
        
        _handlerSecurityDefinitions = NULL;
        _handlers.clear();
        _handlersHighResolutionTime.clear();
        
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
        int channelIdAsInt = channelIdToInt(instrument->_var1);
        THandlers::iterator iter = _handlers.find(channelIdAsInt);
        if ( iter == _handlers.end() ) {
            LOGGER_INFO << "Can't find handler for channelId: " << channelIdAsInt << "\n";
            handler = this->addHandler(instrument->_var1);
        } else {
            handler = iter->second;
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

// Registering all interfaces' callbacks
//
void ChannelPfOnix::registerWithHandler(THandlerPtr handler, bool topOfBookOnly) {    
    if ( _settings._instrumentLoading ) {
        handler->registerSecurityDefinitionListener(this);
        LOGGER_WARN << "Channel is used only for instrument loading security definitions - WILL NOT listen to quotes!"<< "\n";
        return;
    }
    
    OnixS::CME::MarketData::BestBidAskTrackingOptions bbaTrackingOptions;
    
    if ( topOfBookOnly ) {
        bbaTrackingOptions.parameters = OnixS::CME::MarketData::BestBidAskParameters::Price | OnixS::CME::MarketData::BestBidAskParameters::Quantity;        
    }
    
    handler->registerDirectBookUpdateListener(this, bbaTrackingOptions);
    
    handler->registerTradeListener(this);
    handler->registerNewsListener(this);
    handler->registerSecurityStatusListener(this);
    handler->registerStatisticsListener(this);
    handler->registerMessageProcessingListener(this);
    handler->registerStateChangeListener(this);
    handler->registerErrorListener(this);
}

// OnixS::CME::MarketData::SecurityDefinitionListener interface
//
void ChannelPfOnix::onSecurityDefinition(const OnixS::CME::MarketData::SecurityDefinition& securityDefinition,
                                         const OnixS::CME::MarketData::ChannelId& channelId) {
    try {
        if ( !_settings._instrumentLoading ) {        
            LOGGER_ERRO << "SHOULDN'T be called -- '_instrumentLoading'= false" << "\n";
            return;
        }

        if ( _handlerSecurityDefinitions )
            _handlerSecurityDefinitions->onSecurityDefinition(securityDefinition, channelId);

        if ( _settings._verbose ) {
            LOGGER_INFO << "SECURITY_DEFINITION: __________________________" << "\n";
            LOGGER_INFO << "FROM: \n" << channelId << " :: " << securityDefinition.toString() << "\n";
        } 
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void ChannelPfOnix::onSecurityDefinitionDeleted(OnixS::CME::MarketData::SecurityId securityId,
                                                const OnixS::CME::MarketData::ChannelId& channelId) {
    try {
        if ( !_settings._instrumentLoading ) {        
            LOGGER_ERRO << "SHOULDN'T be called -- '_instrumentLoading'= false" << "\n";
            return;
        }
        
        if ( _handlerSecurityDefinitions )
            _handlerSecurityDefinitions->onSecurityDefinitionDeleted(securityId, channelId);

        LOGGER_WARN << "SecurityDefinitionDeleted started for: " << channelId << " :: " << securityId << "\n";    
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

        LOGGER_INFO_EMPHASIS << "SecurityDefinitionsRecoveryStarted started for: " << channelId << "\n";
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

        LOGGER_INFO_EMPHASIS << "SecurityDefinitionsRecoveryStarted finished for: " << channelId << "\n";   
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void ChannelPfOnix::checkExchangeSlow(const OnixS::CME::MarketData::ChannelId& channelId,
                                      tw::price::Quote& quote,
                                      uint64_t maxDelta,
                                      const std::string& source) {
    int channelIdAsInt = channelIdToInt(channelId);
    THandlersHighResolutionTime::iterator iter = _handlersHighResolutionTime.find(channelIdAsInt);
    if ( iter != _handlersHighResolutionTime.end() ) {
        quote._timestamp1 = iter->second;                        
        uint64_t delta = quote._timestamp1-quote._exTimestamp;
        if ( maxDelta < delta ) {
            if ( delta > MAX_DELAY_IN_MICROSEC_GAP ) {
                // Long delay over 1 minute is indicative of snapshot reception
                // on start up? TODO: investigate
                //
                quote._exTimestamp.setToNow();
                quote._timestamp1.setToNow();
            } else {
                quote._status = tw::price::Quote::kExchangeSlow;
            }
        }
    }
}
    
// OnixS::CME::MarketData::DirectBookUpdateListener interface
//
void ChannelPfOnix::onDirectBooksOutOfDate(const OnixS::CME::MarketData::ChannelId& channelId) {
    tw::price::QuoteStore::instance().for_each_quote(QuoteChannelFunctor(tw::price::Quote::kStaleQuote, channelId));
}

// TODO: fast to implement, but not the most efficient way to parse books
// need to revisit later
//
void ChannelPfOnix::onDirectBookUpdated(const OnixS::CME::MarketData::Book& book,
                                        const OnixS::CME::MarketData::ChannelId& channelId) {
    tw::price::QuoteStore::TQuote& quote = tw::price::QuoteStore::instance().getQuoteByKeyNum1(book.securityId());
    if ( !quote.isValid() || !quote.isSubscribed() )
        return;
    
    quote.clearRuntime();
    
    uint32_t numOfLevels = std::min(book.depth(), static_cast<size_t>(tw::price::QuoteStore::TQuote::SIZE));
    for ( uint32_t count = 0; count < numOfLevels; ++count ) {
        // TODO: make sure that OnixS just propagates number of ticks (even though in double format)
        //
        {
            // Set bid
            //
            const OnixS::CME::MarketData::PriceLevel& priceLevelFrom = book.bids()[count];
            tw::price::PriceSizeNumOfOrders& priceLevelTo = quote._book[count]._bid;        
            
            tw::price::Ticks ticks = quote._convertFrom(priceLevelFrom.price());
            if ( ticks != priceLevelTo._price ) {
                if ( 0 == count ) 
                    checkGappedQuote(ticks, priceLevelTo._price, quote, MAX_GAP_FROM_PREVIOUS);
                
                quote.setBidPrice(ticks, count);
            }
            
            tw::price::Size size(static_cast<tw::price::Size::type>(priceLevelFrom.quantity()));
            if ( size != priceLevelTo._size )
                quote.setBidSize(size, count);
            
            priceLevelTo._numOrders = priceLevelFrom.numberOfOrders();
        }
        
        {
            // Set ask
            //
            const OnixS::CME::MarketData::PriceLevel& priceLevelFrom = book.asks()[count];
            tw::price::PriceSizeNumOfOrders& priceLevelTo = quote._book[count]._ask;
            
            tw::price::Ticks ticks = quote._convertFrom(priceLevelFrom.price());
            if ( ticks != priceLevelTo._price ) {
                if ( 0 == count ) 
                    checkGappedQuote(ticks, priceLevelTo._price, quote, MAX_GAP_FROM_PREVIOUS);
                
                quote.setAskPrice(ticks, count);
            }
            
            tw::price::Size size(static_cast<tw::price::Size::type>(priceLevelFrom.quantity()));
            if ( size != priceLevelTo._size )
                quote.setAskSize(size, count);
            
            priceLevelTo._numOrders = priceLevelFrom.numberOfOrders();
        }        
    }
    
    if ( quote.isChanged() ) {
        quote._seqNum = book.lastAppliedMessage().getSeqNum();
        quote._exTimestamp = tw::common::THighResTime::parseCMETime(book.lastAppliedMessage().getSendingTime());
        checkExchangeSlow(channelId, quote, MAX_DELAY_IN_MICROSEC, "BookUpdate");
                
        tw::common_strat::ConsumerProxy::instance().onQuote(quote);
        
        if ( _settings._verbose ) {
            LOGGER_INFO << "BOOKS: __________________________" << "\n";
            LOGGER_INFO << "FROM: \n" << channelId << " :: " << book.toString() << "\n";
            LOGGER_INFO << "TO: \n" << quote.toString() << "\n";
        }
    }
}
    
// OnixS::CME::MarketData::TradeListener interface
//
void ChannelPfOnix::onTrade(const OnixS::CME::MarketData::Trade& trade,
                            const OnixS::CME::MarketData::ChannelId& channelId) {
    tw::price::QuoteStore::TQuote& quote = tw::price::QuoteStore::instance().getQuoteByKeyNum1(trade.securityId());
    if ( !quote.isValid() || !quote.isSubscribed() )
        return;
    
    quote.clearRuntime();
    
    switch ( trade.getTradeCondition() ) {
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
    
    switch ( trade.getAction() ) {
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
    
    switch ( trade.getTickDirection() ) {
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
    
    switch ( trade.getAggressorSide() ) {
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
    
    
    tw::price::Ticks ticks = quote._convertFrom(trade.getPrice());
    checkGappedTrade(ticks, quote, MAX_GAP_FROM_PREVIOUS);
    
    quote._trade._price = ticks;
    quote._trade._size.fromDouble(trade.getQuantity());
    quote.setTrade();
    
    quote._exTimestamp = tw::common::THighResTime::parseCMETime(trade.underlyingMessageGroupEntry().getMessage().getSendingTime());
    checkExchangeSlow(channelId, quote, MAX_DELAY_IN_MICROSEC, "Trade");
    
    tw::common_strat::ConsumerProxy::instance().onQuote(quote);
    if ( _settings._verbose ) {
        LOGGER_INFO << "TRADES: __________________________" << "\n";
        LOGGER_INFO << "FROM: \n" << channelId << " :: " << trade.toString() << "\n";
        LOGGER_INFO << "TO: \n" << quote.toString() << "\n";
    }
}

// OnixS::CME::MarketData::NewsListener interface
//
void ChannelPfOnix::onNews(const OnixS::CME::MarketData::News& news,
                           const OnixS::CME::MarketData::ChannelId& channelId) {
    // TODO: need to implement news' subscriber
    //
    if ( _settings._verbose ) {
        LOGGER_INFO << "NEWS: __________________________" << "\n";
        LOGGER_INFO << "FROM: \n" << channelId << " :: " << news.toString() << "\n";
    }    
}

// OnixS::CME::MarketData::SecurityStatusListener interface
//
void ChannelPfOnix::onSecurityStatus(const OnixS::CME::MarketData::SecurityStatus& status,
                                     const OnixS::CME::MarketData::ChannelId& channelId) {    
    OnixS::CME::MarketData::SecurityId securityId;
    if ( !status.getSecurityId(securityId) ) {
        if ( _settings._verbose ) {
            LOGGER_INFO << "SECURITY_STATUS: __________________________" << "\n";
            LOGGER_INFO << "FROM: \n" << channelId << " :: " << status.toString() << "\n";
        }        
        return;
    }
    
    tw::price::QuoteStore::TQuote& quote = tw::price::QuoteStore::instance().getQuoteByKeyNum1(securityId);
    if ( !quote.isValid() || !quote.isSubscribed() )
        return;
    
    quote.clearRuntime();
    
    switch ( status.getTradingEvent() ) {
        case OnixS::CME::MarketData::SecurityTradingEvents::TradingHaltOrStopSkipe:
            quote._status = tw::price::Quote::kTradingHaltedOrStopped;
            break;
        case OnixS::CME::MarketData::SecurityTradingEvents::ResumeOrOpen:
            quote._status = tw::price::Quote::kTradingResumeOrOpen;
            break;
        case OnixS::CME::MarketData::SecurityTradingEvents::EndOfTradingSession:
            quote._status = tw::price::Quote::kTradingSessionEnd;
            break;
        case OnixS::CME::MarketData::SecurityTradingEvents::PauseInTrading:
            quote._status = tw::price::Quote::kTradingPause;
            break;
        default:
            break;
    }
    
    if ( quote.isError() ) {
        tw::common_strat::ConsumerProxy::instance().onQuote(quote);
        if ( _settings._verbose ) {
            LOGGER_INFO << "SECURITY_STATUS: __________________________" << "\n";
            LOGGER_INFO << "FROM: \n" << channelId << " :: " << status.toString() << "\n";
            LOGGER_INFO << "TO: \n" << quote.toString() << "\n";
        }
    } 
}

// OnixS::CME::MarketData::StatisticsListener interface
//
void ChannelPfOnix::onStatisticsReset(OnixS::CME::MarketData::SecurityId securityId,
                                      const OnixS::CME::MarketData::ChannelId& channelId) {
    tw::price::QuoteStore::TQuote& quote = tw::price::QuoteStore::instance().getQuoteByKeyNum1(securityId);
    if ( !quote.isValid() || !quote.isSubscribed() )
        return;
    
    quote._status = tw::price::Quote::kHLOReset;
    tw::common_strat::ConsumerProxy::instance().onQuote(quote);
    if ( _settings._verbose ) {
        LOGGER_INFO << "STATISTICS_RESET: __________________________" << "\n";
        LOGGER_INFO << "FROM: \n" << channelId << " :: " << "\n";
    }    
}

void ChannelPfOnix::onStatisticsReset(const OnixS::CME::MarketData::Symbol& symbol,
                                      const OnixS::CME::MarketData::ChannelId& channelId) {
    tw::instr::InstrumentPtr instrument = tw::instr::InstrumentManager::instance().getByDisplayName(symbol);
    if ( instrument == NULL ) {
        if ( _settings._verbose ) {
            LOGGER_INFO << "STATISTICS_RESET: __________________________" << "\n";
            LOGGER_INFO << "FROM: \n" << channelId << " :: " << "\n";
        }
        
        return;
    }
    
    onStatisticsReset(instrument->_keyNum1, channelId);
}

void ChannelPfOnix::onStatistics(const OnixS::CME::MarketData::Statistics& statistics,
                                 const OnixS::CME::MarketData::ChannelId& channelId) {
    tw::price::QuoteStore::TQuote& quote = tw::price::QuoteStore::instance().getQuoteByKeyNum1(statistics.securityId());
    if ( !quote.isValid() || !quote.isSubscribed() )
        return;    
    
    quote.clearRuntime();
    switch ( statistics.getType() ) {
        case OnixS::CME::MarketData::StatisticsTypes::OpeningPrice:
            quote.setOpen(quote._convertFrom(statistics.getValue()));
            break;
        case OnixS::CME::MarketData::StatisticsTypes::TradingSessionHighPrice:
            quote.setHigh(quote._convertFrom(statistics.getValue()));
            break;
        case OnixS::CME::MarketData::StatisticsTypes::TradingSessionLowPrice:
            quote.setLow(quote._convertFrom(statistics.getValue()));
            break;
        default:
            break;
    }
    
    if ( quote.isChanged() ) {
        tw::common_strat::ConsumerProxy::instance().onQuote(quote);
        if ( _settings._verbose ) {
            LOGGER_INFO << "STATISTICS: __________________________" << "\n";
            LOGGER_INFO << "FROM: \n" << channelId << " :: " << statistics.toString() << "\n";
            LOGGER_INFO << "TO: \n" << quote.toString() << "\n";
        }
    }
}

// OnixS::CME::MarketData::MessageProcessingListener interface
//
void ChannelPfOnix::onProcessingBegin(const OnixS::CME::MarketData::Message& message,
                                      const OnixS::CME::MarketData::ChannelId& channelId) {
    int channelIdAsInt = channelIdToInt(channelId);
    THandlersHighResolutionTime::iterator iter = _handlersHighResolutionTime.find(channelIdAsInt);
    if ( iter != _handlersHighResolutionTime.end() )
        iter->second = message.getReceivingTime();
}

void ChannelPfOnix::onProcessingEnd(const OnixS::CME::MarketData::Message &message,
                                    const OnixS::CME::MarketData::ChannelId& channelId) {
    // Nothing to do for now
    //
}

// OnixS::CME::MarketData::HandlerStateChangeListener interface
//
void ChannelPfOnix::onHandlerStateChange(const OnixS::CME::MarketData::HandlerStateChange& change,
                                         const OnixS::CME::MarketData::ChannelId& channelId) {
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
        tw::price::QuoteStore::instance().for_each_quote(QuoteChannelFunctor(status, channelId));
        if ( _settings._verbose ) {
            LOGGER_INFO << "HANDLER_STATE_CHANGE: __________________________" << "\n";
            LOGGER_INFO << "FROM: \n" << channelId << " :: " << change.toString() << "\n";
            LOGGER_INFO << "TO: \n" << tw::price::Quote::statusToString(status) << "\n";
        }
    } else if ( _settings._verbose ) {
        LOGGER_INFO << "HANDLER_STATE_CHANGE: __________________________" << "\n";
        LOGGER_INFO << "FROM: \n" << channelId << " :: " << change.toString() << "\n";
    }
}

// OnixS::CME::MarketData::ReplayListener interface
//
void ChannelPfOnix::onReplayError (const OnixS::CME::MarketData::ChannelId& channelId,
                                   const std::string& errorDescription) {
    tw::price::QuoteStore::instance().for_each_quote(QuoteChannelFunctor(tw::price::Quote::kReplayError, channelId));
    
    if ( _settings._verbose ) {
        LOGGER_INFO << "REPLAY_ERROR: __________________________" << "\n";
        LOGGER_INFO << "FROM: \n" << channelId << " :: " << errorDescription << "\n";            
    }
}

void ChannelPfOnix::onReplayFinished (const OnixS::CME::MarketData::ChannelId& channelId) {
    tw::price::QuoteStore::instance().for_each_quote(QuoteChannelFunctor(tw::price::Quote::kReplayFinished, channelId));
    if ( _settings._verbose ) {
        LOGGER_INFO << "REPLAY_FINISHED: __________________________" << "\n";
        LOGGER_INFO << "FROM: \n" << channelId << " :: " << "\n";
    }
}

// OnixS::CME::MarketData::ErrorListener
//
void ChannelPfOnix::onError(const OnixS::CME::MarketData::Error& error,
                            const OnixS::CME::MarketData::ChannelId& channelId) {    
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
    
    if ( tw::price::Quote::kSuccess != status ) {
        tw::price::QuoteStore::instance().for_each_quote(QuoteChannelFunctor(status, channelId));
        LOGGER_INFO << "ERROR: __________________________" << "\n";
        LOGGER_INFO << "FROM: \n" << channelId << " :: " << error.toString() << "\n";
        LOGGER_INFO << "TO: \n" << tw::price::Quote::statusToString(status) << "\n";
    }    
}
    
} // channel_pf_cme
} // tw

