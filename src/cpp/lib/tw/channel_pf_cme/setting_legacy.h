#pragma once

#include <tw/config/settings_config_file.h>
#include <tw/functional/utils.hpp>
#include <tw/instr/instrument_manager.h>

#include <OnixS/CME/MarketData.h>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include <vector>
#include <set>
#include <algorithm>

namespace tw {
namespace channel_pf_cme {
    
class Settings : public tw::config::SettingsConfigFile
{
    typedef tw::config::SettingsConfigFile TParent;
    
public:
    typedef std::vector<std::string> TSymbols;
    typedef std::vector<OnixS::CME::MarketData::ChannelId> TChannelIds;
    typedef OnixS::CME::MarketData::InitializationSettings TGlobalSettingsImpl;
    typedef OnixS::CME::MarketData::HandlerSettings TSettingsImpl;
    
public:
    TSymbols _symbols;
    TChannelIds _channelIds;
    bool _instrumentLoading;
    bool _verbose;
    bool _replay;
    unsigned _quoteNotificationStatsInterval;
    std::string _replayFolder;
    TGlobalSettingsImpl _globalSettings;
    TSettingsImpl _settings;
    bool _processNormalTradesOnly;
    bool _topOfBookOnly;
    
public:
    Settings() : TParent("OnixHandlerSettings" ) {
        TParent::add_options()
            (("channel_pf_cme_onix.symbols_filter"), _symbolsFilterString, "Specifies comma separated list of CME symbols to listen to")
            (("channel_pf_cme_onix.channelIds"), _channelIdsString, "Specifies comma separated list of CME channel Ids to listen to")
            (("channel_pf_cme_onix.instrumentLoading"), _instrumentLoading, "Specifies if to start in instrument loading mode", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
            (("channel_pf_cme_onix.verbose"), _verbose, "Specifies if mode is verbose or not", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
            (("channel_pf_cme_onix.replay"), _replay, "Specifies if to replay data from log file", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
            (("channel_pf_cme_onix.replayFolder"), _replayFolder, "Specifies folder to replay data from.  Ignored if 'reply' is set to 'false'")
            (("channel_pf_cme_onix.licenseStore"), _globalSettings.licenseStore, "Path to the folder in which licenses are stored", tw::config::EnumOptionNeed::eRequired)
            (("channel_pf_cme_onix.fixDialectDescriptionFile"), _globalSettings.fixDialectDescriptionFile, "Path the CME FIX dialect description file", tw::config::EnumOptionNeed::eRequired)         
            (("channel_pf_cme_onix.cacheSecurityDefinitions"), _settings.cacheSecurityDefinitions, "Option to use file-based caching of the security definitions", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
            (("channel_pf_cme_onix.channelsConfigurationFile"), _settings.channelsConfigurationFile, "Path to the channels configuration XML file", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>("config.xml"))
            (("channel_pf_cme_onix.fastTemplatesFile"), _settings.fastTemplatesFile, "Path to CME FAST templates xml file", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>("templates.xml"))
            (("channel_pf_cme_onix.networkInterface"), _settings.networkInterface, "Specifies one or more network interfaces to use while joining the multicast group; use semi-colon delimited list if more than one", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>("eth1"))
            (("channel_pf_cme_onix.networkInterfaceA"), _settings.networkInterfaceA, "Specifies one or more network interfaces to use for \"A\" feeds while joining the multicast group; use semi-colon delimited list if more than one")
            (("channel_pf_cme_onix.networkInterfaceB"), _settings.networkInterfaceB, "Specifies one or more network interfaces to use for \"B\" feeds while joining the multicast group; use semi-colon delimited list if more than one")
            (("channel_pf_cme_onix.recordReceivingTime"), _settings.recordReceivingTime, "Option to record the first time the source multicast packet is received from the kernel - before it is decompressed or processed", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
            (("channel_pf_cme_onix.recoverSecurityDefinitionsOnGap"), _settings.recoverSecurityDefinitionsOnGap, "Option to recover the Security Definition messages each time when the message sequence gap is detected", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
            (("channel_pf_cme_onix.tcpReplayUsername"), _settings.tcpReplayUsername, "Userid or username to be used in the Logon (35=A) message from customer to CME")
            (("channel_pf_cme_onix.tcpReplayPassword"), _settings.tcpReplayPassword, "Password or passphrase to be used in the Logon (35=A) message from customer to CME")
            (("channel_pf_cme_onix.tcpReplayReconnectAttempts"), _settings.tcpReplayReconnectAttempts, "Number of attempts to receive missed messages via the TCP Replay Channel")
            (("channel_pf_cme_onix.tcpReplayReconnectIntervalInMilliseconds"), _settings.tcpReplayReconnectIntervalInMilliseconds, "Interval in milliseconds between the attempts to receive missed messages via the TCP Replay Channel")
            (("channel_pf_cme_onix.useTcpReplay"), _settings.useTcpReplay, "Option to use the TCP Replay feed", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
            (("channel_pf_cme_onix.useIncrementalFeedA"), _settings.useIncrementalFeedA, "Option to use the Incremental feed \"A\"", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(true))
            (("channel_pf_cme_onix.useIncrementalFeedB"), _settings.useIncrementalFeedB, "Option to use the Incremental feed \"B\"", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(true))
            (("channel_pf_cme_onix.useInstrumentReplayFeedA"), _settings.useInstrumentReplayFeedA, "Option to use the Incremental Replay feed \"A\"", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(true))
            (("channel_pf_cme_onix.useInstrumentReplayFeedB"), _settings.useInstrumentReplayFeedB, "Option to use the Incremental feed \"B\"", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
            (("channel_pf_cme_onix.useSnapshotFeedA"), _settings.useSnapshotFeedA, "Option to use the Snapshot feed \"A\"", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(true))
            (("channel_pf_cme_onix.useSnapshotFeedB"), _settings.useSnapshotFeedB, "Option to use the Snapshot feed \"B\"", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
            (("channel_pf_cme_onix.heartbeatInterval"), _settings.heartbeatInterval, "Specifies maximal time interval between two network packets. If no data is received during specified time frame, warning is reported", tw::config::EnumOptionNeed::eOptional, boost::optional<size_t>(30))
            (("channel_pf_cme_onix.logMode"), reinterpret_cast<int32_t&>(_settings.logMode), "Specifies whether the Handler should log its events and which of events should be put into the log", tw::config::EnumOptionNeed::eOptional, boost::optional<int32_t>(static_cast<int32_t>(OnixS::CME::MarketData::LogModes::Regular)))
            (("channel_pf_cme_onix.advancedLogOptions"), _settings.advancedLogOptions, "Additional options to control which data is to be logged. Ignored if logging is disabled", tw::config::EnumOptionNeed::eOptional, boost::optional<unsigned>(OnixS::CME::MarketData::AdvancedLogOptions::LogEverything))
            (("channel_pf_cme_onix.logDirectory"), _settings.logDirectory, "Log files are stored in this directory. Ignored if logging is disabled", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>("logs"))
            (("channel_pf_cme_onix.repeatingGroupEntriesMaxCount"), _settings.repeatingGroupEntriesMaxCount, "Maximum number of repeating group entries", tw::config::EnumOptionNeed::eOptional, boost::optional<size_t>(1000))
            (("channel_pf_cme_onix.incrementalMessageQueueMaxSize"), _settings.incrementalMessageQueueMaxSize, "Maximum number of queued Market Data Incremental Refresh (X) messages", tw::config::EnumOptionNeed::eOptional, boost::optional<size_t>(1000))
            (("channel_pf_cme_onix.maxGapRecoveryCacheSize"), _settings.maxGapRecoveryCacheSize, "Defines max value for size of packet cache which is used to put into line out of order multicast packets", tw::config::EnumOptionNeed::eOptional, boost::optional<size_t>(3))
            (("channel_pf_cme_onix.lostPacketWaitTimeout"), _settings.lostPacketWaitTimeout, "Indicates for how long Handler should wait for the packet before consider it as totally lost. Value must be specified in microseconds (usec)", tw::config::EnumOptionNeed::eOptional, boost::optional<size_t>(1000000))        
            (("channel_pf_cme_onix.quoteNotificationStatisticsInterval"), _quoteNotificationStatsInterval, "Collect quote notification statistics and print them every $n quote updates", tw::config::EnumOptionNeed::eOptional, boost::optional<unsigned>(0))
            (("channel_pf_cme_onix.processNormalTradesOnly"), _processNormalTradesOnly, "Option to process only 'normal' (e.g. not implied) trades", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(true))
            (("channel_pf_cme_onix.topOfBookOnly"), _topOfBookOnly, "Send quote updates only when top of book changes price or quantity", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(true))
        ;
    }
    
    virtual bool parse(const std::string& configFile) {
        const char* DELIM = ",";
        
        if ( !TParent::parse(configFile) )
            return false;
        
        if ( _channelIdsString.empty() || _symbolsFilterString.empty() ) {            
            tw::instr::InstrumentManager::TInstruments instruments = tw::instr::InstrumentManager::instance().getByExchange(tw::instr::eExchange::kCME);
            
            if ( _channelIdsString.empty() ) {
                std::set<OnixS::CME::MarketData::ChannelId> channelIds;
                tw::instr::InstrumentManager::TInstruments::iterator iter = instruments.begin();
                tw::instr::InstrumentManager::TInstruments::iterator end = instruments.end();
            
                for ( ; iter != end; ++iter ) {
                    if ( channelIds.find((*iter)->_var1) == channelIds.end() ) {
                        if ( iter != instruments.begin() )
                            _channelIdsString += DELIM;
                        
                        channelIds.insert((*iter)->_var1);
                        _channelIdsString += ((*iter)->_var1);
                    }                    
                }
            }
            
            if ( _symbolsFilterString.empty() ) {
                std::set<OnixS::CME::MarketData::ChannelId> channelIds;
                tw::instr::InstrumentManager::TInstruments::iterator iter = instruments.begin();
                tw::instr::InstrumentManager::TInstruments::iterator end = instruments.end();
            
                for ( ; iter != end; ++iter ) {
                    if ( iter != instruments.begin() )
                        _symbolsFilterString += DELIM;
                    
                    _symbolsFilterString += (*iter)->_displayName;
                }
            }
            
            LOGGER_WARN << _name << ":: " << configFile << " :: 'channelIds' and/or symbols_filter not set - configured all channelIds/symbols" << "\n";
        }
        
        boost::split(_channelIds, _channelIdsString, boost::is_any_of(DELIM));
        std::for_each(_channelIds.begin(), _channelIds.end(), tw::functional::printHelper<std::string>("List of CME channels to subscribe:"));
        
        boost::split(_symbols, _symbolsFilterString, boost::is_any_of(DELIM));
        std::for_each(_symbols.begin(), _symbols.end(), tw::functional::printHelper<std::string>("List of CME symbols to subscribe:"));
        
        if ( _replay && _replayFolder.empty() ) {
            LOGGER_ERRO << _name << ":: " << "error parsing configFile: " << configFile << " :: 'replayFolder' is not set when 'replayFolder' is true" << "\n";
            return false;
        }
        
        return true;
    }
    
private:
    std::string _symbolsFilterString;
    std::string _channelIdsString;
};
    
} // namespace channel_pf_cme
} // namespace tw
