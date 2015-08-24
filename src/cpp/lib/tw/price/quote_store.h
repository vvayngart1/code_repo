#pragma once

#include <tw/common/defs.h>
#include <tw/common/singleton.h>
#include <tw/functional/utils.hpp>
#include <tw/price/quote.h>
#include <tw/price/client_container.h>
#include <tw/price/ticks_converter.h>
#include <tw/generated/instrument.h>
#include <tw/instr/instrument_manager.h>

#include <algorithm>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/count.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/extended_p_square.hpp>

namespace tw {
namespace price {
using namespace boost::accumulators;

static const boost::array<double,8> PROBS = {{ 0.01,0.1,0.25,0.5,0.75,0.9,0.99,0.999 }};

// TClientContainer interface:
//      bool add(TClientContainer::TClient*)
//      bool rem(TClientContainer::TClient*)
//      bool isEmpty()
//      void onQuote(const tw::price::TQuotePtr&);
//
struct QuoteSubscribers : public Quote
{
    typedef tw::price::ClientContainerMultiple<> TContainer;
    typedef accumulator_set<int64_t, stats<tag::count, tag::max, tag::extended_p_square> > TNotificationStats;
    TContainer _subscribers;

    // collect and print notification stats every  '_notification_stats_interval' quote updates
    // '0' disables notification statistic gathering
    unsigned _quoteNotificationStatsInterval;
    TNotificationStats _notification_stats_onix;
    TNotificationStats _notification_stats_tw;
    TNotificationStats _notification_stats_strat;
        
    // NOTE:  converters will be copied from instrument's tick converter when
    // instrument will be set (use setInstrument() method) - otherwise, converters
    // will contain NULL converters, which will return values passed in
    //
    tw::price::TicksConverter::TConverterFrom  _convertFrom;
    tw::price::TicksConverter::TConverterTo  _convertTo;
    
public:
    QuoteSubscribers() : _quoteNotificationStatsInterval(0),
                         _notification_stats_onix(tag::extended_p_square::probabilities = PROBS),
                         _notification_stats_tw(tag::extended_p_square::probabilities = PROBS),
                         _notification_stats_strat(tag::extended_p_square::probabilities = PROBS) {
        clear();
    }

    void clear() {
        Quote::clear(); 
        _subscribers.clear();
        _convertFrom = tw::price::TicksConverter::createFrom();
        _convertTo = tw::price::TicksConverter::createTo();
        _instrument = tw::instr::InstrumentConstPtr();
    }
    
    void setInstrument(tw::instr::InstrumentConstPtr instrument) {
        _instrument = instrument;
        Quote::_instrument = _instrument.get();
        Quote::_instrumentId = _instrument->_keyId;
        if ( isValid() && (_instrument->_tc != NULL) ) {
            _convertFrom = _instrument->_tc->getConverterFrom();
            _convertTo = _instrument->_tc->getConverterTo();
        }
    }    
    
    void setQuoteNotificationStatsInterval(unsigned quoteNotificationStatsInterval) {
        _quoteNotificationStatsInterval = quoteNotificationStatsInterval;
    }
    
public:
    void notifySubscribers() const {
        QuoteSubscribers* ncThis = const_cast<QuoteSubscribers*>(this);
        ncThis->updateStats();
        {
            tw::common::THighResTimeScope notifyTimer(ncThis->_timestamp3, ncThis->_timestamp4);
            _subscribers.onQuote(*this);
        }
        
        if ( _quoteNotificationStatsInterval > 0 ) {
            if ( Quote::kSuccess != ncThis->_status || !(ncThis->isBookUpdate() || ncThis->isTrade()) )
                return;
            
            ncThis->_notification_stats_onix(ncThis->_timestamp2 - ncThis->_timestamp1);
            ncThis->_notification_stats_tw(ncThis->_timestamp3 - ncThis->_timestamp2);
            ncThis->_notification_stats_strat(ncThis->_timestamp4 - ncThis->_timestamp3);
            
            if ( count(_notification_stats_onix) % _quoteNotificationStatsInterval == 0 ) {
                printNotificationStats();
                ncThis->_notification_stats_onix = TNotificationStats(tag::extended_p_square::probabilities = PROBS);
                ncThis->_notification_stats_tw = TNotificationStats(tag::extended_p_square::probabilities = PROBS);
                ncThis->_notification_stats_strat = TNotificationStats(tag::extended_p_square::probabilities = PROBS);
                ncThis->_intGaps = 0;
            }            
        }
    }
    
