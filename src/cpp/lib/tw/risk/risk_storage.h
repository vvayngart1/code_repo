#pragma once

#include <tw/common/defs.h>
#include <tw/common/singleton.h>

#include <tw/generated/risk_defs.h>

#include <boost/algorithm/string.hpp>

#include <fstream>

namespace tw {
namespace risk {
    
class RiskStorage : public tw::common::Singleton<RiskStorage> {
    typedef tw::channel_db::ChannelDb TChannelDb;
    
public:
    RiskStorage();
    ~RiskStorage();
    
    void clear();
    
public:
    bool init(const tw::common::Settings& settings);
    bool start();    
    void stop();
    
    bool isValid() {
        return _channelDb.isValid();
    }
    
public:
    bool getAllAccounts(std::vector<tw::risk::Account>& results);
    bool getAccount(tw::risk::Account& result, const std::string& name);
    bool getAllAccounts(std::vector<tw::risk::Account>& results, TChannelDb& channelDb);
    bool getAccount(tw::risk::Account& result, const std::string& name, TChannelDb& channelDb);
    
    bool getAllStrategies(std::vector<tw::risk::Strategy>& results);
    bool getStrategies(std::vector<tw::risk::Strategy>& results, const tw::risk::Account& account);
    bool getAllAccountsRiskParams(std::vector<tw::risk::AccountRiskParams>& results);
    bool getAccountRiskParams(std::vector<tw::risk::AccountRiskParams>& results, const tw::risk::Account& account);
    bool disableStratsForAccount(const tw::risk::Account& account);
    
    bool getOrCreateStrategy(tw::risk::Strategy& result);
    bool saveStrategy(const tw::risk::Strategy& value);
    
private:
    TChannelDb _channelDb;
    tw::channel_db::ChannelDb::TConnectionPtr _connection;
    tw::channel_db::ChannelDb::TStatementPtr _statement;                        
    
};

    
} // namespace risk
} // namespace tw
