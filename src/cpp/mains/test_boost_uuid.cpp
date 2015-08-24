#include <tw/common/defs.h>
#include <tw/common/high_res_time.h>
#include <tw/log/defs.h>
#include <tw/common/uuid.h>
#include <tw/channel_or/uuid_factory.h>

#include <boost/lexical_cast.hpp>

#include <vector>
#include <map>
#include <string>

typedef tw::common::TUuid TOrderId;
typedef tw::common::TUuidBuffer TOrderIdBuffer;


typedef std::map<TOrderId, std::string> TOrderIds;
typedef std::map<TOrderIdBuffer, std::string> TOrderIdBuffers;

typedef std::map<TOrderIdBuffer, uint32_t> TOrderIdMap;

int main(int argc, char * argv[]) {
    TOrderId o1 = tw::common::generateUuid();
    TOrderId o2 = tw::common::generateUuid();
    
    LOGGER_INFO << "o1 :: " << o1 << "\n";
    LOGGER_INFO << "o2 :: " << o2 << "\n";
    
    {
        TOrderIds orders;    
        orders[o1] = "1";
        orders[o2] = "2";
        
        for ( TOrderIds::iterator iter = orders.begin(); iter != orders.end(); ++iter ) {
            LOGGER_INFO << iter->first << " :: " << iter->second << "\n";
        }
    }
    
    LOGGER_INFO << "====" << "\n";
    
    {    
        TOrderIdBuffers orders;
        
        orders[o1] = "3";
        orders[o2] = "4";
        
        for ( TOrderIdBuffers::iterator iter = orders.begin(); iter != orders.end(); ++iter ) {
            LOGGER_INFO << iter->first << " :: " << iter->second << "\n";
        }
    }
    
    LOGGER_INFO << "====" << "\n";
    
    TOrderIdBuffer b1 = o1;
    LOGGER_INFO << "b1 :: " << b1 << "\n";
    
    TOrderId o3;
    o3 = b1;
    LOGGER_INFO << "o3 :: " << o3 << "\n";
    
    TOrderIdBuffer b2;
    
    ::memcpy(&b2, &b1, TOrderIdBuffer::size());
    LOGGER_INFO << "b2 :: " << TOrderIdBuffer::size() << " :: " << b2 << "\n";
    
    
    TOrderIdBuffer b3;
    
    b3 = boost::lexical_cast<TOrderIdBuffer>(b2.toString());
    LOGGER_INFO << "b3 :: " << b3 << "\n";
    
    
    tw::common::THighResTime t1;
    tw::common::THighResTime t2;
    double d = 0.0;
    
    t1.setToNow();
    
    uint32_t i = 0;
    for ( i = 0; i < 100; ++i ) {
        o1 = tw::common::generateUuid();
    }
    
    t2.setToNow();
    
    o2 = o1;
    
    d = t2-t1;
    LOGGER_INFO << "\n" << i << " iterations :: " << t2 << " - " << t1 << " = " << d << " us :: " << d/i << " us/one" << "\n";    
    
    
    t1.setToNow();
    
    for ( i = 0; i < 100; ++i ) {
        b1 = o1;
        o2 = b1;
    }
    
    t2.setToNow();
    
    o2 = o1;
    
    d = t2-t1;
    LOGGER_INFO << "\n" << i << " iterations :: " << t2 << " - " << t1 << " = " << d << " us :: " << d/i << " us/one" << "\n";    
    
    i = 1000;
    t1.setToNow();
    
    TOrderIdBuffer b[1000]; // 1000 = 'i'; standard C++ doesn't allow variable length arrays for non-POD types
    
    t2.setToNow();
    
    b[0] = o1;
    
    d = t2-t1;
    LOGGER_INFO << "\n" << i << " iterations :: " << t2 << " - " << t1 << " = " << d << " us :: " << d/i << " us/one" << "\n";    

    // Test UuidFactory
    //
    {
        tw::channel_or::UuidFactory f;
        if ( !f.start(5) ) {
            LOGGER_INFO << " Can't start uuid factory" << "\n";
        } else {
            std::stringstream s;
            for ( i = 0; i < 10; ++i ) {
                t1.setToNow();
                b1 = f.get();
                t2.setToNow();

                tw::common_thread::Thread::yield();
                s << std::setw(3) << i << " :: " << b1.toString() << " = " << t2-t1 << " us" << "\n";
                LOGGER_INFO << s.str();
                s.str("");
            }

            o1 = b1;
            f.stop();
        }
    }
    
    {
        // Check performance of std::map for storing/retrieving uuid
        //
        tw::channel_or::UuidFactory f;
        if ( !f.start() ) {
            LOGGER_INFO << " Can't start uuid factory" << "\n";
        } else {
            TOrderIdMap map_n;
            
            t1.setToNow();
            for ( i = 0; i < 1024; ++i ) {
                b1 = f.get();            
                map_n.insert(TOrderIdMap::value_type(b1, i));
            }
            
            t2.setToNow();
            
            d = t2-t1;
            LOGGER_INFO << map_n.size() << " :: insert map_n = " << d << " us :: " << d/i << " us/one" << "\n";            
            
            
            TOrderIdMap::iterator iter = map_n.begin();
            TOrderIdMap::iterator end = map_n.end();
            
            t1.setToNow();
            for ( ; iter != end; ++iter ) {                
                map_n.find(iter->first);
            }
            
            t2.setToNow();
            
            d = t2-t1;
            LOGGER_INFO << map_n.size() << " :: search map_n = " << d << " us :: " << d/i << " us/one" << "\n";            
            
            t1.setToNow();
            for ( iter = map_n.begin(); iter != end; iter = map_n.begin() ) {                
                map_n.erase(iter);
            }
            
            t2.setToNow();
            
            d = t2-t1;
            LOGGER_INFO << map_n.size() << " :: erase map_n = " << d << " us :: " << d/i << " us/one" << "\n";            
        }
    }
    
    tw::common::TUuidBuffer testUuid;
    LOGGER_INFO << "Empty testUuid: " << testUuid.isValid() << " :: " << testUuid.toString() << "\n";
    
    std::string s = tw::common::TUuidBuffer(tw::common::generateUuid()).toString();
    testUuid.fromString(s);
    
    LOGGER_INFO << "From string testUuid: " << s << " :: "  << testUuid.isValid() << " :: " << testUuid.toString() << "\n";
    
    tw::common::TUuidGenerator uuidGenerator;
    
    tw::common::TUuid uuid1 = uuidGenerator.generateUuid();
    tw::common::TUuid uuid2 = uuidGenerator.generateUuid();
    tw::common::TUuid uuid3 = uuidGenerator.generateUuid();
    LOGGER_INFO << "uuids generated from struct: " << uuid1 << "," << uuid2 << "," << uuid3 << "\n";
    
    if ( uuid1 == uuid2 || 
         uuid1 == uuid3 ||
         uuid2 == uuid3 ) {
        LOGGER_ERRO << "OMG UUIDS ARE BROKEN!";
        return -1;
    }
    
    return 0;
}
