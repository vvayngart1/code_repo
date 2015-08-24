#pragma once

#include <tw/config/settings_config_file.h>
#include <tw/functional/utils.hpp>
#include <tw/instr/instrument_manager.h>
#include <tw/generated/enums_common.h>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

#include <map>
#include <vector>
#include <set>

namespace tw {
namespace channel_or_drop_cme {
    
class SessionSettings : public tw::config::SettingsConfigFile {
    typedef tw::config::SettingsConfigFile TParent;
    typedef std::set<std::string> TApplFeedIds;
    
public:
    SessionSettings(const std::string& name) : TParent("ChannelOrDropCmeOnixSettings."+name) {
        clear();
        _name = name;
        
        TParent::add_options()
            (optionName(".host").c_str(), _host, "drop session host", tw::config::EnumOptionNeed::eRequired)
            (optionName(".port").c_str(), _port, "drop session port", tw::config::EnumOptionNeed::eRequired)
            (optionName(".password").c_str(), _password, "drop session password", tw::config::EnumOptionNeed::eRequired)
            (optionName(".senderCompId").c_str(), _senderCompId, "drop session senderCompId", tw::config::EnumOptionNeed::eRequired)
            (optionName(".senderSubId").c_str(), _senderSubId, "drop session senderSubId", tw::config::EnumOptionNeed::eRequired)
            (optionName(".senderLocationId").c_str(), _senderLocationId, "drop session senderLocationId", tw::config::EnumOptionNeed::eRequired)
            (optionName(".targetCompId").c_str(), _targetCompId, "drop session targetCompId", tw::config::EnumOptionNeed::eRequired)
            (optionName(".targetSubId").c_str(), _targetSubId, "drop session targetSubId", tw::config::EnumOptionNeed::eRequired)
            (optionName(".applFeedIdsFilter").c_str(), _applFeedIdsFilter, "specifies appl feed ids to have fills logged to db", tw::config::EnumOptionNeed::eOptional)
        ;
    }
    
    std::string optionName(const std::string& value) {
        std::string name = _name+value;
        return name;
    }
    
    void clear() {
        _name.clear();
        
        _host.clear();
        _port = 0;
        _password.clear();
        _senderCompId.clear();
        _senderSubId.clear();
        _senderLocationId.clear();
        _targetCompId.clear();
        _targetSubId.clear();
        _applFeedIdsFilter.clear();
        _applFeedIds.clear();
    }    
    
    virtual bool parse(const std::string& configFile) {
        const char* DELIM = ",";
        
        bool status = true;
        try {
        
            if ( !TParent::parse(configFile) )
                return false;
            
            std::vector<std::string> applFeedIds;
            boost::split(applFeedIds, _applFeedIdsFilter, boost::is_any_of(DELIM));
            std::for_each(applFeedIds.begin(), applFeedIds.end(), tw::functional::printHelper<std::string>("List of CME's iLink sessions to listen to in drop copy:"));

            if ( applFeedIds.empty() ) {
                LOGGER_WARN << "No filters set for : " << configFile << " :: " << _name << "\n";
                return true;
            }
            
            std::vector<std::string>::iterator iter = applFeedIds.begin();
            std::vector<std::string>::iterator end = applFeedIds.end();
            for ( ; iter != end; ++iter ) {
                std::string v = std::to_upper(std::trim(*iter));
                _applFeedIds.insert(v);
            }
        } catch(const std::exception& e) {
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
            status = false;
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
            status = false;
        }
        
        return status;
    }
    
    bool hasApplFeedId(const std::string& value) const {
        if ( _applFeedIds.empty() )
            return true;        
        
        std::string v = std::to_upper(std::trim(value));
        return (_applFeedIds.find(v) != _applFeedIds.end());
    }
    
public:
    std::string _name;
    
    std::string _host;
    uint32_t _port;
    std::string _password;
    std::string _senderCompId;
    std::string _senderSubId;
    std::string _senderLocationId;
    std::string _targetCompId;
    std::string _targetSubId;
    
