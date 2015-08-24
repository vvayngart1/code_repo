#pragma once

#include <boost/utility.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include <tw/common/singleton.h>
#include <tw/common/settings.h>
#include <tw/generated/instrument.h>

namespace tw {
namespace instr {
    
class InstrumentManager : public tw::common::Singleton<InstrumentManager> {
public:
    typedef std::vector<InstrumentPtr> TInstruments;
    typedef tw::price::Ticks TTicks;
    
public:
    static void setTickConverter(InstrumentPtr instrument);
    static void setPrecision(InstrumentPtr instrument);
    static void setSuppRest(InstrumentPtr instrument, const tw::instr::Settlement& s, bool verbose=false);
    static void calcSuppRest(const TTicks& h, const TTicks& l, const TTicks& c, const double& settlPrice, InstrumentPtr instrument, tw::instr::Settlement& s, bool useClose=true, bool verbose=false);
    
public:
    InstrumentManager();
    
    void clear();
    
public:

    // Load instruments from storage based on settings (e.g. file/db)
    //
    bool loadInstruments(const tw::common::Settings& settings, bool validate = true);
    
    // Saves instrument to storage
    //
    void saveInstruments();
    
    // This method is used in instrument loader and for unit testing
    //
    void addInstrument(InstrumentPtr instrument);
    
    // This method gets or creates settlement
    //
    tw::instr::Settlement& getOrCreateSettlement(const tw::instr::eExchange& exchange, const std::string& displayName, const std::string& settlDate);
    
    // This method saves settlement info in settlement table
    //
    void saveSettlement(const tw::instr::Settlement& x);
   
public:
    // Query instruments methods
    //    
    InstrumentPtr getByKeyId(uint32_t key) const {
        return getInstrument<0>(key);
    }

    InstrumentPtr getByDisplayName(const std::string& key) const {
        return getInstrument<1>(key);
    }
    
    InstrumentPtr getByKeyNum1(uint32_t key) const {
        return getInstrument<2>(key);
    }
    
    InstrumentPtr getByKeyNum2(uint32_t key) const {
        return getInstrument<3>(key);
    }

    InstrumentPtr getByKeyStr1(const std::string& key) const {
        return getInstrument<4>(key);
    }
    
    InstrumentPtr getByKeyStr2(const std::string& key) const {
        return getInstrument<5>(key);
    }
    
public:
    TInstruments getByExchange(tw::instr::eExchange exchange) const {
        TInstruments instruments;
        
        InstrumentTable::nth_index<0>::type& t = const_cast<InstrumentTable&>(_table).get<0>();
        InstrumentTable::nth_index<0>::type::const_iterator iter = t.begin();
        InstrumentTable::nth_index<0>::type::const_iterator end = t.end();
        
        for ( ; iter != end; ++iter ) {
            if ( (exchange = (*iter)->_exchange) )
                instruments.push_back(*iter);
        }
        
        return instruments;
    }
    
public:
    std::string getSettlKey(const tw::instr::eExchange& exchange, const std::string& displayName, const std::string& settlDate) const {
        return exchange.toString()+std::string("_")+displayName+std::string("_")+settlDate;
    }
    
    std::string getSettlKey(const InstrumentPtr& instr) const {
        return getSettlKey(instr->_exchange, instr->_displayName, instr->_settlDate);
    }
    
    std::string getSettlKey(const tw::instr::Settlement& settl) const {
        return getSettlKey(settl._exchange, settl._displayName, settl._settlDate);
    }
    
private:
    template<uint32_t TIndex, typename TTag>
    InstrumentPtr getInstrument(const TTag& key) const {
        InstrumentPtr instrument;
        
        typename InstrumentTable::nth_index<TIndex>::type& t = const_cast<InstrumentTable&>(_table).get<TIndex>();
        typename InstrumentTable::nth_index<TIndex>::type::const_iterator iter = t.find(key);
        if ( iter != t.end() ) 
            instrument = (*iter);

        return instrument; 
    } 
    
private:
    void loadFromFile(const std::string& fileName, bool validate);
    void loadFromDb(const std::string& connectionString, bool validate);
    
    void load(const std::string& row, bool validate);
    
    void saveToFile();
    void saveToDb();    
    void saveToDb(const tw::instr::Settlement& x, tw::channel_db::ChannelDb* channelDb = NULL);
    
private:    
    // TODO: need to rework to be generated from xml (time permitting)
    //      
    typedef boost::multi_index_container<
        InstrumentPtr,
        boost::multi_index::indexed_by<
            boost::multi_index::hashed_unique<BOOST_MULTI_INDEX_MEMBER(Instrument, uint32_t, _keyId)>,
            boost::multi_index::hashed_unique<BOOST_MULTI_INDEX_MEMBER(Instrument, std::string, _displayName)>,
            boost::multi_index::hashed_unique<BOOST_MULTI_INDEX_MEMBER(Instrument, uint32_t, _keyNum1)>,
            boost::multi_index::hashed_unique<BOOST_MULTI_INDEX_MEMBER(Instrument, uint32_t, _keyNum2)>,
            boost::multi_index::hashed_unique<BOOST_MULTI_INDEX_MEMBER(Instrument, std::string, _keyStr1)>,            
            boost::multi_index::hashed_unique<BOOST_MULTI_INDEX_MEMBER(Instrument, std::string, _keyStr2)>            
        >
    > InstrumentTable;
    

    std::string _header;
    tw::common::Settings _settings;
    InstrumentTable _table; 

    typedef tw::common_thread::Lock TLock;
    TLock _lock;
    
    typedef std::map<std::string, tw::instr::FeesAndMsgRatiosCME> TFeesCME;
    TFeesCME _feesCME;
    
    typedef std::map<std::string, tw::instr::Settlement> TSettlements;
    TSettlements _settlements;
};
    
} // namespace instr
} // namespace tw
