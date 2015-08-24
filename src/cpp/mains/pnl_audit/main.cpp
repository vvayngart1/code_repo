#include <tw/generated/version.h>
#include <tw/log/defs.h>
#include <tw/instr/instrument_manager.h>
#include <tw/common/defs.h>
#include <tw/common/settings.h>
#include <tw/config/settings_cmnd_line.h>

#include "SelectorPnLAudit.h"
#include "PnLAudit.h"

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
    
    if ( !tw::instr::InstrumentManager::instance().loadInstruments(settings) ) {
        LOGGER_ERRO << "failed to load instruments: " << settings._instruments_dataSource << "\n";
        return -1;
    }
    
    tw::pnl_audit::SelectorPnLAudit selector;
    if ( !selector.start(settings) ) {
        LOGGER_ERRO << "failed to start selector: " << settings.toStringDescription() << "\n";
        return -1;
    }
    
    if ( !selector.populateAccounts(settings) ) {
        LOGGER_ERRO << "failed to run selector's getAccounts(): " << settings.toStringDescription() << "\n";
        return -1;
    }
    
    tw::pnl_audit::PnLAudit pnlAudit;
    if ( !pnlAudit.start(settings) ) {
        LOGGER_ERRO << "failed to start pnlAudit: " << settings.toStringDescription() << "\n";
        return -1;
    }
    
    std::vector<std::string> accounts = selector.getAccounts();
    for ( size_t i = 0; i < accounts.size(); ++i ) {
        if ( selector.populatePnLAuditTrailInfosForAccount(settings, accounts[i]) ) {
            if ( !pnlAudit.process(selector.getPnLAuditTrailInfos(), settings, accounts[i]) )
                LOGGER_ERRO << "pnlAudit failed to process records for account: " << accounts[i] << "\n";
        }
    }
    
    tw::log::Logger::instance().stop();
    
    return 0;
}