    void printNotificationStats() const {
        printNotificationStats(_notification_stats_onix, "stats_onix");
        printNotificationStats(_notification_stats_tw, "stats_tw");
        printNotificationStats(_notification_stats_strat, "stats_strat");
    }
    
    void printNotificationStats(const TNotificationStats& stats, const std::string& name) const {
        LOGGER_INFO << "{ \"eventName\": \"QuoteNotification(" << name << ")\"" \
        << ", \"instrumentName\": \"" << this->_instrument->_displayName << "\"" \
        << ", \"count\": " << count(stats) \
        << ", \"intGaps\": " << this->_intGaps \
        << ", \"p010\":" << extended_p_square(stats)[0] \
        << ", \"p100\":" << extended_p_square(stats)[1] \
        << ", \"p250\":" << extended_p_square(stats)[2] \
        << ", \"p500\":" << extended_p_square(stats)[3] \
        << ", \"p750\":" << extended_p_square(stats)[4] \
        << ", \"p900\":" << extended_p_square(stats)[5] \
        << ", \"p990\":" << extended_p_square(stats)[6] \
        << ", \"p999\":" << extended_p_square(stats)[7] \
        << ", \"max\":" << max(stats) \
        << "}\n";
    }
    
    bool isSubscribed() const {
        return !(_subscribers.empty());
    }
    
private:
    tw::instr::InstrumentConstPtr _instrument;
    
}; 
    
class QuoteStore : public tw::common::Singleton<QuoteStore>
{
public:
    typedef QuoteSubscribers TQuote;
    typedef std::tr1::unordered_map<tw::instr::Instrument::TKeyId, TQuote> TQuotes;
    typedef std::tr1::unordered_map<uint32_t, TQuote*> TQuotesIndex;    
    
public:
    QuoteStore() : _quoteNotificationStatsInterval(0) {
        clear();
    }
    
    void clear() {
        _quotes.clear();
        _index1.clear();
        _index2.clear();
    }
    
    void copy(const QuoteStore& rhs) {
        _quotes = rhs._quotes;
        
        TQuotes::iterator iter = _quotes.begin();
        TQuotes::iterator end = _quotes.end();
        for ( ; iter != end; ++iter ) {
            TQuote& quote = iter->second;
            tw::instr::InstrumentPtr instrument = tw::instr::InstrumentManager::instance().getByKeyId(quote._instrumentId);
            if ( instrument ) {
                _index1[instrument->_keyNum1] = &(iter->second);
                _index2[instrument->_keyNum2] = &(iter->second);            
                
                LOGGER_INFO << "Added Quote for: " << instrument->_displayName << " :: " << instrument->_keyNum1 << " :: " << instrument->_keyNum2 << "\n";
            }
        }
    }
    
public:
    bool processCommand(tw::common::Command& cmnd) {
        try {
            if ( cmnd._type != tw::common::eCommandType::kQuoteStore ) {
                LOGGER_ERRO << "cmnd._type != tw::common::eCommandType::kQuoteStore in: "  << cmnd.toString() << "\n";
                return false;
            }
            
            if ( cmnd._subType != tw::common::eCommandSubType::kStatus ) {
                LOGGER_ERRO << "command._subType != tw::common::eCommandSubType::kStatus in: "  << cmnd.toString() << "\n";
                return false;
            }
            
            tw::instr::eExchange exchange;
            std::string displayName;
            
            if ( !cmnd.get("exchange", exchange) ) {
                LOGGER_ERRO << "no \"exchange\" in: "  << cmnd.toString() << "\n";
                return false;
            }
            
            if ( !cmnd.get("displayName", displayName) ) {
                LOGGER_ERRO << "no \"displayName\" in: "  << cmnd.toString() << "\n";
                return false;
            }
            
            tw::instr::InstrumentConstPtr instrument = tw::instr::InstrumentManager::instance().getByDisplayName(displayName);
            if ( instrument == NULL ) {
                LOGGER_ERRO << "Can't find instrument for: " << displayName << " :: " << cmnd.toString() << "\n";
                return false;
            }
            
            TQuotes::iterator iter = _quotes.find(instrument->_keyId);
            if ( iter == _quotes.end() ) {
                LOGGER_ERRO << "Can't find quote for: " << displayName << " :: " << cmnd.toString() << "\n";
                return false;
            }
            
            cmnd._params = iter->second.statsToCommand()._params;
            return true;
        } catch(const std::exception& e) {
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        }
        
        return false;
    }
    
public:
    template <typename TClient>
    bool subscribe(tw::instr::InstrumentConstPtr instrument, TClient* client) {
        return subscribe(instrument->_keyId, client);
    }
    
