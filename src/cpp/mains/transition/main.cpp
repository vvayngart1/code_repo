#include <tw/generated/version.h>
#include <tw/common_strat/strategy_container.h>
#include <tw/common/signal_catcher.h>

#include "SelectorTransition.h"
#include "Transition.h"

int main(int argc, char* argv[])
{
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
    
    if ( !settings.validate() ) {
        LOGGER_ERRO << "failed to validate config: " << cmndLine.configFile() << " :: " << settings.toStringDescription() << "\n";
        return -1;
    }
    
    if ( !tw::log::Logger::instance().start(settings) ) {
        LOGGER_ERRO << "failed to start logger: " << settings.toStringDescription() << "\n";
        return -1;
    }

    LOGGER_WARN << "VERSION:" << VERSION << "\n";
    
    if ( !tw::common_strat::StrategyContainer::instance().init(settings) )
        return -1;
    
    tw::transition::SelectorTransition selector;
    if ( !selector.run(settings) ) {
        LOGGER_ERRO << "failed to run selector: " << settings.toStringDescription() << "\n";
        return -1;
    }
    
    std::vector<tw::transition::Transition> strats;
    tw::transition::SelectorTransition::TPortfolio params = selector.getPortfolio();
    strats.resize(params.size());
    
    // Register all strategies with framework
    //
    for ( size_t i = 0; i < params.size(); ++i ) {
        tw::transition::Transition::TImpl& s = strats[i].getImpl();
        tw::transition::SelectorTransition::TPortfolio::value_type& p = params[i]; 
        
        s.init(p);
        if ( !tw::common_strat::StrategyContainer::instance().add(&s) ) {
            LOGGER_ERRO << "Failed to add to StrategyContainer logic instance w/params: " << p.toStringVerbose() << "\n";
            return -1;
        }

        if ( !tw::common_strat::StrategyContainer::instance().addSubscriptionsPf(&s) ) {
            LOGGER_ERRO << "Failed to addSubscriptionsPf to StrategyContainer for logic instance w/params: " << p.toStringVerbose() << "\n";
            return -1;
        }
    }
    
    if ( !tw::common_strat::StrategyContainer::instance().start() )
        return -1;

    tw::common::SignalCatcher signalCatcher;
    signalCatcher.run();

    tw::common_strat::StrategyContainer::instance().stop();
    tw::log::Logger::instance().stop();
    
    return 0;
}
