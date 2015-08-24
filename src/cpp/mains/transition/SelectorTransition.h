#pragma once

#include <tw/log/logger.h>
#include <tw/config/settings.h>
#include <tw/common/high_res_time.h>
#include <tw/risk/risk_storage.h>
#include "TransitionImpl.h"
#include <vector>

namespace tw {
namespace transition {
    
    class SelectorTransition {
    public:
        typedef std::vector<TransitionParams> TPortfolio;
        
    public:
        SelectorTransition() {            
        }
        
        bool run(const tw::common::Settings& settings) {
            try {
                if ( !_channelDb.init(settings._db_connection_str) )
                    return false;
                _con = _channelDb.getConnection();
                tw::channel_db::ChannelDb::TStatementPtr stmt = _channelDb.getStatement(_con);
                
                tw::risk::Account account;
                if ( !tw::risk::RiskStorage::instance().getAccount(account, settings._trading_account) ) {
                    LOGGER_ERRO << "No configuration for account: " << settings._trading_account << "\n";
                    return false;
                }

                Transition_GetAllForAccount query;
                if ( !query.execute(_channelDb, account._id) ) {
                    LOGGER_ERRO << "Failed to execute Transition_GetAllForAccount query for account: " << account.toString() << "\n" << "\n";      
                    return false;
                }

                if ( query._o1.empty() ) {
                    LOGGER_ERRO << "Transition portfolio is empty for account: " << account << "\n" << "\n";      
                    return false;
                }
                
                std::vector<TransitionParamsWire> o;
                for ( size_t i = 0; i < query._o1.size(); ++i ) {
                    if ( query._o1[i]._enabled )
                        o.push_back(query._o1[i]);
                }
                
                if ( o.empty() ) {
                    LOGGER_ERRO << "Transition portfolio is not enabled for account: " << account << "\n" << "\n";      
                    return false;
                }

                _portfolio.resize(o.size());
                LOGGER_INFO << "Initializing pivot portfolio of size = " << _portfolio.size() << "\n";
                int32_t utcOffset = tw::common::THighResTime::getUtcOffset();
                std::string prefix = tw::common::THighResTime::nowDate();
                LOGGER_INFO << "Selector(): utcOffset=" << utcOffset << ", prefix=" << prefix << "\n";
                std::string start_h;
                std::string start_m;
                std::string start_s;
                std::string close_h;
                std::string close_m;
                std::string close_s;
                std::string force_h;
                std::string force_m;
                std::string force_s;
                
                for ( size_t i = 0; i < o.size(); ++i ) {
                    TPortfolio::value_type& p = _portfolio[i];
                    static_cast<TransitionParamsWire&>(p) = o[i];
                    if ( !getInstrument(p._displayName, p._exchange, p._instrument) )
                        return false;
                    formatTime(p._start_h+utcOffset, start_h); 
                    formatTime(p._start_m, start_m);
                    formatTime(p._start_s, start_s);
                    formatTime(p._close_h+utcOffset, close_h);   
                    formatTime(p._close_m, close_m);
                    formatTime(p._close_s, close_s);
                    formatTime(p._force_h+utcOffset, force_h);
                    formatTime(p._force_m, force_m);
                    formatTime(p._force_s, force_s);
                    p._startTime = prefix.substr(0,4) + "-" + prefix.substr(4,2) + "-" + prefix.substr(6,2) + " " + start_h + ":" + start_m + ":" + start_s;
                    p._closeTime = prefix.substr(0,4) + "-" + prefix.substr(4,2) + "-" + prefix.substr(6,2) + " " + close_h + ":" + close_m + ":" + close_s;
                    p._forceTime = prefix.substr(0,4) + "-" + prefix.substr(4,2) + "-" + prefix.substr(6,2) + " " + force_h + ":" + force_m + ":" + force_s;
                    LOGGER_INFO << "Processed Parameters for [" << p._exchange.toString() << ":" << p._displayName << "]" << "\n" << p.toStringVerbose() << "\n";
                }
            } catch (sql::SQLException &e) {
                LOGGER_ERRO << "exception :: " << e.what() << "\n";
                LOGGER_ERRO << "MySQL error code: " << e.getErrorCode() << "\n";
                LOGGER_ERRO << "SQLState: " << e.getSQLState() << " )" << "\n";
            } catch(const std::exception& e) {
                LOGGER_ERRO << "exception :: " << e.what() << "\n" << "\n";      
                return false;
            } catch(...) {
                LOGGER_ERRO << "exception :: <unknown>" << "\n" << "\n";     
                return false;
            }

            return true;
        }
       
    public:
        TPortfolio& getPortfolio() {
            return _portfolio;
        }
        
        const TPortfolio& getPortfolio() const {
            return _portfolio;
        }
        
    protected:
        void formatTime(int32_t h, std::string& v) {
            if ( h < 10 )
                v = "0" + boost::lexical_cast<std::string>(h);
            else
                v = boost::lexical_cast<std::string>(h);
        }
                
        bool getInstrument(const std::string& displayName, tw::instr::eExchange exchange, tw::instr::InstrumentPtr& instr) {
            try {
                instr = tw::instr::InstrumentManager::instance().getByDisplayName(displayName);
                if ( NULL ==  instr ) {
                    LOGGER_ERRO << "InstrumentManager doesn't have displayName: " << displayName << "\n";
                    return false;
                }

                if ( !tw::channel_or::ProcessorRisk::instance().isTradeable(instr->_keyId) ) {
                    LOGGER_ERRO << "ProcessorRisk doesn't have or is not enabled for displayName: " << displayName << "\n";
                    return false;
                }

                return true;
            } catch(const std::exception& e) {
                LOGGER_ERRO << "exception :: " << e.what() << "\n" << "\n";      
            } catch(...) {
                LOGGER_ERRO << "exception :: <unknown>" << "\n" << "\n";     
            }

            return false;
        }
        
    protected:
        tw::channel_db::ChannelDb _channelDb;
        tw::channel_db::ChannelDb::TConnectionPtr _con;
        TPortfolio _portfolio;
    };
}
}

