#pragma once

#include <tw/config/settings.h>

namespace tw {
namespace config {

class SettingsCmndLine : public Settings {
    typedef Settings TParent;
public:
    SettingsCmndLine(const std::string& name = "Allowable options");
    virtual ~SettingsCmndLine() {}
    
    void clear(const std::string& name = "Allowable options");
    
public:
    virtual bool parse(int argc, char * argv[]);
};

class SettingsCmndLineCommonUse : public SettingsCmndLine {
    typedef SettingsCmndLine TParent;
public:
    SettingsCmndLineCommonUse();
    virtual ~SettingsCmndLineCommonUse() {}
    
    void clear();
    
public:
    virtual bool parse(int argc, char * argv[]);
    
public:
    bool help() const {
        return _optionsValues.count("help") > 0;
    }
    
    const std::string& configFile() const {
        return _configFile;
    }
    
private:    
    std::string _configFile;
};

} // namespace config
} // namespace tw