    std::string _applFeedIdsFilter;    
    TApplFeedIds _applFeedIds;
};

typedef boost::shared_ptr<SessionSettings> TSessionSettingsPtr;
    
class Settings : public tw::config::SettingsConfigFile
{
    typedef tw::config::SettingsConfigFile TParent;
    
public:
    Settings() : TParent("ChannelOrDropCmeOnixSettings" ) {
        TParent::add_options()
            ("global.sessions", _global_sessions, "iLink sessions to create", tw::config::EnumOptionNeed::eRequired)
            ("global.fixDialectDescriptionFile", _global_fixDialectDescriptionFile, "global fixDialectDescriptionFile", tw::config::EnumOptionNeed::eRequired)
            ("global.logDirectory", _global_logDirectory, "global logDirectory", tw::config::EnumOptionNeed::eRequired)
            ("global.licenseStore", _global_licenseStore, "global licenseStore", tw::config::EnumOptionNeed::eRequired)
            ("global.account", _global_account, "internal account for fills", tw::config::EnumOptionNeed::eRequired)
            ("global.accounts_mappings", _global_accounts_mappings, "mappings of tags 50 to internal account", tw::config::EnumOptionNeed::eRequired)
            ("global.simulate", _global_simulate, "indicates if to simulate fills", tw::config::EnumOptionNeed::eOptional)
            ("global.simTimeout", _global_simTimeout, "simulation timeout", tw::config::EnumOptionNeed::eOptional)
            ("global.simSymbol", _global_simSymbol, "simulation symbol", tw::config::EnumOptionNeed::eOptional)
            ("global.simFillsBatch", _global_simFillsBatch, "simulation timeout", tw::config::EnumOptionNeed::eOptional)
        ;
    }
    
    void clear() {
        _global_sessions.clear();
        _global_fixDialectDescriptionFile.clear();
        _global_logDirectory.clear();
        _global_licenseStore.clear();
        _global_account.clear();
        _global_accounts_mappings.clear();
        
        _global_simulate = false;
        _global_simTimeout = 5000;
        _global_simSymbol.clear();
        _global_simFillsBatch = 1;
        
        _sessions.clear();
    }
    
    virtual bool parse(const std::string& configFile) {
        const char* DELIM = ",";
        
        bool status = true;
        try {
        
            if ( !TParent::parse(configFile) )
                return false;

            std::vector<std::string> sessionNames;
            boost::split(sessionNames, _global_sessions, boost::is_any_of(DELIM));
            std::for_each(sessionNames.begin(), sessionNames.end(), tw::functional::printHelper<std::string>("List of CME's iLink sessions:"));

            if ( sessionNames.empty() ) {
                LOGGER_ERRO << "Session names are empty in configFile: " << configFile << "\n";
                return false;
            }
            
            BOOST_FOREACH(const std::string& name, sessionNames) {
                LOGGER_INFO << "Loading session settings for: " << name << "\n";
                TSessionSettingsPtr sessionSettings(new SessionSettings(name));
                if ( sessionSettings->parse(configFile) ) {
                    LOGGER_INFO << "Loaded session settings for: " << name << "\n";
                    LOGGER_INFO << sessionSettings->toString() << "\n";
                    _sessions[name] = sessionSettings;
                } else {
                    LOGGER_INFO << "Failed to load session settings for: " << name << "\n";
                    return false;
                }
            }
        } catch(const std::exception& e) {
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
            status = false;
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
            status = false;
        }
        
        return status;
    }
    
public:
    typedef std::map<std::string, TSessionSettingsPtr> TSessions;
    
    std::string _global_sessions;
    std::string _global_fixDialectDescriptionFile;
    
    std::string _global_logDirectory;
    std::string _global_licenseStore;
    
    std::string _global_account;
    std::string _global_accounts_mappings;
    
    bool _global_simulate;
    uint32_t _global_simTimeout;
    std::string _global_simSymbol;
    uint32_t _global_simFillsBatch;
    
    TSessions _sessions; 
};
    
} // namespace channel_or_drop_cme
} // namespace tw
