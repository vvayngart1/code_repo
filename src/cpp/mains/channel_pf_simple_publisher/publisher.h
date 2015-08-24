#pragma once

#include <tw/log/defs.h>
#include <tw/instr/instrument_manager.h>
#include <tw/channel_pf_cme/settings.h>

#include <tw/common_strat/istrategy.h>
#include <tw/common_strat/strategy_container.h>
#include <tw/common_str_util/fast_stream.h>
#include <tw/common/pool.h>
#include <tw/common_thread/thread_pipe.h>

#include <string>
#include <map>
#include <set>

struct QuoteDataItem {
    QuoteDataItem() {
        clear();
    }
    
    void clear() {
        // Nothing to do
        //
    }
    
    void set(const tw::price::Quote& quote) {
        _timestamp1 = quote._timestamp1;
        _quoteWire = quote;
    }
    
    tw::common::THighResTime _timestamp1;
    tw::price::QuoteWire _quoteWire;
};

typedef boost::shared_ptr<QuoteDataItem> TQuoteDataItemPtr;

// Publisher class, which accepts connections and send subscribed data to them
//
class Publisher : public tw::common_strat::IStrategy {
    typedef std::set<tw::instr::Instrument::TKeyId> TSubscriptions;
    typedef std::map<tw::common_strat::TMsgBusConnection::native_type, TSubscriptions> TConnectionSubscriptions;
    
public:
    Publisher();
    ~Publisher();
    
    void clear();
    void waitForReplayToFinish();
    
public:
    // IStrategy interface
    //
    virtual bool init(const tw::common::Settings& settings, const tw::risk::Strategy& strategyParams);    
    virtual bool start();    
    virtual bool stop();
    
    virtual void recordExternalFill(const tw::channel_or::Fill& fill) {
    }
    
    bool rebuildOrder(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej) {
        return false;
    }
    
    void rebuildPos(const tw::channel_or::PosUpdate& update) {        
    }

    virtual void onConnectionUp(tw::common_strat::TMsgBusConnection::native_type id, tw::common_strat::TMsgBusConnection::pointer connection);
    virtual void onConnectionDown(tw::common_strat::TMsgBusConnection::native_type id);    
    virtual void onConnectionData(tw::common_strat::TMsgBusConnection::native_type id, const tw::common::Command& cmnd);
    
    virtual void onAlert(const tw::channel_or::Alert& alert) {
    }
    
public:
    void onQuote(const tw::price::Quote& quote);
    
private:
    void replay();
    void replayVerify();
    
private:
    void serializeQuote(const tw::price::Quote& quote);
    
    void threadMainRecord();
    void terminateThreadRecord();
    
    void threadMainReplay();
    void terminateThreadReplay();
    
private:
    TQuoteDataItemPtr getItem() {
        return TQuoteDataItemPtr(_pool.obtain(), _pool.getDeleter());
    }    
    
private:
    typedef tw::common_thread::Lock TLock;
    typedef tw::common_thread::Thread TThread;
    typedef tw::common_thread::ThreadPtr TThreadPtr;
    
    typedef tw::common::Pool<QuoteDataItem, TLock> TPool;
    typedef tw::common_thread::ThreadPipe<TQuoteDataItemPtr> TThreadPipe;
    
private:
    tw::common_str_util::FastStream<256> _priceStream;
    std::stringstream _message;
    TConnectionSubscriptions _connectionSubscriptions;
    
    uint32_t _quotesCount;
    std::fstream _file;
    tw::common::Settings _settings;
    tw::channel_pf_cme::Settings _settingsCme;
    
    bool _isDone;
    TThreadPtr _threadRecord;
    TThreadPtr _threadReplay;
    
    TPool _pool;
    TThreadPipe _threadPipe;
};
