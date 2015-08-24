#include <stdint.h>
#include <sstream>

#include <tw/config/settings.h>
#include <tw/log/defs.h>

namespace tw {
namespace config {

Settings::Settings(const std::string& name) {
    clear();
}
    
void Settings::clear(const std::string& name) {
    _name = name;
    _options.clear();
    _optionsDesc = TOptionsDescPtr(new po::options_description(_name));
    _optionsValues.clear();
}

std::string Settings::cleanName(const std::string& name) const {
    std::string cleanName = name;
    
    // Check if 'name' contains ',' (whiche means comma separated list of aliases)
    // for the same option
    //
    if ( name.find(',') != std::string::npos )
        cleanName = name.substr(0, name.find(','));
    else
        cleanName = name;
    
    return cleanName;
}
    

Settings& Settings::operator()(const char* name,
                               const char* description,
                               EnumOptionNeed::kOptionNeed optionNeed) {    
    _optionsDesc->add_options()(name, description);        
    
    add_option(name, optionNeed);        
    return *this;
}

void Settings::add_option(const char* name,
                          EnumOptionNeed::kOptionNeed optionNeed) {
    
    _options[name] = optionNeed;
}

bool Settings::validate() const {
    bool status = true;
    
    TOptions::const_iterator iter = _options.begin();
    TOptions::const_iterator end = _options.end();
    for ( ; iter != end; ++iter ) {
        std::string name = cleanName(iter->first);
        EnumOptionNeed::kOptionNeed optionNeed = iter->second;
        if ( EnumOptionNeed::eRequired == optionNeed ) {
            const po::variable_value& value = _optionsValues[name];
            if ( value.empty() ) {
                LOGGER_ERRO << _name << " :: " << name << " :: " << "<setting not found>" << "\n";
                status = false;
            } else if ( value.value().empty() ) {
                LOGGER_ERRO << _name << " :: " << name << " :: " << "<setting's value not set>" << "\n";
                status = false;
            }   
        }       
    }
    
    return status;
}

std::string Settings::toStringDescription() const {
    std::stringstream out;
    
    out << "\n****************************************\n";
    out << _name << " :: " << "Setting descriptions:" << "\n" << "\n";
    
    out << (*_optionsDesc) << "\n";
    
    return out.str();
}

std::string Settings::toString() const {
    std::stringstream out;
    
    out << "\n========================================\n";    
    
    out << toStringDescription();
    
    out << "\n****************************************\n";
    out << _name << " :: " << "Setting values:" << "\n" << "\n";
    
    TOptions::const_iterator iter = _options.begin();
    TOptions::const_iterator end = _options.end();
    for ( ; iter != end; ++iter ) {
        std::string name = cleanName(iter->first);
        EnumOptionNeed::kOptionNeed optionNeed = iter->second;
        
        out << name << "<" << EnumOptionNeed::toString(optionNeed) << "> = ";
        TVariables::const_iterator iter = _optionsValues.find(name);
        if ( iter == _optionsValues.end() ) {
            out << "<setting not found>";
        } else {
            if ( iter->second.empty() ) {
                out << "<setting's value not set>";
            } else {
                if ( typeid(bool) == iter->second.value().type() ) {
                    out << iter->second.as<bool>();
                } else if ( typeid(int8_t) == iter->second.value().type() ) {
                    out << iter->second.as<int8_t>();
                } else if( typeid(uint8_t) == iter->second.value().type() ) {
                    out << iter->second.as<uint8_t>();
                } else if( typeid(int16_t) == iter->second.value().type() ) {
                    out << iter->second.as<int16_t>();
                } else if( typeid(uint16_t) == iter->second.value().type() ) {
                    out << iter->second.as<uint16_t>();
                } else if( typeid(int32_t) == iter->second.value().type() ) {
                    out << iter->second.as<int32_t>();                    
                } else if( typeid(uint32_t) == iter->second.value().type() ) {
                    out << iter->second.as<uint32_t>();                    
                } else if( typeid(int64_t) == iter->second.value().type() ) {
                    out << iter->second.as<int64_t>();                    
                } else if( typeid(uint64_t) == iter->second.value().type() ) {
                    out << iter->second.as<uint64_t>();                    
                } else if( typeid(float) == iter->second.value().type() ) {
                    out << iter->second.as<float>();
                } else if( typeid(double) == iter->second.value().type() ) {
                    out << iter->second.as<double>();
                } else if( typeid(long double) == iter->second.value().type() ) {
                    out << iter->second.as<long double>();
                } else if( typeid(wchar_t) == iter->second.value().type() ) {
                    out << iter->second.as<wchar_t>();
                } else if( typeid(std::string) == iter->second.value().type() ) {
                    out << iter->second.as<std::string>();
                } else {
                    out << "can't output value - not a basic type: " << iter->second.value().type().name();
                }
            }                
        }
        out << "\n";
    }
    
    return out.str();
}

} // namespace config
} // namespace tw
