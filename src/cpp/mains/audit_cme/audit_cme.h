#pragma once

#include <tw/log/defs.h>
#include <tw/common/settings.h>

class AuditCme {
public:
    AuditCme();
    ~AuditCme();
    
    void clear();
    
public:
    bool process(const tw::common::Settings& settings);
};
