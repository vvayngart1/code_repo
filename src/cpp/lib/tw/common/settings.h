#pragma once

#include <tw/config/settings_config_file.h>

namespace tw {
namespace common {
    
struct eChannelPfHistoricalMode {
    enum _ENUM {
        kUnknown=0,
        
        kNanex=1,
        kAMR=2
    };
    
    eChannelPfHistoricalMode () {
        _enum = kUnknown;
    }
    
    eChannelPfHistoricalMode (_ENUM value) {
        _enum = value;
    }
    
    bool isValid() const {
        return (_enum != kUnknown);
    }
    
    operator _ENUM() const {
        return _enum;
    }

    const char* toString() const {
        switch (_enum) {

            case kNanex: return "Nanex"; 
            case kAMR: return "AMR"; 
            default: return "Unknown";
        }
    }  

    _ENUM _enum;
};
    
struct ePublisherPfMode {
    enum _ENUM {
        kUnknown=0,

        kRealtime=1,
        kRecord=2,
        kReplay=3,
        kReplayVerify=4,
        kReplayVerifyWithTrades=5
    };
    
    ePublisherPfMode () {
        _enum = kUnknown;
    }
    
    ePublisherPfMode (_ENUM value) {
        _enum = value;
    }
    
    bool isValid() const {
        return (_enum != kUnknown);
    }
    
    operator _ENUM() const {
        return _enum;
    }

    const char* toString() const {
        switch (_enum) {

            case kRealtime: return "Realtime"; 
            case kRecord: return "Record"; 
            case kReplay: return "Replay"; 
            case kReplayVerify: return "ReplayVerify"; 
            case kReplayVerifyWithTrades: return "ReplayVerifyWithTrades"; 
            default: return "Unknown";
        }
    }  

    _ENUM _enum;
};

struct ePublisherPfSleepMode {
    enum _ENUM {
        kUnknown=0,

        kLinuxAPI=1,
        kBusyWait=2
    };
    
    ePublisherPfSleepMode () {
        _enum = kUnknown;
    }
    
    ePublisherPfSleepMode (_ENUM value) {
        _enum = value;
    }
    
    bool isValid() const {
        return (_enum != kUnknown);
    }
    
    operator _ENUM() const {
        return _enum;
    }

    const char* toString() const {
        switch (_enum) {

            case kLinuxAPI: return "LinuxAPI"; 
            case kBusyWait: return "BusyWait"; 
            default: return "Unknown";
        }
    }  

    _ENUM _enum;
};

struct eBarsRestoreMode {
    enum _ENUM {
        kUnknown=0,
        
        kDb=1,
        kFile=2
    };
    
    eBarsRestoreMode () {
        _enum = kUnknown;
    }
    
    eBarsRestoreMode (_ENUM value) {
        _enum = value;
    }
    
    bool isValid() const {
        return (_enum != kUnknown);
    }
    
    operator _ENUM() const {
        return _enum;
    }

    const char* toString() const {
        switch (_enum) {

            case kDb: return "Db"; 
            case kFile: return "File"; 
            default: return "Unknown";
        }
    }  

    _ENUM _enum;
};

struct ePnLAuditMode {
    enum _ENUM {
        kUnknown=0,
        
        kAccount=1,
        kStrat=2,
        kBoth=3,
        kPnLAnalysis=4
    };
    
    ePnLAuditMode () {
        _enum = kUnknown;
    }
    
    ePnLAuditMode (_ENUM value) {
        _enum = value;
    }
    
    bool isValid() const {
        return (_enum != kUnknown);
    }
    
    operator _ENUM() const {
        return _enum;
    }

    const char* toString() const {
        switch (_enum) {

            case kAccount: return "Account"; 
            case kStrat: return "Strat"; 
            case kBoth: return "Both";
            case kPnLAnalysis: return "PnLAnalysis";
            default: return "Unknown";
        }
    }  

