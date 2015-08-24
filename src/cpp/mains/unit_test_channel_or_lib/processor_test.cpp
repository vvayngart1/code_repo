#include <tw/channel_or/processor.h>

#include <gtest/gtest.h>

typedef tw::channel_or::TOrderPtr TOrderPtr;
typedef tw::channel_or::Reject Reject;
typedef tw::channel_or::Fill Fill;
typedef tw::channel_or::PosUpdate PosUpdate;
typedef tw::channel_or::Alert Alert;

struct testProcessorCallCounter {
public:
    uint32_t _counterInit;
    uint32_t _counterStart;
    uint32_t _counterStop;
    
    uint32_t _counterSendNew;
    uint32_t _counterSendMod;
    uint32_t _counterSendCxl;
    uint32_t _counterOnCommand;
    uint32_t _counterRecordFill;
    uint32_t _counterRebuildPos;
    uint32_t _counterRebuildOrder;
    
    uint32_t _counterOnNewAck;
    uint32_t _counterOnNewRej;
    uint32_t _counterOnModAck;
    uint32_t _counterOnModRej;
    uint32_t _counterOnCxlAck;
    uint32_t _counterOnCxlRej;
    uint32_t _counterOnFill;
    uint32_t _counterOnAlert;
    uint32_t _counterOnRebuildOrderRej;
    
public:
    testProcessorCallCounter() {
        clear();
    }
    
    void clear() {
        ::memset(this, 0, sizeof(*this));
    }
};

// NOTE: testing tw::channel_or::Processor<Out/In> with
// 2 different types of implementation, hence template
// implementation of testProcessor
//
template <typename TTypeDifferentiator>
class testProcessor {
public:
    TTypeDifferentiator _t;
    bool _return;
    testProcessorCallCounter _callCounter;
    
    testProcessor() {        
        clear();
    }
    
    void clear() {
        _t = TTypeDifferentiator();
        _return = true;
        _callCounter.clear();
    }
    
public:
    // 'Out' methods
    //
    
    // Admin methods
    //
    bool init(const tw::common::Settings& settings) {
        ++_callCounter._counterInit;
        return _return;
    }
    
    bool start() {
        ++_callCounter._counterStart;
        return _return;
    }
    
    void stop() {
        ++_callCounter._counterStop;
    }
    
    // Or related methods
    //
    bool sendNew(const TOrderPtr& order, Reject& rej) {
        ++_callCounter._counterSendNew;
        return _return;
    }
    
    bool sendMod(const TOrderPtr& order, Reject& rej) {
        ++_callCounter._counterSendMod;
        return _return;
    }
    
    bool sendCxl(const TOrderPtr& order, Reject& rej) {
        ++_callCounter._counterSendCxl;
        return _return;
    }
    
    void onCommand(const tw::common::Command& command) {
        ++_callCounter._counterOnCommand;
    }
    
    bool rebuildOrder(const TOrderPtr& order, Reject& rej) {
        ++_callCounter._counterRebuildOrder;
        return _return;
    }
    
    void recordFill(const Fill& fill) {
        ++_callCounter._counterRecordFill;
    }
    
    void rebuildPos(const PosUpdate& update) {
        ++_callCounter._counterRebuildPos;
    }

public:
    // 'In' methods
    //
    void onNewAck(const TOrderPtr& order) {
        ++_callCounter._counterOnNewAck;
    }
    
    void onNewRej(const TOrderPtr& order, const Reject& rej) {
        ++_callCounter._counterOnNewRej;
    }
    
    void onModAck(const TOrderPtr& order) {
        ++_callCounter._counterOnModAck;
    }
    
    void onModRej(const TOrderPtr& order, const Reject& rej) {
        ++_callCounter._counterOnModRej;
    }
    
    void onCxlAck(const TOrderPtr& order) {
        ++_callCounter._counterOnCxlAck;
    }
    
    void onCxlRej(const TOrderPtr& order, const Reject& rej) {
        ++_callCounter._counterOnCxlRej;
    }
    
