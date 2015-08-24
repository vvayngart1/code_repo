#include <tw/common/defs.h>
#include <tw/common/high_res_time.h>
#include <tw/log/defs.h>
#include <tw/common_thread/thread.h>
#include <tw/common_thread/thread_pipe.h>
#include <tw/common_thread/utils.h>
#include <tw/generated/channel_or_defs.h>

#include "./unit_test_channel_or_lib/order_helper.h"

#include <boost/lexical_cast.hpp>

#include <vector>
#include <map>
#include <string>

typedef tw::channel_or::Order TOrder;

typedef tw::common_thread::ThreadPipe<uint32_t> TThreadPipe;
typedef tw::common_thread::ThreadPipe<TOrder> TThreadPipeOrder;

typedef tw::common_thread::ThreadPtr TThreadPtr;
typedef tw::common_thread::Thread TThread;

class Tester {
    static const uint32_t LIMIT = 2000000;
public:
    Tester(uint32_t i = LIMIT) {
        _i = 0;
        _limit = i;
    }
    
    void write() {
        uint32_t i = 0;
        
        _t1.setToNow();
        for ( i = 0; i < LIMIT; ++i ) {
            _tp.push(i);
        }
    }
    
    void read() {
        while ( _i < _limit-1 ) {
            _tp.read(_i);
            while ( _tp.try_read(_i) ) {
            }
        }
        
        _t2.setToNow();
        
        double diff = _t2 - _t1;
        ++_i;
        LOGGER_INFO << "\nDONE: " << _i << " :: " 
                << _limit << " :: " 
                << diff/1000000 << " secs" << " :: " 
                << (_i)/(diff/1000) << " per msec" << " :: " 
                << diff/(_i) << " usecs/one"
                << "\n";
    }    
    
    uint32_t _i;
    uint32_t _limit;
    tw::common::THighResTime _t1;
    tw::common::THighResTime _t2;
    TThreadPipe _tp;
};


class TesterOrder {
    static const uint32_t LIMIT = 2000000;
public:
    TesterOrder(uint32_t i = LIMIT) {
        _i = 0;
        _limit = i;
    }
    
    void write() {
        uint32_t i = 0;
        TOrder order;
        OrderHelper::getOrder(order);
        
        _t1.setToNow();
        for ( i = 0; i < _limit; ++i ) {
            _tp.push(order);
        }
        _t3.setToNow();
        
        TLockGuard lock(_lock);
        double diffWrite = _t3 - _t1;
        LOGGER_INFO << "\nDONE writing " << i << " :: " 
                << _limit << " :: "                 
                << diffWrite/1000000 << " secs for write" << " :: " 
                << (i)/(diffWrite/1000) << " per msec for write" << " :: "                 
                << diffWrite/(i) << " usecs/one"
                << "\n";
    }
    
    void read() {
        while ( ++_i < _limit-1 ) {
            _tp.read(_order);
            while ( _tp.try_read(_order) ) {
                ++_i;
            }
        }
        
        _t2.setToNow();

        TLockGuard lock(_lock);
        double diff = _t2 - _t1;
        --_i;
        LOGGER_INFO << "\nDONE: " << _i << " :: " 
                << _limit << " :: "
                << diff/1000000 << " secs" << " :: " 
                << (_i)/(diff/1000) << " per msec" << " :: " 
                << diff/(_i) << " usecs/one"
                << "\n";
    }    
    
    typedef tw::common_thread::Lock TLock;
    typedef tw::common_thread::LockGuard<TLock> TLockGuard;
    
    TLock _lock;
    
    uint32_t _i;
    uint32_t _limit;
    TOrder _order;
    tw::common::THighResTime _t1;
    tw::common::THighResTime _t2;
    tw::common::THighResTime _t3;
    TThreadPipeOrder _tp;
};

int main(int argc, char * argv[]) {
    {
        Tester t;

        TThreadPtr reader = TThreadPtr(new TThread(boost::bind(&Tester::read, &t)));
        TThreadPtr writer = TThreadPtr(new TThread(boost::bind(&Tester::write, &t)));    

        writer->join();
        reader->join();
    }
    
    {        
        TesterOrder t;

        TThreadPtr reader = TThreadPtr(new TThread(boost::bind(&TesterOrder::read, &t)));
        TThreadPtr writer = TThreadPtr(new TThread(boost::bind(&TesterOrder::write, &t)));    

        writer->join();
        reader->join();
    }
    
    
    {        
        TesterOrder t(200000);
        
        TThreadPtr writer = TThreadPtr(new TThread(boost::bind(&TesterOrder::write, &t)));
        writer->join();
        
        TThreadPtr reader = TThreadPtr(new TThread(boost::bind(&TesterOrder::read, &t)));
        reader->join();
    }
    
    return 0;
}
