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
    typedef std::set<int32_t> TCpuIndexes;
    typedef std::vector<std::string> TChannelIds;
    typedef OnixS::CME::MarketData::InitializationSettings TGlobalSettingsImpl;
    typedef OnixS::CME::MarketData::HandlerSettings TSettingsImpl;
    
public:
    TSymbols _symbols;
    TChannelIds _channelIds;
    TCpuIndexes _cpuIndexes;
    bool _instrumentLoading;
    bool _verbose;
    bool _replay;
    unsigned _quoteNotificationStatsInterval;
    std::string _replayFolder;
    TGlobalSettingsImpl _globalSettings;
    TSettingsImpl _settings;
    bool _processNormalTradesOnly;
    bool _topOfBookOnly;
    bool _naturalRefresh;
    std::string _definitionsFile;
    bool _coalesce;
    
public:
    Settings() : TParent("OnixHandlerSettings" ) {
        TParent::add_options()
            (("channel_pf_cme_onix.channelIds"), _channelIdsString, "Specifies comma separated list of CME channel Ids to listen to")
            (("channel_pf_cme_onix.cpuIndexes"), _cpuIndexesString, "Specifies comma separated list of thread affinity CPU indexes")
            (("channel_pf_cme_onix.symbols_filter"), _symbolsFilterString, "Specifies comma separated list of CME symbols to listen to")
            (("channel_pf_cme_onix.instrumentLoading"), _instrumentLoading, "Specifies if to start in instrument loading mode", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
            (("channel_pf_cme_onix.verbose"), _verbose, "Specifies if mode is verbose or not", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
            (("channel_pf_cme_onix.replay"), _replay, "Specifies if to replay data from log file", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
            (("channel_pf_cme_onix.replayFolder"), _replayFolder, "Specifies folder to replay data from.  Ignored if 'reply' is set to 'false'")
            (("channel_pf_cme_onix.quoteNotificationStatisticsInterval"), _quoteNotificationStatsInterval, "Collect quote notification statistics and print them every $n quote updates", tw::config::EnumOptionNeed::eOptional, boost::optional<unsigned>(0))
            (("channel_pf_cme_onix.processNormalTradesOnly"), _processNormalTradesOnly, "Option to process only 'normal' (e.g. not implied) trades", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(true))
            (("channel_pf_cme_onix.topOfBookOnly"), _topOfBookOnly, "Send quote updates only when top of book changes price or quantity", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(true))
            (("channel_pf_cme_onix.licenseStore"), _licenseStore, "Path to the folder in which licenses are stored", tw::config::EnumOptionNeed::eRequired)
            (("channel_pf_cme_onix.advancedLogOptions"), _settings.advancedLogOptions, "Additional options to control which data is to be logged. Ignored if logging is disabled", tw::config::EnumOptionNeed::eOptional, boost::optional<unsigned>(OnixS::CME::MarketData::AdvancedLogOptions::LogEverything))
            (("channel_pf_cme_onix.cacheSecurityDefinitions"), _settings.cacheSecurityDefinitions, "Option to use file-based caching of the security definitions", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
            (("channel_pf_cme_onix.channelsConfigurationFile"), _channelsConfigurationFile, "Path to the channels configuration XML file", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>("config.xml"))
            (("channel_pf_cme_onix.codingTemplatesFile"), _codingTemplatesFile, "Path to CME FAST templates xml file", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>("templates.xml"))
            (("channel_pf_cme_onix.naturalRefresh"), _naturalRefresh, "Option to use natural refresh of books without recovering dropped packets", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(true))
            (("channel_pf_cme_onix.definitionsFile"), _definitionsFile, "Path to CME FAST secdef.dat file", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>("secdef.dat"))
            (("channel_pf_cme_onix.directBookDepth"), _settings.directBookDepth, "Defines depth of direct order book for the instrument whose definition wasn't received or had no corresponding data", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(10))
            (("channel_pf_cme_onix.heartbeatInterval"), _settings.heartbeatInterval, "Specifies maximal time interval between two network packets. If no data is received during specified time frame, warning is reported. Interval is measured in seconds", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(30))
            (("channel_pf_cme_onix.impliedBookDepth"), _settings.impliedBookDepth, "Defines depth of implied order book for the instrument whose definition wasn't received or had no corresponding data", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(2))
            (("channel_pf_cme_onix.ioCompletionWaitTime"), _settings.ioCompletionWaitTime, "Defines amount of time Handler spends on socket waiting for I/O completion result. Time is measured in milliseconds", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(10))
            (("channel_pf_cme_onix.logMode"), reinterpret_cast<int32_t&>(_settings.logMode), "Specifies whether the Handler should log its events and which of events should be put into the log", tw::config::EnumOptionNeed::eOptional, boost::optional<int32_t>(static_cast<int32_t>(OnixS::CME::MarketData::LogModes::Regular)))
            (("channel_pf_cme_onix.logDirectory"), _logDirectory, "Log files are stored in this directory. Ignored if logging is disabled", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>("logs"))
            (("channel_pf_cme_onix.logFileNamePrefix"), _logFileNamePrefix, "Log file name are started from the prefix. Ignored if logging is disabled", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>("pfCME"))
            (("channel_pf_cme_onix.logFileSizeLimit"), _settings.logFileSizeLimit, "Log file size limit (in bytes). Handler detaches logged data into separate file upon reaching given size limit", tw::config::EnumOptionNeed::eOptional, boost::optional<long long>(2145386496))
            (("channel_pf_cme_onix.lostPacketWaitTimeout"), _settings.lostPacketWaitTimeout, "Indicates for how long Handler should wait for the packet before consider it as totally lost. Value must be specified in microseconds (usec)", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(1000000))
            (("channel_pf_cme_onix.maxGapRecoveryCacheSize"), _settings.maxGapRecoveryCacheSize, "Defines max value for size of packet cache which is used to put into line out of order multicast packets", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(3))
            (("channel_pf_cme_onix.maxPacketSize"), _settings.maxPacketSize, "Max size for network packet transmitted by MDP", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(4096))
            (("channel_pf_cme_onix.networkInterface"), _networkInterface, "Specifies one or more network interfaces to use while joining the multicast group; use semi-colon delimited list if more than one", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>("eth1"))
            (("channel_pf_cme_onix.networkInterfaceA"), _networkInterfaceA, "Specifies one or more network interfaces to use for \"A\" feeds while joining the multicast group; use semi-colon delimited list if more than one")
            (("channel_pf_cme_onix.networkInterfaceB"), _networkInterfaceB, "Specifies one or more network interfaces to use for \"B\" feeds while joining the multicast group; use semi-colon delimited list if more than one")
            (("channel_pf_cme_onix.packetQueueMaxSize"), _settings.packetQueueMaxSize, "Maximum number of market data packets to be hold while Handler does recovery. To be used to limit the memory usage when order books or TCP Replay recovery takes too much time", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(10000))
            (("channel_pf_cme_onix.packetsPullingStrategy"), reinterpret_cast<int32_t&>(_settings.packetsPullingStrategy), "Defines market data processing strategy", tw::config::EnumOptionNeed::eOptional, boost::optional<int32_t>(static_cast<int32_t>(OnixS::CME::MarketData::PacketsPullingStrategies::Direct)))
            (("channel_pf_cme_onix.recoverSecurityDefinitionsOnGap"), _settings.recoverSecurityDefinitionsOnGap, "Option to recover the Security Definition messages each time when the message sequence gap is detected", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
            (("channel_pf_cme_onix.repeatingGroupEntriesMaxCount"), _settings.repeatingGroupEntriesMaxCount, "Maximum number of repeating group entries", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(1000))
            (("channel_pf_cme_onix.tcpReplayPassword"), _tcpReplayPassword, "Password or passphrase to be used in the Logon (35=A) message from customer to CME")
            (("channel_pf_cme_onix.tcpReplayReconnectAttempts"), _settings.tcpReplayReconnectAttempts, "Number of attempts to receive missed messages via the TCP Replay Channel")
            (("channel_pf_cme_onix.tcpReplayReconnectInterval"), _settings.tcpReplayReconnectInterval, "Interval between the attempts to receive missed packets via the TCP Replay feed if previous attempt either failed or was rejected. Interval is measured in milliseconds", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(500))
            (("channel_pf_cme_onix.tcpReplayRequestTimeframe"), _settings.tcpReplayRequestTimeframe, "Amount of time allocated to process TCP replay request. Handler interrupts request processing if it doesn't accomplish within given time frame. Interval is measured in seconds", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(30))
            (("channel_pf_cme_onix.tcpReplayUsername"), _tcpReplayUsername, "Userid or username to be used in the Logon (35=A) message from customer to CME")
            (("channel_pf_cme_onix.tcpSocketReceiveBufferSize"), _settings.tcpSocketReceiveBufferSize, "Defines size of buffer in bytes for TCP replay receiving socket. Note: Default value is zero which means system default setting will be used", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(0))
            (("channel_pf_cme_onix.udpSocketBufferSize"), _settings.udpSocketBufferSize, "Defines size of buffer in bytes for UDP/multicast sockets. Note: Default value is zero which means system default setting will be used", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(0))
            (("channel_pf_cme_onix.useIncrementalFeedA"), _settings.useIncrementalFeedA, "Option to use the Incremental feed \"A\"", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(true))
            (("channel_pf_cme_onix.useIncrementalFeedB"), _settings.useIncrementalFeedB, "Option to use the Incremental feed \"B\"", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(true))
            (("channel_pf_cme_onix.useInstrumentReplayFeedA"), _settings.useInstrumentReplayFeedA, "Option to use the Incremental Replay feed \"A\"", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(true))
            (("channel_pf_cme_onix.useInstrumentReplayFeedB"), _settings.useInstrumentReplayFeedB, "Option to use the Incremental feed \"B\"", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
            (("channel_pf_cme_onix.useSnapshotFeedA"), _settings.useSnapshotFeedA, "Option to use the Snapshot feed \"A\"", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(true))
            (("channel_pf_cme_onix.useSnapshotFeedB"), _settings.useSnapshotFeedB, "Option to use the Snapshot feed \"B\"", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
            (("channel_pf_cme_onix.useTcpReplay"), _settings.useTcpReplay, "Option to use the TCP Replay feed", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
            (("channel_pf_cme_onix.useUniversalTime"), _settings.useUniversalTime, "Indicates whether local or UTC time is used by the Handler while assigning timestamps to log events and packets received", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(true))
            (("channel_pf_cme_onix.coalesce"), _coalesce, "Indicates whether to coalesce all trades/book changes before clients updates or not", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(true))
        ;
    }
    
    virtual bool parse(const std::string& configFile) {
        const char* DELIM = ",";
        
        if ( !TParent::parse(configFile) )
            return false;
        
        if ( _channelIdsString.empty() || _symbolsFilterString.empty() ) {            
            tw::instr::InstrumentManager::TInstruments instruments = tw::instr::InstrumentManager::instance().getByExchange(tw::instr::eExchange::kCME);
            
            if ( _channelIdsString.empty() ) {
                std::set<std::string> channelIds;
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
                std::set<std::string> channelIds;
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
        
        std::vector<std::string> cpuIndexes;
        boost::split(cpuIndexes, _cpuIndexesString, boost::is_any_of(DELIM));
        std::for_each(cpuIndexes.begin(), cpuIndexes.end(), tw::functional::printHelper<std::string>("List of thread affinity CPUs:"));
        for ( size_t i = 0; i < cpuIndexes.size(); ++i ) {
            if ( !cpuIndexes[i].empty() )
                _cpuIndexes.insert(::atol(cpuIndexes[i].c_str()));
        }
        
        if ( _replay && _replayFolder.empty() ) {
            LOGGER_ERRO << _name << ":: " << "error parsing configFile: " << configFile << " :: 'replayFolder' is not set when 'replayFolder' is true" << "\n";
            return false;
        }
        
        _globalSettings.licenseStore = _licenseStore;
        _settings.channelsConfigurationFile = _channelsConfigurationFile;
        _settings.codingTemplatesFile = _codingTemplatesFile;
        _settings.logDirectory = _logDirectory;
        _settings.logFileNamePrefix = _logFileNamePrefix;
        _settings.networkInterface = _networkInterface;
        _settings.networkInterfaceA = _networkInterfaceA;
        _settings.networkInterfaceB = _networkInterfaceB;
        _settings.tcpReplayPassword = _tcpReplayPassword;
        _settings.tcpReplayUsername = _tcpReplayUsername;
        
        return true;
    }
    
private:
    std::string _licenseStore;
    std::string _channelsConfigurationFile;
    std::string _codingTemplatesFile;
    std::string _logDirectory;
    std::string _logFileNamePrefix;
    std::string _networkInterface;
    std::string _networkInterfaceA;
    std::string _networkInterfaceB;
    std::string _tcpReplayPassword;
    std::string _tcpReplayUsername;
    
    std::string _symbolsFilterString;
    std::string _channelIdsString;
    std::string _cpuIndexesString;
};
    
} // namespace channel_pf_cme
} // namespace tw