    void onFill(const Fill& fill) {
        ++_callCounter._counterOnFill;
    }
    
    void onAlert(const Alert& alert) {
        ++_callCounter._counterOnAlert;
    }
    
    void onRebuildOrderRej(const TOrderPtr& order, const Reject& rej) {
        ++_callCounter._counterOnRebuildOrderRej;
    }
};

bool checkCounters(const testProcessorCallCounter& callCounter, const int32_t index = -1) {
    bool status = true;
    for (int32_t i = 0; i < 19; ++i ) {
        uint32_t expected = ( index == i ) ? 1 : 0;
        switch (i) {
            case 0:
                EXPECT_EQ(callCounter._counterInit, expected);
                if ( status && callCounter._counterInit != expected )
                    status = false;
                break;
            case 1:
                EXPECT_EQ(callCounter._counterStart, expected);
                if ( status && callCounter._counterStart != expected )
                    status = false;
                break;
            case 2:
                EXPECT_EQ(callCounter._counterStop, expected);
                if ( status && callCounter._counterStop != expected )
                    status = false;
                break;
            case 3:
                EXPECT_EQ(callCounter._counterSendNew, expected);
                if ( status && callCounter._counterSendNew != expected )
                    status = false;
                break;
            case 4:
                EXPECT_EQ(callCounter._counterSendMod, expected);
                if ( status && callCounter._counterSendMod != expected )
                    status = false;
                break;
            case 5:
                EXPECT_EQ(callCounter._counterSendCxl, expected);
                if ( status && callCounter._counterSendCxl != expected )
                    status = false;
                break;
            case 6:
                EXPECT_EQ(callCounter._counterOnCommand, expected);
                if ( status && callCounter._counterOnCommand != expected )
                    status = false;
                break;
            case 7:
                EXPECT_EQ(callCounter._counterRecordFill, expected);
                if ( status && callCounter._counterRecordFill != expected )
                    status = false;
                break;
            case 8:
                EXPECT_EQ(callCounter._counterRebuildPos, expected);
                if ( status && callCounter._counterRebuildPos != expected )
                    status = false;
                break;
            case 9:
                EXPECT_EQ(callCounter._counterRebuildOrder, expected);
                if ( status && callCounter._counterRebuildOrder != expected )
                    status = false;
                break;
            case 10:
                EXPECT_EQ(callCounter._counterOnNewAck, expected);
                if ( status && callCounter._counterOnNewAck != expected )
                    status = false;
                break;
            case 11:
                EXPECT_EQ(callCounter._counterOnNewRej, expected);
                if ( status && callCounter._counterOnNewRej != expected )
                    status = false;
                break;
            case 12:
                EXPECT_EQ(callCounter._counterOnModAck, expected);
                if ( status && callCounter._counterOnModAck != expected )
                    status = false;
                break;
            case 13:
                EXPECT_EQ(callCounter._counterOnModRej, expected);
                if ( status && callCounter._counterOnModRej != expected )
                    status = false;
                break;
            case 14:
                EXPECT_EQ(callCounter._counterOnCxlAck, expected);
                if ( status && callCounter._counterOnCxlAck != expected )
                    status = false;
                break;
            case 15:
                EXPECT_EQ(callCounter._counterOnCxlRej, expected);
                if ( status && callCounter._counterOnCxlRej != expected )
                    status = false;
                break;
            case 16:
                EXPECT_EQ(callCounter._counterOnFill, expected);
                if ( status && callCounter._counterOnFill != expected )
                    status = false;
                break;
            case 17:
                EXPECT_EQ(callCounter._counterOnAlert, expected);
                if ( status && callCounter._counterOnAlert != expected )
                    status = false;
                break;
            case 18:
                EXPECT_EQ(callCounter._counterOnRebuildOrderRej, expected);
                if ( status && callCounter._counterOnRebuildOrderRej != expected )
                    status = false;
                break;
        }
    }
    return status;
}

typedef testProcessor<uint32_t> TProcessor1;
typedef testProcessor<std::string> TProcessor2;

