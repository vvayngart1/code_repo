
#include "tw/common/high_res_time.h"

#include <tw/instr/instrument_manager.h>

#include <tw/common/filesystem.h>
#include <tw/price/ticks_converter.h>
#include <tw/price/quote_store.h>

#include <tw/common/fractional.h>
#include <tw/common_trade/suppRestCalc.h>

#include <fstream>

namespace tw {
namespace instr {

void InstrumentManager::setTickConverter(InstrumentPtr instrument) {
    if ( NULL != instrument->_tc )
        return;
    
    // Set instrument tick converter
    //
    instrument->_tc = tw::price::TicksConverterPtr(new tw::price::TicksConverter(instrument));
}
    
void InstrumentManager::setPrecision(InstrumentPtr instrument) {
    // Set instrument precision
    //
    instrument->_precision = tw::common::Fractional::getPrecision(instrument->_tickNumerator, instrument->_tickDenominator);
}

void InstrumentManager::calcSuppRest(const TTicks& high, const TTicks& low, const TTicks& close, const double& settlPrice, InstrumentPtr instrument, tw::instr::Settlement& s, bool useClose, bool verbose) {
    typedef tw::common_trade::SuppRestCalc TImpl;
    double PP = 0.0;
    double S1 = 0.0;
    double R1 = 0.0;
    double S2 = 0.0;
    double R2 = 0.0;
    
    double c_settlPrice = useClose ? close.toDouble() : settlPrice;
    
    // Set instrument precision
    //
    if ( !TImpl::calcPP(high, low, c_settlPrice, PP) ) {
        LOGGER_ERRO << "Failed to calc PP for: "
                    << s._exchange << "::" << s._displayName << " ==> "
                    << "high=" << high
                    << ",low=" << low
                    << ",c_settlPrice=" << c_settlPrice
                    << "\n";
        return;
    }
    
    if ( !TImpl::calcS1(high, PP, S1) ) {
        LOGGER_ERRO << "Failed to calc S1 for: "
                    << s._exchange << "::" << s._displayName << " ==> "
                    << "high=" << high
                    << ",low=" << low
                    << ",c_settlPrice=" << c_settlPrice
                    << ",PP=" << s._PP
                    << "\n";
        return;
    }
    
    if ( !TImpl::calcR1(low, PP, R1) ) {
        LOGGER_ERRO << "Failed to calc R1 for: "
                    << s._exchange << "::" << s._displayName << " ==> "
                    << "high=" << high
                    << ",low=" << low
                    << ",c_settlPrice=" << c_settlPrice
                    << ",PP=" << PP
                    << "\n";
        return;
    }
    
    if ( !TImpl::calcS2(high, low, PP, S2) ) {
        LOGGER_ERRO << "Failed to calc S2 for: "
                    << s._exchange << "::" << s._displayName << " ==> "
                    << "high=" << high
                    << ",low=" << low
                    << ",c_settlPrice=" << c_settlPrice
                    << ",PP=" << PP
                    << "\n";
        return;
    }
    
    if ( !TImpl::calcR2(high, low, PP, R2) ) {
        LOGGER_ERRO << "Failed to calc R2 for: "
                    << s._exchange << "::" << s._displayName << " ==> "
                    << "high=" << high
                    << ",low=" << low
                    << ",c_settlPrice=" << c_settlPrice
                    << ",PP=" << PP
                    << "\n";
        return;
    }
    
    setTickConverter(instrument);
    
    s._PP = instrument->_tc->toFractionalExchangePrice(PP)*instrument->_displayFormat;
    s._S1 = instrument->_tc->toFractionalExchangePrice(S1)*instrument->_displayFormat;
    s._R1 = instrument->_tc->toFractionalExchangePrice(R1)*instrument->_displayFormat;
    s._S2 = instrument->_tc->toFractionalExchangePrice(S2)*instrument->_displayFormat;
    s._R2 = instrument->_tc->toFractionalExchangePrice(R2)*instrument->_displayFormat;
    
    if ( verbose )
        LOGGER_INFO << "Configured support/resistence for settlement: "
                << s._exchange << "::" << s._displayName << " ==> "
                << "_high=" << s._high
                << ",_low=" << s._low
                << ",_close=" << s._close
                << ",_settlPrice=" << s._settlPrice
                << ",_settlDate=" << s._settlDate
                << ",_PP=" << s._PP
                << ",_S1=" << s._S1
                << ",_R1=" << s._R1
                << ",_S2=" << s._S2
                << ",_R2=" << s._R2            
                << "\n";
}

void InstrumentManager::setSuppRest(InstrumentPtr instrument, const tw::instr::Settlement& s, bool verbose) {
    setTickConverter(instrument);
    
    instrument->_high = instrument->_tc->fromExchangePrice(boost::lexical_cast<double>(s._high)/instrument->_displayFormat);
    instrument->_low = instrument->_tc->fromExchangePrice(boost::lexical_cast<double>(s._low)/instrument->_displayFormat);
    instrument->_close = instrument->_tc->fromExchangePrice(boost::lexical_cast<double>(s._close)/instrument->_displayFormat);
    instrument->_settlPrice = instrument->_tc->fractionalTicks(boost::lexical_cast<double>(s._settlPrice)/instrument->_displayFormat);
    
    // Recalculate PP and the rest in case any of inputs were changed in database either manually or automatically
    //
    tw::instr::Settlement local_s = s;
    calcSuppRest(instrument->_high, instrument->_low, instrument->_close, instrument->_settlPrice, instrument, local_s, verbose);
    
    instrument->_PP = instrument->_tc->fractionalTicks(local_s._PP/instrument->_displayFormat);
    instrument->_S1 = instrument->_tc->fractionalTicks(local_s._S1/instrument->_displayFormat);
    instrument->_R1 = instrument->_tc->fractionalTicks(local_s._R1/instrument->_displayFormat);
    instrument->_S2 = instrument->_tc->fractionalTicks(local_s._S2/instrument->_displayFormat);
    instrument->_R2 = instrument->_tc->fractionalTicks(local_s._R2/instrument->_displayFormat);
    
    if ( verbose )
        LOGGER_INFO << "Configured support/resistence for instrument: "
                << instrument->_exchange << "::" << instrument->_displayName << " ==> "
                << "\nsettl from database: " << s.toStringVerbose()
                << "\nsettl recalculated: " << local_s.toStringVerbose()
                << "\n_high=" << instrument->_high
                << "\n_low=" << instrument->_low
                << "\n_close=" << instrument->_close
                << "\n_settlPrice=" << instrument->_settlPrice
                << "\n_PP=" << instrument->_PP
                << "\n_S1=" << instrument->_S1
                << "\n_R1=" << instrument->_R1
                << "\n_S2=" << instrument->_S2
                << "\n_R2=" << instrument->_R2            
                << "\n\n";
}

InstrumentManager::InstrumentManager() {
    clear();
}

void InstrumentManager::clear() {
    _header.clear();
    _settings.clear();
    _table.clear();    
}

bool InstrumentManager::loadInstruments(const tw::common::Settings& settings, bool validate) {
    try {
        if ( settings._instruments_dataSourceType == "file" )
            loadFromFile(settings._instruments_dataSource, validate);
        else if ( settings._instruments_dataSourceType == "db" )
            loadFromDb(settings._db_connection_str, validate);
        else
            throw std::invalid_argument(settings._instruments_dataSourceType + std::string(" unsupported data source"));

        _settings = settings;
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "exception :: " << e.what() << "\n" << "\n";      
        return false;
    } catch(...) {
        LOGGER_ERRO << "exception :: <unknown>" << "\n" << "\n";     
        return false;
    }

    return true;
}

void InstrumentManager::loadFromFile(const std::string& fileName, bool validate) {
    std::ifstream file(fileName.c_str());
    std::string line;

    if( !file.good() || file.eof() )
        throw std::invalid_argument("failed to read file " + fileName);

    // Save the first line as a header
    //
    std::getline(file, _header);
    for( std::getline(file, line); file.good() && !file.eof(); std::getline(file, line) ) {
        load(line, validate);
    }
    file.close();
}

void InstrumentManager::loadFromDb(const std::string& connectionString, bool validate) {
    tw::channel_db::ChannelDb channelDb;
    
    if ( !channelDb.init(connectionString) || !channelDb.isValid() )
        throw std::invalid_argument("failed to open db connection: " + connectionString);
    
    // Load CME fees
    //
    {
        tw::instr::FeesAndMsgRatiosCME_GetAll query;
        if ( !query.execute(channelDb) )
            throw std::invalid_argument("failed to execute FeesAndMsgRatiosCME_GetAll query on connection: " + connectionString);
        
        for ( size_t i = 0; i < query._o1.size(); ++i ) {
            _feesCME[query._o1[i]._symbol] = query._o1[i];
        }
    }
        
    // Load settlements for yesterday
    //
    if ( validate ) {
        tw::instr::Settlements_GetAll query;
        if ( !query.execute(channelDb, tw::common::THighResTime::dateISOString(-1)) )
            throw std::invalid_argument("failed to execute Settlements_GetAll query on connection: " + connectionString);
        
        for ( size_t i = 0; i < query._o1.size(); ++i ) {
            _settlements[getSettlKey(query._o1[i])] = query._o1[i];
        }  
    }
    
    tw::instr::Instruments_GetAll query;
    if ( !query.execute(channelDb) )
        throw std::invalid_argument("failed to execute Instruments_GetAll query on connection: " + connectionString);
    
    // Save header
    //
    _header = Instrument::header();
    
    for ( size_t i = 0; i < query._o1.size(); ++i ) {
        load(query._o1[i].toString(), validate);
    }    
}

void InstrumentManager::load(const std::string& row, bool validate) {
    InstrumentPtr instrument(new Instrument());
    if ( !instrument->fromString(row) ) {
        LOGGER_ERRO << "Couldn't parse Instrument: " << row << "\n";
        return;
    }
    
    if ( validate && !instrument->isValid() ) {
        LOGGER_ERRO << "Invalid Instrument: " << row << "\n";
        return;
    }

    // Load spread legs for spreads
    //
    if (  eInstrType::kSpread ==  instrument->_instrType ) {
        instrument->_legs.resize(instrument->_numberOfLegs);

        for ( uint32_t counter = 0; counter < instrument->_numberOfLegs; ++counter ) {
            instrument->_legs[counter] = SpreadLegPtr(new SpreadLeg());
            SpreadLegPtr& leg = instrument->_legs[counter];

            // TODO: need to come up with algo of validating spread ratios!
            //
            leg->_index = counter+1;
            leg->_parent = instrument;

            // TODO: Right now only 4 legged spreads are supported, make more
            // flexible in the future
            //
            switch ( counter ) {
                case 0:
                    leg->_ratio = instrument->_ratioLeg1;
                    leg->_instrument = getByDisplayName(instrument->_displayNameLeg1);                        
                    break;
                case 1:
                    leg->_ratio = instrument->_ratioLeg2;
                    leg->_instrument = getByDisplayName(instrument->_displayNameLeg2);
                    break;
                case 2:
                    leg->_ratio = instrument->_ratioLeg3;
                    leg->_instrument = getByDisplayName(instrument->_displayNameLeg3);
                    break;
                case 3:
                    leg->_ratio = instrument->_ratioLeg4;
                    leg->_instrument = getByDisplayName(instrument->_displayNameLeg4);
                    break;

            }

            if ( validate && !leg->isValid() ) {
                LOGGER_ERRO << "Invalid leg Instrument: " << row << "\n";
                return;
            }
        }
    }

    if ( validate )
        setTickConverter(instrument);
    
    setPrecision(instrument);
    
    // Allow empty exchange specific keys, set them to keyId if empty
    //
    if ( 0 == instrument->_keyNum1 )
        instrument->_keyNum1 = instrument->_keyId;
    
    if ( 0 == instrument->_keyNum2 )
        instrument->_keyNum2 = instrument->_keyId;
    
    if ( instrument->_keyStr1.empty() )
        instrument->_keyStr1 = boost::lexical_cast<std::string>(instrument->_keyId);
    
    if ( instrument->_keyStr2.empty() )
        instrument->_keyStr2 = boost::lexical_cast<std::string>(instrument->_keyId);
    
    if ( tw::instr::eExchange::kCME == instrument->_exchange ) {
        TFeesCME::iterator iterFees = _feesCME.find(instrument->_symbol);
        if ( iterFees != _feesCME.end() ) {
            instrument->_feeExLiqAdd = iterFees->second._feeEx;
            instrument->_feeExLiqRem = iterFees->second._feeEx;
            instrument->_feeExClearing = iterFees->second._feeExClearing;
            instrument->_feeBrokerage = iterFees->second._feeBrokerage;
            instrument->_feePerTrade = iterFees->second._feePerTrade;
            
            if ( _settings._instruments_verbose )
                LOGGER_INFO << "Configured fees for instrument: "
                    << instrument->_exchange << "::" << instrument->_symbol << "_" << instrument->_displayName << " ==> "
                    << "feeExLiqAdd=" << instrument->_feeExLiqAdd
                    << ",_feeExLiqRem=" << instrument->_feeExLiqRem
                    << ",_feeExClearing=" << instrument->_feeExClearing
                    << ",_feeBrokerage=" << instrument->_feeBrokerage
                    << ",_feePerTrade=" << instrument->_feePerTrade
                    << "\nfrom: " << iterFees->second.toStringVerbose()
                    << "\n";
        }
    }
    
    if ( validate ) {
        instrument->_settlDate = tw::common::THighResTime::dateISOString(-1);        
        std::string key = getSettlKey(instrument);
        TSettlements::iterator iterSettl = _settlements.find(key);
        if ( iterSettl == _settlements.end() ) {
            if ( _settings._instruments_verbose )
                LOGGER_WARN << "Can't find settlement info for: " << key << "\n";
        } else {
            const tw::instr::Settlement& s = iterSettl->second;
            setSuppRest(instrument, s, _settings._instruments_verbose);
        }
    }
    
    _table.insert(instrument);
    if ( _settings._instruments_createQuotesOnLoad )
        tw::price::QuoteStore::instance().getQuote(instrument);
}

void InstrumentManager::addInstrument(InstrumentPtr instrument) {
    tw::common_thread::LockGuard<TLock> lock(_lock);
 
    _table.insert(instrument);
    if ( _settings._instruments_verbose )
        LOGGER_WARN << "Added [Instrument]: " << instrument->_displayName << "\n";
}

tw::instr::Settlement& InstrumentManager::getOrCreateSettlement(const tw::instr::eExchange& exchange, const std::string& displayName, const std::string& settlDate) {
    std::string key = getSettlKey(exchange, displayName, settlDate);
    TSettlements::iterator iter = _settlements.find(key);
    if ( iter == _settlements.end() ) {
       tw::instr::Settlement s;
       s._exchange = exchange;
       s._displayName = displayName;
       s._settlDate = settlDate;
       
       iter = _settlements.insert(TSettlements::value_type(key, s)).first;
    }
    
    return iter->second;
}

void InstrumentManager::saveInstruments() {
    try {
        if ( _settings._instruments_dataSourceTypeOutput == "file" )
            saveToFile();
        else if (_settings._instruments_dataSourceTypeOutput == "db" )
            saveToDb();
        else
            throw std::invalid_argument(_settings._instruments_dataSourceType + std::string(" unsupported data source"));
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "exception :: " << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "exception :: <unknown>" << "\n" << "\n";
    }
}

void InstrumentManager::saveSettlement(const tw::instr::Settlement& x) {
    try {
        if (_settings._instruments_dataSourceTypeOutput == "db" )
            saveToDb(x);
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "exception :: " << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "exception :: <unknown>" << "\n" << "\n";
    }
}

void InstrumentManager::saveToFile() {
    // Swap files
    //
    if ( !tw::common::Filesystem::swap(_settings._instruments_dataSource) )
        return;
    
    // Write to file
    //
    std::ofstream file(_settings._instruments_dataSource.c_str(), std::ios_base::out | std::ios_base::trunc);

    if( !file.good() )
        throw std::invalid_argument("failed to read file " + _settings._instruments_dataSource);
    
    
    // Output all symbols to a new file
    //
    InstrumentTable::nth_index<0>::type& t = const_cast<InstrumentTable&>(_table).get<0>();
    // Put all in the map sorted by instrument id
    //
    std::map<uint32_t, std::string> m;
    {
        InstrumentTable::nth_index<0>::type::const_iterator iter = t.begin();
        InstrumentTable::nth_index<0>::type::const_iterator end = t.end();
        
        for ( ; iter != end; ++iter ) {
            m[(*iter)->_keyId] = (*iter)->toString();
        }
    }
    
    {
        std::map<uint32_t, std::string>::iterator iter = m.begin();
        std::map<uint32_t, std::string>::iterator end = m.end();
        
        file << _header << "\n";
        for ( ; iter != end; ++iter ) {
            file << iter->second;
        }
    }
    
    file.close();    
}

void InstrumentManager::saveToDb() {
    tw::channel_db::ChannelDb channelDb;
    
    if ( !channelDb.init(_settings._db_connection_str) || !channelDb.isValid() )
        throw std::invalid_argument("failed to open db connection: " + _settings._db_connection_str);
    
    uint32_t count = 0;
    
    // Output all symbols to a db
    //
    InstrumentTable::nth_index<0>::type& t = const_cast<InstrumentTable&>(_table).get<0>();
    InstrumentTable::nth_index<0>::type::const_iterator iter = t.begin();
    InstrumentTable::nth_index<0>::type::const_iterator end = t.end();
    
    LOGGER_INFO << "Starting to save instruments to DB..." << "\n";
    
    for ( ; iter != end; ++iter ) {
        const Instrument& instrument = *(*iter);
        if ( instrument._dirty ) {
            tw::channel_db::ChannelDb::TConnectionPtr connection = channelDb.getConnection();
            tw::channel_db::ChannelDb::TStatementPtr statement = channelDb.getStatement(connection);

            tw::instr::Instruments_SaveInstrument query;
            query.execute(statement, instrument);

            if ( 0 == ((++count)%500) )
                LOGGER_INFO << "Instruments saved: " << count << "\n";
        }
    }
    
    LOGGER_INFO << "DONE saving instruments to DB - count: " << count << "\n";
    
    LOGGER_INFO << "Starting to save settlements to DB..." << "\n";
    
    // Save all settlements
    //
    {
        count = 0;
        TSettlements::iterator iter = _settlements.begin();
        TSettlements::iterator end = _settlements.end();
        for ( ; iter != end; ++ iter ) {
            saveToDb(iter->second, &channelDb);
            
            if ( 0 == ((++count)%500) )
                LOGGER_INFO << "Settlements saved: " << count << "\n";
        }
    }
    
    LOGGER_INFO << "DONE saving settlements to DB - count: " << count << "\n";
}

void InstrumentManager::saveToDb(const tw::instr::Settlement& x, tw::channel_db::ChannelDb* channelDb) {
    tw::channel_db::ChannelDb localChannelDb;
    if ( NULL == channelDb ) {
        channelDb = &localChannelDb;
        if ( !channelDb->init(_settings._db_connection_str) || !channelDb->isValid() )
            throw std::invalid_argument("failed to open db connection: " + _settings._db_connection_str);
    }
    
    tw::channel_db::ChannelDb::TConnectionPtr connection = channelDb->getConnection();
    tw::channel_db::ChannelDb::TStatementPtr statement = channelDb->getStatement(connection);

    tw::instr::Settlements_SaveInstrument query;
    query.execute(statement, x);    
}

} // namespace instr
} // namesapce tw
