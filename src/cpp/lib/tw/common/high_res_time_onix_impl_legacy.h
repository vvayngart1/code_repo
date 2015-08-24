#pragma once

#include <OnixS/CME/MarketData/Time.h>

#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

namespace tw {
namespace common {
    
// TODO: for now getting high resolution timer from Onix
// substitute later with internal implementation
//
typedef OnixS::CME::MarketData::HighResolutionTimeFields THighResTimeFields;
typedef OnixS::CME::MarketData::HighResolutionTime THighResTimeImpl;
typedef OnixS::CME::MarketData::HighResolutionTimeSpan THighResolutionTimeSpan;

struct eHighResTimeUnits {
    enum _ENUM {
        kUnknown=0,

        kUsecs=1,
        kSecs=2,
        kMins=3,
        kHours=4
    };
    
    eHighResTimeUnits () {
        _enum = kUnknown;
    }
    
    eHighResTimeUnits (_ENUM value) {
        _enum = value;
    }
    
    bool isValid() const {
        return (_enum != kUnknown);
    }
    
    operator _ENUM() const {
        return _enum;
    }

    const char* toString() const {
        switch (_enum) {

            case kUsecs: return "uSecs"; 
            case kSecs: return "Secs"; 
            case kMins: return "Mins"; 
            case kHours: return "Hours"; 
            default: return "Unknown";
        }
    }  

    void fromString(const std::string& value) {

        if ( boost::iequals(value, "usecs") ) {
            _enum = kUsecs;
            return; 
        }

        if ( boost::iequals(value, "Secs") ) {
            _enum = kSecs;
            return; 
        }

        if ( boost::iequals(value, "Mins") ) {
            _enum = kMins;
            return; 
        }

        if ( boost::iequals(value, "Hours") ) {
            _enum = kHours;
            return; 
        }

        _enum = kUnknown;
    }

    _ENUM _enum;
};

class THighResTime {
public:
    static THighResTime now() {
        THighResTime value;
        value._impl = THighResTimeImpl::now();
        return value;       
    }
    
    static std::string nowDate() {
        THighResTime value;
        value._impl = THighResTimeImpl::now();
        return value.toString().substr(0,8);       
    }
    
    // Example: "20111222-13:55:21.154378"
    //
    static THighResTime parse(const std::string& time) {
        THighResTime value;
        value._impl = THighResTimeImpl::parse(time);
        return value;       
    }
    
    // Example: "2012-01-17 14:41:01.548" => "20120117-14:41:01.548"
    //
    static THighResTime parseSqlTime(const std::string& time) {
        std::string t = time;
        boost::algorithm::erase_all(t, "-");
        boost::algorithm::replace_first(t, " ", "-");        
        
        return parse(t);
    }
    
    // Example: "20120117144101548" => "20120117-14:41:01.548"
    //
    static THighResTime parseCMETime(const std::string& time) {
        if ( time.length() < 17 )
            return now();
        
        std::string t = time;
        t.insert(14, ".");
        t.insert(12, ":");
        t.insert(10, ":");
        t.insert(8, "-");
        
        return parse(t);
    }
    
    // This will produce output in HH:MM:SS.ffffff format.
    // NOTE: will work for durations of up to 24 hours only!
    //    
    static std::string deltaToString(int64_t delta) {
        boost::posix_time::time_duration td = boost::posix_time::microseconds(delta);
        return boost::posix_time::to_simple_string(td);
    }
    
    // This will produce output in YYYYMMDD format.
    //    
    static std::string dateISOString(int32_t offset=0) {
        if ( !offset )
            return boost::gregorian::to_iso_string(boost::gregorian::day_clock::local_day());
        
        return boost::gregorian::to_iso_string(boost::gregorian::day_clock::local_day()+boost::gregorian::days(offset));
    }
    
    static int32_t getUtcOffset() {
        std::time_t current_time;
        std::time(&current_time);
        struct std::tm *timeinfo = std::localtime(&current_time);
        return (-timeinfo->tm_gmtoff/3600);
    }
    
    // This will produce output in YYYY-MM-DD HH:MM::SS format.
    //    
    static std::string sqlString(int32_t offsetInSec=0) {
        return boost::posix_time::to_iso_extended_string(boost::posix_time::second_clock::local_time()+boost::posix_time::seconds(offsetInSec));
    }
    
public:
    THighResTime() {
        clear();
    }
    
