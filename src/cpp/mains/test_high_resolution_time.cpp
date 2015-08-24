
#include <boost/algorithm/string/erase.hpp>

#include <tw/common/defs.h>
#include <tw/common/high_res_time.h>
#include <tw/common_thread/utils.h>
#include <tw/log/defs.h>

#include <boost/lexical_cast.hpp>
#include <time.h>

const uint32_t MILLION = 1000000;
const uint32_t THOUSAND = 1000;

void sleepUSecs(uint32_t usecs) {
    tw::common::THighResTime start = tw::common::THighResTime::now();
    while ( true ) {
        if ( (tw::common::THighResTime::now() - start) >= usecs )
            break;
    }
}

void sleepUSecs2(uint32_t usecs) {
    struct timeval t;
    t.tv_sec = usecs / 1000000L;
    t.tv_usec = usecs % 1000000L;
    
    ::select(0, NULL, NULL, NULL, &t);
}

std::string startTime(const std::string& start_time) {
    tw::common::THighResTime startTime = tw::common::THighResTime::parseSqlTime(tw::common::THighResTime::sqlString().substr(0,10) + " " + start_time);
    return startTime.toString();
}

bool isStart(const tw::common::THighResTime& now, const std::string& start_time) {
    if (start_time.empty())
        return false;

    std::string pftime = "2014-06-18 16:30:00";
    std::string sttime = "2014-06-18 18:00:00";
    tw::common::THighResTime hires_pftime = tw::common::THighResTime::parseSqlTime(pftime);
    tw::common::THighResTime hires_sttime = tw::common::THighResTime::parseSqlTime(sttime);
    tw::common::THighResTime nowtime = tw::common::THighResTime::now();
    
    tw::common::THighResTimeImpl start_temp = nowtime.getImpl();
    start_temp += (hires_sttime.getImpl() - hires_pftime.getImpl());
    tw::common::THighResTime start = start_temp;
    LOGGER_INFO << "calculating start: nowtime=" << nowtime.toString() << ", hires_sttime=" << hires_sttime.toString() << ", hires_pftime=" << hires_pftime.toString() << ", start=" << start.toString() << "\n";
    LOGGER_INFO << "start HH:mm:ss = " << start.toString().substr(9,8);
    
    // tw::common::THighResTime start = nowtime + (hires_sttime - hires_pftime);
    
    // tw::common::THighResTime a = tw::common::THighResTime::now();
    // tw::common::THighResTimeImpl b(a.getImpl());
    // d -= b.getImpl() - a.getImpl();
    // start = d;
    // LOGGER_INFO << "calculating hist shift time: a=" << a.toString() << ", b=" << b.toString() << ", c=" << c.toString() << ", nowtime=" << nowtime.toString() << "\n";
    
    return true;
    // return ( (time < start) ? true: false);
}

bool isEnd(tw::common::THighResTime now, const std::string& end_time) {
    if (end_time.empty())
        return false;
    
    std::string sqlstring = tw::common::THighResTime::sqlString(0, true);
    std::string endtime = sqlstring.substr(0,10) + " " + end_time;
    tw::common::THighResTime time = tw::common::THighResTime::parseSqlTime(endtime);
    
    LOGGER_INFO << "endtime = " << time.toString() << "\n";
    LOGGER_INFO << "now = " << now.toString() << "\n";
    
    return ( (time < now) ? true: false);
}

tw::common::THighResTime getAdjustedNow(const std::string& shifttime) {
    tw::common::THighResTime nowtime;
    
    tw::common::THighResTime a = tw::common::THighResTime::parseSqlTime(shifttime); 
    tw::common::THighResTime b = tw::common::THighResTime::parse(tw::common::THighResTime::nowDate() + std::string("-08:40:00.000000"));
    tw::common::THighResTime c = tw::common::THighResTime::now();
    
    LOGGER_INFO << "nowtime=" << c.toString() 
                << ", shifttime=" << shifttime 
                << ", starttime=" << b.toString() 
                << "\n";

    tw::common::THighResTimeImpl d(c.getImpl());
    d -= b.getImpl() - a.getImpl();
    nowtime = d;
        
    LOGGER_INFO << "adjusted nowTime = " << nowtime.toString() << "\n";

    return nowtime;
}
        
