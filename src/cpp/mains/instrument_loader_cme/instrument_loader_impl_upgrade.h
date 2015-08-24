#include "instrument_loader_upgrade.h"

#include <tw/common/fractional.h>
#include "tw/common/filesystem.h"
#include "tw/common/command.h"
#include <tw/common_thread/utils.h>
#include <tw/log/defs.h>
#include <tw/instr/instrument_manager.h>

#include "tw/generated/instrument.h"

#include <vector>
#include <boost/shared_ptr.hpp>
#include <string>
#include <deque>

typedef uint32_t tag;

const tag tag_CFICode = 461;
const tag tag_UnderlyingProduct = 462;
const tag tag_SecurityExchange = 207;
const tag tag_SecuritySymbol = 55;
const tag tag_SecurityDesc = 107;
const tag tag_SecurityGroup = 1151;
const tag tag_MinPriceIncrement = 969;
const tag tag_MinPriceIncrementAmount = 1146;
const tag tag_UnitOfMeasuryQty = 1147;
const tag tag_DisplayFactor = 9787;
const tag tag_TradingReferencePrice = 1150;
const tag tag_NoEvents = 864;
const tag tag_EventType = 865;
const tag tag_EventDate = 866;
const tag tag_EventTime = 1145;
const tag tag_SecurityId = 48;

const tag tag_NbInstAttrib = 870;
const tag tag_InstAttribType = 871;
const tag tag_InstAttribValue = 872;

const tag tag_NoLegs = 555;
const tag tag_LegRatioQty = 623;
const tag tag_LegSecurityId = 602;
const tag tag_LegSide = 602;

bool checkRequireFields(const OnixS::CME::MarketData::Message& message) {
    if ( !message.get(tag_CFICode) )
        return false;                

    if ( !message.get(tag_UnderlyingProduct) )
        return false;

    if ( !message.get(tag_SecurityExchange) )
        return false;
    
    if ( !message.get(tag_SecuritySymbol) )
        return false;
    
    if ( !message.get(tag_SecurityDesc) )
        return false;

    if ( !message.get(tag_SecurityGroup) )
        return false;

    if ( !message.get(tag_MinPriceIncrement) )
        return false;
    
    if ( !message.get(tag_MinPriceIncrementAmount) )
        return false;
    
    if ( !message.get(tag_UnitOfMeasuryQty) )
        return false;
    
    if ( !message.get(tag_DisplayFactor) )
        return false;
    
    if ( !message.get(tag_NoEvents) )
        return false;
    
    if ( !message.get(tag_SecurityId) )
        return false;
    
    return true;
}

InstrumentLoader::InstrumentLoader() {
}

InstrumentLoader::~InstrumentLoader() {
}
    
void InstrumentLoader::clear() {
    _channelsStatus.clear();
    _securityIdToDisplayName.clear();
}

