#pragma once

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

namespace tw {
namespace common_thread {   
    typedef boost::thread Thread;
    typedef boost::shared_ptr<Thread> ThreadPtr;
    
} // common_thread
} // tw

