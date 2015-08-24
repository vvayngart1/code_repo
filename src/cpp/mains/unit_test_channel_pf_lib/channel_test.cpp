#include <tw/channel_pf/channel.h>
#include <tw/price/client_container.h>


#include <mains/unit_test_price_lib/instr_helper.h>
#include <gtest/gtest.h>

class TestChannelImpl : public tw::channel_pf::IChannelImpl
{
    typedef tw::channel_pf::IChannelImpl TParent;
    
public:
    TestChannelImpl() : _return(true),
                        _countSubcriptions(0) {        
    }
    
public:
    bool _return;
    uint32_t _countSubcriptions;

public:
    virtual bool init(const tw::common::Settings& config) {
        return _return;
    }
    
    virtual bool start() {
        return _return;
    }
    
    virtual void stop() {
    }

    virtual bool subscribe(tw::instr::InstrumentConstPtr instrument) {
        if ( _return )
            ++_countSubcriptions;
            
        return _return;
    }
    
    virtual bool unsubscribe(tw::instr::InstrumentConstPtr instrument) {
        if ( _return )
            --_countSubcriptions;
        
        return _return;
    }    
};

int32_t g_counter = 0;

class TestClient1 {
public:    
    int32_t _counter;
    tw::price::Quote _quote;
    
    TestClient1() {
        clear();
    }
    
    void clear() {
        _counter = 0;
        _quote.clear();
    }

public:
    // Interface method required by channel
    //
    void onQuote(const tw::price::Quote& quote) {
        ++g_counter;
        ++_counter;
        _quote = quote;
    }
};

class TestClient2 {
public:    
    int32_t _counter;
    tw::price::Quote _quote;
    
    TestClient2() {
        clear();
    }
    
    void clear() {
        _counter = 0;
        _quote.clear();
    }

public:
    // Interface method required by channel
    //
    void onQuote(const tw::price::Quote& quote) {
        ++g_counter;
        ++_counter;
        _quote = quote;
    }
};


typedef tw::channel_pf::Channel TChannel;

TEST(ChannelPfLibTestSuit, channelBasicFunctionality)
{
    // Empty config
    //
    tw::common::Settings config;
        
    TestChannelImpl channelImpl;
    TChannel channel;
    
    EXPECT_FALSE(channel.isActive());
        
    channelImpl._return = true;
    EXPECT_TRUE(!channel.init(config, NULL));
    
    channelImpl._return = false;
    EXPECT_TRUE(!channel.init(config, &channelImpl));
    EXPECT_TRUE(!channel.start());
    EXPECT_FALSE(channel.isActive());
    channel.stop();
    EXPECT_FALSE(channel.isActive());
    
    channelImpl._return = true;
    EXPECT_TRUE(channel.init(config, &channelImpl));
    EXPECT_TRUE(channel.start());
    EXPECT_TRUE(channel.isActive());
    channel.stop();
    EXPECT_FALSE(channel.isActive());
}