int main(int argc, char * argv[]) {
    LOGGER_INFO << "isStart() = " << isStart(tw::common::THighResTime::now(), "10:30:00") << "\n";
    LOGGER_INFO << "isStart() = " << isStart(tw::common::THighResTime::now(), "12:00:00") << "\n";
    
    LOGGER_INFO << "isEnd() = " << isEnd(tw::common::THighResTime::now(), "20:00:00") << "\n";
    LOGGER_INFO << "isEnd() = " << isEnd(tw::common::THighResTime::now(), "15:00:00") << "\n";
    
    LOGGER_INFO << "startTime(07:30:00) = " << startTime("07:30:00") << "\n";
    LOGGER_INFO << "startTime(08:30:00) = " << startTime("08:30:00") << "\n";
    
    getAdjustedNow("2014-06-18 07:30:00.000");
    getAdjustedNow("2014-06-18 17:30:00.000");
    
    tw::common::THighResTime t1 = tw::common::THighResTime::now();
    tw::common::THighResTime t2 = tw::common::THighResTime::now();
    tw::common::THighResTime t3;
    tw::common::THighResTime t4;
    tw::common::THighResTime t5;
    
    LOGGER_INFO <<  " :: " << t1 << "\n";
    LOGGER_INFO <<  " :: " << t2 << "\n";
    
    LOGGER_INFO << " :: " << (t2-t1) << " usec" << "\n";
    LOGGER_INFO << " :: " << tw::common::THighResTime::deltaToString(t2-t1) << "\n";
    
    t3 = tw::common::THighResTime::parse("20111222-13:55:21.154378");
    t4 = tw::common::THighResTime::parse("20111222-15:56:25.3247895");
    
    LOGGER_INFO <<  " :: " << t3 << "\n";
    LOGGER_INFO <<  " :: " << t4 << "\n";
    LOGGER_INFO << " :: " << (t4-t3) << " usec" << "\n";
    LOGGER_INFO << " :: " << tw::common::THighResTime::deltaToString(t4-t3) << "\n";
    
    t4 = boost::lexical_cast<tw::common::THighResTime>(t2.toString());    
    
    LOGGER_INFO <<  " :: " << t4 << "\n";
    LOGGER_INFO << " :: " << (t4-t1) << " usec" << "\n";
    LOGGER_INFO << " :: " << tw::common::THighResTime::deltaToString(t4-t1) << "\n";
    
    t3 = tw::common::THighResTime::parseSqlTime("2012-01-17 14:41:01.548");    
    LOGGER_INFO <<  " :: " << t3 << "\n";
    
    t4 = tw::common::THighResTime::parse("20120117-14:41:01.548932");    
    LOGGER_INFO <<  " :: " << t4 << "\n";
    
    LOGGER_INFO << " :: " << (t4-t3) << " usec" << "\n";
    LOGGER_INFO << " :: " << tw::common::THighResTime::deltaToString(t4-t3) << "\n";
    
    LOGGER_INFO << "t4.getUsecFromMidnight(): " << t4.getUsecsFromMidnight() << "\n";    
    
    t5 = tw::common::THighResTime::parseCMETime("20120117144101548932");    
    LOGGER_INFO <<  " :: " << t4 << "\n";
    
    LOGGER_INFO << "\n\tToday's date: " << tw::common::THighResTime::dateISOString()
                << "\n\tYesterday's date: " << tw::common::THighResTime::dateISOString(-1)
                << "\n\tTomorrow's date: " << tw::common::THighResTime::dateISOString(1) << "\n";
    
    std::string s1 = "2013-04-22";    
    LOGGER_INFO << "Before: " << s1 << "\n";
    
    boost::erase_all(s1, "-");    
    LOGGER_INFO << "After: " << s1 << "\n";
    
    uint32_t lookbackMinutes = 0;
    std::string timestamp;
    
    timestamp = tw::common::THighResTime::sqlString(lookbackMinutes*-60);    
    LOGGER_INFO << "sqlString timestamp: " << timestamp << " offset: " << lookbackMinutes << "\n";
    
    lookbackMinutes = 100;
    timestamp = tw::common::THighResTime::sqlString(lookbackMinutes*-60);    
    LOGGER_INFO << "sqlString timestamp: " << timestamp << " offset: " << lookbackMinutes << "\n";
    
    // Time from midnight
    //
    t1.setToNow();
    LOGGER_INFO << "Full timer: " << t1.toString()
                << ", Time from midnight -- hours=" << t1.getUnitsFromMidnight(tw::common::eHighResTimeUnits::kHours)
                << ",mins=" << t1.getUnitsFromMidnight(tw::common::eHighResTimeUnits::kMins)
                << ",secs=" << t1.getUnitsFromMidnight(tw::common::eHighResTimeUnits::kSecs)
                << ",usecs=" << t1.getUnitsFromMidnight(tw::common::eHighResTimeUnits::kUsecs)
                << ",usecs2=" << t1.getUsecsFromMidnight()
                << "\n";
    
    tw::common::THighResTime t6;
    tw::common::THighResTime t7 = tw::common::THighResTime::now();
    
    LOGGER_INFO << "t6 (empty timestamp): " << t6.toString()
                << ", t7 (now): " << t7.toString()
                << ", t7-t6: " << (t7-t6)
                << "\n";
    
    
    tw::common::THighResTime start1 = tw::common::THighResTime::parse("20120117-12:00:00.000000"); 
    tw::common::THighResTime start2 = tw::common::THighResTime::parse("20120117-14:41:01.000000"); 
    tw::common::THighResTime now = tw::common::THighResTime::parse("20120117-15:42:02.000000"); 
    
    tw::common::THighResTimeImpl nowAdjustedImpl(now.getImpl());
    nowAdjustedImpl -= start2.getImpl()-start1.getImpl();
    tw::common::THighResTime nowAdjusted(nowAdjustedImpl);
    
    LOGGER_INFO << "start1: " << start1.toString()
                << ", start2: " << start2.toString()
                << ", now: " << now.toString()
                << ", nowAdjusted: " << nowAdjusted.toString()
                << "\n";
    
    uint32_t usecs = 32;
    for ( int i = 0; i < 20; ++i ) {
        start1.setToNow();    
        tw::common_thread::sleepUSecsBusyWait(usecs);
        start2.setToNow();
    
        LOGGER_INFO << "sleep_start: " << start1.toString()
                    << ", sleep_end: " << start2.toString()                
                    << ", usecs: " << usecs
                    << ", actual_sleep: " << start2-start1
                    << ", delta: " << (start2-start1) - usecs 
                    << "\n";
    }
    
    LOGGER_INFO << "===============================\n";
    
    for ( int i = 0; i < 15; ++i ) {
        start1.setToNow();
        sleepUSecs2(usecs);
        start2.setToNow();
    
        LOGGER_INFO << "sleep_start: " << start1.toString()
                    << ", sleep_end: " << start2.toString()                
                    << ", usecs: " << usecs
                    << ", actual_sleep: " << start2-start1
                    << ", delta: " << (start2-start1) - usecs
                    << "\n";
    }
    
    LOGGER_INFO << "===============================\n";
    
    usecs = 1325568;
    for ( int i = 0; i < 5; ++i ) {
        start1.setToNow();
        tw::common_thread::sleepUSecsBusyWait(usecs);
        start2.setToNow();
    
        LOGGER_INFO << "sleep_start: " << start1.toString()
                    << ", sleep_end: " << start2.toString()                
                    << ", usecs: " << usecs
                    << ", actual_sleep: " << start2-start1
                    << ", delta: " << (start2-start1) - usecs
                    << "\n";
    }
    
    LOGGER_INFO << "===============================\n";
    
    usecs = 1325568;
    for ( int i = 0; i < 15; ++i ) {
        start1.setToNow();
        sleepUSecs2(usecs);
        start2.setToNow();
    
        LOGGER_INFO << "sleep_start: " << start1.toString()
                    << ", sleep_end: " << start2.toString()                
                    << ", usecs: " << usecs
                    << ", actual_sleep: " << start2-start1
                    << ", delta: " << (start2-start1) - usecs
                    << "\n";
    }
    
    return 0;
}
