#include "publisher.h"

#include "tw/common_thread/utils.h"
#include "tw/generated/bars.h"
#include "tw/common_trade/bars_manager.h"
#include <tw/price/ticks_converter.h>

#include <iomanip>
#include <fstream>

static const uint32_t TIMESTAMP_LENGTH = 26;
static const uint32_t HEADER_LENGTH = 26;
static const uint32_t BODY_LENGTH = sizeof(tw::price::QuoteWire);
static const uint32_t FOOTER_LENGTH = sizeof(char);
static const uint32_t DATA_LENGTH = HEADER_LENGTH+BODY_LENGTH+FOOTER_LENGTH;
static const uint32_t MESSAGE_LENGTH = TIMESTAMP_LENGTH+HEADER_LENGTH+BODY_LENGTH+FOOTER_LENGTH;
static const uint32_t USECS_IN_MINUTE = 60 * 1000 * 1000;

static const std::string NULL_TIMESTAMP = "00000000";
static const std::string COMMA_DELIM = ",";

typedef tw::common_trade::HLOCInfo TBarInfo;
typedef tw::common_trade::Bar TBar;
typedef std::map<tw::instr::Instrument::TKeyId, TBar> TBars;

typedef tw::common_trade::PriceTradesExtendedInfo TPriceTradesInfo;
typedef std::map<tw::instr::Instrument::TKeyId, TPriceTradesInfo> TPriceTradesInfos;

typedef tw::common_trade::PriceTradesInfo TPriceTradesBasicInfo;
typedef std::deque<TPriceTradesBasicInfo> TRangePriceTradesInfos;
typedef std::map<tw::instr::Instrument::TKeyId, TRangePriceTradesInfos> TInstrumentRangePriceTradesInfos;

typedef tw::common_trade::RangeBreakoutTradeInfo TRangeBreakoutTradeInfo;
typedef std::vector<TRangeBreakoutTradeInfo> TRangeBreakoutTradeInfos;
typedef std::map<tw::instr::Instrument::TKeyId, TRangeBreakoutTradeInfos> TInstrumentRangeBreakoutTradeInfo;

static bool sortPriceTradesInfo(const tw::common_trade::PriceTradesInfo& a, const tw::common_trade::PriceTradesInfo& b) {
    return (a._price > b._price);
}

static std::string priceTradesInfoToString(TPriceTradesInfo& info, const std::string& displayName) {
    tw::common_str_util::TFastStream s;
    s << "\nReplayVerifyWithTrades-TRADE levels"
      << "," << displayName
      << "," << info._priceBid
      << "," << info._volBid
      << "," << info._volAsk
      << "," << info._priceAsk
      << "," << info._timestamp;
    
    return s.str();
}

static void barTradesToString(TBar& bar) {
    std::vector<tw::common_trade::PriceTradesInfo> trades;
        
    for ( std::tr1::unordered_map<tw::price::Ticks::type, tw::common_trade::PriceTradesInfo>::const_iterator iter = bar._trades.begin(); iter != bar._trades.end(); ++iter )
        trades.push_back(iter->second);

    std::sort(trades.begin(), trades.end(), sortPriceTradesInfo);
    
    std::vector<tw::common_trade::PriceTradesInfo>::iterator iter = trades.begin();
    std::vector<tw::common_trade::PriceTradesInfo>::iterator end = trades.end();
    
    tw::common_str_util::FastStream<1024*4> s;
    for ( ; iter != end; ++iter ) { 
        s << "\nReplayVerifyWithTrades-BAR levels"
          << "," << bar._displayName
          << "," << iter->_price
          << "," << iter->_volBid
          << "," << iter->_volAsk
          << "," << bar._open_timestamp.toString();
    }
    
    bar._tradesLog = s.str();
}

static int64_t getSecsWithUsecs(const tw::common::THighResTime& t) {
    return static_cast<int64_t>(t.seconds()*1000000 + t.microseconds());
}

Publisher::Publisher() : _message(),
                         _connectionSubscriptions() {
    _quotesCount = 0;
    _params._name = "channel_pf_simple_publisher";
    _isDone = false;
}

Publisher::~Publisher() {
    stop();
}

void Publisher::clear() {
    _priceStream.clear();
    _message.str("");
    _connectionSubscriptions.clear();
    
    _quotesCount = 0;
    if ( _file.is_open() )
        _file.close();
    
    terminateThreadRecord();
    terminateThreadReplay();
}

