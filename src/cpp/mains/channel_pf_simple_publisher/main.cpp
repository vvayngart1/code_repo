#include "publisher.h"

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
    
    // Disable realtime quotes if in replay mode
    //
    if ( tw::common::ePublisherPfMode::kReplay == settings._publisher_pf_mode )
        settings._strategy_container_channel_pf = false;
    
    if ( !tw::log::Logger::instance().start(settings) ) {
        LOGGER_ERRO << "failed to start logger: " << settings.toStringDescription() << "\n";
        return -1;
    }
    
    Publisher publisher;
    if ( !tw::common_strat::StrategyContainer::instance().init(settings) )
        return -1;
    
    if ( !tw::common_strat::StrategyContainer::instance().add(&publisher) )
        return -1;
    
    if ( !tw::common_strat::StrategyContainer::instance().addSubscriptionsPf(&publisher) )
        return -1;
    
    if ( !tw::common_strat::StrategyContainer::instance().start() )
        return -1;
    
    switch(settings._publisher_pf_mode) {
        case tw::common::ePublisherPfMode::kReplay:
        case tw::common::ePublisherPfMode::kReplayVerify:
        case tw::common::ePublisherPfMode::kReplayVerifyWithTrades:
            publisher.waitForReplayToFinish();
            break;
        default:
        {
            tw::common::SignalCatcher signalCatcher;
            signalCatcher.run();
        }
            break;
    }
    
    std::cout << "Removing subscriptions..." << std::endl;
    tw::common_strat::StrategyContainer::instance().removeSubscriptionsPf(&publisher);
    std::cout << "Removed subscriptions" << std::endl;
    
    std::cout << "Removing publisher..." << std::endl;
    tw::common_strat::StrategyContainer::instance().remove(&publisher);
    std::cout << "Removed publisher" << std::endl;
    
    std::cout << "Stopping StrategyContainer..." << std::endl;
    tw::common_strat::StrategyContainer::instance().stop();
    std::cout << "Stopped StrategyContainer" << std::endl;
    
    tw::log::Logger::instance().stop();    
    
    return 0;
}
