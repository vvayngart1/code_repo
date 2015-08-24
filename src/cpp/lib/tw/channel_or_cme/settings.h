#pragma once

#include <tw/config/settings_config_file.h>
#include <tw/functional/utils.hpp>
#include <tw/instr/instrument_manager.h>
#include <tw/generated/enums_common.h>
#include <tw/generated/channel_or_defs.h>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

#include <map>

namespace tw {
namespace channel_or_cme {
    
class SessionSettings : public tw::config::SettingsConfigFile {
    typedef tw::config::SettingsConfigFile TParent;
    
public:
    SessionSettings(const std::string& name) : TParent("ChannelOrCmeOnixSettings."+name) {
        clear();
        _name = name;
        
        TParent::add_options()
            (optionName(".host").c_str(), _host, "iLink session host", tw::config::EnumOptionNeed::eRequired)
            (optionName(".port").c_str(), _port, "iLink session port", tw::config::EnumOptionNeed::eRequired)
            (optionName(".heartBeatInt").c_str(), _heartBeatInt, "iLink session heartBeatInt", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(30U))
            (optionName(".senderCompId").c_str(), _senderCompId, "iLink session senderCompId", tw::config::EnumOptionNeed::eRequired)
            (optionName(".senderSubId").c_str(), _senderSubId, "iLink session senderSubId", tw::config::EnumOptionNeed::eRequired)
            (optionName(".targetCompId").c_str(), _targetCompId, "iLink session targetCompId", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>("CME"))
            (optionName(".targetSubId").c_str(), _targetSubId, "iLink session targetSubId", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>("G"))
            (optionName(".senderLocationId").c_str(), _senderLocationId, "iLink session senderLocationId", tw::config::EnumOptionNeed::eRequired)
            (optionName(".applicationSystemName").c_str(), _applicationSystemName, "iLink asession pplicationSystemName", tw::config::EnumOptionNeed::eRequired)
            (optionName(".applicationSystemVersion").c_str(), _applicationSystemVersion, "iLink session applicationSystemVersion", tw::config::EnumOptionNeed::eRequired)
            (optionName(".applicationSystemVendor").c_str(), _applicationSystemVendor, "iLink session applicationSystemVendor", tw::config::EnumOptionNeed::eRequired)
            (optionName(".password").c_str(), _password, "iLink session password", tw::config::EnumOptionNeed::eRequired)
            (optionName(".account").c_str(), _account, "iLink session account", tw::config::EnumOptionNeed::eRequired)
            (optionName(".handlInst").c_str(), _handlInst, "iLink session handlInst", tw::config::EnumOptionNeed::eRequired)
            (optionName(".customerOrFirm").c_str(), _customerOrFirm, "iLink session customerOrFirm", tw::config::EnumOptionNeed::eRequired)
            (optionName(".ctiCode").c_str(), _ctiCode, "iLink session ctiCode", tw::config::EnumOptionNeed::eRequired)            
            (optionName(".resetSeqNumFlag").c_str(), _resetSeqNumFlag, "indicates if iLink session needs to reset seq num", tw::config::EnumOptionNeed::eOptional)
            (optionName(".enabled").c_str(), _enabled, "indicates if iLink session is enabled", tw::config::EnumOptionNeed::eRequired)
            (optionName(".storageType").c_str(), _storageType, "indicates if to use OnixS or custom message type", tw::config::EnumOptionNeed::eOptional)
            (optionName(".isAcceptor").c_str(), _isAcceptor, "indicates if iLink session is acceptor or connector", tw::config::EnumOptionNeed::eOptional)
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
        _heartBeatInt = 0;

        _password.clear();
        _senderCompId.clear();
        _senderSubId.clear();
        _targetCompId.clear();
        _targetSubId.clear();
        _senderLocationId.clear();
        _applicationSystemName.clear();
        _applicationSystemVersion.clear();
        _applicationSystemVendor.clear();

        _account.clear();
        _handlInst.clear();
        _customerOrFirm.clear();
        _ctiCode.clear();
        
        _resetSeqNumFlag = false;
        _enabled = false;
        _isAcceptor = false;
        
        _storageType = tw::common::eChannelOrCMEStorageType::kCustom;
    }
    
public:
    std::string _name;
    
    std::string _host;
    uint32_t _port;
    uint32_t _heartBeatInt;
    
    std::string _password;
    std::string _senderCompId;
    std::string _senderSubId;
    std::string _senderLocationId;
    std::string _targetCompId;
    std::string _targetSubId;
    std::string _applicationSystemName;
    std::string _applicationSystemVersion;
    std::string _applicationSystemVendor;
    
    std::string _account;
    std::string _handlInst;
    
    std::string _customerOrFirm;
    std::string _ctiCode;
    
    bool _resetSeqNumFlag;
    bool _enabled;
    bool _isAcceptor;
    
