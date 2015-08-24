#include <tw/config/settings_cmnd_line.h>
#include <tw/instr/instrument_manager.h>

#include <tw/channel_pf_cme/settings.h>

#include <tw/common_strat/istrategy.h>
#include <tw/common_strat/strategy_container.h>

class TestStrategy : public tw::common_strat::IStrategy {
public:
    TestStrategy() {
        _params._name = "TestStrategy";
    }
    
public:
    virtual bool init(const tw::common::Settings& settings, const tw::risk::Strategy& strategyParams) {
        if ( !IStrategy::init(settings, strategyParams) )
            return false;
        
        tw::channel_pf_cme::Settings settingsCme;
        if (!settingsCme.parse(settings._channel_pf_cme_dataSourceType)) {
            LOGGER_ERRO << "failed to parse config file settings: " << settings._channel_pf_cme_dataSourceType << " :: " << settingsCme.toStringDescription() << "\n";
            return false;
        }
        
        if ( settingsCme._symbols.empty() ) {
            LOGGER_ERRO << "No symbols' specified to filter" << "\n";
            return false;
        }
        
        for ( uint32_t count = 0; count < settingsCme._symbols.size(); ++count ) {
            addQuoteSubscription(settingsCme._symbols[count]);
        }
        
        return true;
    }
    
    virtual bool start() {
        return true;
    }
    
    virtual bool stop() {
        return true;
    }
    
    virtual void recordExternalFill(const tw::channel_or::Fill& fill) {
        
    }
    
    bool rebuildOrder(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej) {
        return false;
    }
    
    void rebuildPos(const tw::channel_or::PosUpdate& update) {
        return;
    }
    
    void onConnectionUp(tw::common_strat::TMsgBusConnection::native_type id, tw::common_strat::TMsgBusConnection::pointer connection) {
        
    }
    
    virtual void onConnectionDown(tw::common_strat::TMsgBusConnection::native_type id) {
        
    }
    
    virtual void onConnectionData(tw::common_strat::TMsgBusConnection::native_type id, const tw::common::Command& cmnd) {
        
    }
    
    virtual void onAlert(const tw::channel_or::Alert& alert) {
        
    }
        
public:
    void onQuote(const tw::price::Quote& quote) {
        LOGGER_INFO << quote.toString() << "\n";
    }    
};

int main(int argc, char * argv[]) {
    tw::config::SettingsCmndLineCommonUse cmndLine;
    if (!cmndLine.parse(argc, argv)) {
        LOGGER_ERRO << "failed to parse command line arguments" << cmndLine.toStringDescription() << "\n";
        return -1;
    }

    if (cmndLine.help()) {
        LOGGER_INFO << "<USAGE>: " << cmndLine.toStringDescription() << "\n";
        return 0;
    }
    
    tw::common::Settings settings;
    if ( !settings.parse(cmndLine.configFile()) ) {
        LOGGER_ERRO << "failed to parse config: " << cmndLine.configFile() << " :: " << settings.toStringDescription() << "\n";
        return -1;
    }
    
    if ( !tw::log::Logger::instance().start(settings) ) {
        LOGGER_ERRO << "failed to start logger: " << settings.toStringDescription() << "\n";
        return -1;
    }
    
    TestStrategy strategy;
    if ( !tw::common_strat::StrategyContainer::instance().init(settings) )
        return -1;
    
    if ( !tw::common_strat::StrategyContainer::instance().add(&strategy) )
        return -1;
    
    if ( !tw::common_strat::StrategyContainer::instance().start() )
        return -1;
    
    std::cout << "\n" << "Press ENTER to exit. " << "\n";    
    
    char userInput; 
    std::cin.getline(&userInput, 1);
    
    tw::common_strat::StrategyContainer::instance().remove(&strategy);    
    tw::common_strat::StrategyContainer::instance().stop();
    tw::log::Logger::instance().stop();
    
    return 0;
}