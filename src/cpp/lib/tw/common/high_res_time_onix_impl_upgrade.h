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
//typedef OnixS::CME::MarketData::HighResolutionTimeFields THighResTimeFields;
//typedef OnixS::CME::MarketData::HighResolutionTime THighResTimeImpl;
    
typedef OnixS::CME::MarketData::TimeSpan THighResTimeSpanImpl;
typedef OnixS::CME::MarketData::Timestamp THighResTimeImpl;

static const std::string INVALID_TIME = "00000000-00:00:00.000000";

typedef struct HighResTimeFields {
    HighResTimeFields() {
        clear();
    }
    
    void clear() {
        year = 0;
        month = 0;
        day = 0;
        hour = 0;
        minute = 0;
        seconds = 0;
        microseconds = 0;
    }
    
    unsigned short year;
    unsigned short month;
    unsigned short day;
    unsigned short hour;
    unsigned short minute;
    unsigned short seconds;
    unsigned int microseconds;
} THighResTimeFields;

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
    static const uint32_t STANDARD_LENGTH = 24;
    static const uint32_t MIN_STANDARD_LENGTH = 17;
    
public:
    static THighResTime now() {
        THighResTime value;
        value._impl = THighResTimeImpl::utcNow();
        return value;       
    }
    
    static std::string nowDate() {
        return now()._impl.toString().substr(0,8);
    }
    
    // Example: "20111222-13:55:21.154378"
    //
    static THighResTime parse(const std::string& time) {
        THighResTime value;
        if ( STANDARD_LENGTH == time.length() )
            value._impl = THighResTimeImpl::deserialize(time);
        else if ( STANDARD_LENGTH < time.length() )
            value._impl = THighResTimeImpl::deserialize(time.substr(0, STANDARD_LENGTH));
        else if ( MIN_STANDARD_LENGTH < time.length() )
            value._impl = THighResTimeImpl::deserialize(time + std::string(STANDARD_LENGTH-time.length(), '0'));
        else if ( MIN_STANDARD_LENGTH == time.length() )
            value._impl = THighResTimeImpl::deserialize(time + std::string(".") + std::string(STANDARD_LENGTH-time.length()-1, '0'));        
            
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
        if ( time.length() < MIN_STANDARD_LENGTH )
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
    
    static int32_t daysToMinutes(int32_t days) {
        return days * 24 * 60;
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
    static std::string sqlString(int32_t offsetInSec=0, bool GMT_mode=false) {
        if (!GMT_mode)
            return boost::posix_time::to_iso_extended_string(boost::posix_time::second_clock::local_time()+boost::posix_time::seconds(offsetInSec));
        
        return boost::posix_time::to_iso_extended_string(boost::posix_time::second_clock::universal_time()+boost::posix_time::seconds(offsetInSec));
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
    
    THighResTimeImpl& getImpl() {
        return _impl;
    }
    
    const THighResTimeImpl& getImpl() const {
        return _impl;
    }
    
    unsigned short hour() const {
        return _impl.hour();
    }
    
    unsigned short minute() const {
        return _impl.minute();
    }
    
    unsigned short seconds() const {
        return _impl.second();
    }
    
    unsigned int microseconds() const {
        return _impl.microsecond();
    }
    
public:
    bool isValid() const {
        return (1UL != _impl.year());
    }
    
    void setToNow() {
        _impl = THighResTimeImpl::utcNow();
    }
    
    uint64_t getUsecsFromMidnight() const {
        return getUnitsFromMidnight(tw::common::eHighResTimeUnits::kUsecs);
    }
    
    uint64_t getUnitsFromMidnight(eHighResTimeUnits units) const {
        return getUnitsFromMidnight(units, getFields(_impl));
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
        if ( !isValid() )
            return INVALID_TIME;
        
        std::string s = _impl.toString();
        return (s.length() > STANDARD_LENGTH ? s.substr(0, STANDARD_LENGTH) : s);
    }
    
    bool operator==(const THighResTime& rhs) const {
        return _impl == rhs._impl;
    }

    bool operator!=(const THighResTime& rhs) const {
        return _impl != rhs._impl;
    }
    
    bool operator<(const THighResTime& rhs) const {
        return _impl < rhs._impl;
    }

    int64_t operator-(const THighResTime& rhs) const {
        THighResTimeSpanImpl delta = _impl-rhs._impl;
        return delta.totalSeconds()*1000000 + delta.nanoseconds()/1000;
    }

    int64_t deltaSeconds(const THighResTime& rhs) const {
        THighResTimeSpanImpl delta = _impl-rhs._impl;
        return delta.totalSeconds();
    }
    
    friend std::ostream& operator<<(std::ostream& ostream, const THighResTime& x) {
        return ostream << x.toString();
    }

    friend bool operator>>(std::istream& istream, THighResTime& x) {
        try {
            std::string str;
            istream >> str;
            
            if ( INVALID_TIME == str )
                return true;

            x = THighResTime::parse(str);
        } catch(...) {
            return false;
        }

        return true;
    }
private:
    template <typename TType>
    THighResTimeFields getFields(const TType& value) const {
        THighResTimeFields f;
        f.hour = value.hour();
        f.minute = value.minute();
        f.seconds = value.second();
        f.microseconds = value.microsecond();
        
        return f;
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