TEST(ChannelPfLibTestSuit, channelContainer)
{
    g_counter = 0;    
    tw::common::Settings config;
    
    TestChannelImpl channelImpl;
    TChannel channel;
    EXPECT_FALSE(channel.isActive());
    
    TInstrumentPtr instrument = InstrHelper::getNQH2();
    ASSERT_TRUE(instrument->isValid());
    
    TChannel::TQuoteStore::TQuote& quote = TChannel::TQuoteStore::instance().getQuote(instrument);
    ASSERT_TRUE(quote.isValid());
    ASSERT_EQ(quote.getInstrument(), instrument.get());
    
    channelImpl._return = true;
    EXPECT_TRUE(channel.init(config, &channelImpl));
    EXPECT_TRUE(channel.start());
    EXPECT_TRUE(channel.isActive());

    channel.stop();
    EXPECT_FALSE(channel.isActive());
    
    TestClient1 t1c1, t1c2;
    TestClient2 t2c;
    
    EXPECT_TRUE(channel.subscribe(instrument, &t1c1));
    EXPECT_EQ(channelImpl._countSubcriptions, 1UL);
    EXPECT_TRUE(!channel.subscribe(instrument, &t1c1));
    EXPECT_TRUE(channel.subscribe(instrument, &t1c2));
    EXPECT_EQ(channelImpl._countSubcriptions, 1UL);
    EXPECT_TRUE(channel.subscribe(instrument, &t2c));    
    EXPECT_EQ(channelImpl._countSubcriptions, 1UL);
    
    EXPECT_EQ(g_counter, 0);
    EXPECT_EQ(t1c1._counter, 0);
    
    quote._seqNum = 1000;
    quote.notifySubscribers();
    
    EXPECT_EQ(g_counter, 3);    
    EXPECT_EQ(t1c1._counter, 1);
    EXPECT_EQ(t1c1._quote._seqNum, 1000UL);
    EXPECT_EQ(t1c2._counter, 1);
    EXPECT_EQ(t1c2._quote._seqNum, 1000UL);
    EXPECT_EQ(t2c._counter, 1);
    EXPECT_EQ(t2c._quote._seqNum, 1000UL);
    
    quote._seqNum = 2000;
    quote.notifySubscribers();
    
    EXPECT_EQ(g_counter, 6);
    EXPECT_EQ(t1c1._counter, 2);
    EXPECT_EQ(t1c1._quote._seqNum, 2000UL);
    EXPECT_EQ(t1c2._counter, 2);
    EXPECT_EQ(t1c2._quote._seqNum, 2000UL);
    EXPECT_EQ(t2c._counter, 2);
    EXPECT_EQ(t2c._quote._seqNum, 2000UL);
    
    EXPECT_TRUE(channel.unsubscribe(instrument, &t1c1));
    EXPECT_EQ(channelImpl._countSubcriptions, 1UL);
    EXPECT_TRUE(!channel.unsubscribe(instrument, &t1c1));
    
    quote._seqNum = 3000;
    quote.notifySubscribers();
    
    EXPECT_EQ(g_counter, 8);
    EXPECT_EQ(t1c1._counter, 2);
    EXPECT_EQ(t1c1._quote._seqNum, 2000UL);
    EXPECT_EQ(t1c2._counter, 3);
    EXPECT_EQ(t1c2._quote._seqNum, 3000UL);
    EXPECT_EQ(t2c._counter, 3);
    EXPECT_EQ(t2c._quote._seqNum, 3000UL);
    
    
    EXPECT_TRUE(channel.unsubscribe(instrument, &t1c2));
    EXPECT_EQ(channelImpl._countSubcriptions, 1UL);
    EXPECT_TRUE(!channel.unsubscribe(instrument, &t1c2));
    
    quote._seqNum = 4000;
    quote.notifySubscribers();
    
    EXPECT_EQ(g_counter, 9);
    EXPECT_EQ(t1c1._counter, 2);
    EXPECT_EQ(t1c1._quote._seqNum, 2000UL);
    EXPECT_EQ(t1c2._counter, 3);
    EXPECT_EQ(t1c2._quote._seqNum, 3000UL);
    EXPECT_EQ(t2c._counter, 4);
    EXPECT_EQ(t2c._quote._seqNum, 4000UL);
    
    
    EXPECT_TRUE(channel.unsubscribe(instrument, &t2c));
    EXPECT_EQ(channelImpl._countSubcriptions, 0UL);
    EXPECT_TRUE(!channel.unsubscribe(instrument, &t2c));
    
    quote._seqNum = 5000;
    quote.notifySubscribers();
    
    EXPECT_EQ(g_counter, 9);
    EXPECT_EQ(t1c1._counter, 2);
    EXPECT_EQ(t1c1._quote._seqNum, 2000UL);
    EXPECT_EQ(t1c2._counter, 3);
    EXPECT_EQ(t1c2._quote._seqNum, 3000UL);
    EXPECT_EQ(t2c._counter, 4);
    EXPECT_EQ(t2c._quote._seqNum, 4000UL);
}

TEST(ChannelPfLibTestSuit, subscribeFailsAfterStart)
{
    tw::common::Settings config;
    
    TestChannelImpl channelImpl;
    TChannel channel;
    EXPECT_FALSE(channel.isActive());
    
    TInstrumentPtr instrument = InstrHelper::getNQH2();
    ASSERT_TRUE(instrument->isValid());
    
    TChannel::TQuoteStore::TQuote& quote = TChannel::TQuoteStore::instance().getQuote(instrument);
    ASSERT_TRUE(quote.isValid());
    ASSERT_EQ(quote.getInstrument(), instrument.get());
    
    TestClient1 t1c1;

    channelImpl._return = true;
    EXPECT_TRUE(channel.init(config, &channelImpl));

    EXPECT_TRUE(channel.subscribe(instrument, &t1c1));
    EXPECT_TRUE(channel.unsubscribe(instrument, &t1c1));

    EXPECT_TRUE(channel.subscribe(instrument, &t1c1));
    
    EXPECT_TRUE(channel.start());
    EXPECT_TRUE(channel.isActive());

    EXPECT_FALSE(channel.subscribe(instrument, &t1c1));
    EXPECT_FALSE(channel.unsubscribe(instrument, &t1c1));

    channel.stop();
    EXPECT_FALSE(channel.isActive());

    EXPECT_TRUE(channel.unsubscribe(instrument, &t1c1));
}