    THighResTime(const THighResTime& rhs) {
        *this = rhs;
    }
    
    THighResTime(const THighResTimeImpl& rhs) {
        *this = rhs;
    }
    
    THighResTime& operator=(const THighResTime& rhs) {
        clear();
        if ( this != &rhs )
            _impl = rhs._impl;
        
        return *this;
    }
    
    THighResTime& operator=(const THighResTimeImpl& rhs) {
        clear();
        _impl = rhs;
        
        return *this;
    }
    
    void clear() {
        _impl = THighResTimeImpl();
    }
    
    unsigned short hour() const {
        THighResTimeFields f;
        _impl.getFields(&f);
        
        return f.hour;
    }
    
    unsigned short minute() const {
        THighResTimeFields f;
        _impl.getFields(&f);
        
        return f.minute;
    }
    
    unsigned short seconds() const {
        THighResTimeFields f;
        _impl.getFields(&f);
        
        return f.seconds;
    }
    
    unsigned int microseconds() const {
        THighResTimeFields f;
        _impl.getFields(&f);
        
        return f.microseconds;
    }
    
public:
    void setToNow() {
        _impl.setToNow();
    }
    
    bool isBad() const {
        return _impl.isBad();
    }
    
    uint64_t getUsecsFromMidnight() const {
        return getUnitsFromMidnight(tw::common::eHighResTimeUnits::kUsecs);
    }
    
    uint64_t getUnitsFromMidnight(eHighResTimeUnits units) const {
        THighResTimeFields f;
        _impl.getFields(&f);
        
        return getUnitsFromMidnight(units, f);
    }
    
    static uint64_t getUnitsFromMidnight(eHighResTimeUnits units, const THighResTimeFields& f) {
        uint64_t v = 0;
        switch (units) {
            case eHighResTimeUnits::kUsecs:
                v += static_cast<uint64_t>(f.hour) * 60 * 60 * 1000 * 1000;
                v += static_cast<uint64_t>(f.minute) * 60 * 1000 * 1000;
                v += static_cast<uint64_t>(f.seconds) * 1000 * 1000;
                v += static_cast<uint64_t>(f.microseconds);
                break;
            case eHighResTimeUnits::kSecs:
                v += static_cast<uint64_t>(f.hour) * 60 * 60;
                v += static_cast<uint64_t>(f.minute) * 60;
                v += static_cast<uint64_t>(f.seconds);
                break;
            case eHighResTimeUnits::kMins:
                v += static_cast<uint64_t>(f.hour) * 60;
                v += static_cast<uint64_t>(f.minute);
                break;
            case eHighResTimeUnits::kHours:
                v += static_cast<uint64_t>(f.hour);
                break;
            default:
                break;
        }
        
        return v;
    }
    
    std::string toString() const {
        return _impl.toString();
    }
    
    bool operator==(const THighResTime& rhs) const {
        return _impl == rhs._impl;
    }

    bool operator!=(const THighResTime& rhs) const {
        return _impl != rhs._impl;
    }
    
    bool operator<(const THighResTime& rhs) const {
        return (static_cast<int64_t>(_impl - rhs._impl) < 0);
    }

    int64_t operator-(const THighResTime& rhs) const {
        return static_cast<int64_t>(_impl - rhs._impl);
    }    
    
    friend std::ostream& operator<<(std::ostream& ostream, const THighResTime& x) {
        return ostream << x.toString();
    }

    friend bool operator>>(std::istream& istream, THighResTime& x) {
        try {
            std::string str;
            istream >> str;

            x = THighResTime::parse(str);
        } catch(...) {
            return false;
        }

        return true;
    }
    
private:
    THighResTimeImpl _impl;
};

class THighResTimeScope {
public:
    THighResTimeScope(THighResTime& t1,
                      THighResTime& t2) : _t1(t1),
                                          _t2(t2) {
        _t1.setToNow();
    }
                      
    THighResTimeScope(const THighResTime& now,
                      THighResTime& t1,
                      THighResTime& t2) : _t1(t1),
                                          _t2(t2) {
        _t1 = now;
    }
                      
    ~THighResTimeScope() {
        _t2.setToNow();
    }
    
private:
    THighResTime& _t1;
    THighResTime& _t2;
};


} // namespace common
} // namespace tw
