#include <tw/channel_pf_historical/channel_pf_historical.h>
#include <tw/log/defs.h>
#include <tw/price/quote_store.h>
#include <tw/common_strat/consumer_proxy.h>
#include <tw/price/ticks_converter.h>
#include <tw/common/high_res_time.h>

#include <stdio.h>
#include <tw/common_str_util/fast_numtoa.h>

#include <boost/algorithm/string/split.hpp>

/*
static uint32_t MAX_GAP_FROM_PREVIOUS = 5;
static uint32_t MAX_DELAY_IN_MICROSEC = 250 * 1000; // 250 ms
static uint32_t MAX_DELAY_IN_MICROSEC_GAP = 60 * 1000 * 1000; // 1 min
*/

namespace tw {
namespace channel_pf_historical {

ChannelPfHistorical::ChannelPfHistorical() : _settings() {
}

ChannelPfHistorical::~ChannelPfHistorical() {
    stop();
}

// tw::channel_pf::IChannelImpl interface
//
bool ChannelPfHistorical::init(const tw::common::Settings& settings) {
    bool status = true;
    try {
        _settings = settings;                
        tw::price::QuoteStore::instance().setQuoteNotificationStatsInterval(10000);
        return true;
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

bool ChannelPfHistorical::start() {
    bool status = true;
    try {
        LOGGER_INFO_EMPHASIS << "Starting channel price feed Historical ..." << "\n";        
        
        switch ( _settings._channel_pf_historical_mode ) {
            case tw::common::eChannelPfHistoricalMode::kNanex:
                _connection = tw::common_comm::TcpIpClientConnection::create(_settings._channel_pf_historical_host, _settings._channel_pf_historical_port, this, false);
                break;
            case tw::common::eChannelPfHistoricalMode::kAMR:
                _connection = tw::common_comm::TcpIpClientConnection::create(_settings._channel_pf_historical_host, _settings._channel_pf_historical_port, this, true);
                break;
            default:
                LOGGER_ERRO << "Unknown mode for price feed Historical server" << "\n";
                return false;
        }
        
        if ( !_connection ) {
            LOGGER_ERRO << "Failed to create connection to Historical server" << "\n";
            return false;
        }
        
        if ( !_connection->start() ) {
            LOGGER_ERRO << "Failed to connect to Historical server" << "\n";
            return false;
        }
        
        LOGGER_INFO_EMPHASIS << "Started channel price feed Historical" << "\n";
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

void ChannelPfHistorical::stop() {    
    try {
        LOGGER_INFO_EMPHASIS << "Stopping channel price feed Historical..." << "\n";
        
        if ( _connection ) {
            _connection->stop();
            _connection.reset();
        }
        
        LOGGER_INFO_EMPHASIS << "Stopped channel price feed Historical" << "\n";
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

bool ChannelPfHistorical::subscribe(tw::instr::InstrumentConstPtr instrument) {
    LOGGER_INFO << "Request to subscribe to: " << instrument->_displayName << "\n";
    return true;
}

bool ChannelPfHistorical::unsubscribe(tw::instr::InstrumentConstPtr instrument) {    
    return true;
}

// tw::common_comm::TcpIpClientCallback interface
//
void ChannelPfHistorical::onConnectionUp(TConnection::native_type id, TConnection::pointer connection) {
    LOGGER_INFO << id << " :: Connected to Historical Server"  << "\n";
}

void ChannelPfHistorical::onConnectionError(TConnection::native_type id, const std::string& message) {
    LOGGER_ERRO << id << " :: Connection Error to Historical Server"  << "\n";
}

void ChannelPfHistorical::onConnectionData(TConnection::native_type id, const std::string& message) {
    try {
        _tokens.clear();
        boost::algorithm::split(_tokens, message, boost::algorithm::is_any_of(","));
        
        if ( _tokens.size() < 2 )
            return;
        
        tw::price::QuoteStore::TQuote& quote = tw::price::QuoteStore::instance().getQuoteByDisplayName(_tokens[3]);
        if ( !quote.isValid() || !quote.isSubscribed() )
            return;
        
        quote.clearRuntime();
        
        if ( !_tokens[1].empty() ) {
            switch ( _tokens[1][0] ) {
                case 'Q':
                {
                    if ( _tokens.size() < 9 )
                        return;
                    
                    tw::price::Size bidSize(::atol(_tokens[4].c_str()));
                    tw::price::Ticks bidPrice(::atol(_tokens[5].c_str()));
                    tw::price::Ticks askPrice(::atol(_tokens[6].c_str()));                    
                    tw::price::Size askSize(::atol(_tokens[7].c_str()));
                    
                    tw::price::PriceLevel& priceLevelTo = quote._book[0];
                    if ( priceLevelTo._bid._size != bidSize )
                        quote.setBidSize(bidSize, 0);
                    
                    if ( priceLevelTo._bid._price != bidPrice )
                        quote.setBidPrice(bidPrice, 0);
                    
                    if ( priceLevelTo._ask._price != askPrice )
                        quote.setAskPrice(askPrice, 0);
                    
                    if ( priceLevelTo._ask._size != askSize )
                        quote.setAskSize(askSize, 0);
                    
                }
                    break;
                case 'T':
                    if ( _tokens.size() < 7 )
                        return;
                    
                    quote._trade._price.set(::atol(_tokens[5].c_str()));
                    quote._trade._size.set(::atol(_tokens[4].c_str()));
                    quote.setTrade();
                    break;
                default:
                    return;
            }
        }                 
        
        if ( quote.isChanged() ) {
            quote._seqNum = ::atol(_tokens[0].c_str());
            quote._exTimestamp.setToNow();
            tw::common_strat::ConsumerProxy::instance().onQuote(quote);
            
            if ( _settings._common_comm_verbose ) {
                LOGGER_INFO << "BOOKS: __________________________" << "\n";
                LOGGER_INFO << "FROM: \n" << id << " :: " << message << "\n";
                LOGGER_INFO << "TO: \n" << quote.toString() << "\n";
            }
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}


int32_t ChannelPfHistorical::process(TConnection::native_type id, char* data, size_t size) {
    try {
        const uint32_t TIMESTAMP_LENGTH = 26;
        const uint32_t HEADER_LENGTH = 26;
        const uint32_t BODY_LENGTH = sizeof(tw::price::QuoteWire);
        const uint32_t FOOTER_LENGTH = sizeof(char);
        const uint32_t MESSAGE_LENGTH = TIMESTAMP_LENGTH+HEADER_LENGTH+BODY_LENGTH+FOOTER_LENGTH;
        
        if ( size < MESSAGE_LENGTH )
            return 0;
        
        tw::common::THighResTime now = tw::common::THighResTime::now();
        tw::price::QuoteWire* quoteWire = reinterpret_cast<tw::price::QuoteWire*>(data+TIMESTAMP_LENGTH+HEADER_LENGTH);
        tw::price::QuoteStore::TQuote& quote = tw::price::QuoteStore::instance().getQuoteByKeyId(quoteWire->_instrumentId);
        if ( !quote.isValid() || !quote.isSubscribed() )            
            return MESSAGE_LENGTH;
        
        quote.clearRuntime();
        
        static_cast<tw::price::QuoteWire&>(quote) = *quoteWire;
        
        if ( quote.isChanged() ) {
            quote._exTimestamp = tw::common::THighResTime::parse(std::string(data, TIMESTAMP_LENGTH));
            quote._timestamp1 = now;
            quote._timestamp2.setToNow();
            tw::common_strat::ConsumerProxy::instance().onQuote(quote);
            
            if ( _settings._common_comm_verbose ) {
                LOGGER_INFO << "BOOKS: __________________________" << "\n";
                LOGGER_INFO << "TO: \n" << quote.toString() << "\n";
            }
        }
        
        return MESSAGE_LENGTH;
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    return size;
}
    
} // channel_pf_historical
} // tw

