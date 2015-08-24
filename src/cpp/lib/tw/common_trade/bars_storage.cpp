#include <tw/common_trade/bars_storage.h>

#include <tw/common_strat/consumer_proxy.h>
#include <tw/common_thread/utils.h>
#include <tw/common_strat/strategy_container.h>

namespace tw {
namespace common_trade {

// struct BarsStorageItem
//
uint32_t BarsStorageItem::_counter = 0;

BarsStorageItem::BarsStorageItem() {
    clear();
}
    
void BarsStorageItem::clear() {
    _id = 0;
    _type = tw::common::eBarsStorageItemType::kUnknown;
    _bar.clear();
    _barPattern.clear();
}
    
// class BarsStorage
//
BarsStorage::BarsStorage() {
    clear();
}

BarsStorage::~BarsStorage() {
    stop();
}

void BarsStorage::clear() {
    _isDoneDb = false;
    _isDoneDbThread = false;
    
    _stopPersistingToDb = false;
    
    _threadDb.reset();
    
    _itemsCacheSize = 0;
    _itemsCache.clear();
    
    clearQueries();
    _statement.reset();        
    _connection.reset();
    
    _settings._bars_storage_processPatterns = true;
}

void BarsStorage::clearQueries() {
    _bars_SaveBar.clear();
    _barsPatterns_SaveBarPattern.clear();
}

void BarsStorage::closeDbResources() {
    if ( _statement )
        _statement->close();

    if ( _connection )
        _connection->close();
}
    
bool BarsStorage::init(const tw::common::Settings& settings) {
    bool status = true;
    
    try {
        _settings = settings;
        if ( !_settings._bars_storage_outputToDb ) {
            LOGGER_WARN << "_bars_outputToDb=false - bars will not be saved to DB" << "\n";
            return true;
        }
        
        if ( _settings._bars_storage_historical_bars_source.empty() ) {
            LOGGER_ERRO << "bars_storage.historical_bars_source is not set" << "\n";
            return false;
        }
        
        if ( !_channelDb.init(settings._db_connection_str) ) {
            LOGGER_ERRO << "Failed to init channelDb with: "  << settings._db_connection_str << "\n";
            return false;
        }
        
        if ( !_channelDb.isValid() )
            return false;
        
        _connection = _channelDb.getConnection();
        if ( !_connection ) {
            LOGGER_ERRO << "Can't open connection to db"  << "\n" << "\n";
            status = false;
        }
        
        _statement = _channelDb.getStatement(_connection);
        if ( !_statement ) {
            LOGGER_ERRO << "Can't get statement for db"  << "\n" << "\n";
            status = false;
        }
        
        _threadDb = TThreadPtr(new TThread(boost::bind(&BarsStorage::threadMainPersistToDb, this)));
        
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    if ( !status )
        stop();
    
    return true;
}

bool BarsStorage::start() {
    return true;
}

void BarsStorage::stop() {
    try {
        if ( isStopped() )
            return;
        
        terminateThreadDb();        
        flushDbCache();
        closeDbResources();
        clear();
        
        LOGGER_INFO << "Stopped BarsStorage" << "\n";
        
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

bool BarsStorage::readBars(uint32_t x, uint32_t barLength, uint32_t lookbackMinutes, uint32_t atrNumOfPeriods, TBars& bars) {
    try {
        // Get instrument
        //
        tw::instr::InstrumentPtr instrument = tw::instr::InstrumentManager::instance().getByKeyId(x);
        if ( !instrument ) {
            LOGGER_ERRO << "Can't find instrument for keyId: " << x << "\n";
            return false;
        }
        
        if ( 0 == barLength ) {
            LOGGER_ERRO << "0 == barLength for keyId: " << x << "\n";
            return false;
        }
        
        if ( 0 == lookbackMinutes ) {
            LOGGER_ERRO << "0 == lookbackMinutes for keyId: " << x << "\n";
            return false;
        }
        
        std::string timestamp = tw::common::THighResTime::sqlString(lookbackMinutes*-60);
        
        Bars_GetHistory query;        
        if ( !query.execute(_channelDb, _settings._bars_storage_historical_bars_source, instrument->_displayName, instrument->_exchange.toString(), barLength, atrNumOfPeriods, timestamp) )
            return false;
        
        bars = query._o1;
        
        LOGGER_INFO << "Executed query 'Bars_GetHistory' for: " << instrument->_exchange.toString() << "::" << instrument->_displayName << " -- timestamp >=" << timestamp << ", number of rows returned = " << bars.size() << "\n";
        return true;
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    return false;
}

bool BarsStorage::doPersist(const TBarsStorageItemPtr& item) {
    try {
        if ( !isValid() ) {
            LOGGER_ERRO << "Can't persist item: " << item->toString() << "\n";
            return false;
        }
        
        if ( canPersistToDb() ) {
            _threadPipeDb.push(item);
            return true;
        }
        
        tw::channel_or::Alert alert;
        alert._type = tw::channel_or::eAlertType::kChannelDbMaxQueueSize;
        alert._text = "BarsStorage exceeded DB storage's max queue size of: " + boost::lexical_cast<std::string>(_settings._bars_storage_max_queue_size_db);
        tw::common_strat::ConsumerProxy::instance().onAlert(alert);
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    
    return true;
}

bool sortPriceTradesInfo(const PriceTradesInfo& a, const PriceTradesInfo& b) {
    return (a._price > b._price);
}

void barTradesToString(Bar& bar) {
    std::vector<tw::common_trade::PriceTradesInfo> trades;
        
    for ( std::tr1::unordered_map<tw::price::Ticks::type, tw::common_trade::PriceTradesInfo>::const_iterator iter = bar._trades.begin(); iter != bar._trades.end(); ++iter )
        trades.push_back(iter->second);

    std::sort(trades.begin(), trades.end(), sortPriceTradesInfo);
    
    std::vector<tw::common_trade::PriceTradesInfo>::iterator iter = trades.begin();
    std::vector<tw::common_trade::PriceTradesInfo>::iterator end = trades.end();
    
    tw::common_str_util::TFastStream s;
    for ( ; iter != end; ++iter ) { 
        s << "p=" << iter->_price
          << ",vB=" << iter->_volBid
          << ",vA=" << iter->_volAsk
          << ";";
    }
    
    bar._tradesLog = s.str();
}

bool BarsStorage::flushToDb() {
    try {
        while ( !_itemsCache.empty() && !_stopPersistingToDb ) {
            clearQueries();
            
            // Lock for thread synchronization
            //
            tw::common_thread::LockGuard<TLock> lock(_dbLock);
            
            size_t count = std::min(static_cast<size_t>(_settings._bars_storage_batch_cache_size), _itemsCache.size());
            TItemsCache::iterator first = _itemsCache.begin();
            TItemsCache::iterator last = first;
            for ( size_t i = 0; i < count; ++i, ++last ) {
                TBarsStorageItemPtr& item = _itemsCache[i];
                switch ( item->_type ) {
                    case tw::common::eBarsStorageItemType::kBar:
                        barTradesToString(item->_bar);
                        _bars_SaveBar.add(item->_bar);
                        break;
                    case tw::common::eBarsStorageItemType::kBarPattern:
                        _barsPatterns_SaveBarPattern.add(item->_barPattern);
                        break;
                    default:
                        LOGGER_ERRO << "Unknown item type "  << item->toString() << "\n" << "\n";
                        break;
                }
            }
            
            if ( _stopPersistingToDb )
                return true;
            
            _connection->setAutoCommit(false);
            
            if ( _bars_SaveBar.count() > 0 && !_stopPersistingToDb ) {
                if ( !_bars_SaveBar.execute(_statement) ) {
                     _connection->rollback();
                     return false;
                }
            }
            
            if ( _barsPatterns_SaveBarPattern.count() > 0 && !_stopPersistingToDb ) {
                if ( !_barsPatterns_SaveBarPattern.execute(_statement) ) {
                     _connection->rollback();
                     return false;
                }
            }
            
            if ( _stopPersistingToDb ) {
                _connection->rollback();
                return true;
            }
            
            _connection->commit();
            
            _itemsCache.erase(first, last);
            _itemsCacheSize = _itemsCache.size();
        }
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    
    return true;
}

bool BarsStorage::persistToDb(const TBarsStorageItemPtr& item) {
    try {
        if ( !item )
            return false;
        
        if ( !_connection || !_statement ) {
            LOGGER_ERRO << "_connection or _statement" << "\n" << "\n";
            return false;
        }
        
        if ( _itemsCache.size() > _settings._bars_storage_max_batch_cache_size )
            return false;
        
        item->setSource( _source ); //stamp the source before we save to DB        
        if ( tw::common::eBarsStorageItemType::kUnknown != item->_type ) {
            _itemsCache.push_back(item);
            _itemsCacheSize = _itemsCache.size();
        }
        
        if ( _itemsCache.size() < _settings._bars_storage_batch_cache_size )
            return true;
        
        flushToDb();        
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    
    return true;
}

void BarsStorage::flushDbCache() {
    try {
        {
            TItemsCache::iterator iter = _itemsCache.begin();
            TItemsCache::iterator end = _itemsCache.end();
            for ( ; iter != end; ++iter ) {
                _threadPipeDb.push_front(*iter);
            }
            _itemsCache.clear();
            _itemsCacheSize = 0;
        }
        
        LOGGER_INFO << "Trying to flush items count: "  << _threadPipeDb.size() << "\n";
        
        TBarsStorageItemPtr item;
        while ( _threadPipeDb.try_read(item) ) {
            if ( !persistToDb(item) || !flushToDb() ) {
                _threadPipeDb.push_front(item);
                LOGGER_ERRO << "Failed to flush to db item: "  << item->toString() << "\n";
                break;
            }
            
            LOGGER_INFO << "Flushed to db item: "  << item->toString() << "\n";
        }
        
        _threadPipeDb.clear();
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void BarsStorage::terminateThreadDb() {
    LOGGER_INFO << "Started to terminate threadDb" << "\n";    
    
    try {
        // Wait for db thread to finish
        //
        _isDoneDb = true;
        
        if ( _threadDb ) {
            _threadPipeDb.stop();
            uint32_t secsToWait = 2;
            if ( !_threadDb->timed_join(boost::posix_time::seconds(secsToWait)) ) {
                LOGGER_WARN << "_threadDb didn't finish in " << secsToWait << " secs" << "\n";
                _threadDb->interrupt();
                if ( !_threadDb->timed_join(boost::posix_time::seconds(secsToWait)) ) {
                    LOGGER_WARN << "_threadDb didn't finish in another " << secsToWait << " secs" << "\n";
                    closeDbResources();
                    if ( !_threadDb->timed_join(boost::posix_time::seconds(secsToWait)) ) {
                        LOGGER_WARN << "_threadDb didn't finish in another " << secsToWait << " secs after closing db resources" << "\n";
                    }
                }
            }
            
            _threadDb.reset();
            
            // Wait for 2 seconds for thread to terminate
            //
            for ( uint32_t i = 0; i < 20 && !_isDoneDbThread; ++i ) {
                tw::common_thread::sleep(100);                
            }
            
            if ( !_isDoneDbThread ) {
                LOGGER_ERRO << "_threadDb didn't terminate in another " << secsToWait << " secs" << "\n";
            }
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    LOGGER_INFO << "Finished terminating threadDb" << "\n";    
    return;
}

void BarsStorage::threadMainPersistToDb() {
    LOGGER_INFO << "Started" << "\n";
    
    const unsigned BAR_SLEEP_MSEC = 1000;
    
    try {
        TBarsStorageItemPtr item;
        while( !_isDoneDb ) {
            while ( _threadPipeDb.try_read(item, false) ) {
                if ( persistToDb(item) ) {
                   _threadPipeDb.pop();                    
                } else {
                   LOGGER_ERRO << "persistToDb() failed" << "\n";
                   //give a transient error some time to recover
                   tw::common_thread::sleep(BAR_SLEEP_MSEC);                    
                }
                item.reset();
            }
            
            // Flush batch every 1000 ms
            //
            tw::common_thread::sleep(BAR_SLEEP_MSEC);
            if ( !flushToDb() )
                LOGGER_ERRO << "flushDb() failed" << "\n";
        }
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    _isDoneDbThread = true;
    LOGGER_INFO << "Finished" << "\n";
}

} // namespace common_trade
} // namespace tw
