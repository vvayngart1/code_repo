#pragma once

#include <tw/log/logger.h>
#include <tw/config/settings.h>
#include <tw/risk/risk_storage.h>
#include <tw/generated/channel_or_defs.h>

#include <vector>

namespace tw {
namespace pnl_audit {
    
    struct  PnLAuditTrail_GetAllAccounts {    
        bool execute(tw::channel_db::ChannelDb& channelDb)
        {
            bool status = true;
            std::stringstream sql;
            try {
                sql << "SELECT distinct `accountId` FROM PnLAuditTrail order by cast(`accountId` as unsigned)";
                tw::channel_db::ChannelDb::TConnectionPtr connection = channelDb.getConnection();
                tw::channel_db::ChannelDb::TStatementPtr statement = channelDb.getStatement(connection);
                tw::channel_db::ChannelDb::TResultSetPtr res = channelDb.executeQuery(statement, sql.str());

                if ( !res ) {
                    LOGGER_ERRO << "res is NULL " << "\n";
                    return false;
                }
                
                while (res->next()) {
                    _o1.push_back(res->getString("accountId"));
                }
            } catch(const std::exception& e) {            
                status = false;
                LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
            } catch(...) {
                status = false;
                LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";        
            }
            
            if ( !status ) {
                try {
                    LOGGER_ERRO << "Failed to execute sql: " << sql.str() << "\n" << "\n";
                } catch(...) {
                }
            }
            
            return status;
        }       
        
        std::vector<std::string> _o1;
    };
    
    class SelectorPnLAudit {
    public:
        typedef std::vector<tw::channel_or::PnLAuditTrailInfo> TPnLAuditTrailInfos;
        
    public:
        SelectorPnLAudit() {            
        }
        
        bool start(tw::common::Settings& settings) {
            try {
                if ( settings._pnl_audit_start_time.empty() )
                    settings._pnl_audit_start_time = tw::common::THighResTime::now().sqlString().substr(0, 10);
                
                if ( settings._pnl_audit_end_time.empty() )
                    settings._pnl_audit_end_time = tw::common::THighResTime::now().sqlString();
                
                if ( !_channelDb.init(settings._db_connection_str) )
                    return false;

                _con = _channelDb.getConnection();
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
        
        bool populateAccounts(const tw::common::Settings& settings) {
            try {
                _accounts.clear();
                
                LOGGER_INFO << "Running PnLAuditTrail_GetAllAccounts query" << "\n";

                PnLAuditTrail_GetAllAccounts query;
                if ( !query.execute(_channelDb) ) {
                    LOGGER_ERRO << "Failed to execute PnLAuditTrail_GetAllAccounts query" << "\n";
                    return false;
                }

                if ( query._o1.empty() ) {
                    LOGGER_ERRO << "Accounts are empty" << "\n";
                    return false;
                }
                
                _accounts = query._o1;
                
                LOGGER_INFO << "Accounts number of records: " << _accounts.size() << "\n";
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
        
        bool populatePnLAuditTrailInfosForAccount(const tw::common::Settings& settings, const std::string& account) {
            try {
                _pnlAuditTrailInfos.clear();
                
                LOGGER_INFO << "Running PnLAuditTrail_GetAllForDate query for: start_time=" << settings._pnl_audit_start_time
                            << ",end_time=" << settings._pnl_audit_end_time
                            << "\n";

                tw::channel_or::PnLAuditTrail_GetAllForDate query;
                if ( !query.execute(_channelDb,  account, settings._pnl_audit_start_time, settings._pnl_audit_end_time) ) {
                    LOGGER_ERRO << "Failed to execute PnLAuditTrail_GetAllForDate query for: "
                                << "account=" << account
                                << ",start_time=" << settings._pnl_audit_start_time
                                << ",end_time=" << settings._pnl_audit_end_time
                                << "\n";
                    return false;
                }

                if ( query._o1.empty() ) {
                    LOGGER_ERRO << "PnLAuditTrailInfos are empty for: "
                                << "account=" << account
                                << ",start_time=" << settings._pnl_audit_start_time
                                << ",end_time=" << settings._pnl_audit_end_time
                                << "\n";
                    return false;
                }
                
                _pnlAuditTrailInfos = query._o1;
                
                LOGGER_INFO << "PnLAuditTrailInfos number of records: " << _pnlAuditTrailInfos.size() << "\n";
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
        const std::vector<std::string>& getAccounts() const {
            return _accounts;
        }            
        
        const TPnLAuditTrailInfos& getPnLAuditTrailInfos() const {
            return _pnlAuditTrailInfos;
        }            
        
    protected:
        tw::channel_db::ChannelDb _channelDb;
        tw::channel_db::ChannelDb::TConnectionPtr _con;
        
        std::vector<std::string> _accounts;
        TPnLAuditTrailInfos _pnlAuditTrailInfos;
    };

} // namespace scalper
} // namespace tw

