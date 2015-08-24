#pragma once

#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <tw/common/high_res_time.h>

namespace tw {
namespace common_thread {   
    inline void sleep(uint32_t msecs) {
        boost::this_thread::sleep(boost::posix_time::milliseconds(msecs));
    }
    
    inline void sleepUSecs(uint32_t usecs) {
        boost::this_thread::sleep(boost::posix_time::microseconds(usecs));
    }
    
    inline void sleepUSecsBusyWait(uint32_t usecs) {
        tw::common::THighResTime start = tw::common::THighResTime::now();
        while ( true ) {
            if ( (tw::common::THighResTime::now() - start) >= usecs )
                break;
        }
    }
} // common_thread
} // tw

