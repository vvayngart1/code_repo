#pragma once

#include <tw/common/command.h>
#include <tw/common/pool.h>
#include <tw/common/singleton.h>
#include <tw/common/filesystem.h>
#include <tw/common_thread/locks.h>
#include <tw/common_thread/thread.h>
#include <tw/common_thread/thread_pipe.h>
#include <tw/generated/channel_or_defs.h>
#include <tw/generated/enums_common.h>
#include <tw/channel_or/settings.h>
#include <tw/channel_db/channel_db.h>

#include <boost/algorithm/string.hpp>

#include <fstream>

namespace tw {
namespace channel_or {
    
struct ChannelOrStorageItem {
    static uint32_t _counter;    
    
    ChannelOrStorageItem();
    void clear();
    
    std::string toString() const;    
    bool fromString(const std::string& line);
    
    void set(const tw::common::Command& command) {
        _type = tw::common::eChannelOrStorageItemType::kCommand;
        _command = command;
    }
    
    void set(const tw::channel_or::Order& order) {
        _type = tw::common::eChannelOrStorageItemType::kOrder;
        _order = order;
    }
    
    void set(const tw::channel_or::Order& order, const tw::channel_or::Reject& rej) {
        _type = tw::common::eChannelOrStorageItemType::kOrderAndRej;
        _order = order;
        _rej = rej;
    }
    
    void set(const tw::channel_or::Fill& fill) {
        _type = tw::common::eChannelOrStorageItemType::kFill;
        _fill = fill;
        if ( _fill._order )
            _order = *(_fill._order);
    }
    
    void set(const tw::channel_or::FixSessionCMEState& state) {
        _type = tw::common::eChannelOrStorageItemType::kFixSessionCMEState;
        _stateCME = state;
    }
    
    void set(const tw::channel_or::FixSessionCMEMsg& msg) {
        _type = tw::common::eChannelOrStorageItemType::kFixSessionCMEMsg;
        _msgCME = msg;
    }
    
    
    void set(const tw::channel_or::FillDropCopy& fill) {
        _type = tw::common::eChannelOrStorageItemType::kFillDropCopy;
        _fillDropCopy = fill;
    }
        
    void set(const tw::channel_or::PnLAuditTrailInfo& pnlAuditTrailInfo) {
        _type = tw::common::eChannelOrStorageItemType::kPnLAuditTrailInfo;
        _pnlAuditTrailInfo = pnlAuditTrailInfo;
    }
    
    uint32_t _id;
    tw::common::eChannelOrStorageItemType _type;
    tw::common::Command _command;
    tw::channel_or::Order _order;
    tw::channel_or::Reject _rej;
    tw::channel_or::Fill _fill;
    tw::channel_or::FixSessionCMEState _stateCME;
    tw::channel_or::FixSessionCMEMsg _msgCME;
    tw::channel_or::FillDropCopy _fillDropCopy;
    tw::channel_or::PnLAuditTrailInfo _pnlAuditTrailInfo;
    bool _isSendToMsgBus;
};

typedef boost::shared_ptr<ChannelOrStorageItem> TChannelOrStorageItemPtr;

class ChannelOrStorageFile {
public:
    ChannelOrStorageFile();
    ~ChannelOrStorageFile();
    
public:
    bool init(const tw::channel_or::Settings& settings);    
    bool start();    
    bool stop();
    
public:
    bool write(const ChannelOrStorageItem& item);    
    bool read(ChannelOrStorageItem& item);    
    bool rewindRead();
    
public:
    bool exists() const {
        return tw::common::Filesystem::exists(_filename);
    }
    
    bool remove(bool backup) {
        if ( backup )        
            return tw::common::Filesystem::swap(_filename);
        
        return tw::common::Filesystem::remove(_filename);
    }
    
private:
    std::string _filename;
    std::ifstream _reader;
    std::ofstream _writer;
};
    
class ChannelOrStorage : public tw::common::Singleton<ChannelOrStorage> {
public:
    ChannelOrStorage();
    ~ChannelOrStorage();
    
    void clear();
    
public:
    bool init(const tw::common::Settings& settings);
    bool start();    
    void stop();
    
    void onCommand(const tw::common::Command& command);
    
public:
    // Persistent storage reads
    //
    bool getOpenOrders(std::vector<Order>& results);
    bool getOpenOrdersForAccount(std::vector<Order>& results, const tw::risk::TAccountId& accId);
    bool getFillsForDate(std::vector<Fill>& results, const std::string& date);    
    bool getFillsForAccountForDate(std::vector<Fill>& results, const tw::risk::TAccountId& accId, const std::string& date);
    bool getPositions(std::vector<PosUpdate>& results);    
    bool getPositionsForAccount(std::vector<PosUpdate>& results, const tw::risk::TAccountId& accId);
    
    bool getPositionsDropCopy(std::vector<PosUpdateDropCopy>& results);    
    bool getPositionsDropCopyForAccount(std::vector<PosUpdateDropCopy>& results, const std::string& acc);
    
    bool getMessagingForExchanges(std::vector<MessagingForExchange>& results);
    
    // NOTE: 'result' is in/out parameter, it is expected to have
    // senderCompID, targetCompID, year and week fields populated
    //
    bool getFixSessionCMEStatesForSession(FixSessionCMEState& result);
    
public:
    bool isValid() {
        return ((_threadDb != NULL) || (_threadFile != NULL));
    }
    
