#include "order_entry.h"

#include <tw/channel_or_cme/settings.h>
#include <tw/common/signal_catcher.h>

int main(int argc, char * argv[]) {
    tw::config::SettingsCmndLineCommonUse cmndLine;
    if ( !cmndLine.parse(argc, argv) ) {
        LOGGER_ERRO << "failed to parse command line arguments" << cmndLine.toStringDescription() << "\n";
        return -1;
    }

    if ( cmndLine.help() ) {
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
    
    OrderEntry orderEntry;
    if ( !tw::common_strat::StrategyContainer::instance().init(settings) )
        return -1;
    
    if ( !tw::common_strat::StrategyContainer::instance().add(&orderEntry) )
        return -1;
    
    if ( !tw::common_strat::StrategyContainer::instance().start() )
        return -1;
    
    // TODO: delete after testing is done
    //
    tw::channel_or_cme::Settings cme_settings;
    if ( !cme_settings.parse(settings._channel_or_cme_dataSource) )
        return -1;
    
    tw::common::SignalCatcher signalCatcher;
    signalCatcher.run();
    
    tw::common_strat::StrategyContainer::instance().stop();
    tw::log::Logger::instance().stop();
    
    return 0;
}