    tw::common::eChannelOrCMEStorageType _storageType;
};

class SettingsGlobal : public tw::config::SettingsConfigFile
{
    typedef tw::config::SettingsConfigFile TParent;
    
public:
    SettingsGlobal(const std::string& name="ChannelOrCmeOnixSettingsGlobal") : TParent(name) {
        TParent::add_options()
            ("global.sessions", _global_sessions, "iLink sessions to create", tw::config::EnumOptionNeed::eOptional)
            ("global.schedulerSettingsFile", _global_schedulerSettingsFile, "global schedulerSettingsFile", tw::config::EnumOptionNeed::eRequired)
            ("global.fixDialectDescriptionFile", _global_fixDialectDescriptionFile, "global fixDialectDescriptionFile", tw::config::EnumOptionNeed::eRequired)
            ("global.logDirectory", _global_logDirectory, "global logDirectory", tw::config::EnumOptionNeed::eRequired)
            ("global.licenseStore", _global_licenseStore, "global licenseStore", tw::config::EnumOptionNeed::eRequired)
        
            ("global.exchange_sim_port", _global_exchange_sim_port, "exchange sim acceptor port", tw::config::EnumOptionNeed::eOptional)
            ("global.exchange_sim_matcher_percentCancelFront", _exchange_sim_params._percentCancelFront, "percent to cancel from front of the queue in exchange sim", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(20))
            ("global.exchange_sim_matcher_verbose", _exchange_sim_params._verbose, "indicates if to use OnixS or custom message type", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
            ("global.exchange_sim_ack_delay_ms", _exchange_sim_params._ack_delay_ms, "delay to send ack", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(0))        
            ("global.exchange_sim_rej_as_bussiness_rej", _exchange_sim_params._rej_as_bussiness_rej, "indicates if to use regular or business (session) level reject message", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
            ("global.exchange_sim_new_ack_seq_count", _exchange_sim_params._new_ack_seq_count, "number of new acks in a row, disabled if = -1", tw::config::EnumOptionNeed::eOptional, boost::optional<int32_t>(-1))
            ("global.exchange_sim_new_rej_seq_count", _exchange_sim_params._new_rej_seq_count, "number of new rejs in a row, disabled if = -1 or if new_ack_seq_count = -1", tw::config::EnumOptionNeed::eOptional, boost::optional<int32_t>(-1))
            ("global.exchange_sim_mod_ack_seq_count", _exchange_sim_params._mod_ack_seq_count, "number of mod acks in a row, disabled if = -1", tw::config::EnumOptionNeed::eOptional, boost::optional<int32_t>(-1))
            ("global.exchange_sim_mod_rej_seq_count", _exchange_sim_params._mod_rej_seq_count, "number of mod rejs in a row, disabled if = -1 or if mod_ack_seq_count = -1", tw::config::EnumOptionNeed::eOptional, boost::optional<int32_t>(-1))
            ("global.exchange_sim_cxl_ack_seq_count", _exchange_sim_params._cxl_ack_seq_count, "number of cxl acks in a row, disabled if = -1", tw::config::EnumOptionNeed::eOptional, boost::optional<int32_t>(-1))
            ("global.exchange_sim_cxl_rej_seq_count", _exchange_sim_params._cxl_rej_seq_count, "number of cxl rejs in a row, disabled if = -1 or if cxl_ack_seq_count = -1", tw::config::EnumOptionNeed::eOptional, boost::optional<int32_t>(-1))
            
        ;
    }
    
public:
    std::string _global_sessions;
    std::string _global_schedulerSettingsFile;
    std::string _global_fixDialectDescriptionFile;
    
    std::string _global_logDirectory;
    std::string _global_licenseStore;
    
    uint32_t _global_exchange_sim_port;
    tw::channel_or::SimulatorMatcherParams _exchange_sim_params;
};

typedef boost::shared_ptr<SessionSettings> TSessionSettingsPtr;
    
class Settings : public SettingsGlobal
{
    typedef SettingsGlobal TParent;
    
public:
    Settings() : TParent("ChannelOrCmeOnixSettings" ) {
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
                LOGGER_WARN << "Session names are empty in configFile: " << configFile << "\n";
                return true;
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
    
    bool getFirstEnabled(TSessionSettingsPtr& sessionSettings) {
        TSessions::iterator iter = _sessions.begin();
        TSessions::iterator end = _sessions.end();
        
        for ( ; iter != end; ++ iter ) {
            if ( iter->second->_enabled ) {
                sessionSettings = iter->second;
                return true;
            }
        }
        
        return false;
    }
    
public:
    typedef std::map<std::string, TSessionSettingsPtr> TSessions;
    
    TSessions _sessions; 
};
    
} // namespace channel_or_cme
} // namespace tw
