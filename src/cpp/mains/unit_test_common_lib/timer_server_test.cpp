#include <tw/common/timer_server.h>
#include <tw/common_thread/utils.h>
#include <tw/common_strat/consumer_proxy_impl_st.h>
#include <tw/common_strat/consumer_proxy.h>

#include <gtest/gtest.h>

typedef tw::common::TimerServer TTimerServer;

class TestTimerClient : public tw::common::TimerClient {
public:
    TestTimerClient() : _count(0),
                        _maxCount(0),
                        _id(0) {        
    }
    
    virtual bool onTimeout(const tw::common::TTimerId& id) {
        if ( id != _id )
            return false;
        
        if ( ++_count == _maxCount )
            return false;
        
        return true;
    }

    uint32_t _count;
    uint32_t _maxCount;
    tw::common::TTimerId _id;
};

class CancelableClient : public tw::common::TimerClient {
public:
    CancelableClient() : _active(true) {        
    }
    virtual bool onTimeout(const tw::common::TTimerId& id) {
        return _active;
    }    
    bool _active;
};
TEST(CommonLibTestSuit, timer_server_cancels) 
{
    // disable for now
    return;
    
    tw::common::TTimerId tid;
    CancelableClient c1;
    CancelableClient c2;
    CancelableClient c3;
    
    tw::common_strat::ConsumerProxyImplSt st;
    tw::common_strat::ConsumerProxy::instance().registerCallbackTimer(&st);
    
    TTimerServer& ts = tw::common::TimerServer::instance();
    
    ASSERT_TRUE(ts.start());
    
    // Sleep for 1 sec to let all threads of TimerServer to start
    //
    tw::common_thread::sleep(1000);    

    ASSERT_EQ(0U, ts.numberOfClients());
    
    ASSERT_TRUE(ts.registerClient(&c1, 100U, false, tid));
    ASSERT_TRUE(ts.registerClient(&c2, 100U, false, tid));
    ASSERT_TRUE(ts.registerClient(&c3, 100U, false, tid));
    
    ASSERT_EQ(3U, ts.numberOfClients());
        
    tw::common_thread::sleep(101);

    ASSERT_EQ(3U, ts.numberOfClients());

    c1._active = false;
    c2._active = false;

    tw::common_thread::sleep(101);
    
    ASSERT_EQ(1U, ts.numberOfClients());    
    
    ASSERT_TRUE(ts.stop());
}


TEST(CommonLibTestSuit, timer_server)
{
    // disable for now
    return;
    
    TestTimerClient c1;
    TestTimerClient c2;
    TestTimerClient c3;
    
    tw::common_strat::ConsumerProxyImplSt st;
    tw::common_strat::ConsumerProxy::instance().registerCallbackTimer(&st);
    
    TTimerServer& ts = tw::common::TimerServer::instance();
    
    ASSERT_TRUE(ts.start());
    
    // Sleep for 1 sec to let all threads of TimerServer to start
    //
    tw::common_thread::sleep(1000);
    
    c1._maxCount = 5;
    ASSERT_TRUE(ts.registerClient(&c1, 100, false, c1._id));
    ASSERT_EQ(c1._id, 1U);
    
    c2._maxCount = 10;
    ASSERT_TRUE(ts.registerClient(&c2, 200, false, c2._id));
    ASSERT_EQ(c2._id, 2U);
    
    c3._maxCount = std::numeric_limits<uint32_t>::max();
    ASSERT_TRUE(ts.registerClient(&c3, 200, true, c3._id));
    ASSERT_EQ(c3._id, 3U);
    
    tw::common_thread::sleep(1050);
    
    ASSERT_EQ(c1._count, 5U);
    ASSERT_EQ(c2._count, 5U);
    ASSERT_EQ(c3._count, 1U);
    
    tw::common_thread::sleep(1050);
    
    ASSERT_EQ(c1._count, 5U);
    ASSERT_EQ(c2._count, 10U);
    ASSERT_EQ(c3._count, 1U);
    
    tw::common_thread::sleep(1050);
    
    ASSERT_EQ(c1._count, 5U);
    ASSERT_EQ(c2._count, 10U);
    ASSERT_EQ(c3._count, 1U);

    ASSERT_TRUE(ts.stop());    
}

// make sure that a client that registers a small timeout (< 100) actually fires
TEST(CommonLibTestSuit, timer_server_small_timeout)
{
    // disable for now
    return;
    
    TestTimerClient c1;
    
    tw::common_strat::ConsumerProxyImplSt st;
    tw::common_strat::ConsumerProxy::instance().registerCallbackTimer(&st);
    
    TTimerServer& ts = tw::common::TimerServer::instance();
    
    ASSERT_TRUE(ts.start());
    
    // Sleep for 1 sec to let all threads of TimerServer to start
    //
    tw::common_thread::sleep(1000);
    
    c1._maxCount = 1;
    ASSERT_TRUE(ts.registerClient(&c1, 1, true, c1._id));
    ASSERT_EQ(1U, c1._id);
    
    TestTimerClient c2;
    c2._maxCount = 10;
    
    ASSERT_TRUE(ts.registerClient(&c2, 10, false, c2._id));
    ASSERT_EQ(2U, c2._id);

    tw::common_thread::sleep(1000);
    
    ASSERT_EQ(1U, c1._count);
    ASSERT_EQ(10U, c2._count);

    ASSERT_TRUE(ts.stop());    
}