bool InstrumentLoader::loadAndUpdateInstrumentsFees(const tw::common::Settings& settings) {
    try {
        tw::channel_db::ChannelDb channelDb;
    
        if ( !channelDb.init(settings._db_connection_str) || !channelDb.isValid() )
            throw std::invalid_argument("failed to open db connection: " + settings._db_connection_str);

        tw::instr::FeesAndMsgRatiosCME_GetAll query;
        if ( !query.execute(channelDb) )
            throw std::invalid_argument("failed to execute FeesAndMsgRatiosCME_GetAll query on connection: " + settings._db_connection_str);

        // Get all instruments for CME
        //
        tw::instr::InstrumentManager::TInstruments instruments = tw::instr::InstrumentManager::instance().getByExchange(tw::instr::eExchange::kCME);
        if ( instruments.empty() ) {
                LOGGER_WARN << "No instruments configured for CME on connection: " << settings._db_connection_str;
        }
        
        // Update instruments with configured fees
        //
        for ( size_t i = 0; i < query._o1.size(); ++i ) {
            tw::instr::InstrumentManager::TInstruments::iterator iter = instruments.begin();
            tw::instr::InstrumentManager::TInstruments::iterator end = instruments.end();
            for ( ; iter != end; ++iter ) {
                tw::instr::Instrument& instrument = *(iter->get());
                if ( instrument._symbol == query._o1[i]._symbol ) {
                    instrument._feeExLiqAdd = query._o1[i]._feeEx;
                    instrument._feeExLiqRem = query._o1[i]._feeEx;
                    instrument._feeExClearing = query._o1[i]._feeExClearing;
                    instrument._feeBrokerage = query._o1[i]._feeBrokerage;
                    instrument._feePerTrade = query._o1[i]._feePerTrade;
                }
            }                
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

bool InstrumentLoader::load(const tw::common::Settings& settings) {
    bool status = true;
    try {
        // Get and save CME xml files from FTP
        //
        
        // Load instruments
        //
        if ( !tw::instr::InstrumentManager::instance().loadInstruments(settings, false) )
            return false;
        
        // Get all configured fees and update instruments
        //
        if ( !loadAndUpdateInstrumentsFees(settings) )
            return false;
        
        // Create and use CME channel to get security definitions
        //
        tw::channel_pf_cme::ChannelPfOnix channelCme;
        if ( !channelCme.init(settings) )
            return false;
        
        // Remember all channel ids to get security definitions from
        //
        tw::channel_pf_cme::Settings::TChannelIds::const_iterator iter = channelCme.getSettings()._channelIds.begin();
        tw::channel_pf_cme::Settings::TChannelIds::const_iterator end = channelCme.getSettings()._channelIds.end();
        
        for ( ; iter!= end; ++iter ) {
            _channelsStatus[OnixS::CME::MarketData::ChannelId(*iter)] = false;
        }
        
        // Start channelCme
        //
        if ( !channelCme.start(this) )
            return false;
        
        for ( uint64_t count = 0; !isDone(); ++count ) {
            tw::common_thread::sleep(1000);
            if ( !(count%10) ) {                
                // Lock for thread synchronization
                //
                tw::common_thread::LockGuard<TLock> lock(_lock);

                TChannelsStatus::iterator iter = _channelsStatus.begin();
                TChannelsStatus::iterator end = _channelsStatus.end();

                std::stringstream out;
                for ( ; iter != end; ++iter ) {
                    if ( !(iter->second) ) {
                        out << iter->first << " :: " << iter->second << "\n";
                    }                        
                }
                
                LOGGER_INFO << "Channels NOT DONE yet: " << "\n" << out.str();
            }
        }
        
        // Stop channelCme
        //
        channelCme.stop();
        
        // Check if need to update settlements
        //
        if ( settings._instruments_updateSettlements ) {
            // Loop through and process all settlement files
            //
            if ( tw::common::Filesystem::exists_dir(settings._instruments_settlementsPath) ) {
                tw::common::Filesystem::TFilesList files = tw::common::Filesystem::getDirFiles(settings._instruments_settlementsPath);
                if ( files.empty() ) {
                    LOGGER_ERRO << "Can't process settlements - no files in _instruments_settlementsPath: " << settings._instruments_settlementsPath << "\n";
                } else {
                    for ( size_t i = 0; i < files.size(); ++i ) {
                        processSettlementFile(files[i]);
                    }
                }
                
            } else {
                LOGGER_ERRO << "Can't process settlements - invalid _instruments_settlementsPath: " << settings._instruments_settlementsPath << "\n";
            }
        }
        
        LOGGER_INFO << "Channels DONE - saving instruments..." << "\n";
        
        // Save updated instruments
        //
        tw::instr::InstrumentManager::instance().saveInstruments();
        
        LOGGER_INFO << "DONE saving instruments" << "\n";
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }

    return status;
}

void InstrumentLoader::onSecurityDefinition(const OnixS::CME::MarketData::SecurityDefinition& securityDefinition,
                                            const OnixS::CME::MarketData::ChannelId& channelId) {
    try {
        
        const OnixS::CME::MarketData::Message& message = securityDefinition.message();
        if ( !checkRequireFields(message) )
            return;
        
        if ( message.get(tag_NoLegs) && message.getInt32(tag_NoLegs) > 1 )
            return;
        
        // TODO: for now loads only futures, NO options/spreads
        //
        std::string temp;
        
        temp = message.get(tag_CFICode).toString();
        if ( temp.empty() || temp[0] != 'F' )
            return;
        
        const std::string& displayName = message.get(tag_SecurityDesc).toString();
        bool insert = false;
        tw::instr::InstrumentPtr instrumentPtr = tw::instr::InstrumentManager::instance().getByDisplayName(displayName);
        if ( !instrumentPtr ) {
            instrumentPtr.reset(new tw::instr::Instrument());
            instrumentPtr->_keyId = message.getInt32(tag_SecurityId);
            instrumentPtr->_displayName = displayName;
            instrumentPtr->_exchange = tw::instr::eExchange::kCME;
            instrumentPtr->_instrType = tw::instr::eInstrType::kFuture;
            
            insert = true;
        }
        
        tw::instr::Instrument* instrument = const_cast<tw::instr::Instrument*>(instrumentPtr.get());
        tw::instr::Instrument instrumentPrevState = *instrument;
        
        {
            // Lock for thread synchronization
            //
            tw::common_thread::LockGuard<TLock> lock(_lock);            
            _securityIdToDisplayName[message.getInt32(tag_SecurityId)] = displayName;
        }
            
        instrument->_CFICode = temp;
            
        switch (message.getInt32(tag_UnderlyingProduct)) {
            case 2:
                instrument->_category._enum = tw::instr::eInstrCatg::kAgricultural;
                break;
            case 4:
                instrument->_category._enum = tw::instr::eInstrCatg::kCurrency;
                break;
            case 5:
                instrument->_category._enum = tw::instr::eInstrCatg::kEquity;
                break;
            case 12:
                instrument->_category._enum = tw::instr::eInstrCatg::kAlt;
                break;
            case 14:
                instrument->_category._enum = tw::instr::eInstrCatg::kInterestRates;
                break;
            case 16:
                instrument->_category._enum = tw::instr::eInstrCatg::kEnergy;
                break;
            case 17:
                instrument->_category._enum = tw::instr::eInstrCatg::kMetal;
                break;
            default:
                return;
        }
        
        // TODO: CME's security definitions don't appear to contain settlement type,
        // so hard coded for cash for now - need to implement correctly!
        //
        instrument->_settlType._enum = tw::instr::eSettlType::kCash;
        
        // TODO: hard coded to USD - is this correct?
        //
        instrument->_priceCurrency._enum = tw::instr::eCurrency::kUSD;
        
        instrument->_contractSize = message.getDecimal(tag_UnitOfMeasuryQty);
        
        double tickSize = message.getDecimal(tag_MinPriceIncrement);
        tw::common::Fractional::TFraction fraction = tw::common::Fractional::getFraction(tickSize);
        instrument->_tickNumerator = fraction.first;
        instrument->_tickDenominator = fraction.second;
        
        OnixS::CME::MarketData::Group group = message.getGroup(tag_NoEvents);
        for ( size_t index = 0; index < group.size(); ++index ) {
            OnixS::CME::MarketData::GroupInstance groupInstance = group[index];
            if ( groupInstance.get(tag_EventDate) && groupInstance.get(tag_EventDate).toString().size() > 7 ) {
                temp = groupInstance.get(tag_EventDate).toString();
                temp += " ";
                if ( groupInstance.get(tag_EventTime) && groupInstance.get(tag_EventTime).toString().size() > 8 )
                    temp += groupInstance.get(tag_EventTime).toString();
                else
                    temp += "000000000";

                temp.insert(4, "-");
                temp.insert(7, "-");
                temp.insert(13, ":");
                temp.insert(16, ":");
                temp.insert(19, ".");
                switch ( groupInstance.getInt32(tag_EventType) ) {
                    case 5:
                        instrument->_firstTradingDay = temp;
                        break;
                    case 7:
                        instrument->_lastTradingDay = temp;
                        instrument->_expirationDate = temp;                        
                        break;                                
                }
            } else {
                LOGGER_WARN << "displayName: " << displayName
                            << ",tag_EventDate=" << (groupInstance.get(tag_EventDate) ? groupInstance.get(tag_EventDate).toString() : "<NULL>")
                            << ",tag_EventTime=" << (groupInstance.get(tag_EventTime) ? groupInstance.get(tag_EventTime).toString() : "<NULL>")
                            << " are NOT in right format for index: " << index << " out of: " << group.size() << "\n";
            }
        }
        
        temp = message.get(tag_SecurityExchange).toString();
        if ( temp == "XCBT" )
            instrument->_subexchange._enum = tw::instr::eSubExchange::kCBOT;
        else if ( temp == "XCME" )
            instrument->_subexchange._enum = tw::instr::eSubExchange::kCME;
        else if ( temp == "XNYM" )
            instrument->_subexchange._enum = tw::instr::eSubExchange::kNYMEX;
        else if ( temp == "XCEC" )
            instrument->_subexchange._enum = tw::instr::eSubExchange::kCEC;
        else if ( temp == "XKBT" )
            instrument->_subexchange._enum = tw::instr::eSubExchange::kKBT;
        else if ( temp == "XMGE" )
            instrument->_subexchange._enum = tw::instr::eSubExchange::kMGE;
        else if ( temp == "DUMX" )
            instrument->_subexchange._enum = tw::instr::eSubExchange::kDUMX;            
        
        instrument->_mdVenue = "CME Globex";
        
        instrument->_keyNum1 = instrument->_keyNum2 = message.getInt32(tag_SecurityId);
        instrument->_keyStr1 = instrument->_keyStr2 = message.get(tag_SecurityId).toString();
        
        instrument->_var1 = channelId.data();
        instrument->_instrType._enum = tw::instr::eInstrType::kFuture;
        
        instrument->_displayFormat = message.getDecimal(tag_DisplayFactor);
        
        instrument->_productCode = message.get(tag_SecurityGroup).toString();
        
        // NOTE: CBOT interest rate (tw::instr::eInstrCatg::kInterestRates == instrument->_category) futures and agricultural futures for corn (Symbol(55)=ZC),
        // soybeans (Symbol(55)=ZS), wheat (Symbol(55)=ZW), oats (Symbol(55)=ZO), hogs (Symbol(55)=HE), cattle (Symbol(55)=LE)  don't conform to formula:
        //      tickValue = tickSize * contractSize * displayFormat
        // but rather to:
        //      tickValue = tickSize * contractSize * displayFormat / 100        
        // TODO: ALSO EuroDollars (Symbol(55)=GE) futures don't conform to formula above.
        // It appears that unsupported (according to CME) tag_MinPriceIncrementAmount(1146)
        // has the right value for tickValue for EuroDollars, so for now using that value - 
        // need to monitor security definitions changes since using currently unsupported
        // by CME field
        //
        instrument->_symbol =  message.get(tag_SecuritySymbol).toString();
        instrument->_tickValue = tickSize * instrument->_contractSize * instrument->_displayFormat;
        if ( tw::instr::eInstrCatg::kInterestRates == instrument->_category || "ZC" == instrument->_symbol || "ZS" == instrument->_symbol || "ZW" == instrument->_symbol || "ZO" == instrument->_symbol || "HE" == instrument->_symbol || "LE" == instrument->_symbol )
            instrument->_tickValue /= 100;
        
        if ( "GE" == instrument->_symbol && "GE" == instrument->_productCode )
            instrument->_tickValue = message.getDecimal(tag_MinPriceIncrementAmount);
        
        instrument->_numberOfLegs = 0;        
        if ( message.get(tag_NoLegs) ) {
            // TODO: currently support spreads with only up to 4 legs
            //
            group = message.getGroup(tag_NoLegs);
            if ( group.size() <= 4 ) {
                instrument->_instrType._enum = tw::instr::eInstrType::kSpread;
                instrument->_numberOfLegs = group.size();
                for ( size_t index = 0; index < group.size(); ++index ) {
                    OnixS::CME::MarketData::GroupInstance groupInstance = group[index];
                    temp = groupInstance.get(tag_LegSecurityId).toString();
                    int32_t ratio = ( groupInstance.getInt32(tag_LegSide) != 2 ) ? groupInstance.getInt32(tag_LegRatioQty) : -1 *  groupInstance.getInt32(tag_LegRatioQty);
                    switch ( index ) {
                        case 0:
                            instrument->_displayNameLeg1 = temp;
                            instrument->_ratioLeg1 = ratio;                    
                            break;
                        case 1:
                            instrument->_displayNameLeg2 = temp;
                            instrument->_ratioLeg2 = ratio;
                            break;
                        case 2:
                            instrument->_displayNameLeg3 = temp;
                            instrument->_ratioLeg3 = ratio;
                            break;
                        case 3:
                            instrument->_displayNameLeg4 = temp;
                            instrument->_ratioLeg4 = ratio;
                            break;
                    }            
                }
            }
        }
        
        // TODO: Not setting the following (for now):
        //
        //instrument->_exchange._enum = tw::instr::eExchange::kCME;
        //instrument->_description;
        
        if ( instrument->isEqual(instrumentPrevState) ) {
            instrument->_dirty = false;
            return;
        }
        
        if ( insert )
            tw::instr::InstrumentManager::instance().addInstrument(instrumentPtr);
        
        double minPriceIncrement = message.getDecimal(tag_MinPriceIncrement);
        double unitOfMeasuryQty = message.getDecimal(tag_UnitOfMeasuryQty);
        double minPriceIncrementAmount = message.getDecimal(tag_MinPriceIncrementAmount);
        double displayFactor = message.getDecimal(tag_DisplayFactor);
        
        instrument->_dirty = true;
        LOGGER_INFO << "FIX Message: " << message.toString() << "\n";
        LOGGER_INFO << "Loaded: " << instrument->toStringVerbose() << "\n";
        
        std::stringstream os;
        os << "\n";
        os << "Symbol(107): " << std::setw(10) << std::right <<  message.get(tag_SecurityDesc);
        os << std::setw(20) << std::right;
        os << "MinPrIncr(969): " << std::setw(10) << std::right <<  minPriceIncrement;
        os << std::setw(30) << std::right;
        os << "UnitOfMsrQty(1147): " << std::setw(10) << std::right << unitOfMeasuryQty;
        os << std::setw(30) << std::right;
        os << "MinPrIncrAmnt(1146): " << std::setw(10) << std::right << minPriceIncrementAmount;
        os << std::setw(20) << std::right;
        os << "DispFactr(9787): " << std::setw(10) << std::right << displayFactor;
        os << "\n";
        os << "MinPrIncr(969)*UnitOfMsrQty(1147)*DispFactr(9787): ";
        os << minPriceIncrement << "*";
        os << unitOfMeasuryQty << "*";
        os << displayFactor << "=";
        os << (minPriceIncrement*unitOfMeasuryQty*displayFactor);
        
        LOGGER_INFO << os.str() << "\n";
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void InstrumentLoader::onSecurityDefinitionDeleted(OnixS::CME::MarketData::SecurityId securityId,
                                                   const OnixS::CME::MarketData::ChannelId& channelId) {
    // Not implemented: nothing to do
    //
}

void InstrumentLoader::onSecurityDefinitionsRecoveryStarted(const OnixS::CME::MarketData::ChannelId& channelId) {
    // Not implemented: nothing to do
    //
}

void InstrumentLoader::onSecurityDefinitionsRecoveryFinished(const OnixS::CME::MarketData::ChannelId& channelId){
    try {
        {
            // Lock for thread synchronization
            //
            tw::common_thread::LockGuard<TLock> lock(_lock);
        
            TChannelsStatus::iterator iter = _channelsStatus.find(channelId);
            if ( iter == _channelsStatus.end() ) {
                LOGGER_ERRO << "Can't find channelId: " << channelId.data() << "\n";
                return;
            }
            
            iter->second = true;
        }
        
        {
            // Substitute all securityIds with displayNames
            //
            tw::instr::eExchange exchange;
            exchange._enum = tw::instr::eExchange::kCME;
            tw::instr::InstrumentManager::TInstruments instruments = tw::instr::InstrumentManager::instance().getByExchange(exchange);
            tw::instr::InstrumentManager::TInstruments::iterator iter = instruments.begin();
            tw::instr::InstrumentManager::TInstruments::iterator end = instruments.end();

            for ( ; iter != end; ++iter ) {
                if ( (*iter)->_instrType._enum == tw::instr::eInstrType::kSpread ) {
                    for ( size_t index = 0; index < (*iter)->_numberOfLegs; ++index ) {
                        switch ( index ) {
                            case 0:                                
                                (*iter)->_displayNameLeg1 = getDisplayName((*iter)->_displayNameLeg1);
                                break;
                            case 1:
                                (*iter)->_displayNameLeg2 = getDisplayName((*iter)->_displayNameLeg2);
                                break;
                            case 2:
                                (*iter)->_displayNameLeg3 = getDisplayName((*iter)->_displayNameLeg3);
                                break;
                            case 3:
                                (*iter)->_displayNameLeg4 = getDisplayName((*iter)->_displayNameLeg4);
                                break;
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
}
    
bool InstrumentLoader::isDone() {
    // Lock for thread synchronization
    //
    tw::common_thread::LockGuard<TLock> lock(_lock);
        
    TChannelsStatus::iterator iter = _channelsStatus.begin();
    TChannelsStatus::iterator end = _channelsStatus.end();
    
    for ( ; iter != end; ++iter ) {
        if ( !(iter->second) )
            return false;
    }
    
    return true;
}

std::string InstrumentLoader::getDisplayName(const std::string& securityId) {
    // Lock for thread synchronization
    //
    tw::common_thread::LockGuard<TLock> lock(_lock);
    
    TSecurityIdToDisplayName::iterator iter = _securityIdToDisplayName.find(::atol(securityId.c_str()));
    if ( iter != _securityIdToDisplayName.end() )
        return iter->second;
    
    return securityId;
}

void InstrumentLoader::processSettlementFile(const std::string& filename) {
    try {
        std::ifstream reader;
       
        reader.open(filename.c_str(), std::ios_base::in);
        if( !reader.good() ) {
            LOGGER_ERRO << "failed to open (reader) file: " << filename << "\n";
            return;
        }
        
        std::string expDate;
        tw::instr::Settlement s;        
        s._exchange = tw::instr::eExchange::kCME;
        
        reader.seekg(0, std::ios::beg);
        std::string line;
        while ( !reader.eof() ) {
            std::getline(reader, line);
            if ( !line.empty() ) {
                tw::common::Command cmnd;
                if ( cmnd.fromString(std::string("InstrumentLoader,Settlement,")+line) ) {
                    if ( !cmnd.get("sym", s._displayName) || !cmnd.get("exp", expDate) || !cmnd.get("settlPrice", s._settlPrice) || !cmnd.get("high", s._high) || !cmnd.get("low", s._low) || !cmnd.get("settlDate", s._settlDate) )
                        continue;

                    uint32_t month = ::atol(expDate.substr(4, 2).c_str());
                    switch (month) {
                        case 1: s._displayName += "F"; break;
                        case 2: s._displayName += "G"; break;
                        case 3: s._displayName += "H"; break;
                        case 4: s._displayName += "J"; break;
                        case 5: s._displayName += "K"; break;
                        case 6: s._displayName += "M"; break;
                        case 7: s._displayName += "N"; break;
                        case 8: s._displayName += "Q"; break;
                        case 9: s._displayName += "U"; break;
                        case 10: s._displayName += "V"; break;
                        case 11: s._displayName += "X"; break;
                        case 12: s._displayName += "Z"; break;
                    }
                    
                    s._displayName += expDate.substr(3, 1);
                    tw::instr::InstrumentPtr instrument = tw::instr::InstrumentManager::instance().getByDisplayName(s._displayName);
                    if ( instrument ) {
                        LOGGER_INFO << "Processing settlement for: " << s._displayName << ": " << line << " :: " << filename << "\n";
                    
                        boost::erase_all(s._settlDate, "-");

                        // !!!! - TODD: for now set 'close' to settlPrice - need tp obtain from CQG?
                        //
                        if ( !cmnd.get("close", s._close) )
                            s._close = s._settlPrice;

                        tw::instr::InstrumentManager::setTickConverter(instrument);

                        tw::price::Ticks high = instrument->_tc->fromExchangePrice(boost::lexical_cast<double>(s._high)/instrument->_displayFormat);
                        tw::price::Ticks low = instrument->_tc->fromExchangePrice(boost::lexical_cast<double>(s._low)/instrument->_displayFormat);                    
                        tw::price::Ticks close = instrument->_tc->fromExchangePrice(boost::lexical_cast<double>(s._close)/instrument->_displayFormat);
                        double settlPrice = instrument->_tc->fractionalTicks(boost::lexical_cast<double>(s._settlPrice)/instrument->_displayFormat);

                        // Calculate PivotPoint related stats and add to InstrumentManager
                        //
                        tw::instr::InstrumentManager::instance().calcSuppRest(high, low, close, settlPrice, instrument, s);
                        tw::instr::InstrumentManager::instance().getOrCreateSettlement(s._exchange, s._displayName, s._settlDate) = s;
                    }
                } else {
                    LOGGER_WARN << "failed to process settlement: " << line << " :: " << filename << "\n";
                }
            }
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}
    
