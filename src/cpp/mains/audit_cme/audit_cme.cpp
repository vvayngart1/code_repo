#include "audit_cme.h"

#include <tw/common/filesystem.h>
#include <tw/channel_db/channel_db.h>
#include <tw/channel_or_cme/settings.h>
#include <tw/generated/channel_or_defs.h>

#include <OnixS/FIXEngine.h>

#include <fstream>
#include <bits/basic_string.h>

typedef std::vector<tw::channel_or::FixSessionCMEMsgForAudit> TRecords;

std::string getValue(const OnixS::FIX::Message& message, int tag) {    
    if ( message.contain(tag) )
        return std::replace_all(message.get(tag), ",", "_");
    
    return std::string("");
}

AuditCme::AuditCme() {
}

AuditCme::~AuditCme() {
}

bool AuditCme::process(const tw::common::Settings& settings) {
    try {
        // Backup any existent file
        //
        if ( tw::common::Filesystem::exists(settings._audit_cme_outputFileName) ) {
            if ( !tw::common::Filesystem::swap(settings._audit_cme_outputFileName) ) {
                LOGGER_ERRO << "Can't backup file: "  << settings._audit_cme_outputFileName << "\n";
                return false;
            }
        }
        
        if ( !tw::instr::InstrumentManager::instance().loadInstruments(settings, false) )
            return false;
        
        // Open file to output to
        //
        std::ofstream writer;
        writer.open(settings._audit_cme_outputFileName.c_str(), std::ios_base::out | std::ios_base::app);
        if( !writer.good() ) {
            LOGGER_ERRO << "Can't open for writing file: "  << settings._audit_cme_outputFileName << "\n";
            return false;
        }
        
        // Start global OnixS engine for all CME connections
        //
        tw::channel_or_cme::SettingsGlobal settingsCme;
        if ( !settingsCme.parse(settings._channel_or_cme_dataSource) ) {
            LOGGER_ERRO << "Failed to parse cme settings: "  << settings._channel_or_cme_dataSource << "\n";
            return false;
        }
        
        OnixS::FIX::EngineSettings engineSettings;
        engineSettings.dialect(settingsCme._global_fixDialectDescriptionFile);
        engineSettings.licenseStore(settingsCme._global_licenseStore);
        engineSettings.logDirectory(settingsCme._global_logDirectory);
        
        OnixS::FIX::Engine::init(engineSettings);
        
        
        tw::channel_db::ChannelDb channelDb;
        
        if ( !channelDb.init(settings._db_connection_str) ) {
            LOGGER_ERRO << "Failed to init channelDb with: "  << settings._db_connection_str << "\n";
            return false;
        }
        
        tw::channel_or::FixSessionCMEMsgs_GetAll query;
        if ( !query.execute(channelDb) ) {
            LOGGER_ERRO << "Failed to execute FixSessionCMEMsgs_GetAll() for db: "  << settings._db_connection_str << "\n";
            return false;
        }
        
        writer <<  tw::channel_or::AuditCme::header() << std::endl;
        
        TRecords::iterator iter = query._o1.begin();
        TRecords::iterator end = query._o1.end();
        
        for ( ; iter != end; ++iter ) {
            tw::channel_or::FixSessionCMEMsgForAudit& record = (*iter);
            
            OnixS::FIX::Message message(record._message);
            tw::channel_or::AuditCme auditRecord;
            
            if ( !message.getType().empty() ) {
                switch ( message.getType()[0]) {
                    case 'D':   // New order single
                        auditRecord._messageType = "NEW ORDER";
                        break;
                    case 'F':   // Cancel
                        auditRecord._messageType = "CANCEL";
                        break;
                    case 'G':   // Modify
                        auditRecord._messageType = "MODIFY";
                        break;
                    case '8':   // Execution report
                    {
                        auditRecord._messageType = "EXECUTION";
                        switch ( message.get(OnixS::FIX::FIX42::Tags::ExecType)[0] ) {
                            case '0':                            
                                auditRecord._orderStatus = "NEW ORDER ACK";
                                break;
                            case '1':
                                auditRecord._orderStatus = "PARTIAL FILL";
                                auditRecord._lastShares = getValue(message, OnixS::FIX::FIX42::Tags::LastShares);
                                auditRecord._fillPrice = getValue(message, OnixS::FIX::FIX42::Tags::LastPx);
                                break;
                            case '2':
                                auditRecord._orderStatus = "COMPLETE FILL";
                                auditRecord._lastShares = getValue(message, OnixS::FIX::FIX42::Tags::LastShares);
                                auditRecord._fillPrice = getValue(message, OnixS::FIX::FIX42::Tags::LastPx);
                                break;
                            case '4':
                                auditRecord._orderStatus = "CANCEL ACK";
                                break;
                            case '5':
                                auditRecord._orderStatus = "MODIFY ACK";
                                break;
                            case '8':
                                auditRecord._orderStatus = "ORDER REJECTED";
                                break;
                            case 'C':
                                auditRecord._orderStatus = "EXPIRED";
                                break;
                            case 'H':
                                auditRecord._orderStatus = "TRADE CANCEL";
                                break;                            
                            default:                                
                                break;
                        }                        
                    }
                        auditRecord._reasonCode = getValue(message, OnixS::FIX::FIX42::Tags::OrdRejReason);
                        break;
                    case '9':   // Order Cancel reject
                        auditRecord._messageType = "ORDER_CANCEL_REJECT";
                        auditRecord._reasonCode = getValue(message, OnixS::FIX::FIX42::Tags::CxlRejReason);
                        break;
                    case '3':   // Business reject
                        auditRecord._messageType = "BUSINESS_REJECT";
                        auditRecord._reasonCode = getValue(message, OnixS::FIX::FIX42::Tags::Text);
                        break;
                    default:
                        continue;
                }
                
                auditRecord._serverTransactionNumber = boost::lexical_cast<std::string>(record._index);
                auditRecord._serverTimestamp = getValue(message, OnixS::FIX::FIX42::Tags::SendingTime);
                auditRecord._senderLocationID = getValue(message, OnixS::FIX::FIX42::Tags::SenderLocationID);
                auditRecord._manualOrderIdentifier = getValue(message, 1028);
                auditRecord._exchangeCode = "CME";              // TODO: what values to put?
                auditRecord._origin = "0";                      // NOTE: Per Ron Timpone from RCG as noted in email from 03/04/2015
                if ( tw::channel_or::eDirection::kOutbound == record._direction ) {
                    auditRecord._messageDirection = "TO CME";       // NOTE: Per Ron Timpone from RCG email from 03/05/2015
                    std::string senderCompId = getValue(message, OnixS::FIX::FIX42::Tags::SenderCompID);
                    if ( senderCompId.length() > 3 )
                        auditRecord._sessionID = senderCompId.substr(0, 3);
                    
                    if ( senderCompId.length() > 5 )
                        auditRecord._executingFirmNumber = senderCompId.substr(3, 3);
                    
                    auditRecord._tag50 = getValue(message, OnixS::FIX::FIX42::Tags::SenderSubID);
                } else {
                    auditRecord._messageDirection = "FROM CME";     // NOTE: Per Ron Timpone from RCG email from 03/05/2015
                    std::string targetCompId = getValue(message, OnixS::FIX::FIX42::Tags::TargetCompID);
                    if ( targetCompId.length() > 3 )
                        auditRecord._sessionID = targetCompId.substr(0, 3);
                    
                    if ( targetCompId.length() > 5 )
                        auditRecord._executingFirmNumber = targetCompId.substr(3, 3);
                    
                    auditRecord._tag50 = getValue(message, OnixS::FIX::FIX42::Tags::TargetSubID);
                }
                auditRecord._status = "OK";                     // TODO: what values to put?
                
                auditRecord._accountNumber = getValue(message, OnixS::FIX::FIX42::Tags::Account);
                auditRecord._clientOrderID = getValue(message, OnixS::FIX::FIX42::Tags::ClOrdID);
                auditRecord._correlationClOrdID = getValue(message, 9717);
                auditRecord._hostOrderNumber = getValue(message, OnixS::FIX::FIX42::Tags::OrderID);
                
                if ( message.contain(OnixS::FIX::FIX42::Tags::Side) ) {
                    if ( message.get(OnixS::FIX::FIX42::Tags::Side) == OnixS::FIX::FIX42::Values::Side::Buy )
                        auditRecord._buyOrSellIndicator = "B";
                    else if ( message.get(OnixS::FIX::FIX42::Tags::Side) == OnixS::FIX::FIX42::Values::Side::Sell )
                        auditRecord._buyOrSellIndicator = "S";
                    else
                        auditRecord._buyOrSellIndicator = "?";
                }
                
                auditRecord._orderQty = getValue(message, OnixS::FIX::FIX42::Tags::OrderQty);
                auditRecord._maxShow = getValue(message, OnixS::FIX::FIX42::Tags::MaxShow);
                auditRecord._securityDesc = getValue(message, OnixS::FIX::FIX42::Tags::SecurityDesc);
                auditRecord._symbol = getValue(message, OnixS::FIX::FIX42::Tags::Symbol);
                auditRecord._strikePrice = getValue(message, OnixS::FIX::FIX42::Tags::StrikePrice);
                auditRecord._limitPrice = getValue(message, OnixS::FIX::FIX42::Tags::Price);
                auditRecord._stopPrice = getValue(message, OnixS::FIX::FIX42::Tags::StopPx);
                
                if ( message.contain(OnixS::FIX::FIX42::Tags::OrdType) ) {
                    if ( message.get(OnixS::FIX::FIX42::Tags::OrdType) == OnixS::FIX::FIX42::Values::OrdType::Limit )
                        auditRecord._orderType = "2";
                    else
                        auditRecord._orderType = "?";
                }
                
                if ( !message.contain(OnixS::FIX::FIX42::Tags::TimeInForce) ) {
                    auditRecord._orderQualifier = "Day";
                } else {
                    if ( message.get(OnixS::FIX::FIX42::Tags::TimeInForce) == OnixS::FIX::FIX42::Values::TimeInForce::Day )
                        auditRecord._orderQualifier = "Day";
                    else if ( message.get(OnixS::FIX::FIX42::Tags::TimeInForce) == OnixS::FIX::FIX42::Values::TimeInForce::Good_Till_Cancel )
                        auditRecord._orderQualifier = "GTC";
                    else
                        auditRecord._orderQualifier = "?";
                }
                
                if ( message.contain(OnixS::FIX::FIX42::Tags::CustomerOrFirm) ) {
                    if ( message.get(OnixS::FIX::FIX42::Tags::CustomerOrFirm) == OnixS::FIX::FIX42::Values::CustomerOrFirm::Customer )
                        auditRecord._customerTypeIndicator = "4";
                    else if ( message.get(OnixS::FIX::FIX42::Tags::CustomerOrFirm) == OnixS::FIX::FIX42::Values::CustomerOrFirm::Firm )
                        auditRecord._customerTypeIndicator = "2";
                    else
                        auditRecord._customerTypeIndicator = "?";
                }
                
                auditRecord._giveUpFirm = "";           // TODO: what values to put?
                auditRecord._giveUpIndicator = "";      // TODO: what values to put?
                auditRecord._allocAccount = "";         // TODO: what values to put?
                
                tw::instr::InstrumentPtr instrument = tw::instr::InstrumentManager::instance().getByDisplayName(auditRecord._securityDesc);
                if ( instrument ) {
                    auditRecord._CFICode = instrument->_CFICode;
                    if ( instrument->_expirationDate.length() > 6 )
                        auditRecord._maturityDate = instrument->_expirationDate.substr(0, 7);
                    else
                        auditRecord._maturityDate = instrument->_expirationDate;
                } else {                
                    auditRecord._CFICode = getValue(message, OnixS::FIX::FIX43::Tags::CFICode);
                    auditRecord._maturityDate = getValue(message, OnixS::FIX::FIX42::Tags::MaturityMonthYear);
                    auditRecord._maturityDate += " ";
                    auditRecord._maturityDate += getValue(message, OnixS::FIX::FIX42::Tags::MaturityDay);
                }
                
                writer << auditRecord.toString() << std::endl;
            }
        }
        
        // Stop global engine
        //
        OnixS::FIX::Engine::shutdown();
        
        writer.close();
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    
    return true;
}