    _ENUM _enum;
};

struct eStratQuoteProcessingMode {
    enum _ENUM {
        kUnknown=0,
        
        kTrades=1,
        kL1=2,
        kTradesL1Prices=3,
        kTradesL1=4
    };
    
    eStratQuoteProcessingMode () {
        _enum = kUnknown;
    }
    
    eStratQuoteProcessingMode (_ENUM value) {
        _enum = value;
    }
    
    bool isValid() const {
        return (_enum != kUnknown);
    }
    
    operator _ENUM() const {
        return _enum;
    }

    const char* toString() const {
        switch (_enum) {

            case kTrades: return "Trades"; 
            case kL1: return "L1"; 
            case kTradesL1Prices: return "TradesL1Prices";
            case kTradesL1: return "TradesL1";
            default: return "Unknown";
        }
    }  

    _ENUM _enum;
};

struct eStratClosesMode {
    enum _ENUM {
        kUnknown=0,
        
        kBars=1,
        kWbos=2,
        kWbosAverage=3
    };
    
    eStratClosesMode () {
        _enum = kUnknown;
    }
    
    eStratClosesMode (_ENUM value) {
        _enum = value;
    }
    
    bool isValid() const {
        return (_enum != kUnknown);
    }
    
    operator _ENUM() const {
        return _enum;
    }

    const char* toString() const {
        switch (_enum) {
            case kBars: return "Bars"; 
            case kWbos: return "Wbos";
            case kWbosAverage: return "WbosAverage";
            default: return "Unknown";
        }
    }  

