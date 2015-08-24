#include <tw/channel_or_drop_cme/settings.h>
#include <tw/channel_or_drop_cme/channel_or_drop_onix.h>

#include <tw/channel_or/channel_or_storage.h>
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
    
    tw::channel_or_drop_cme::ChannelOrDropOnix dropClient;
    if ( !dropClient.start(settings) )
        return -1;
    
    tw::common::SignalCatcher signalCatcher;
    signalCatcher.run();
    
    dropClient.stop();
    
    tw::log::Logger::instance().stop();
    
    return 0;
}
