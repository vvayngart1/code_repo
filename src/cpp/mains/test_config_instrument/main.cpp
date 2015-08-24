#include "settings.h"

#include <tw/config/settings_cmnd_line.h>
#include <tw/instr/instrument_manager.h>

#include <string>
#include <vector>

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

    Settings settingsConfigFile;
    if (!settingsConfigFile.parse(cmndLine.configFile())) {
        LOGGER_ERRO << "failed to parse config file settings: " << cmndLine.configFile() << " :: " << settingsConfigFile.toStringDescription() << "\n";
        return -1;
    }
    
    if ( !settingsConfigFile.validate() ) {
        LOGGER_ERRO << "failed to validate config file settings: " << cmndLine.configFile() << " :: " << settingsConfigFile.toStringDescription() << "\n";
        return -1;
    }

    LOGGER_INFO << settingsConfigFile.toString() << "\n";
    
    tw::common::Settings settings;
    if ( !settings.parse(cmndLine.configFile()) ) {
        LOGGER_ERRO << "failed to parse config: " << cmndLine.configFile() << " :: " << settings.toStringDescription() << "\n";
        return -1;
    }
    
    if ( !tw::log::Logger::instance().start(settings) ) {
        LOGGER_ERRO << "failed to start logger: " << settings.toStringDescription() << "\n";
        return -1;
    }
    
    if ( !tw::instr::InstrumentManager::instance().loadInstruments(settings) ) {
        LOGGER_ERRO << "failed to load instruments: " << cmndLine.configFile() << " :: " << settings.toStringDescription() << "\n";
        return -1;
    } 
    
    uint32_t keyInt = 3;
    tw::instr::InstrumentConstPtr instrument = tw::instr::InstrumentManager::instance().getByKeyId(keyInt);
    if ( instrument != NULL ) {
        LOGGER_INFO << "Key: " << keyInt << " :: " << instrument->toString() << "\n"; 
    } else {
        LOGGER_INFO << "Key: " << keyInt << " :: can't find instrument" << "\n";
    }    
    
    std::string keyStr = "BRNZ1";
    instrument = tw::instr::InstrumentManager::instance().getByDisplayName(keyStr);
    if ( instrument != NULL ) {
        LOGGER_INFO << "Key: " << keyStr << " :: " << instrument->toString() << "\n"; 
    } else {
        LOGGER_INFO << "Key: " << keyStr << " :: can't find instrument" << "\n";
    }
    
    keyStr = "NQZ1";
    instrument = tw::instr::InstrumentManager::instance().getByDisplayName(keyStr);
    if ( instrument != NULL ) {
        LOGGER_INFO << "Key: " << keyStr << " :: " << instrument->toString() << "\n"; 
    } else {
        LOGGER_INFO << "Key: " << keyStr << " :: can't find instrument" << "\n";
    }
    
    uint32_t keyNum1 = 10;
    uint32_t keyNum2 = 11;
    
    std::string keyStr1 = "100A";
    std::string keyStr2 = "100B";    
    
    instrument = tw::instr::InstrumentManager::instance().getByKeyNum1(keyNum1);
    if ( instrument != NULL ) {
        LOGGER_INFO << "KeyNum1: " << keyNum1 << " :: " << instrument->toString() << "\n"; 
    } else {
        LOGGER_INFO << "KeyNum1: " << keyNum1 << " :: can't find instrument" << "\n";
    }
    
    instrument = tw::instr::InstrumentManager::instance().getByKeyNum2(keyNum2);
    if ( instrument != NULL ) {
        LOGGER_INFO << "KeyNum1: " << keyNum2 << " :: " << instrument->toString() << "\n"; 
    } else {
        LOGGER_INFO << "KeyNum1: " << keyNum2 << " :: can't find instrument" << "\n";
    }
    
    instrument = tw::instr::InstrumentManager::instance().getByKeyStr1(keyStr1);
    if ( instrument != NULL ) {
        LOGGER_INFO << "keyStr1: " << keyStr1 << " :: " << instrument->toString() << "\n"; 
    } else {
        LOGGER_INFO << "keyStr1: " << keyStr1 << " :: can't find instrument" << "\n";
    }
    
    instrument = tw::instr::InstrumentManager::instance().getByKeyStr2(keyStr2);
    if ( instrument != NULL ) {
        LOGGER_INFO << "keyStr2: " << keyStr2 << " :: " << instrument->toString() << "\n"; 
    } else {
        LOGGER_INFO << "keyStr2: " << keyStr2 << " :: can't find instrument" << "\n";
    }
    
    tw::log::Logger::instance().stop();

    return 0;
}
