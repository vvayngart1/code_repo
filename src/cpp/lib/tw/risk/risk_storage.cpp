#include <tw/risk/risk_storage.h>

namespace tw {
namespace risk {

// class ChannelOrStorage
//
RiskStorage::RiskStorage() {
    clear();
}

RiskStorage::~RiskStorage() {
    stop();
}

void RiskStorage::clear() {
}
    
bool RiskStorage::init(const tw::common::Settings& settings) {
    try {                
        if ( !_channelDb.init(settings._db_connection_str) ) {
            LOGGER_ERRO << "Failed to init channelDb with: "  << settings._db_connection_str << "\n";
            return false;
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

bool RiskStorage::start() {
    bool status = true;
    try {
        LOGGER_INFO << "Starting..." << "\n";
        
        if ( !_channelDb.isValid() )
            return false;
        
        _connection = _channelDb.getConnection();
        if ( !_connection ) {
            LOGGER_ERRO << "Can't open connection to db"  << "\n" << "\n";
            status = false;
        }
        
        _statement = _channelDb.getStatement(_connection);
        if ( !_connection ) {
            LOGGER_ERRO << "Can't open connection to db"  << "\n" << "\n";
            status = false;
        }
        
        LOGGER_INFO << "Started" << "\n";
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

void RiskStorage::stop() {
    try {
        LOGGER_ERRO << "Stopping..." << "\n";
        
        if ( _statement ) {
            _statement->close();
            _statement.reset();
        }
        
        if ( _connection ) {
            _connection->close();
            _connection.reset();
        }
    
        clear();
        
        LOGGER_ERRO << "Stopped" << "\n";
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

bool RiskStorage::getAllAccounts(std::vector<tw::risk::Account>& results) {
    return getAllAccounts(results, _channelDb);
}

bool RiskStorage::getAccount(tw::risk::Account& result, const std::string& name) {
    return getAccount(result, name, _channelDb);
}

bool RiskStorage::getAllAccounts(std::vector<tw::risk::Account>& results, TChannelDb& channelDb) {
    try {
        Accounts_GetAll query;

        if ( !query.execute(channelDb) )
            return false;
        
        results = query._o1;
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    
    return true;
}

bool RiskStorage::getAccount(tw::risk::Account& result, const std::string& name, TChannelDb& channelDb) {
    try {
        std::vector<tw::risk::Account> results;
        if ( !getAllAccounts(results, channelDb) )
            return false;
        
        for ( size_t i = 0; i < results.size(); ++i ) {
            if ( name == results[i]._name ) {
                result = results[i];
                return true;
            }
        }
        
        return false;
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    
    return true;
}

bool RiskStorage::getAllStrategies(std::vector<tw::risk::Strategy>& results) {
    try {
        Strategies_GetAll query;

        if ( !query.execute(_channelDb) )
            return false;
        
        results = query._o1;
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    
    return true;
}

bool RiskStorage::getStrategies(std::vector<tw::risk::Strategy>& results, const tw::risk::Account& account) {
    try {
        std::vector<tw::risk::Strategy> all;
        
        if ( !getAllStrategies(all) )
            return false;
        
        for ( size_t i = 0; i < all.size(); ++i ) {
            if ( account._id == all[i]._accountId )
                results.push_back(all[i]);
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

bool RiskStorage::getAllAccountsRiskParams(std::vector<tw::risk::AccountRiskParams>& results) {
    try {
        AccountsRiskParams_GetAll query;

        if ( !query.execute(_channelDb) )
            return false;
        
        results = query._o1;
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    
    return true;
}

bool RiskStorage::getAccountRiskParams(std::vector<tw::risk::AccountRiskParams>& results, const tw::risk::Account& account) {
    try {
        std::vector<tw::risk::AccountRiskParams> all;
        
        if ( !getAllAccountsRiskParams(all) )
            return false;
        
        for ( size_t i = 0; i < all.size(); ++i ) {
            if ( account._id == all[i]._accountId )
                results.push_back(all[i]);
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

bool RiskStorage::disableStratsForAccount(const tw::risk::Account& account) {
    try {
        std::vector<tw::risk::Strategy> strats;
        if ( !tw::risk::RiskStorage::instance().getStrategies(strats, account) )            
            return false;
        
        for ( size_t i = 0; i < strats.size(); ++i ) {
            strats[i]._tradeEnabled = false;
            if ( !saveStrategy(strats[i]) )
                return false;                
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

bool RiskStorage::getOrCreateStrategy(tw::risk::Strategy& result) {
    try {
        Strategy_GetOrCreate query;
        return query.execute(_channelDb, result);
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    
    return true;
}


bool RiskStorage::saveStrategy(const tw::risk::Strategy& value) {
    try {
        
        if ( !_connection || !_statement ) {
            LOGGER_ERRO << "_connection or _statement is NULL" << "\n" << "\n";
            return false;
        }
        
        Strategy_Save query;
        return query.execute(_statement, value);
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    
    return true;
}

} // namespace risk
} // namespace tw
