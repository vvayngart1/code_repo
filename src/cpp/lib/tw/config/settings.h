#pragma once

#include <utility>
#include <string>
#include <map>

#include <boost/lexical_cast.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>

namespace tw {
namespace config {

namespace po = boost::program_options;

struct EnumOptionNeed {
    enum kOptionNeed {
        eUnknown = -1,
        eOptional = 0,
        eRequired = 1        
    };
    
    static const char* toString(kOptionNeed optionNeed) {
        switch (optionNeed) {
            case eOptional:
                return "eOptional";
            case eRequired:
                return "eRequired";
            default:
                return "eUnknown";
        }
    }
};

class Settings {
 
public:
    Settings(const std::string& name = "Allowable options");
    void clear(const std::string& name = "Allowable options");
    
public:
    Settings& add_options() {
        return *this;
    }
    
    Settings& operator()(const char* name,
                         const char* description,
                         EnumOptionNeed::kOptionNeed optionNeed = EnumOptionNeed::eOptional);    
    
    template <typename TType>
    Settings& operator()(const char* name,
                         TType& value,
                         const char* description = NULL,
                         EnumOptionNeed::kOptionNeed optionNeed = EnumOptionNeed::eOptional,
                         boost::optional<TType> defaultValue = boost::optional<TType>() ) {       
        if ( NULL == description ) {
            if ( defaultValue.is_initialized() ) {
                _optionsDesc->add_options()(name, po::value<TType>(&value)->default_value(*defaultValue));
            } else {
                _optionsDesc->add_options()(name, po::value<TType>(&value));                
            }
        } else {
            if ( defaultValue.is_initialized() ) {
                _optionsDesc->add_options()(name, po::value<TType>(&value)->default_value(*defaultValue), description);
            } else {
                _optionsDesc->add_options()(name, po::value<TType>(&value), description);                
            }
        }
        
        add_option(name, optionNeed);        
        return *this;    
    }
    
public:
    // Checks if all required settings are set
    //
    bool validate() const;    
    
    // Get all settings description and values
    //
    std::string toString() const;
    
    // Get all settings description
    //
    std::string toStringDescription() const;
    
protected:
    void add_option(const char* name,
                    EnumOptionNeed::kOptionNeed optionNeed);
    
private:
    std::string cleanName(const std::string& name) const;
    
protected:
    typedef std::map<std::string, EnumOptionNeed::kOptionNeed> TOptions;
    typedef boost::shared_ptr<po::options_description> TOptionsDescPtr;
    typedef po::variables_map TVariables;
    
protected:
    std::string _name;
    TOptions _options;
    TOptionsDescPtr _optionsDesc;
    TVariables _optionsValues;    
};

} // namespace config
} // namespace tw
