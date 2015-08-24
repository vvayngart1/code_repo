#pragma once

#include <tw/config/settings.h>

namespace tw {
namespace config {

class SettingsConfigFile : public Settings {
    typedef Settings TParent;
public:
    SettingsConfigFile(const std::string& name = "Allowable options");
    void clear(const std::string& name = "Allowable options");
    
    virtual ~SettingsConfigFile() {}
    
public:
    virtual bool parse(const std::string& configFile);
    const std::string& fileName() const {
        return _fileName;
    }
    
private:
    std::string _fileName;    
};

} // namespace config
} // namespace tw