    bool canPersist() const {
        return (_threadPipeFile.size() > _settings._storage_max_queue_size_file ) ? false : true;
    }
    
    bool canPersistToDb() const {
        return (_threadPipeDb.size() > _settings._storage_max_queue_size_db ) ? false : true;
    }
    
    // Persistent storage writes
    //
    bool persist(const tw::common::Command& command);
    bool persist(const tw::channel_or::TOrderPtr& order);
    bool persist(const tw::channel_or::TOrderPtr& order, const tw::channel_or::Reject& rej);
    bool persist(const tw::channel_or::Fill& fill);
    bool persist(const tw::channel_or::FixSessionCMEState& state);
    bool persist(const tw::channel_or::FixSessionCMEMsg& msg);
    
    // Drop copy related persistent storage writes
    //
    bool persist(const tw::channel_or::FillDropCopy& fill);
    
    // PnLAuditTRail related persistent storage writes
    //
    bool persist(const tw::channel_or::PnLAuditTrailInfo& pnlAuditTrailInfo);
    
private:
    TChannelOrStorageItemPtr getItem(bool restored = false) {
        TChannelOrStorageItemPtr item(_pool.obtain(), _pool.getDeleter());
        if ( !restored )
            item->_id = ++(item->_counter);
        
        return item;
    }    
    
private:
    void clearQueries();
    void closeDbResources();
    
    bool doPersist(const TChannelOrStorageItemPtr& item);
    bool flushToDb();
    bool doFlushToDb();
    bool persistToDb(const TChannelOrStorageItemPtr& item);
    bool persistToFile(const TChannelOrStorageItemPtr& item);
    
    void flushDbCache();
    void flushFileCache();
    
    void terminateThreadDb();
    void threadMainPersistToDb();
    void threadMainPersistToFile();
    
    void sendToMsgBus(const TChannelOrStorageItemPtr& item);
    
    bool isStopped() const {
        return ( !_threadDb 
                && !_threadFile 
                && _threadPipeDb.empty()
                && _threadPipeFile.empty()
                && _itemsCache.empty() );
    }
    
    void startThreadPersistToFile(const std::string& reason);
    
private:
    void addMessagingForExchanges(const std::string& symbol, const tw::instr::eExchange& exchange, const TAccountId& accountId, const tw::channel_or::Messaging& v) {
        tw::channel_or::MessagingForExchange m;
        static_cast<tw::channel_or::Messaging&>(m) = v;
        m._symbol = symbol;
        m._exchange = exchange;
        m._accountId = accountId;
        m._messaging_YYYYMMDD = tw::common::THighResTime::dateISOString();
        _messagingForExchanges_Save.add(m);
    }
    
private:
    typedef tw::common_thread::Thread TThread;
    typedef tw::common_thread::ThreadPtr TThreadPtr;
    
    typedef tw::common::Pool<ChannelOrStorageItem, tw::common_thread::Lock> TPool;
    typedef tw::common_thread::ThreadPipe<TChannelOrStorageItemPtr> TThreadPipe;
    
    typedef tw::channel_db::ChannelDb TChannelDb;
    typedef std::vector<TChannelOrStorageItemPtr> TItemsCache;
    
    typedef tw::common_thread::Lock TLock;
    
private:
    TLock _lock;
    TLock _dbLock;
    
    bool _isDoneDb;
    bool _isDoneDbThread;
    bool _isDoneFile;
    bool _stopPersistingToDb;
    tw::channel_or::Settings _settings;
    TChannelDb _channelDb;
    ChannelOrStorageFile _channelOrStorageFile;
    
    tw::channel_db::ChannelDb::TConnectionPtr _connection;
    tw::channel_db::ChannelDb::TStatementPtr _statement;
    
    TPool _pool;
    TThreadPtr _threadDb;
    TThreadPtr _threadFile;
    TThreadPipe _threadPipeDb;
    TThreadPipe _threadPipeFile;
    
    size_t _itemsCacheSize;
    TItemsCache _itemsCache;
    
    ChannelOrStorageStatsEntered _statsEntered;
    ChannelOrStorageStatsPersisted _statsPersistedDb;
    ChannelOrStorageStatsEntered _statsPersistedFile;
    
    tw::channel_or::CommandsLog_SaveCommandLog _commandsLog_SaveCommandLog;
    tw::channel_or::Orders_SaveOrderTransaction _orders_SaveOrderTransaction;
    tw::channel_or::Orders_SaveOrder _orders_SaveOrder;        
    tw::channel_or::Fills_SaveFill _fills_SaveFill;
    tw::channel_or::Positions_SavePosUpdate _positions_SavePosUpdate;        
    tw::channel_or::FixSessionCMEStates_SaveState _fixSessionCMEStates_SaveState;
    tw::channel_or::FixSessionCMEMsgs_SaveMsg _fixSessionCMEMsgs_SaveMsg;
    
    tw::channel_or::FillsDropCopy_SaveFill _fillsDropCopy_SaveFill;
    tw::channel_or::PositionsDropCopy_SavePosUpdate _positionsDropCopy_SavePosUpdate;
    
    tw::channel_or::MessagingForExchanges_Save _messagingForExchanges_Save;
    
    tw::channel_or::PnLAuditTrail_SavePnLAuditTrail _pnlAuditTrail_SavePnLAuditTrail;
};

    
} // namespace channel_or
} // namespace tw
