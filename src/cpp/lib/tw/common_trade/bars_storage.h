#pragma once

#include <tw/common/command.h>
#include <tw/common/pool.h>
#include <tw/common/singleton.h>
#include <tw/common/filesystem.h>
#include <tw/common_thread/locks.h>
#include <tw/common_thread/thread.h>
#include <tw/common_thread/thread_pipe.h>
#include <tw/channel_db/channel_db.h>

#include <tw/generated/bars.h>
#include <tw/generated/enums_common.h>

#include <boost/algorithm/string.hpp>
#include <fstream>

namespace tw {
namespace common_trade {
    
struct BarsStorageItem {
    static uint32_t _counter;    
    
    BarsStorageItem();
    void clear();
    
    void set(const Bar& v) {
        _type = tw::common::eBarsStorageItemType::kBar;
        _bar = v;
    }
    
    void set(const BarPattern& v) {
        _type = tw::common::eBarsStorageItemType::kBarPattern;
        _barPattern = v;
    }
    
    std::string toString() const {
        std::string r;
        
        r = _type.toString() + std::string(" ==> ");
        switch ( _type ) {
            case tw::common::eBarsStorageItemType::kBar:
                r += _bar.toString();
                break;
            case tw::common::eBarsStorageItemType::kBarPattern:
                r += _barPattern.toString();
                break;
            default:
                r += "<Unknown>";
                break;
        }
        
        return r;                
    }
    
    void setSource(std::string source) {
        switch ( _type ) {
            case tw::common::eBarsStorageItemType::kBar:
                _bar._source = source;
                break;
            case tw::common::eBarsStorageItemType::kBarPattern:
                _barPattern._source = source;
                break;
            default:
                LOGGER_ERRO << "unknown eBarsStorageItemType!\n";
        }
    }
    
    uint32_t _id;
    tw::common::eBarsStorageItemType _type;
    Bar _bar;
    BarPattern _barPattern;
};

typedef boost::shared_ptr<BarsStorageItem> TBarsStorageItemPtr;

class BarsStorage : public tw::common::Singleton<BarsStorage> {
public:
    typedef std::vector<Bar> TBars;
    
public:
    BarsStorage();
    ~BarsStorage();
    
    void clear();
    
public:
    bool init(const tw::common::Settings& settings);
    bool start();    
    void stop();
    
    void setSource(const std::string& source) {
        _source = source;
        LOGGER_INFO << "set BarsStorage._source to: " << source << "\n";
    }
    
    bool outputToDb() const {
        return _settings._bars_storage_outputToDb;
    }
    
    bool readBars(uint32_t x, uint32_t barLength, uint32_t lookbackMinutes, uint32_t atrNumOfPeriods, TBars& bars);
    
    bool processPatterns() const {
        return _settings._bars_storage_processPatterns;
    }
    
public:
    bool isValid() {
        return ((_threadDb != NULL));
    }
    
    bool canPersistToDb() const {
        return (_threadPipeDb.size() > _settings._bars_storage_max_queue_size_db ) ? false : true;
    }

    // Persistent storage writes
    //
    template <typename TItemType>
    bool persist(const TItemType& v) {
        try {
            if ( !_settings._bars_storage_outputToDb )
                return true;
            
            TBarsStorageItemPtr item = getItem();
            item->set(v);

            return doPersist(item);
        } catch(const std::exception& e) {        
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
            return false;
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
            return false;
        }

        return true;
    }
    
private:
    TBarsStorageItemPtr getItem(bool restored = false) {
        TBarsStorageItemPtr item(_pool.obtain(), _pool.getDeleter());
        if ( !restored )
            item->_id = ++(item->_counter);
        
        return item;
    }    
    
private:
    void clearQueries();
    void closeDbResources();
    
    bool doPersist(const TBarsStorageItemPtr& item);
    bool flushToDb();
    bool persistToDb(const TBarsStorageItemPtr& item);
    
    void flushDbCache();
    
    void terminateThreadDb();
    void threadMainPersistToDb();
    
    bool isStopped() const {
        return ( !_threadDb 
                && _threadPipeDb.empty()
                && _itemsCache.empty() );
    }
    
private:
    typedef tw::common_thread::Thread TThread;
    typedef tw::common_thread::ThreadPtr TThreadPtr;
    
    typedef tw::common::Pool<BarsStorageItem, tw::common_thread::Lock> TPool;
    typedef tw::common_thread::ThreadPipe<TBarsStorageItemPtr> TThreadPipe;
    
    typedef tw::channel_db::ChannelDb TChannelDb;
    typedef std::vector<TBarsStorageItemPtr> TItemsCache;
    
    typedef tw::common_thread::Lock TLock;
    
private:
    TLock _lock;
    TLock _dbLock;
    
    bool _isDoneDb;
    bool _isDoneDbThread;
    bool _stopPersistingToDb;
    tw::common::Settings _settings;
    TChannelDb _channelDb;
    
    tw::channel_db::ChannelDb::TConnectionPtr _connection;
    tw::channel_db::ChannelDb::TStatementPtr _statement;
    
    TPool _pool;
    TThreadPtr _threadDb;
    TThreadPipe _threadPipeDb;
    
    size_t _itemsCacheSize;
    TItemsCache _itemsCache;
    
    Bars_SaveBar _bars_SaveBar;
    BarsPatterns_SaveBarPattern _barsPatterns_SaveBarPattern;
    
    std::string _source;
};

    
} // namespace channel_or
} // namespace tw