// NOTE: the order of template initialization is from last processor
// to first one
//
typedef tw::channel_or::ProcessorOut<TProcessor2> TProcessorOut2;
typedef tw::channel_or::ProcessorOut<TProcessor1, TProcessorOut2> TProcessorOut1;

typedef tw::channel_or::ProcessorIn<TProcessor1> TProcessorIn2;
typedef tw::channel_or::ProcessorIn<TProcessor2, TProcessorIn2> TProcessorIn1;

TEST(ChannelOrLibTestSuit, processor_base)
{
    // Stubs for passing to processors
    //
    TOrderPtr order;
    Fill fill;
    tw::common::Command command;
    PosUpdate update;
    Alert alert;
    Reject rej;
    tw::common::Settings settings;
    
    // Instantiation of processors
    //
    TProcessor1 impl1;
    TProcessor2 impl2;
    
    TProcessorOut2 out2(impl2);
    TProcessorOut1 out1(impl1, out2);
    
    TProcessorIn2 in2(impl1);
    TProcessorIn1 in1(impl2, in2);    
    
    EXPECT_TRUE(checkCounters(impl1._callCounter));
    EXPECT_TRUE(checkCounters(impl2._callCounter));    
    
    // Test 'Out' processor
    //
    
    // Admin methods
    //
    
    // Init
    //
    EXPECT_TRUE(out1.init(settings));
    EXPECT_TRUE(checkCounters(impl1._callCounter, 0));
    EXPECT_TRUE(checkCounters(impl2._callCounter, 0));
    
    impl1.clear();
    impl2.clear();
    
    impl1._return = false;    
    EXPECT_TRUE(!out1.init(settings));
    EXPECT_TRUE(checkCounters(impl1._callCounter, 0));
    EXPECT_TRUE(checkCounters(impl2._callCounter));
    
    // Start
    //
    impl1.clear();
    impl2.clear();
    
    EXPECT_TRUE(out1.start());
    EXPECT_TRUE(checkCounters(impl1._callCounter, 1));
    EXPECT_TRUE(checkCounters(impl2._callCounter, 1));
    
    impl1.clear();
    impl2.clear();
    
    impl1._return = false;    
    EXPECT_TRUE(!out1.start());
    EXPECT_TRUE(checkCounters(impl1._callCounter, 1));
    EXPECT_TRUE(checkCounters(impl2._callCounter));
    
    
    // Stop
    //
    impl1.clear();
    impl2.clear();
    
    out1.stop();
    EXPECT_TRUE(checkCounters(impl1._callCounter, 2));
    EXPECT_TRUE(checkCounters(impl2._callCounter, 2));
    
    
    // Or related methods
    //
    
    // New order
    //
    impl1.clear();
    impl2.clear();
    
    EXPECT_TRUE(out1.sendNew(order, rej));
    EXPECT_TRUE(checkCounters(impl1._callCounter, 3));
    EXPECT_TRUE(checkCounters(impl2._callCounter, 3));
    
    impl1.clear();
    impl2.clear();
    
    impl1._return = false;    
    EXPECT_TRUE(!out1.sendNew(order, rej));
    EXPECT_TRUE(checkCounters(impl1._callCounter, 3));
    EXPECT_TRUE(checkCounters(impl2._callCounter));
    
    
    // Mod order
    //
    impl1.clear();
    impl2.clear();
    
    EXPECT_TRUE(out1.sendMod(order, rej));
    EXPECT_TRUE(checkCounters(impl1._callCounter, 4));
    EXPECT_TRUE(checkCounters(impl2._callCounter, 4));
    
    impl1.clear();
    impl2.clear();
    
    impl1._return = false;    
    EXPECT_TRUE(!out1.sendMod(order, rej));
    EXPECT_TRUE(checkCounters(impl1._callCounter, 4));
    EXPECT_TRUE(checkCounters(impl2._callCounter));
    
    // Cxl order
    //
    impl1.clear();
    impl2.clear();
    
    EXPECT_TRUE(out1.sendCxl(order, rej));
    EXPECT_TRUE(checkCounters(impl1._callCounter, 5));
    EXPECT_TRUE(checkCounters(impl2._callCounter, 5));
    
    impl1.clear();
    impl2.clear();
    
    impl1._return = false;    
    EXPECT_TRUE(!out1.sendCxl(order, rej));
    EXPECT_TRUE(checkCounters(impl1._callCounter, 5));
    EXPECT_TRUE(checkCounters(impl2._callCounter));    
    
    // Command
    //
    impl1.clear();
    impl2.clear();
    
    out1.onCommand(command);
    EXPECT_TRUE(checkCounters(impl1._callCounter, 6));
    EXPECT_TRUE(checkCounters(impl2._callCounter, 6));
    
    // Fill
    //
    impl1.clear();
    impl2.clear();
    
    out1.recordFill(fill);
    EXPECT_TRUE(checkCounters(impl1._callCounter, 7));
    EXPECT_TRUE(checkCounters(impl2._callCounter, 7));
    
    // Pos
    //
    impl1.clear();
    impl2.clear();
    
    out1.rebuildPos(update);
    EXPECT_TRUE(checkCounters(impl1._callCounter, 8));
    EXPECT_TRUE(checkCounters(impl2._callCounter, 8));    
    
    // Rebuild order
    //
    impl1.clear();
    impl2.clear();
    
    impl1._return = false;    
    EXPECT_TRUE(!out1.rebuildOrder(order, rej));
    EXPECT_TRUE(checkCounters(impl1._callCounter, 9));
    EXPECT_TRUE(checkCounters(impl2._callCounter));
    
    
    // Test 'In' processor
    //
    
    // New ack
    //
    impl1.clear();
    impl2.clear();
    
    in1.onNewAck(order);
    EXPECT_TRUE(checkCounters(impl1._callCounter, 10));
    EXPECT_TRUE(checkCounters(impl2._callCounter, 10));
    
    // New rej
    //
    impl1.clear();
    impl2.clear();
    
    in1.onNewRej(order, rej);
    EXPECT_TRUE(checkCounters(impl1._callCounter, 11));
    EXPECT_TRUE(checkCounters(impl2._callCounter, 11));
    
    // Mod ack
    //
    impl1.clear();
    impl2.clear();
    
    in1.onModAck(order);
    EXPECT_TRUE(checkCounters(impl1._callCounter, 12));
    EXPECT_TRUE(checkCounters(impl2._callCounter, 12));
    
    // Mod rej
    //
    impl1.clear();
    impl2.clear();
    
    in1.onModRej(order, rej);
    EXPECT_TRUE(checkCounters(impl1._callCounter, 13));
    EXPECT_TRUE(checkCounters(impl2._callCounter, 13));
    
    // Cxl ack
    //
    impl1.clear();
    impl2.clear();
    
    in1.onCxlAck(order);
    EXPECT_TRUE(checkCounters(impl1._callCounter, 14));
    EXPECT_TRUE(checkCounters(impl2._callCounter, 14));
    
    // Cxl rej
    //
    impl1.clear();
    impl2.clear();
    
    in1.onCxlRej(order, rej);
    EXPECT_TRUE(checkCounters(impl1._callCounter, 15));
    EXPECT_TRUE(checkCounters(impl2._callCounter, 15));
    
    // Fill
    //
    impl1.clear();
    impl2.clear();
    
    in1.onFill(fill);
    EXPECT_TRUE(checkCounters(impl1._callCounter, 16));
    EXPECT_TRUE(checkCounters(impl2._callCounter, 16));
    
    
    // Alert
    //
    impl1.clear();
    impl2.clear();
    
    in1.onAlert(alert);
    EXPECT_TRUE(checkCounters(impl1._callCounter, 17));
    EXPECT_TRUE(checkCounters(impl2._callCounter, 17)); 
    
    // Alert
    //
    impl1.clear();
    impl2.clear();
    
    in1.onRebuildOrderRej(order, rej);
    EXPECT_TRUE(checkCounters(impl1._callCounter, 18));
    EXPECT_TRUE(checkCounters(impl2._callCounter, 18));
    
}