    _ENUM _enum;
};


// Global settings file
//
class Settings : public tw::config::SettingsConfigFile
{
    typedef tw::config::SettingsConfigFile TParent;
    
public:
    Settings() : TParent("GlobalSettings" ) {
        TParent::add_options()
            (("log.enabled"), _log_enabled, "specifies if logging is enabled", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(true))
            (("log.toConsole"), _log_toConsole, "specifies if to log output to console", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(true))
            (("log.toFile"), _log_toFile, "specifies if to log output to file", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(true))
            (("log.fileName"), _log_fileName, "specifies filename of the log file", tw::config::EnumOptionNeed::eOptional)
            (("log.level"), _log_level, "specifies log level - cna be: INFO, WARN, ERROR", tw::config::EnumOptionNeed::eOptional)
            (("log.maxQueueSize"), _log_maxQueueSize, "specifies max size of queued log messages to output", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(1000))
        
            (("db.connection_str"), _db_connection_str, "specifies db connection string")
            (("db.listen_connection_str"), _db_listen_connection_str, "specifies db connection string for Vert which is object of listener")    
            
            (("trading.account"), _trading_account, "specifies trading account")
            (("trading.print_pnl"), _trading_print_pnl, "specifies if to log pnl on every update for debugging purposes", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
            (("trading.auto_pilot"), _trading_auto_pilot, "specifies if to run strategy without UI controller", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
            (("trading.start_delay_sec"), _trading_start_delay_sec, "specifies delay from start of strat to turn strat on. 0 disables", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(0))
            (("trading.recording_mode"), _trading_recording_mode, "specifies if to run strategy in just recording mode", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
            (("trading.regulate"), _trading_regulate, "specifies if to run strategy without regulator", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(true))
            (("trading.stale_quotes_timeout"), _trading_stale_quotes_timeout, "specifies timeout in ms to consider quotes stale", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(10000))
            (("trading.wtp_use_quotes"), _trading_wtp_use_quotes, "specifies if to use book/trades in determining if it's a wash trade", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(true))
            (("trading.strat_quote_processing_mode"), reinterpret_cast<int32_t&>(_trading_strat_quote_processing_mode), "specifies which changes (e.g. book/trades or both) to process in strat", tw::config::EnumOptionNeed::eOptional, boost::optional<int32_t>(static_cast<int32_t>(eStratQuoteProcessingMode::kTradesL1)))
            (("trading.strat_closes_mode"), reinterpret_cast<int32_t&>(_trading_strat_closes_mode), "specifies which changes (e.g. book/trades or both) to process in strat", tw::config::EnumOptionNeed::eOptional, boost::optional<int32_t>(static_cast<int32_t>(eStratClosesMode::kBars)))
            (("trading.markets_prelim_counter"), _markets_prelim_counter, "specifies minimum number of marked markets to consider prelim total PnL", tw::config::EnumOptionNeed::eOptional, boost::optional<int32_t>(10000))
            (("trading.markets_finish_counter"), _markets_finish_counter, "specifies maximum number of marked markets to unconditionally exit portfolio", tw::config::EnumOptionNeed::eOptional, boost::optional<int32_t>(20000))
            (("trading.markets_prelim_cutoff"), _markets_prelim_cutoff, "if @ markets_prelim_counter, total PnL is smaller, then exit portfolio", tw::config::EnumOptionNeed::eOptional, boost::optional<int32_t>(10000))
            
            (("trading.verbose"), _trading_verbose, "specifies if to use verbose mode in trading strategy", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
        
            (("trading.listener_host"), _trading_listener_host, "listener ip address", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>("localhost"))
            (("trading.listener_port"), _trading_listener_port, "listener port", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>("123456"))
            
            (("instruments.createQuotesOnLoad"), _instruments_createQuotesOnLoad, "specifies if to create quote objects at instrument loading", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(true))
            (("instruments.dataSourceType"), _instruments_dataSourceType, "data source type (e.g. 'file', 'db', etc.)", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>("file"))
            (("instruments.dataSource"), _instruments_dataSource, "data source string (e.g file name, db connection string)")
            (("instruments.dataSourceTypeOutput"), _instruments_dataSourceTypeOutput, "data source type of output for instrument loader(e.g. 'file', 'db', etc.)", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>("file"))
            (("instruments.updateSettlements"), _instruments_updateSettlements, "specifies if to update settlements table ", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
            (("instruments.settlementsPath"), _instruments_settlementsPath, "dir name to settlements files", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>(""))
            (("instruments.verbose"), _instruments_verbose, "specifies if to print instruments infos on start up ", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
        
            (("common_comm.tcp_ip_server_port"), _common_comm_tcp_ip_server_port, "specifies tcp ip server port number")
            (("common_comm.heartbeat_interval"), _common_comm_heartbeat_interval, "specifies tcp ip server heartbeat interval for clients", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(10000))
            (("common_comm.verbose"), _common_comm_verbose, "specifies if tcp/ip connection is verbose", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
            (("common_comm.tcp_verbose"), _common_comm_tcp_verbose, "specifies if logging tcp/ip traffic", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
            (("common_comm.tcp_no_delay"), _common_comm_tcp_no_delay, "specifies if tcp/ip nagle is disabled or not", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
        
            (("strategy_container.channel_pf"), _strategy_container_channel_pf, "specifies if to create channel_pf objects", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(true))
            (("strategy_container.channel_or"), _strategy_container_channel_or, "specifies if to create channel_or objects", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(true))
            (("strategy_container.dataSourceType"), _strategy_container_dataSourceType, "data source type (e.g. 'file', 'db', etc.)", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>("file"))
            (("strategy_container.dataSource"), _strategy_container_dataSource, "data source string (e.g file name, db connection string)")
            (("strategy_container.stuck_orders_timeout"), _strategy_container_stuck_orders_timeout, "time interval to consider order to be 'stuck'", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(1500))
        
            (("channel_pf_cme.dataSourceType"), _channel_pf_cme_dataSourceType, "data source type (e.g. 'file', 'db', etc.)", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>("file"))
            (("channel_pf_cme.dataSource"), _channel_pf_cme_dataSource, "data source string (e.g file name, db connection string)")
            (("channel_pf_cme.isMultithreaded"), _channel_pf_cme_isMultithreaded, "specifies if channel_pf_cme has separate thread for quotes notifications to clients", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
   
            (("channel_pf_historical.mode"), reinterpret_cast<int32_t&>(_channel_pf_historical_mode), "specifies if/how to process historical prices", tw::config::EnumOptionNeed::eOptional, boost::optional<int32_t>(static_cast<int32_t>(eChannelPfHistoricalMode::kUnknown)))
            (("channel_pf_historical.host"), _channel_pf_historical_host, "ip of historical price feed host", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>("127.0.0.1"))
            (("channel_pf_historical.port"), _channel_pf_historical_port, "port of historical price feed host", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(3001))
            (("channel_pf_historical.date"), _channel_pf_historical_date, "date for historical start", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>(""))
            (("channel_pf_historical.time"), _channel_pf_historical_time, "time for historical start", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>(""))
            
            (("channel_or.dataSourceType"), _channel_or_dataSourceType, "data source type (e.g. 'file', 'db', etc.)", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>("file"))
            (("channel_or.dataSource"), _channel_or_dataSource, "data source string (e.g file name, db connection string)")
        
            (("channel_or_cme.dataSourceType"), _channel_or_cme_dataSourceType, "data source type (e.g. 'file', 'db', etc.)", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>("file"))
            (("channel_or_cme.dataSource"), _channel_or_cme_dataSource, "data source string (e.g file name, db connection string)")
        
            (("channel_or_drop_cme.dataSourceType"), _channel_or_drop_cme_dataSourceType, "data source type (e.g. 'file', 'db', etc.)", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>("file"))
            (("channel_or_drop_cme.dataSource"), _channel_or_drop_cme_dataSource, "data source string (e.g file name, db connection string)")
        
            (("audit_cme.outputFileName"), _audit_cme_outputFileName, "cme dir/file name to output audit files to")
            
            (("spreader_perf_reporter.report_instr_blotter"), _spreader_perf_reporter_report_instr_blotter, "specifies if to report instr blotter for all strategies combined", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
        
            (("bars_manager.bars_restore_mode"), reinterpret_cast<int32_t&>(_bars_manager_bars_restore_mode), "specifies whether to restore bars from database or file", tw::config::EnumOptionNeed::eOptional, boost::optional<int32_t>(static_cast<int32_t>(eBarsRestoreMode::kUnknown)))
        
            (("bars_storage.outputToDb"), _bars_storage_outputToDb, "specifies if to save bars and bars patterns to db", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
            (("bars_storage.processPatterns"), _bars_storage_processPatterns, "specifies if to process bars patterns", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(true))
        
            (("bars_storage.max_queue_size_db"), _bars_storage_max_queue_size_db, "storage's max memory queue size for db", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(1000))
            (("bars_storage.batch_cache_size"), _bars_storage_batch_cache_size, "storage's batch cache size", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(100))
            (("bars_storage.max_batch_cache_size"), _bars_storage_max_batch_cache_size, "storage's max batch cache size", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(400))
            (("bars_storage.historical_bars_source"), _bars_storage_historical_bars_source, "source (e.g. bars_recorder) of historical bars")
        
            (("vert_storage.outputToDb"), _vert_storage_outputToDb, "specifies if to save vert info to db", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
            (("vert_storage.max_queue_size_db"), _vert_storage_max_queue_size_db, "storage's max memory queue size for db", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(1000))
            (("vert_storage.batch_cache_size"), _vert_storage_batch_cache_size, "storage's batch cache size", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(100))
            (("vert_storage.max_batch_cache_size"), _vert_storage_max_batch_cache_size, "storage's max batch cache size", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(400))
            (("vert_storage.update_interval_ms"), _vert_storage_update_interval_ms, "sampling frequency in milliseconds", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(1000))
        
            (("vert_storage.vert_analytics_db_connection_str"), _vert_storage_vert_analytics_db_connection_str, "specifies db connection string to vert analytics", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>(""))
       
            (("vert_storage.vert_score_server_clients_dbs"), _vert_storage_vert_score_server_clients_dbs, "specifies db connection strings for clients' dbs", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>(""))
            (("vert_storage.vert_score_server_clients_trading_accounts"), _vert_storage_vert_score_server_clients_trading_accounts, "specifies clients' trading accounts", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>(""))
        
            (("publisher_pf.mode"), reinterpret_cast<int32_t&>(_publisher_pf_mode), "specifies publisher_pf publishes realtime prices, stores realtime prices or replays stored historical prices", tw::config::EnumOptionNeed::eOptional, boost::optional<int32_t>(static_cast<int32_t>(ePublisherPfMode::kRealtime)))
            (("publisher_pf.sleep_mode"), reinterpret_cast<int32_t&>(_publisher_pf_sleep_mode), "specifies publisher_pf replay sleep timer type", tw::config::EnumOptionNeed::eOptional, boost::optional<int32_t>(static_cast<int32_t>(ePublisherPfSleepMode::kLinuxAPI)))
            (("publisher_pf.log_dir"), _publisher_pf_log_dir, "specifies publisher_pf log dir for historical prices", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>(""))
            (("publisher_pf.replay_filename"), _publisher_pf_replay_filename, "specifies publisher_pf replay filename for historical prices", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>(""))
            (("publisher_pf.start_time"), _publisher_pf_start_time, "specifies publisher_pf replay start time for historical prices", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>(""))
            (("publisher_pf.end_time"), _publisher_pf_end_time, "specifies publisher_pf replay end time for historical prices", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>(""))
            (("publisher_pf.symbol"), _publisher_pf_symbol, "specifies publisher_pf symbol for historical prices replayVerify() mode, empty value indicates all symbols", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>(""))
            (("publisher_pf.confirm_break"), _publisher_pf_confirm_break, "specifies if to confirm breakout in sim trades in replayVerify() mode", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
        
            (("publisher_pf.range"), _publisher_pf_range, "range in ticks to analyze", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(0))
            (("publisher_pf.minVol"), _publisher_pf_minVol, "range's minVol to analyze", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(0))
            (("publisher_pf.stopInTicks"), _publisher_pf_stopInTicks, "range's trade stop to analyze", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(0))
            (("publisher_pf.minVolRatioFor"), _publisher_pf_minVolRatioFor, "range's minVolRatioFor to analyze", tw::config::EnumOptionNeed::eOptional, boost::optional<float>(0.0))
            (("publisher_pf.tradeDuration"), _publisher_pf_tradeDuration, "range's tradeDuration to analyze", tw::config::EnumOptionNeed::eOptional, boost::optional<uint32_t>(0))
        
            (("pnl_audit.mode"), reinterpret_cast<int32_t&>(_pnl_audit_mode), "specifies whether pnl_audit produces results for account, strat or both", tw::config::EnumOptionNeed::eOptional, boost::optional<int32_t>(static_cast<int32_t>(ePnLAuditMode::kBoth)))
            (("pnl_audit.start_time"), _pnl_audit_start_time, "specifies start_time for pnl_audit records", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>(""))
            (("pnl_audit.end_time"), _pnl_audit_end_time, "specifies start_time for pnl_audit records", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>(""))
            
        ;
    }
    
public:
    bool _log_enabled;
    bool _log_toConsole;
    bool _log_toFile;    
    std::string _log_fileName;
    std::string _log_level;
    uint32_t _log_maxQueueSize;
    
    std::string _db_connection_str;
    std::string _db_listen_connection_str;
    
    std::string _trading_account;
    bool _trading_print_pnl;
    bool _trading_auto_pilot;
    uint32_t _trading_start_delay_sec;
    bool _trading_recording_mode;
    bool _trading_regulate;
    uint32_t _trading_stale_quotes_timeout;
    bool _trading_wtp_use_quotes;
    eStratQuoteProcessingMode _trading_strat_quote_processing_mode;
    eStratClosesMode _trading_strat_closes_mode;
    int32_t _markets_prelim_counter;
    int32_t _markets_finish_counter;
    int32_t _markets_prelim_cutoff;
    bool _trading_verbose;
    
    std::string _trading_listener_host;
    std::string _trading_listener_port;
    
    bool _instruments_createQuotesOnLoad;
    std::string _instruments_dataSourceType;
    std::string _instruments_dataSource;
    std::string _instruments_dataSourceTypeOutput;
    bool _instruments_updateSettlements;
    std::string _instruments_settlementsPath;
    bool _instruments_verbose;
    
    uint32_t _common_comm_tcp_ip_server_port;
    uint32_t _common_comm_heartbeat_interval;
    bool _common_comm_verbose;
    bool _common_comm_tcp_verbose;
    bool _common_comm_tcp_no_delay;
    
    bool _strategy_container_channel_pf;
    bool _strategy_container_channel_or;
    std::string _strategy_container_dataSourceType;
    std::string _strategy_container_dataSource;
    uint32_t _strategy_container_stuck_orders_timeout;
    
    std::string _channel_pf_cme_dataSourceType;
    std::string _channel_pf_cme_dataSource;
    bool _channel_pf_cme_isMultithreaded;
    
    eChannelPfHistoricalMode _channel_pf_historical_mode;
    std::string _channel_pf_historical_host;
    uint32_t _channel_pf_historical_port;
    std::string _channel_pf_historical_date;
    std::string _channel_pf_historical_time;
    
    std::string _channel_or_dataSourceType;
    std::string _channel_or_dataSource;
    
    std::string _channel_or_cme_dataSourceType;
    std::string _channel_or_cme_dataSource;
    
    std::string _channel_or_drop_cme_dataSourceType;
    std::string _channel_or_drop_cme_dataSource;
    
    std::string _audit_cme_outputFileName;
    
    bool _spreader_perf_reporter_report_instr_blotter;
    
    eBarsRestoreMode _bars_manager_bars_restore_mode;
    
    bool _bars_storage_outputToDb;
    bool _bars_storage_processPatterns;
    uint32_t _bars_storage_max_queue_size_db;
    uint32_t _bars_storage_batch_cache_size;
    uint32_t _bars_storage_max_batch_cache_size;
    std::string _bars_storage_historical_bars_source;
    
    bool _vert_storage_outputToDb;
    uint32_t _vert_storage_max_queue_size_db;
    uint32_t _vert_storage_batch_cache_size;
    uint32_t _vert_storage_max_batch_cache_size;
    uint32_t _vert_storage_update_interval_ms;
    std::string _vert_storage_vert_analytics_db_connection_str;
    std::string _vert_storage_vert_score_server_clients_dbs;
    std::string _vert_storage_vert_score_server_clients_trading_accounts;
    
    ePublisherPfMode _publisher_pf_mode;
    ePublisherPfSleepMode _publisher_pf_sleep_mode;
    std::string _publisher_pf_log_dir;
    std::string _publisher_pf_replay_filename;
    std::string _publisher_pf_start_time;
    std::string _publisher_pf_end_time;
    std::string _publisher_pf_symbol;
    bool _publisher_pf_confirm_break;
    uint32_t _publisher_pf_range;
    uint32_t _publisher_pf_minVol;
    uint32_t _publisher_pf_stopInTicks;
    float _publisher_pf_minVolRatioFor;
    uint32_t _publisher_pf_tradeDuration;
    
    ePnLAuditMode _pnl_audit_mode;
    std::string _pnl_audit_start_time;
    std::string _pnl_audit_end_time;
    
};
    
} // namespace channel_pf_cme
} // namespace tw
