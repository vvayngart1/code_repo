#include <tw/config/settings_cmnd_line.h>
#include <tw/log/defs.h>

namespace tw {
namespace config {

SettingsCmndLine::SettingsCmndLine(const std::string& name) {
    clear(name);
}

void SettingsCmndLine::clear(const std::string& name) {
    TParent::clear(name);
}
    
bool SettingsCmndLine::parse(int argc, char * argv[]) {       
    try {
        po::store(po::parse_command_line(argc, argv, *(TParent::_optionsDesc)), TParent::_optionsValues);
        po::notify(TParent::_optionsValues);
    } catch(const std::exception& e) {
        std::stringstream s;
        
        s << _name << ":: " << "exception in parsing command line: " << e.what() << "\n" << "\n";
        s << *(TParent::_optionsDesc) << "\n";
        
        LOGGER_ERRO << s.str();
        
        return false;
      }	
    
    return true;
}

SettingsCmndLineCommonUse::SettingsCmndLineCommonUse() {
    clear();
}

void SettingsCmndLineCommonUse::clear() {
    TParent::clear("SettingsCmndLineCommonUse options");
    _configFile.clear();
}
    
bool SettingsCmndLineCommonUse::parse(int argc, char * argv[]) {
    try {
        TParent::add_options()
            ("help,h","print help")
            ("config,c", _configFile, "config file name", EnumOptionNeed::eRequired);

        if ( !TParent::parse(argc, argv) )
            return false;    
    
        if ( !help() )
            return TParent::validate();      
    } catch(const std::exception& e) {
        std::stringstream s;
        
        s << _name << ":: " << "exception in parsing command line: " << e.what() << "\n" << "\n";
        s << *(TParent::_optionsDesc) << "\n";
        
        LOGGER_ERRO << s.str();
        
        return false;
      }	
    
    return true;
}

} // namespace config
} // namespace tw
