#include <tw/common/pool.h>
#include <tw/common_thread/thread.h>
#include <tw/common_thread/thread_pipe.h>
#include <tw/generated/channel_or_defs.h>

#include <gtest/gtest.h>

TEST(CommonLibTestSuit, pool)
{
    typedef tw::channel_or::Order TOrder;
    typedef tw::channel_or::TOrderPtr TOrderPtr;
    typedef tw::common::Pool<TOrder> TPool;
    typedef stdext::hash<TOrder> THash;
    
    THash hash;
    TPool pool(3);
    
    ASSERT_EQ(pool.capacity(), 3U);
    ASSERT_EQ(pool.size(), 3U);
    
    TOrderPtr o1(pool.obtain(), pool.getDeleter());
    TOrderPtr o2 = o1;
    TOrderPtr o3;
    TOrderPtr o4;
    
    size_t h = hash(o1.get());
    ASSERT_EQ(reinterpret_cast<TOrder*>(h), o1.get());
    
    ASSERT_EQ(pool.capacity(), 3U);
    ASSERT_EQ(pool.size(), 2U);
    
    o1.reset();
    
    ASSERT_EQ(pool.capacity(), 3U);
    ASSERT_EQ(pool.size(), 2U);
    
    o2.reset();
    
    ASSERT_EQ(pool.capacity(), 3U);
    ASSERT_EQ(pool.size(), 3U);
    
    o1.reset(pool.obtain(), pool.getDeleter());
    
    ASSERT_EQ(pool.capacity(), 3U);
    ASSERT_EQ(pool.size(), 2U);
    
    
    o2.reset(pool.obtain(), pool.getDeleter());
    
    ASSERT_EQ(pool.capacity(), 3U);
    ASSERT_EQ(pool.size(), 1U);
    
    
    o3.reset(pool.obtain(), pool.getDeleter());
    
    ASSERT_EQ(pool.capacity(), 3U);
    ASSERT_EQ(pool.size(), 0U);
    
    o4.reset(pool.obtain(), pool.getDeleter());
    
    ASSERT_EQ(pool.capacity(), 4U);
    ASSERT_EQ(pool.size(), 0U);
    
    o1.reset();
    
    ASSERT_EQ(pool.capacity(), 4U);
    ASSERT_EQ(pool.size(), 1U);
    
    o2.reset();
    
    ASSERT_EQ(pool.capacity(), 4U);
    ASSERT_EQ(pool.size(), 2U);
    
    
    o3.reset();
    
    ASSERT_EQ(pool.capacity(), 4U);
    ASSERT_EQ(pool.size(), 3U);
    
    o4.reset();
    
    ASSERT_EQ(pool.capacity(), 4U);
    ASSERT_EQ(pool.size(), 4U);
    
}

class TestItem {
public:
    TestItem() {
        clear();
    }
    
    void clear() {
        _i = 0;
    }
    
    uint32_t _i;
};

typedef boost::shared_ptr<TestItem> TTestItemPtr;
typedef tw::common::Pool<TestItem, tw::common_thread::Lock> TPoolMultiThreadUse;
typedef tw::common_thread::ThreadPipe<TTestItemPtr> TThreadPipe;

typedef tw::common_thread::Thread TThread;
typedef tw::common_thread::ThreadPtr TThreadPtr;

class PoolMultipleThreadsTester {
public:
    PoolMultipleThreadsTester(uint32_t i,
                              TPoolMultiThreadUse& pool) : _i(i),
                                                           _pool(pool) {
        
    }
    
    void start() {
        _reader = TThreadPtr(new TThread(boost::bind(&PoolMultipleThreadsTester::read, this)));
        _writer = TThreadPtr(new TThread(boost::bind(&PoolMultipleThreadsTester::write, this)));
    }
    
    void stop() {
        _writer->join();
        _reader->join();
    }
    
    void write() {
        for ( uint32_t i = 0; i < _i; ++i ) {
            TTestItemPtr item(_pool.obtain(), _pool.getDeleter());
            item->_i = i;
            
            _threadPipe.push(item);
        }
    }
    
    void read() {
        TTestItemPtr item;
        for ( uint32_t i = 0; i < _i; ++i ) {
            _threadPipe.read(item);
            _s << item->_i;
        }
    }
    
    uint32_t _i;
    TPoolMultiThreadUse& _pool;
    std::stringstream _s;
    
    TThreadPtr _writer;
    TThreadPtr _reader;
    
    TThreadPipe _threadPipe;
};

TEST(CommonLibTestSuit, pool_multiple_threads_use)
{
    uint32_t itemNumber = 10000;
    uint32_t initialCapacity = 500;
    
    TPoolMultiThreadUse pool(initialCapacity);
    
    
    PoolMultipleThreadsTester t1(itemNumber, pool);
    PoolMultipleThreadsTester t2(itemNumber, pool);
    
    t1.start();
    t2.start();
    
    t1.stop();
    t2.stop();
    
    ASSERT_EQ(pool.capacity(), pool.size());
    ASSERT_TRUE(pool.size() >= initialCapacity);
    
    std::stringstream s;
    
    for ( uint32_t i = 0; i < itemNumber; ++i ) {
        s << i;
    }
    
    ASSERT_EQ(s.str(), t1._s.str());
    ASSERT_EQ(s.str(), t2._s.str());
}