    template <typename TClient>
    bool subscribeFront(tw::instr::InstrumentConstPtr instrument, TClient* client) {
        return subscribeFront(instrument->_keyId, client);
    }
    
    template <typename TClient>
    bool unsubscribe(tw::instr::InstrumentConstPtr instrument, TClient* client) {
        return unsubscribe(instrument->_keyId, client);
    }
    
    template <typename TClient>
    bool subscribe(const tw::instr::Instrument::TKeyId& key, TClient* client) {
        TQuote& quote = getQuoteByKeyId(key);
        if ( !quote.isValid() )
            return false;
        
        return quote._subscribers.add(client);
    }
    
    template <typename TClient>
    bool subscribeFront(const tw::instr::Instrument::TKeyId& key, TClient* client) {
        TQuote& quote = getQuoteByKeyId(key);
        if ( !quote.isValid() )
            return false;
        
        return quote._subscribers.addFront(client);
    }
    
    template <typename TClient>
    bool unsubscribe(const tw::instr::Instrument::TKeyId& key, TClient* client) {
        TQuote& quote = getQuoteByKeyId(key);
        if ( !quote.isValid() )
            return false;
        
        return quote._subscribers.rem(client);
    }
    
public:
    // Get methods
    //
    
    // getQuote(tw::instr::InstrumentConstPtr) gets (and creates if not found)
    // quote for an instrument
    //
    TQuote& getQuote(tw::instr::InstrumentConstPtr instrument) {
        if ( !instrument )
            return _nullQuote;
        
        TQuotes::iterator iter = _quotes.find(instrument->_keyId);
        if ( iter == _quotes.end() ) {
            iter = _quotes.insert(TQuotes::value_type(instrument->_keyId, TQuote())).first;
            iter->second.setInstrument(instrument);
            iter->second.setQuoteNotificationStatsInterval(_quoteNotificationStatsInterval);
            
            _index1[instrument->_keyNum1] = &(iter->second);
            _index2[instrument->_keyNum2] = &(iter->second);
            
            LOGGER_INFO << "Added Quote for: " << instrument->_displayName 
                    << " :: " << instrument->_keyNum1 << " :: " << instrument->_keyNum2 << "\n";
        }
        
        return iter->second;
    }
    
    // getQuoteByKeyId(tw::instr::InstrumentConstPtr) gets (and creates if not found)
    // quote for an instrument
    //
    TQuote& getQuoteByKeyId(const tw::instr::Instrument::TKeyId& key) {
        return getQuote(tw::instr::InstrumentManager::instance().getByKeyId(key));
    }
    
    TQuote& getQuoteByDisplayName(const std::string& key) {
        return getQuote(tw::instr::InstrumentManager::instance().getByDisplayName(key));
    }
    
    // getQuoteByKeyNum1(uint32_t key) gets (and DOESN'T create if not found)
    // quote for an instrument's keyNum1
    //
    TQuote& getQuoteByKeyNum1(uint32_t key) {
        TQuotesIndex::iterator iter = _index1.find(key);
        if ( iter != _index1.end() )
            return (*iter->second);
        
        return _nullQuote;
    }
    
    // getQuoteByKeyNum1(uint32_t key) gets (and DOESN'T create if not found)
    // quote for an instrument's keyNum2
    //
    TQuote& getQuoteByKeyNum2(uint32_t key) {
        TQuotesIndex::iterator iter = _index2.find(key);
        if ( iter != _index1.end() )
            return (*iter->second);
        
        return _nullQuote;
    }    
    
    const TQuotes& quotes() const {
        return _quotes;
    }
    
public:
    template <typename TFunctor>
    void for_each_quote(TFunctor f) {
        tw::functional::for_each2nd(_quotes, f);
    }
    
public:
    void setQuoteNotificationStatsInterval(unsigned quoteNotificationStatsInterval) {
        _quoteNotificationStatsInterval = quoteNotificationStatsInterval;
    }

private:    
    TQuote _nullQuote;
    TQuotes _quotes;    
    TQuotesIndex _index1;
    TQuotesIndex _index2;
    unsigned _quoteNotificationStatsInterval;
};

} // namespace price
} // namespace tw

