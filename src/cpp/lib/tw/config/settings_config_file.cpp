#include <tw/config/settings_config_file.h>
#include <tw/log/defs.h>

#include <fstream>

namespace tw {
namespace config {

SettingsConfigFile::SettingsConfigFile(const std::string& name) : _fileName() {
    clear(name);
}

void SettingsConfigFile::clear(const std::string& name) {
    TParent::clear(name);
}

bool SettingsConfigFile::parse(const std::string& configFile) {       
    try {
        std::ifstream ifs(configFile.c_str());
        po::store(po::parse_config_file(ifs, *(TParent::_optionsDesc), true), TParent::_optionsValues);
        po::notify(TParent::_optionsValues);
        _fileName = configFile;
    } catch(const std::exception& e) {
        std::stringstream s;
        
        s << _name << ":: " << "exception in parsing configFile: " << configFile << " :: " << e.what() << "\n" << "\n";
        s << *(TParent::_optionsDesc) << "\n";
        
        LOGGER_ERRO << s.str();
        
        return false;
    }	
    
    return true;
}

} // namespace config
} // namespace tw
