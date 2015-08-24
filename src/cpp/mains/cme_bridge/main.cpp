#include <tw/channel_or_cme_bridge/route_manager_cme.h>

#include <tw/config/settings_cmnd_line.h>
#include <tw/config/settings_config_file.h>
#include <tw/channel_or_cme/settings.h>
#include <tw/instr/instrument_manager.h>
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
        
    tw::channel_or_cme_bridge::RouteManagerCME routeManager;
    if ( !routeManager.start(settings) )
        return -1;
    
    tw::common::SignalCatcher signalCatcher;
    signalCatcher.run();
    
    routeManager.stop();
    tw::log::Logger::instance().stop();
    
    return 0;
}