void Publisher::waitForReplayToFinish() {
    if ( !_threadReplay )
        return;
    
    LOGGER_INFO << "Started to wait for _threadReplay to finish..." << "\n";    
    
    try {
        _threadReplay->join();
        _threadReplay.reset();
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    LOGGER_WARN << "_threadReplay finished" << "\n";
    return;
}

void Publisher::terminateThreadRecord() {
    if ( !_threadRecord )
        return;
    
    LOGGER_INFO << "Started to terminate threadRecord" << "\n";    
    
    try {
        // Wait for db thread to finish
        //
        _isDone = true;
        _threadPipe.stop();
        uint32_t secsToWait = 2;
        if ( !_threadRecord->timed_join(boost::posix_time::seconds(secsToWait)) ) {
            LOGGER_WARN << "_threadRecord didn't finish in " << secsToWait << " secs" << "\n";
            _threadRecord->interrupt();
            if ( !_threadRecord->timed_join(boost::posix_time::seconds(secsToWait)) ) {
                LOGGER_WARN << "_threadRecord didn't finish in another " << secsToWait << " secs" << "\n";
                if ( _file.is_open() )
                    _file.close();
                if ( !_threadRecord->timed_join(boost::posix_time::seconds(secsToWait)) ) {
                    LOGGER_WARN << "_threadRecord didn't finish in another " << secsToWait << " secs after closing db resources" << "\n";
                }
            }
        }

        _threadRecord.reset();
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    LOGGER_INFO << "Finished terminating threadRecord" << "\n";    
    return;
}

void Publisher::terminateThreadReplay() {
    if ( !_threadReplay )
        return;
    
    LOGGER_INFO << "Started to terminate threadReplay" << "\n";    
    
    try {
        // Wait for db thread to finish
        //
        _isDone = true;
        uint32_t secsToWait = 2;
        if ( !_threadReplay->timed_join(boost::posix_time::seconds(secsToWait)) ) {
            LOGGER_WARN << "_threadReplay didn't finish in " << secsToWait << " secs" << "\n";
            _threadReplay->interrupt();
            if ( !_threadReplay->timed_join(boost::posix_time::seconds(secsToWait)) ) {
                LOGGER_WARN << "_threadReplay didn't finish in another " << secsToWait << " secs" << "\n";
                if ( _file.is_open() )
                    _file.close();
                if ( !_threadReplay->timed_join(boost::posix_time::seconds(secsToWait)) ) {
                    LOGGER_WARN << "_threadReplay didn't finish in another " << secsToWait << " secs after closing db resources" << "\n";
                }
            }
        }

        _threadReplay.reset();
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    LOGGER_INFO << "Finished terminating threadReplay" << "\n";    
    return;
}

bool Publisher::init(const tw::common::Settings& settings, const tw::risk::Strategy& strategyParams) {
    try {
        if ( !IStrategy::init(settings, strategyParams) )
            return false;
        
        if (!_settingsCme.parse(settings._channel_pf_cme_dataSource)) {
            LOGGER_ERRO << "failed to parse config file settings: " << settings._channel_pf_cme_dataSource << " :: " << _settingsCme.toStringDescription() << "\n";
            return false;
        }
        
        if ( _settingsCme._symbols.empty() ) {
            LOGGER_ERRO << "No symbols' specified to filter" << "\n";
            return false;
        }
        
        switch ( settings._publisher_pf_mode ) {
            case tw::common::ePublisherPfMode::kRealtime:
            case tw::common::ePublisherPfMode::kRecord:
            {
                if ( tw::common::ePublisherPfMode::kRecord == settings._publisher_pf_mode ) {
                    std::string filename = settings._publisher_pf_log_dir + "/" + "publisher_pf_" + tw::common::THighResTime::dateISOString() + ".log";
                    _file.open(filename.c_str(), std::fstream::out | std::fstream::app);
                    if ( !_file.good() ) {
                        LOGGER_ERRO << "Can't open for writing file: " << filename << "\n";
                        return false;
                    }
                }

                for ( uint32_t count = 0; count < _settingsCme._symbols.size(); ++count ) {
                    addQuoteSubscription(_settingsCme._symbols[count]);
                }
            }
                break;
            case tw::common::ePublisherPfMode::kReplay:
            case tw::common::ePublisherPfMode::kReplayVerify:
            case tw::common::ePublisherPfMode::kReplayVerifyWithTrades:
            {
                // Open file with historical data
                //
                _file.open(settings._publisher_pf_replay_filename.c_str(), std::fstream::in);
                if ( !_file.good() ) {
                    LOGGER_ERRO << "Can't open for reading file: " << settings._publisher_pf_replay_filename << "\n";
                    return false;
                }
            }
                break;
            default:
                break;
        }
        
        _settings = settings;
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }

    return true;
}  

bool Publisher::start() {
    try {
        _isDone = false;
        if ( tw::common::ePublisherPfMode::kReplay == _settings._publisher_pf_mode || tw::common::ePublisherPfMode::kReplayVerify == _settings._publisher_pf_mode || tw::common::ePublisherPfMode::kReplayVerifyWithTrades == _settings._publisher_pf_mode ) {
            // Create thread for reading the file and processing quotes
            //            
            _threadReplay = TThreadPtr(new TThread(boost::bind(&Publisher::threadMainReplay, this)));
        } else if ( tw::common::ePublisherPfMode::kRecord == _settings._publisher_pf_mode ) {
            // Create thread for reading the file and processing quotes
            //            
            _threadRecord = TThreadPtr(new TThread(boost::bind(&Publisher::threadMainRecord, this)));
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    return true;
}

bool Publisher::stop() {
    try {
        clear();
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    
    return true;
}

void Publisher::onConnectionUp(tw::common_strat::TMsgBusConnection::native_type id, tw::common_strat::TMsgBusConnection::pointer connection) {
    // Not implemented - nothing to do
    //
}
    
void Publisher::onConnectionDown(tw::common_strat::TMsgBusConnection::native_type id) {
    try {
        TConnectionSubscriptions::iterator iter = _connectionSubscriptions.find(id);
        if ( iter != _connectionSubscriptions.end() ) {
            _connectionSubscriptions.erase(iter);
            LOGGER_INFO << "Erased subscriber: " << id << "\n";            
        } else {
            LOGGER_ERRO << "Can find subscriber: " << id << "\n";
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void Publisher::onConnectionData(tw::common_strat::TMsgBusConnection::native_type id, const tw::common::Command& cmnd) {
    try {
        LOGGER_INFO << id << " :: " << cmnd.toString() << "\n";
        
        if ( tw::common::eCommandType::kStrat == cmnd._type ) {
            tw::instr::eExchange exchange;
            
            if ( !cmnd.get("exchange", exchange) || !exchange.isValid() ) {
                LOGGER_ERRO << "Unsupported exchange in cmnd:" << cmnd.toString() << "\n";
                return;
            }
            
            if ( tw::common::eCommandSubType::kList == cmnd._subType ) {
                tw::instr::InstrumentManager::TInstruments instruments;
                instruments = getByExchange(exchange);

                if ( instruments.empty() ) {
                    LOGGER_ERRO << "No instruments for exchange: " << exchange << "\n";
                    return;
                }

                for (size_t count = 0; count < instruments.size(); ++count ) {
                    _message.str("");
                    _message <<  "R," 
                             << count+1 << "," 
                             << instruments.size() << "," 
                             << instruments[count]->_displayName << ","
                             << instruments[count]->_tickNumerator << ","
                             << instruments[count]->_tickDenominator
                             << "\n";

                    tw::common_strat::StrategyContainer::instance().sendToMsgBusConnection(id, _message.str());
                }
            } else if ( tw::common::eCommandSubType::kSub == cmnd._subType || tw::common::eCommandSubType::kUnsub == cmnd._subType ) {
                std::string displayName;
                if ( !cmnd.get("displayName", displayName) ) {
                    LOGGER_ERRO << "No 'displayName' in cmnd: " << cmnd.toString() << "\n";
                    return;
                }
                
                tw::instr::InstrumentConstPtr instrument = tw::instr::InstrumentManager::instance().getByDisplayName(displayName);
                if ( instrument == NULL ) {
                    LOGGER_ERRO << "Can't find instrument for: " << displayName << cmnd.toString() << "\n";
                    return;
                }

                if ( tw::common::eCommandSubType::kSub == cmnd._subType ) {
                    _connectionSubscriptions[id].insert(instrument->_keyId);
                    serializeQuote(tw::price::QuoteStore::instance().getQuote(instrument));

                    tw::common_strat::StrategyContainer::instance().sendToMsgBusConnection(id, _message.str());
                } else {
                    _connectionSubscriptions[id].erase(instrument->_keyId);
                }
            } else {
                LOGGER_ERRO << "Unsupported cmnd : " << cmnd.toString() << "\n";
            }
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}
    
void Publisher::onQuote(const tw::price::Quote& quote) {    
    try {
        if ( !quote.isNormalTrade() && !quote.isLevelUpdate(0) )
            return;
        if ( quote.isTrade() && !quote.isNormalTrade() )
            return;
        if ( !quote._book[0].isValid() )
            return; 
                    
        switch (quote._status) {
            case tw::price::Quote::kSuccess:
            case tw::price::Quote::kExchangeSlow:
                break;
            case tw::price::Quote::kExchangeDown:
            case tw::price::Quote::kDropRate:
            case tw::price::Quote::kTradingHaltedOrStopped:
            case tw::price::Quote::kTradingPause:
            case tw::price::Quote::kConnectionHandlerError:
            case tw::price::Quote::kPriceGap:
                // Critical error quotes - going into 'NoOrders' mode
                LOGGER_WARN << "ErrorQuote w/status: " << tw::price::Quote::statusToString(quote._status) << "\n";
                return;
            default:
                LOGGER_WARN << "\n\t" << "!= kSuccess && !=kExchangeSlow; status = " << tw::price::Quote::statusToString(quote._status) << " and " << quote._instrument->_displayName << " @ " << quote._timestamp1.toString() << "\n";
                return;
        }
        
        if ( tw::common::ePublisherPfMode::kRecord == _settings._publisher_pf_mode ) {
            TQuoteDataItemPtr item = getItem();
            item->set(quote);
            _threadPipe.push(item);
            
            return;
        }
        
        TConnectionSubscriptions::iterator iter = _connectionSubscriptions.begin();
        TConnectionSubscriptions::iterator end = _connectionSubscriptions.end();
        
        _message.str("");
        for ( ; iter != end; ++iter ) {
            TSubscriptions& subscriptions = iter->second;
            if ( subscriptions.find(quote.getInstrument()->_keyId) != subscriptions.end() ) {
                if ( _message.str().empty() )
                    serializeQuote(quote);
                
                tw::common_strat::StrategyContainer::instance().sendToMsgBusConnection(iter->first, _message.str());
            }
        }        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void Publisher::serializeQuote(const tw::price::Quote& quote) {
    static uint64_t counter = 0;
    const tw::price::TicksConverter::TConverterTo& c = quote.getInstrument()->_tc->getConverterTo();
    
    _priceStream.clear();
    _priceStream << c(quote._open) << "," << c(quote._high) << "," << c(quote._low);
    
    _message.str("");
    _message << "Q,"
            << quote.getInstrument()->_displayName << ","
            << quote._seqNum << ","
            << quote._timestamp1.toString() << " :: "
            << ++counter << " - "
            << std::setw(2) << static_cast<double>(quote._timestamp1-quote._exTimestamp)/1000 << "ms :: "
            << std::setw(2) << quote._timestamp2 - quote._timestamp1 << " :: "
            << std::setw(2) << quote._timestamp3 - quote._timestamp2 << ","
            << quote.statusToString(quote._status) << ","
            << _priceStream.c_str() << ","
            << quote._flag << ","
            << (quote.isNormalTrade() ? "T" : "Q") << ","
            << quote._trade.toShortString(c) << ",";
            
    for ( uint32_t count = 0; count < tw::price::Quote::SIZE; ++count ) {
        _message << quote._book[count]._bid.toShortString(false, c) << ",";
    }
    
    for ( uint32_t count = 0; count < tw::price::Quote::SIZE; ++count ) {
        _message << quote._book[count]._ask.toShortString(false, c);
        if ( count != tw::price::Quote::SIZE -1 )
            _message << ",";
        else
            _message << "\n";            
    }   
}

void Publisher::replay() {
    try {
        std::string buffer;
        buffer.reserve(MESSAGE_LENGTH);
        
        typedef tw::common_trade::HLOCInfo TBarInfo;
        typedef std::map<tw::instr::Instrument::TKeyId, TBarInfo> TBars;
        TBars bars;
    
        _file.seekg(0);
        
        tw::common::THighResTime currHistoricalTimestamp;
        tw::common::THighResTime prevHistoricalTimestamp;
        tw::common::THighResTime firstHistoricalTimestamp;
        
        tw::common::THighResTime currPriceFeedTimestamp;
        tw::common::THighResTime firstPriceFeedTimestamp;
        
        tw::common::THighResTime startTimestamp;
        tw::common::THighResTime endTimestamp;
        
        bool setTimeWindow = (!_settings._publisher_pf_start_time.empty() || !_settings._publisher_pf_end_time.empty());
        
        uint32_t count = 0;
        int64_t deltaUsecs = 0;
        int64_t firstDeltaUsecs = 0;
        int64_t deltaUsecsToSleep = 0;
        int64_t adjUsecs = 0;
        
        while ( !_isDone && !_file.eof() ) {
            buffer.clear();
            buffer.resize(MESSAGE_LENGTH);
            buffer[TIMESTAMP_LENGTH-2] = ':';
            buffer[TIMESTAMP_LENGTH-1] = ' ';

            _file.read(&buffer[TIMESTAMP_LENGTH], MESSAGE_LENGTH-TIMESTAMP_LENGTH);
            if ( _file.eof() ) {
                LOGGER_INFO << "REPLAYED ENTIRE FILE - EXITING.." << "\n";
                break;
            }
            
            if ( NULL_TIMESTAMP != buffer.substr(TIMESTAMP_LENGTH, NULL_TIMESTAMP.length()) ) {
                currHistoricalTimestamp = tw::common::THighResTime::parse(buffer.substr(TIMESTAMP_LENGTH, 24));
                if ( 1 == ++count ) {
                    if ( setTimeWindow ) {
                        if ( !_settings._publisher_pf_start_time.empty() ) {
                            startTimestamp = tw::common::THighResTime::parse(buffer.substr(TIMESTAMP_LENGTH, 8) + "-" + _settings._publisher_pf_start_time + ".000000");
                            LOGGER_INFO << "Will skip until startTimestamp=" << startTimestamp.toString() << "\n";
                        }
                        
                        if ( !_settings._publisher_pf_end_time.empty() ) {
                            endTimestamp = tw::common::THighResTime::parse(buffer.substr(TIMESTAMP_LENGTH, 8) + "-" + _settings._publisher_pf_end_time + ".000000");
                            LOGGER_INFO << "Will process till endTimestamp=" << endTimestamp.toString() << "\n";
                        }
                        
                        setTimeWindow = false;
                    }
                    
                    if ( !_settings._publisher_pf_start_time.empty() && currHistoricalTimestamp < startTimestamp  ) {
                        --count;
                        continue;
                    }
                        
                    firstHistoricalTimestamp = prevHistoricalTimestamp = currHistoricalTimestamp;
                    currPriceFeedTimestamp.setToNow();
                    
                    // Adjust replay time to historical time by starting on the same microseconds boundary
                    // NOTE: replay might be delayed up to 1 minute to realign microseconds boundaries
                    //
                    uint32_t histUsecs = getSecsWithUsecs(currHistoricalTimestamp);
                    uint32_t currUsecs = getSecsWithUsecs(currPriceFeedTimestamp);
                    
                    int32_t deltaUsecs = histUsecs - currUsecs;
                    if ( deltaUsecs < 0 )
                        deltaUsecs += USECS_IN_MINUTE;
                    
                    LOGGER_INFO << "Realigning microseconds boundaries: h=" << currHistoricalTimestamp.toString()
                                << ",c=" << currPriceFeedTimestamp.toString()
                                << ",deltaUsecs=" << deltaUsecs
                                << ",sleepMode=" << _settings._publisher_pf_sleep_mode.toString()
                                << "\n";
                    
                    if ( tw::common::ePublisherPfSleepMode::kBusyWait == _settings._publisher_pf_sleep_mode )
                        tw::common_thread::sleepUSecsBusyWait(deltaUsecs);
                    else
                        tw::common_thread::sleepUSecs(deltaUsecs);
                    
                    firstPriceFeedTimestamp.setToNow();
                    currPriceFeedTimestamp = firstPriceFeedTimestamp;
                    currUsecs = getSecsWithUsecs(currPriceFeedTimestamp);
                    firstDeltaUsecs = deltaUsecs = histUsecs - currUsecs;
                    LOGGER_INFO << "Realigned microseconds boundaries: h=" << currHistoricalTimestamp.toString()
                                << ",c=" << currPriceFeedTimestamp.toString()
                                << ",deltaUsecs=" << deltaUsecs
                                << "\n";
                } else if ( !_settings._publisher_pf_end_time.empty() && endTimestamp < currHistoricalTimestamp  ) {
                    LOGGER_INFO << "REPLAYED FILE till endTimestamp=" << endTimestamp << " - EXITING.." << "\n";
                    break;
                }

                deltaUsecs = currHistoricalTimestamp - prevHistoricalTimestamp;
                currPriceFeedTimestamp.setToNow();
                deltaUsecsToSleep = 0;
                if ( deltaUsecs > 0 ) {
                    adjUsecs = (currPriceFeedTimestamp-firstPriceFeedTimestamp) - (currHistoricalTimestamp-firstHistoricalTimestamp) - firstDeltaUsecs;
                    if ( adjUsecs < 0 ) {
                        deltaUsecsToSleep = -adjUsecs;

                        if ( tw::common::ePublisherPfSleepMode::kBusyWait == _settings._publisher_pf_sleep_mode )
                            tw::common_thread::sleepUSecsBusyWait(deltaUsecsToSleep);
                        else
                            tw::common_thread::sleepUSecs(deltaUsecsToSleep);
                    }
                }

                tw::common::THighResTime::now().toString().copy(&buffer[0], TIMESTAMP_LENGTH);
                tw::common_strat::StrategyContainer::instance().sendToAllMsgBusConnections(buffer);
                
                tw::price::QuoteWire* quoteWire=reinterpret_cast<tw::price::QuoteWire*>(&buffer[TIMESTAMP_LENGTH+HEADER_LENGTH]);
                tw::price::QuoteStore::TQuote& quote = tw::price::QuoteStore::instance().getQuoteByKeyId(quoteWire->_instrumentId);
                if ( quote.isValid() )
                    static_cast<tw::price::QuoteWire&>(quote) = *quoteWire;
                
                if ( _settingsCme._verbose ) {
                    LOGGER_INFO << "TIMESTAMP: " << buffer.substr(0, 24)
                            << ",HEADER: " << buffer.substr(TIMESTAMP_LENGTH, 24)
                            << ",count=" << count
                            << ",d=" << deltaUsecs
                            << ",dS=" << deltaUsecsToSleep
                            << ",adj=" << adjUsecs
                            << ",pH=" << prevHistoricalTimestamp.toString()
                            << ",H=" << currHistoricalTimestamp.toString()
                            << ",T=" << currPriceFeedTimestamp.toString()
                            << "\n";
                    
                    if ( quote.isValid() )
                        LOGGER_INFO << "QUOTE: " << quote.toString() << "\n";
                }
                
                if ( quote.isValid() ) {
                    static_cast<tw::price::QuoteWire&>(quote) = *quoteWire;
                    TBarInfo& bar = bars[quoteWire->_instrumentId];
                    if ( 0 == bar._index ) {
                        ++bar._index;
                        bar._open_timestamp = currPriceFeedTimestamp;
                        bar._source = "replayVerify";
                        bar._displayName = static_cast<tw::price::Quote&>(quote)._instrument->_displayName;
                        bar._exchange = static_cast<tw::price::Quote&>(quote)._instrument->_exchange;
                        bar._duration = 60;
                    } else if ( 0 < (currPriceFeedTimestamp.getUnitsFromMidnight(tw::common::eHighResTimeUnits::kMins) - bar._open_timestamp.getUnitsFromMidnight(tw::common::eHighResTimeUnits::kMins)) ) {
                        if ( bar._open.isValid() ) {
                            bar._range = bar._high - bar._low;
                            if ( bar._close > bar._open )
                                bar._dir = tw::common_trade::ePatternDir::kUp;
                            else if ( bar._close < bar._open )
                                bar._dir = tw::common_trade::ePatternDir::kDown;
                            else
                                bar._dir = tw::common_trade::ePatternDir::kUnknown;
                        }
                        
                        LOGGER_INFO << "BAR FORMED: " << bar.toString() << "," << currPriceFeedTimestamp.toString() << "," << currHistoricalTimestamp.toString() << "\n";
                        ++bar._index;
                        bar._open_timestamp = currPriceFeedTimestamp;
                        bar._high = bar._low = bar._open = bar._close = tw::price::Ticks();
                        bar._volume.clear();
                        bar._numOfTrades = 0;
                    } 
                    
                    if ( quote.isNormalTrade() ) { 
                        if ( 0 == bar._numOfTrades ) {
                            bar._high = bar._low = bar._open = bar._close = quote._trade._price;
                        } else {
                            bar._close = quote._trade._price;
                            if ( quote._trade._price > bar._high )
                                bar._high = quote._trade._price;
                            else if ( quote._trade._price < bar._low )
                                bar._low = quote._trade._price;
                        }
                        bar._volume += quote._trade._size;
                        ++bar._numOfTrades;
                        
                        LOGGER_INFO << "T: " << buffer.substr(0, 24)
                                    << ",H: " << buffer.substr(TIMESTAMP_LENGTH, 24)
                                    << ",c=" << count
                                    << ",PFTs=" << currPriceFeedTimestamp.toString()
                                    << ",displayName=" << (quote.isValid() ? static_cast<tw::price::Quote&>(quote)._instrument->_displayName : "<Unknown>")
                                    << "," << (quote.isValid() ? quote.toShortString() : "")
                                    << "," << (quote.isValid() ? quote.toStringWhatChanged() : "")
                                    << "\n";
                    }
                }
                
                prevHistoricalTimestamp = currHistoricalTimestamp;
            }
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void Publisher::replayVerify() {
    LOGGER_INFO << "Started" << "\n";
    
    try {
        TBars bars;
        TPriceTradesInfos priceTradesInfos;
        TInstrumentRangePriceTradesInfos instrumentRangePriceTradesInfos;
        TInstrumentRangeBreakoutTradeInfo instrumentRangeBreakoutTradeInfo;
        
        uint32_t count = 0;
        
        std::string buffer;
        buffer.reserve(DATA_LENGTH);
    
        _file.seekg(0);
        
        tw::common::THighResTime currHistoricalTimestamp;
        
        tw::common::THighResTime startTimestamp;
        tw::common::THighResTime endTimestamp;

        bool canTrade = false;
        bool setTimeWindow = (!_settings._publisher_pf_start_time.empty() || !_settings._publisher_pf_end_time.empty());
        while ( !_isDone && !_file.eof() ) {
            buffer.clear();
            buffer.resize(DATA_LENGTH);

            _file.read(&buffer[0], DATA_LENGTH);
            if ( _file.eof() || !_file || _file.gcount() != DATA_LENGTH ) {                
                std::string s;

                TBars::iterator iter = bars.begin();
                TBars::iterator end = bars.end();
                for ( ; iter != end; ++iter ) {
                    if ( !s.empty() )
                        s += ",";
                    
                    s += iter->second._displayName;
                }
                
                LOGGER_INFO << "LIST OF SYMBOLS PRESENT: " << s << "\n";
                LOGGER_INFO << "REPLAYED ENTIRE FILE - EXITING.." << "\n";
                break;
            }
            
            if ( NULL_TIMESTAMP != buffer.substr(0, NULL_TIMESTAMP.length()) ) {
                currHistoricalTimestamp = tw::common::THighResTime::parse(buffer.substr(0, TIMESTAMP_LENGTH-2));
                if ( 1 == ++count ) {
                    if ( setTimeWindow ) {
                        std::string t;
                        if ( !_settings._publisher_pf_start_time.empty() ) {
                            t = buffer.substr(0, 8) + "-" + _settings._publisher_pf_start_time + ".000000";
                            startTimestamp = tw::common::THighResTime::parse(t);
                            LOGGER_INFO << "Will skip until startTimestamp=" << startTimestamp.toString() << "\n";
                        }
                        
                        if ( !_settings._publisher_pf_end_time.empty() ) {
                            t = buffer.substr(0, 8) + "-" + _settings._publisher_pf_end_time + ".000000";
                            endTimestamp = tw::common::THighResTime::parse(t);
                            LOGGER_INFO << "Will process till endTimestamp=" << endTimestamp.toString() << "\n";
                        }
                        
                        setTimeWindow = false;
                    }
                    
                    if ( !_settings._publisher_pf_start_time.empty() && currHistoricalTimestamp < startTimestamp  ) {
                        --count;
                        continue;
                    }
                } else if ( !_settings._publisher_pf_end_time.empty() && endTimestamp < currHistoricalTimestamp  ) {
                    // Print simulate trades
                    //
                    if ( _settings._publisher_pf_range > 0 ) {
                        TInstrumentRangeBreakoutTradeInfo::iterator iter = instrumentRangeBreakoutTradeInfo.begin();
                        TInstrumentRangeBreakoutTradeInfo::iterator end = instrumentRangeBreakoutTradeInfo.end();
                        
                        for ( ; iter != end; ++iter ) {
                            TRangeBreakoutTradeInfos& infos = iter->second;
                            std::string displayName = static_cast<tw::price::Quote&>(tw::price::QuoteStore::instance().getQuoteByKeyId(iter->first))._instrument->_displayName;
                            LOGGER_INFO << "SIMULATED TRADES for: " << displayName << "\n";
                            tw::price::Ticks minPnl = tw::price::Ticks(0);
                            tw::price::Ticks maxPnl = tw::price::Ticks(0);
                            tw::price::Ticks tradePnl = tw::price::Ticks(0);
                            uint32_t winners = 0;
                            double timeToMaxPnl = 0;
                            double timeToMinPnl = 0;
                            uint32_t numberOfTrades = infos.size();
                            for ( size_t i = 0; i < numberOfTrades; ++i ) {
                                LOGGER_INFO << "TRADE " << (i+1) << ": \n" << infos[i].toStringVerbose() << "\n";
                                minPnl += infos[i]._minPnL;
                                maxPnl += infos[i]._maxPnL;
                                tradePnl += infos[i]._endPnL;
                                if ( infos[i]._endPnL.get() > 0 )
                                    ++winners;
                                timeToMaxPnl += infos[i]._timeToMaxPnL;
                                timeToMinPnl += infos[i]._timeToMinPnL;
                            }
                            
                            if ( 0 == numberOfTrades )
                                numberOfTrades = 1;
                            
                            tw::common_str_util::TFastStream s;
                            s.setPrecision(2);
                            
                            s << "date=" << currHistoricalTimestamp.toString().substr(0, 8)
                              << ",range=" << _settings._publisher_pf_range
                              << ",minVol=" << _settings._publisher_pf_minVol
                              << ",ratioFor=" << _settings._publisher_pf_minVolRatioFor
                              << ",timer=" << _settings._publisher_pf_tradeDuration
                              << ",stop=" << _settings._publisher_pf_stopInTicks
                              << ",confirm=" << _settings._publisher_pf_confirm_break
                              << ",trades=" << infos.size()
                              << ",w=" << winners
                              << ",l=" << (numberOfTrades-winners)
                              << ",minPnl=" << (minPnl.toDouble() / numberOfTrades)
                              << ",maxPnl=" << (maxPnl.toDouble() / numberOfTrades)
                              << ",tradePnl=" << (tradePnl.toDouble() / numberOfTrades)
                              << ",timeToMaxPnl=" << (timeToMaxPnl / numberOfTrades)
                              << ",timeToMinPnl=" << (timeToMinPnl / numberOfTrades)
                              << "\n";
                            
                            LOGGER_INFO << s.c_str() << "\n";
                            LOGGER_INFO << "____________________________________" << "\n";
                        }
                    }
                    
                    LOGGER_INFO << "REPLAYED FILE till endTimestamp=" << endTimestamp << " - EXITING.." << "\n";
                    break;
                }
                
                tw::price::QuoteWire* quoteWire=reinterpret_cast<tw::price::QuoteWire*>(&buffer[HEADER_LENGTH]);
                tw::price::QuoteStore::TQuote& quote = tw::price::QuoteStore::instance().getQuoteByKeyId(quoteWire->_instrumentId);
                if ( quote.isValid() && ( _settings._publisher_pf_symbol.empty() || _settings._publisher_pf_symbol == static_cast<tw::price::Quote&>(quote)._instrument->_displayName) ) {
                    static_cast<tw::price::QuoteWire&>(quote) = *quoteWire;
                    TBar& bar = bars[quoteWire->_instrumentId];
                    if ( 0 == bar._index ) {
                        ++bar._index;
                        bar._open_timestamp = currHistoricalTimestamp;
                        bar._source = "replayVerify";
                        bar._displayName = static_cast<tw::price::Quote&>(quote)._instrument->_displayName;
                        bar._exchange = static_cast<tw::price::Quote&>(quote)._instrument->_exchange;
                        bar._duration = 60;
                    } else if ( 0 < (currHistoricalTimestamp.getUnitsFromMidnight(tw::common::eHighResTimeUnits::kMins) - bar._open_timestamp.getUnitsFromMidnight(tw::common::eHighResTimeUnits::kMins)) ) {
                        if ( bar._open.isValid() ) {
                            bar._range = bar._high - bar._low;
                            if ( bar._close > bar._open )
                                bar._dir = tw::common_trade::ePatternDir::kUp;
                            else if ( bar._close < bar._open )
                                bar._dir = tw::common_trade::ePatternDir::kDown;
                            else
                                bar._dir = tw::common_trade::ePatternDir::kUnknown;
                        }
                        
                        if ( 0 == _settings._publisher_pf_range ) {
                            LOGGER_INFO << "BAR FORMED: " << static_cast<TBarInfo&>(bar).toString() << "," << currHistoricalTimestamp.toString() << "\n";
                            if ( tw::common::ePublisherPfMode::kReplayVerifyWithTrades == _settings._publisher_pf_mode ) {
                                barTradesToString(bar);
                                LOGGER_INFO << bar._tradesLog << "\n";
                            }
                        }
                        
                        ++bar._index;
                        bar._open_timestamp = currHistoricalTimestamp;
                        bar._high = bar._low = bar._open = bar._close = tw::price::Ticks();
                        bar._volume.clear();
                        bar._numOfTrades = 0;
                        bar._trades.clear();
                        bar._volBid = bar._volAsk = tw::price::Size(0);
                    } 
                    
                    if ( quote.isNormalTrade() ) { 
                        if ( 0 == bar._numOfTrades ) {
                            bar._high = bar._low = bar._open = bar._close = quote._trade._price;
                        } else {
                            bar._close = quote._trade._price;
                            if ( quote._trade._price > bar._high )
                                bar._high = quote._trade._price;
                            else if ( quote._trade._price < bar._low )
                                bar._low = quote._trade._price;
                        }
                        bar._volume += quote._trade._size;
                        ++bar._numOfTrades;
                        
                        if ( tw::common::ePublisherPfMode::kReplayVerifyWithTrades == _settings._publisher_pf_mode ) {
                            tw::price::Size volBid;
                            tw::price::Size volAsk;                            
                            tw::common_trade::updateTrade(volBid, volAsk, quote);
                            
                            if ( 0 == _settings._publisher_pf_range ) {
                                LOGGER_INFO << "price=" << quote._trade._price 
                                            << ",bid=" << quote._book[0]._bid._price 
                                            << ",ask=" << quote._book[0]._ask._price 
                                            << ",volBid=" << volBid 
                                            << ",volAsk=" << volAsk 
                                            << ",timestamp=" << currHistoricalTimestamp.toString() 
                                            << "\n";
                            }
                            
                            tw::common_trade::updateTrades(bar, quote);
                            
                            TPriceTradesInfo& priceTradesInfo =  priceTradesInfos[quoteWire->_instrumentId];
                            if ( !priceTradesInfo._priceBid.isValid() || !priceTradesInfo._priceBid.isValid() ) {
                                priceTradesInfo._priceBid = priceTradesInfo._priceAsk = quote._trade._price;
                                if ( volBid.get() > 0 )
                                    ++priceTradesInfo._priceAsk;
                                else
                                    --priceTradesInfo._priceBid;
                                
                                priceTradesInfo._volBid = volBid;
                                priceTradesInfo._volAsk = volAsk;
                                priceTradesInfo._timestamp = currHistoricalTimestamp.toString();
                            } else {
                                if ( quote._trade._price != priceTradesInfo._priceBid && quote._trade._price != priceTradesInfo._priceAsk ) {
                                    if ( 0 == _settings._publisher_pf_range )
                                        LOGGER_INFO << priceTradesInfoToString(priceTradesInfo, bar._displayName) << "\n";
                                    priceTradesInfo._volBid = priceTradesInfo._volAsk = tw::price::Size(0);

                                    if ( quote._trade._price < priceTradesInfo._priceBid ) {
                                        priceTradesInfo._priceAsk = priceTradesInfo._priceBid;
                                        priceTradesInfo._priceBid = quote._trade._price;
                                    } else if ( quote._trade._price > priceTradesInfo._priceAsk ) {
                                        priceTradesInfo._priceBid = priceTradesInfo._priceAsk;
                                        priceTradesInfo._priceAsk = quote._trade._price;
                                    } else if ( volBid.get() > 0 ) {
                                        priceTradesInfo._priceBid = quote._trade._price;
                                    } else {
                                        priceTradesInfo._priceAsk = quote._trade._price;
                                    }
                                } 

                                priceTradesInfo._volBid += volBid;
                                priceTradesInfo._volAsk += volAsk;
                                priceTradesInfo._timestamp = currHistoricalTimestamp.toString();
                            }
                            
                            // Check if need to simulate trades
                            //
                            if ( _settings._publisher_pf_range > 0 ) {
                                TPriceTradesBasicInfo p;
                                p._price = quote._trade._price;
                                p._volBid = volBid;
                                p._volAsk = volAsk;
                                TRangePriceTradesInfos& rangePriceTradesInfos = instrumentRangePriceTradesInfos[quoteWire->_instrumentId];
                                if ( rangePriceTradesInfos.empty() ) {
                                    rangePriceTradesInfos.push_back(p);
                                } else {
                                    if ( p._price > rangePriceTradesInfos.back()._price ) {
                                        canTrade = true;
                                        rangePriceTradesInfos.push_back(p);
                                        while ( (p._price - rangePriceTradesInfos.front()._price).get() > _settings._publisher_pf_range ) {
                                            rangePriceTradesInfos.pop_front();
                                        }
                                    } else if ( p._price < rangePriceTradesInfos.front()._price ) {
                                        canTrade = true;
                                        rangePriceTradesInfos.push_front(p);
                                        while ( (rangePriceTradesInfos.back()._price - p._price).get() > _settings._publisher_pf_range ) {
                                            rangePriceTradesInfos.pop_back();
                                        }
                                    } else {
                                        bool isFound = false;
                                        TRangePriceTradesInfos::iterator iter = rangePriceTradesInfos.begin();
                                        TRangePriceTradesInfos::iterator end = rangePriceTradesInfos.end();
                                        for ( ; iter != end && !isFound; ++ iter ) {
                                            if ( (*iter)._price == p._price ) {
                                               (*iter)._volBid += p._volBid;
                                               (*iter)._volAsk += p._volAsk;
                                               isFound = true;
                                            }
                                        }
                                        
                                        if ( !isFound ) {
                                            iter = rangePriceTradesInfos.begin();
                                            for ( ; iter != end; ++ iter ) {
                                                if ( (*iter)._price > p._price ) {
                                                    rangePriceTradesInfos.insert(iter, p);
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }
                                
                                TRangeBreakoutTradeInfos& rangeBreakoutTradeInfos = instrumentRangeBreakoutTradeInfo[quoteWire->_instrumentId];
                                if ( canTrade ) {
                                    // Walk through all open orders and indicate that the next trade can happen
                                    //
                                    {
                                        TRangeBreakoutTradeInfos::reverse_iterator iter = rangeBreakoutTradeInfos.rbegin();
                                        TRangeBreakoutTradeInfos::reverse_iterator end = rangeBreakoutTradeInfos.rend();
                                        for ( ; iter != end && (*iter)._isOpen && !(*iter)._canTradeNext; ++ iter ) {
                                            (*iter)._canTradeNext = true;
                                        }
                                    }
                                    
                                    if ( _settings._publisher_pf_confirm_break && !rangeBreakoutTradeInfos.empty() ) {
                                        TRangeBreakoutTradeInfo& prevTrade = rangeBreakoutTradeInfos.back();
                                        if ( !prevTrade._isConfirmed ) {
                                            if ( prevTrade._isBuy ) {
                                                if ( p._price > prevTrade._priceTradesInfos.back()._price ) {
                                                    prevTrade._isConfirmed = true;
                                                    prevTrade._price = (p._price > quote._book[0]._ask._price) ? (p._price+tw::price::Ticks(1)) : quote._book[0]._ask._price;
                                                } else if ( p._price < prevTrade._priceTradesInfos.front()._price ) {
                                                    rangeBreakoutTradeInfos.pop_back();
                                                }
                                            } else {
                                                if ( p._price > prevTrade._priceTradesInfos.back()._price ) {
                                                    rangeBreakoutTradeInfos.pop_back();
                                                } else if ( p._price < prevTrade._priceTradesInfos.front()._price ) {
                                                    prevTrade._isConfirmed = true;
                                                    prevTrade._price = (p._price < quote._book[0]._bid._price) ? (p._price-tw::price::Ticks(1)) : quote._book[0]._bid._price;
                                                }
                                            }
                                        }
                                    }
                                    
                                    volBid.set(0);
                                    volAsk.set(0);
                                    TRangePriceTradesInfos::iterator iter = rangePriceTradesInfos.begin();
                                    TRangePriceTradesInfos::iterator end = rangePriceTradesInfos.end();
                                    for ( ; iter != end; ++ iter ) {
                                        volBid += (*iter)._volBid;
                                        volAsk += (*iter)._volAsk;
                                    }

                                    if ( (volBid+volAsk).get() > _settings._publisher_pf_minVol ) {
                                        double ratioBuy = volAsk.toDouble()/(volBid.get() > 0 ? volBid.toDouble() : 1.0);
                                        double ratioSell = volBid.toDouble()/(volAsk.get() > 0 ? volAsk.toDouble() : 1.0);

                                        TRangeBreakoutTradeInfo t;
                                        t._isOpen = false;
                                        if ( ratioBuy > _settings._publisher_pf_minVolRatioFor ) {
                                            t._isBuy = true;
                                            t._isOpen = true;
                                            t._preConfirmedPrice = t._price = (p._price > quote._book[0]._ask._price) ? (p._price+tw::price::Ticks(1)) : quote._book[0]._ask._price;
                                        } else if ( ratioSell > _settings._publisher_pf_minVolRatioFor ) {
                                            t._isBuy = false;
                                            t._isOpen = true;
                                            t._preConfirmedPrice = t._price = (p._price < quote._book[0]._bid._price) ? (p._price-tw::price::Ticks(1)) : quote._book[0]._bid._price;
                                        }

                                        if ( t._isOpen ) {
                                            canTrade = false;
                                            bool newTrade = false;
                                            if ( rangeBreakoutTradeInfos.empty() ) {
                                                newTrade = true;
                                            } else {
                                                TRangeBreakoutTradeInfo& prevTrade = rangeBreakoutTradeInfos.back();
                                                if ( prevTrade._isOpen ) {
                                                    if ( prevTrade._isBuy != t._isBuy ) {
                                                        // Walk backward and close all trades since signal the other way
                                                        //
                                                        TRangeBreakoutTradeInfos::reverse_iterator iter = rangeBreakoutTradeInfos.rbegin();
                                                        TRangeBreakoutTradeInfos::reverse_iterator end = rangeBreakoutTradeInfos.rend();
                                                        for ( ; iter != end && (*iter)._isOpen; ++ iter ) {
                                                            tw::price::Ticks pnl = (*iter)._isBuy ? (quote._book[0]._bid._price - (*iter)._price) : ((*iter)._price - quote._book[0]._ask._price);                                                            

                                                            (*iter)._endPnL = pnl;
                                                            (*iter)._endTimestamp = currHistoricalTimestamp;
                                                            (*iter)._isOpen = false;
                                                            (*iter)._reasonForExit = "opposite side signal";
                                                        }

                                                        newTrade = true;
                                                    } else if ( prevTrade._canTradeNext ) {                                                        
                                                        newTrade = true;
                                                    }
                                                } else {
                                                    newTrade = true;
                                                }
                                            }
                                            
                                            if ( newTrade) {
                                                t._tradeTimestamp = currHistoricalTimestamp;
                                                t._volObserved = volBid+volAsk;
                                                t._volObservedRatioFor = t._isBuy ? ratioBuy : ratioSell;
                                                t._isConfirmed = false;
                                                t._canTradeNext = false;
                                                t._priceTradesInfos = rangePriceTradesInfos;
                                                rangeBreakoutTradeInfos.push_back(t);
                                            }
                                        }
                                    }
                                }
                                
                                {
                                    TRangeBreakoutTradeInfos::iterator iter = rangeBreakoutTradeInfos.begin();
                                    TRangeBreakoutTradeInfos::iterator end = rangeBreakoutTradeInfos.end();
                                    for ( ; iter != end; ++ iter ) {
                                        TRangeBreakoutTradeInfo& t = *iter;
                                        if ( t._isOpen ) {
                                            tw::price::Ticks pnl = t._isBuy ? (quote._book[0]._ask._price - t._price) : (t._price - quote._book[0]._bid._price);
                                            if ( !t._minPnL.isValid() || 0 == t._minPnL.get() || pnl < t._minPnL ) {
                                                t._minPnL = pnl;
                                                t._minPnLTimestamp = currHistoricalTimestamp;
                                            }
                                            
                                            if ( !t._maxPnL.isValid() || 0 == t._maxPnL.get() || pnl > t._maxPnL ) {
                                                t._maxPnL = pnl;
                                                t._maxPnLTimestamp = currHistoricalTimestamp;
                                            }
                                            
                                            t._endPnL = pnl;
                                            t._endTimestamp = currHistoricalTimestamp;
                                            
                                            int64_t deltaSec = currHistoricalTimestamp.deltaSeconds(t._tradeTimestamp);
                                            if ( deltaSec > _settings._publisher_pf_tradeDuration ) {
                                                (*iter)._reasonForExit = "timer";
                                                t._isOpen = false;
                                            }
                                            
                                            int32_t stop = _settings._publisher_pf_stopInTicks;
                                            if ( _settings._publisher_pf_stopInTicks > 0 && pnl.get() < -stop ) {
                                                (*iter)._reasonForExit = "stop";
                                                t._isOpen = false;
                                            }
                                            
                                            if ( !t._isOpen ) {
                                                t._timeToMinPnL = t._minPnLTimestamp.deltaSeconds(t._tradeTimestamp);
                                                t._timeToMaxPnL = t._maxPnLTimestamp.deltaSeconds(t._tradeTimestamp);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    LOGGER_INFO << "Finished" << "\n";
}

void Publisher::threadMainReplay() {
    LOGGER_INFO << "Started for settings._publisher_pf_mode=" << _settings._publisher_pf_mode.toString() << "\n";
    
    if ( tw::common::ePublisherPfMode::kReplay == _settings._publisher_pf_mode )
        replay();
    else if ( tw::common::ePublisherPfMode::kReplayVerify == _settings._publisher_pf_mode || tw::common::ePublisherPfMode::kReplayVerifyWithTrades == _settings._publisher_pf_mode )
        replayVerify();
    
    LOGGER_INFO << "Finished" << "\n";
}

void Publisher::threadMainRecord() {
    LOGGER_INFO << "Started for settings._publisher_pf_mode=" << _settings._publisher_pf_mode.toString() << "\n";
    
    try {
        TQuoteDataItemPtr item;
        uint32_t maxQueueSize = 0;
        while( !_isDone ) {
            _threadPipe.read(item);
            if ( item ) {
                if ( maxQueueSize < _threadPipe.size() )
                maxQueueSize = _threadPipe.size();
                
                _file << item->_timestamp1.toString() << ": ";
                _file.write(reinterpret_cast<const char*>(&item->_quoteWire), sizeof(tw::price::QuoteWire));
                _file << '\n';
                if ( 0 == ((++_quotesCount)%1000) ) {
                    _file.flush();
                    LOGGER_INFO << "maxQueueSize (last 1000 quotes): " << maxQueueSize << ", totalQuotes: " << _quotesCount << "\n";
                    maxQueueSize = 0;
                }
                
                item.reset();
            }
        }
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    LOGGER_INFO << "Finished" << "\n";
}
