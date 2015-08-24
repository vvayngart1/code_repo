#pragma once

#include <map>

#include <tw/config/settings_config_file.h>

enum EOrderChannelSource
{   
    OrderChannelSource_CNX,
    OrderChannelSource_CME,
    OrderChannelSource_CME_2,
    OrderChannelSource_CME_3,
    OrderChannelSource_CME_4,
    OrderChannelSource_CME_Session,
    OrderChannelSource_EBS
};

class Settings : public tw::config::SettingsConfigFile
{
    typedef tw::config::SettingsConfigFile TParent;
        
    struct RiskParameters {
        typedef int64_t  qty_t; 
    
        qty_t max_absolute_position;
        qty_t max_order_size;
    };
    
public:
    Settings() : TParent("TestConfigInstrument" ) {
        
        boost::optional<size_t> defaultValue = size_t(0);
        
        TParent::add_options()
            ("marketdata.config_file", marketDataConfigFile, "quickfix config for market data")
            ("order_channels.CME.config_file", cmeOrderChannelConfigFiles[OrderChannelSource_CME],"quickfix config file for order channel 'CME'")
            ("order_channels.CME_2.config_file", cmeOrderChannelConfigFiles[OrderChannelSource_CME_2],"quickfix config file for order channel 'CME_2'")
            ("order_channels.CME_3.config_file", cmeOrderChannelConfigFiles[OrderChannelSource_CME_3],"quickfix config file for order channel 'CME_3'")
            ("order_channels.CME_4.config_file", cmeOrderChannelConfigFiles[OrderChannelSource_CME_4],"quickfix config file for order channel 'CME_4'")       
            ("algo.algo_id", algoId, "unique id for this strategy instance", tw::config::EnumOptionNeed::eRequired)
            ("algo.user_id", userId, "user id of the trader operating this strategy instance", tw::config::EnumOptionNeed::eRequired)
            ("algo.account_id", accountId, "account id of the trader operating this strategy instance", tw::config::EnumOptionNeed::eRequired)
            ("algo.max_queue_size", max_queue_size, "maximum queue size for the algo to trigger", tw::config::EnumOptionNeed::eOptional, boost::optional<size_t>(0))
            ("algo.quote_latency_slip", quote_latency_slip, "slip for quote latency delay", tw::config::EnumOptionNeed::eOptional, boost::optional<double>(10.0))
            ("algo.start_immediately", startImmediately, "useful for FIXTest", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(false))
            ("activemq.host", activemqConfig["ACTIVEMQ_host"], "ActiveMQ host name", tw::config::EnumOptionNeed::eRequired)
            ("activemq.port", activemqConfig["ACTIVEMQ_port"], "ActiveMQ port nb", tw::config::EnumOptionNeed::eRequired)
            ("activemq.user", activemqConfig["ACTIVEMQ_user"], "ActiveMQ user name")
            ("activemq.password", activemqConfig["ACTIVEMQ_password"], "ActiveMQ password", tw::config::EnumOptionNeed::eRequired)
            ("auto_hedger.slip", hedge_slip, "Hedge slip", tw::config::EnumOptionNeed::eRequired)
            ("auto_hedger.algo_symbol", hedge_algo_symbol, "Internal Hedge Symbol", tw::config::EnumOptionNeed::eRequired)
            ("auto_hedger.trade_symbol", hedge_order_symbol, "Hedge Symbol used on order", tw::config::EnumOptionNeed::eRequired)
            ("isa.ab_risk_limit", isa_ab_risk_limit, "Max ISA position for spread AB", tw::config::EnumOptionNeed::eOptional, boost::optional<int>(0))
            ("isa.bc_risk_limit", isa_bc_risk_limit, "Max ISA position for spread BC", tw::config::EnumOptionNeed::eOptional, boost::optional<int>(0))
            ("wombat.source_name", wombat_source_name, "Wombat source name", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>("CME"))
            ("wombat.port_name", wombat_port_name, "Wombat port name", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>("ldma_tport"))
            ("srlabs.config_location", srlabs_config_location, "SR Labs configuration file location", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>(""))
            ("srlabs.config_name", srlabs_config_name, "SR Labs configuration file name", tw::config::EnumOptionNeed::eOptional, boost::optional<std::string>(""))
            ("srlabs.process_trades", process_trades, "SR Labs process trade messages defaults to true", tw::config::EnumOptionNeed::eOptional, boost::optional<bool>(true))
            ("cpu_affinity.critical_thread", critical_thread_cpu_affinity, "Cpu affinity for the critical thread", tw::config::EnumOptionNeed::eOptional, boost::optional<int32_t>(-1))
            ("risk.max_order_size", riskParameters.max_order_size, "Max order size", tw::config::EnumOptionNeed::eRequired)
            ("risk.max_absolute_position", riskParameters.max_absolute_position, "Max absolute position", tw::config::EnumOptionNeed::eRequired)
            ("risk.max_nb_circuit_trade", max_nb_trade, "Max number of trade", tw::config::EnumOptionNeed::eRequired)
            ;
    }        

public:
    std::string marketDataConfigFile;
    std::map<EOrderChannelSource, std::string> cmeOrderChannelConfigFiles;
    std::string algoId;
    std::string userId;
    std::string accountId;
    bool startImmediately;
    std::map<std::string, std::string> activemqConfig;
    RiskParameters riskParameters;
    int32_t max_nb_trade;
    size_t max_queue_size;
    double quote_latency_slip;
    float hedge_slip;
    std::string hedge_algo_symbol;
    std::string hedge_order_symbol;

    std::string wombat_source_name;
    std::string wombat_port_name;

    std::string srlabs_config_location;
    std::string srlabs_config_name;

    int32_t isa_ab_risk_limit;
    int32_t isa_bc_risk_limit;
    
    int32_t critical_thread_cpu_affinity; // -1 dont set

    bool process_trades;
};
