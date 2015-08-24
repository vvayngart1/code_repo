#include <tw/channel_or/channel_or_storage.h>

#include <tw/common_strat/consumer_proxy.h>
#include <tw/common_thread/utils.h>
#include <tw/common_strat/strategy_container.h>

#include <tw/generated/commands_common.h>

#include <OnixS/FIXEngine.h>

namespace tw {
namespace channel_or {

// struct ChannelOrStorageItem
//
const char STORAGE_ITEM_FIELDS_DELIM=0x03;
const char FIELDS_DELIM_REPLACEMENT=0x04;

const std::string& get_FIELDS_DELIM_REPLACEMENT() {
    static std::string s;
    s = FIELDS_DELIM_REPLACEMENT;
    return s;
}

bool is_STORAGE_ITEM_FIELDS_DELIM(const char c) {
    return c==STORAGE_ITEM_FIELDS_DELIM;
}
  
uint32_t ChannelOrStorageItem::_counter = 0;

ChannelOrStorageItem::ChannelOrStorageItem() {
    clear();
}
    
void ChannelOrStorageItem::clear() {
    _id = 0;
    _type = tw::common::eChannelOrStorageItemType::kUnknown;
    _command.clear();
    _order.clear();
    _rej.clear();
    _fill.clear();
    _stateCME.clear();
    _msgCME.clear();
    _fillDropCopy.clear();
    _isSendToMsgBus = false;
}
    
std::string ChannelOrStorageItem::toString() const {
    std::stringstream out;
    out << _id << STORAGE_ITEM_FIELDS_DELIM;
    out << _type << STORAGE_ITEM_FIELDS_DELIM;
    
    switch ( _type ) {
        case tw::common::eChannelOrStorageItemType::kCommand:
            out << _command.toString();
            break;
        case tw::common::eChannelOrStorageItemType::kOrder:
            out << _order.toString();
            break;
        case tw::common::eChannelOrStorageItemType::kOrderAndRej:
            out << _order.toString() << STORAGE_ITEM_FIELDS_DELIM;
            out << _rej.toString();
            break;
        case tw::common::eChannelOrStorageItemType::kFill:
            out << _fill.toString();
            if ( _fill._order ) {
                out << STORAGE_ITEM_FIELDS_DELIM;
                out << _order.toString();
            }
            break;
        case tw::common::eChannelOrStorageItemType::kFixSessionCMEState:
            out << _stateCME.toString();
            break;
        case tw::common::eChannelOrStorageItemType::kFixSessionCMEMsg:
            const_cast<ChannelOrStorageItem*>(this)->_msgCME._message = std::replace_all(_msgCME._message, FIELDS_DELIM, get_FIELDS_DELIM_REPLACEMENT());
            out << _msgCME.toString();
            break;
        case tw::common::eChannelOrStorageItemType::kFillDropCopy:
            out << _fillDropCopy.toString();
            break;
        case tw::common::eChannelOrStorageItemType::kPnLAuditTrailInfo:
            out << _pnlAuditTrailInfo.toString();
            break;
        default:
            LOGGER_ERRO << "unknown type: " << _type << "\n";
            out.str("");
            break;
    }
    
    return out.str();
}

bool ChannelOrStorageItem::fromString(const std::string& line) {
    std::vector<std::string> values;
    boost::split(values, line, is_STORAGE_ITEM_FIELDS_DELIM);

    size_t s1 = values.size();
    size_t s2 = 3;
    if ( s1 < s2 ) {
        LOGGER_ERRO << "incorrect number of fields: " << s1 << " :: " << s2 << "\n"; 
        return false;
    }

    _id = boost::lexical_cast<uint32_t>(values[0]);
    _type = boost::lexical_cast<tw::common::eChannelOrStorageItemType>(values[1]);
    switch ( _type ) {
        case tw::common::eChannelOrStorageItemType::kCommand:
            if ( !_command.fromString(values[2]) )
                return false;
            break;
        case tw::common::eChannelOrStorageItemType::kOrder:
            if ( !_order.fromString(values[2]) )
                return false;
            break;
        case tw::common::eChannelOrStorageItemType::kOrderAndRej:
            s2 = 4;
            if ( s1 < s2 ) {
                LOGGER_ERRO << "incorrect number of fields: " << s1 << " :: " << s2 << "\n"; 
                return false;
            }
            if ( !_order.fromString(values[2]) )
                return false;
            
            if ( !_rej.fromString(values[3]) )
                return false;
            break;
        case tw::common::eChannelOrStorageItemType::kFill:
            if ( !_fill.fromString(values[2]) )
                return false;
            
            if ( 4 == s1 ) {
                _fill._order = tw::channel_or::ProcessorOrders::instance().createOrder(false);
                if ( !_fill._order->fromString(values[3]) )
                    return false;
                
                _order = *(_fill._order);
            }
            break;
        case tw::common::eChannelOrStorageItemType::kFixSessionCMEState:
            if ( !_stateCME.fromString(values[2]) )
                return false;
            break;
        case tw::common::eChannelOrStorageItemType::kFixSessionCMEMsg:
            if ( !_msgCME.fromString(values[2]) )
                return false;
            _msgCME._message = std::replace_all(_msgCME._message, get_FIELDS_DELIM_REPLACEMENT(), FIELDS_DELIM);
            break;
        case tw::common::eChannelOrStorageItemType::kFillDropCopy:
            if ( !_fillDropCopy.fromString(values[2]) )
                return false;
            break;
        case tw::common::eChannelOrStorageItemType::kPnLAuditTrailInfo:
            if ( !_pnlAuditTrailInfo.fromString(values[2]) )
                return false;
            break;
        default:
            LOGGER_ERRO << "unknown type: " << _type << "\n";
            return false;
    }
    
    return true;
}

    
// struct ChannelOrStorageFile
//

ChannelOrStorageFile::ChannelOrStorageFile() {
    stop();
}

ChannelOrStorageFile::~ChannelOrStorageFile() {
    stop();
}    

bool ChannelOrStorageFile::init(const tw::channel_or::Settings& settings) {
    try {
        if ( settings._storage_file_path.empty() )
            _filename = "./";
        else
            _filename = settings._storage_file_path;

        _filename += "ChannelOrStorageFile.log";
    } catch(const std::exception& e) {
        LOGGER_ERRO << "exception :: " << e.what() << "\n" << "\n";      
        return false;
    } catch(...) {
        LOGGER_ERRO << "exception :: <unknown>" << "\n" << "\n";     
        return false;
    }

    return true;
}

bool ChannelOrStorageFile::start() {
    try {
        _writer.open(_filename.c_str(), std::ios_base::out | std::ios_base::app);
        if( !_writer.good() )
            throw std::invalid_argument("failed to open (writer) file " + _filename);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "exception :: " << e.what() << "\n" << "\n";      
        return false;
    } catch(...) {
        LOGGER_ERRO << "exception :: <unknown>" << "\n" << "\n";     
        return false;
    }

    return true;
}

bool ChannelOrStorageFile::stop() {
    try {
        if ( _writer.rdbuf()->is_open() )
            _writer.close();

        if ( _reader.rdbuf()->is_open() )
            _reader.close();

    } catch(const std::exception& e) {
        LOGGER_ERRO << "exception :: " << e.what() << "\n" << "\n";      
        return false;
    } catch(...) {
        LOGGER_ERRO << "exception :: <unknown>" << "\n" << "\n";     
        return false;
    }    

    return true;
}

bool ChannelOrStorageFile::write(const ChannelOrStorageItem& item) {
    try {
        if ( !_writer.rdbuf()->is_open() )
            throw std::invalid_argument("Not open (writer) file: " + _filename);

        _writer << item.toString() << std::endl;
    } catch(const std::exception& e) {
        LOGGER_ERRO << "exception :: " << e.what() << "\n" << "\n";      
        return false;
    } catch(...) {
        LOGGER_ERRO << "exception :: <unknown>" << "\n" << "\n";     
        return false;
    }

    return true;
}

bool ChannelOrStorageFile::read(ChannelOrStorageItem& item) {
    std::string line;
    try {
        item.clear();
        if ( !_reader.rdbuf()->is_open() ) {
            _reader.open(_filename.c_str(), std::ios_base::in);
            if( !_reader.good() )
                throw std::invalid_argument("failed to open (reader) file " + _filename);
            
            if ( !rewindRead() )
                return false;
        }

        if( !_reader.good() )
            throw std::invalid_argument("failed to read file " + _filename);
        
        while ( true ) {
            // This is not an error, just an indication
            // that no more items are left to be read
            //
            if( _reader.eof() ) {
                item.clear();
                return true;
            }
            
            // Skip all incorrect or corrupted messages
            //
            std::getline(_reader, line);
            if ( !line.empty() ) {
                if ( item.fromString(line) ) {
                    item._counter = item._id;
                    return true;
                }
            
                LOGGER_WARN << "Malformed line: " << line << "\n" << "\n";
            }
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "exception :: " << e.what() << " for line: " << line << "\n" << "\n";      
        return false;
    } catch(...) {
        LOGGER_ERRO << "exception :: <unknown>" << "\n" << "\n";     
        return false;
    }

    return true;
}

bool ChannelOrStorageFile::rewindRead() {
    try {            
        if ( !_reader.rdbuf()->is_open() )
            throw std::invalid_argument("Not open (reader) file: " + _filename);

        _reader.seekg(0, std::ios::beg);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "exception :: " << e.what() << "\n" << "\n";      
        return false;
    } catch(...) {
        LOGGER_ERRO << "exception :: <unknown>" << "\n" << "\n";     
        return false;
    }

    return true;
}

// class ChannelOrStorage
//
ChannelOrStorage::ChannelOrStorage() {
    clear();
}

ChannelOrStorage::~ChannelOrStorage() {
    stop();
}

void ChannelOrStorage::clear() {
    _isDoneDb = false;
    _isDoneDbThread = false;
    _isDoneFile = false;
    _stopPersistingToDb = false;
    
    _threadDb.reset();
    _threadFile.reset();
    
    _itemsCacheSize = 0;
    _itemsCache.clear();
    
    _statsEntered.clear();
    _statsPersistedDb.clear();
    _statsPersistedFile.clear();
    
    clearQueries();
    _statement.reset();        
    _connection.reset();
}

void ChannelOrStorage::clearQueries() {
    _commandsLog_SaveCommandLog.clear();
    _orders_SaveOrderTransaction.clear();
    _orders_SaveOrder.clear();
    _fills_SaveFill.clear();
    _positions_SavePosUpdate.clear();        
    _fixSessionCMEStates_SaveState.clear();
    _fixSessionCMEMsgs_SaveMsg.clear();
    
    _fillsDropCopy_SaveFill.clear();
    _positionsDropCopy_SavePosUpdate.clear();
    
    _messagingForExchanges_Save.clear();
    _pnlAuditTrail_SavePnLAuditTrail.clear();
}

void ChannelOrStorage::closeDbResources() {
    if ( _statement )
        _statement->close();

    if ( _connection )
        _connection->close();
}
    
bool ChannelOrStorage::init(const tw::common::Settings& settings) {
    bool status = true;
    
    try {
        if ( settings._channel_or_dataSourceType != "file" ) {
            LOGGER_ERRO << "channel_or_dataSourceType is not supported: "  << settings._channel_or_dataSourceType << "\n";
            return false;
        }
        
        if ( !_settings.parse(settings._channel_or_dataSource) ) {
            LOGGER_ERRO << "Can't parse config file: "  << settings._channel_or_dataSource << "\n";
            return false;
        }
        
        if ( !_settings.validate() ) {
            LOGGER_ERRO << "Settings are invalid in config file: "  << settings._channel_or_dataSource << "\n";
            return false;
        }
        
        if ( !_channelDb.init(settings._db_connection_str) ) {
            LOGGER_ERRO << "Failed to init channelDb with: "  << settings._db_connection_str << "\n";
            return false;
        }
        
        if ( !_channelOrStorageFile.init(_settings) ) {
            LOGGER_ERRO << "Failed to init channelOrStorageFile with: "  << _settings.toString() << "\n";
            return false;
        }        
        
        // Crash recovery and start up (e.g. to get sequence numbers to CME corrected)
        //
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
        
        
        if ( !_channelOrStorageFile.start() )
            return false;
        
        // Check if any records needs to be transferred from file to DB
        //
        bool restoredFromFile = false;
        uint32_t count = 0;
        do {
            TChannelOrStorageItemPtr item = getItem(true);
            if ( !_channelOrStorageFile.read(*item) )
                return false;
            
            if ( tw::common::eChannelOrStorageItemType::kUnknown == item->_type )
                break;
            
            if ( !restoredFromFile ) {                
                LOGGER_INFO << "Restoring db records from file..."  << "\n";
                restoredFromFile = true;
                std::cout << '.';
            }
            
            if ( 0 == (++count % 100) )
                std::cout << '.';
            
            if ( !persistToDb(item) )
                return false;
        } while (true);
        
        // Remove file if succeeded transferring records to DB
        //
        if ( restoredFromFile ) {
            if ( !flushToDb() )
                return false;
            
            LOGGER_INFO << "\n" << "Done Restoring db records from file"  << "\n";
            
            if ( !_channelOrStorageFile.remove(true) )
                return false;
            
            // Restart channelOrStorageFile - opens new empty file for processing
            //
            if ( !_channelOrStorageFile.stop() )
                return false;
            
            if ( !_channelOrStorageFile.start() )
                return false;
        }
        
        _threadDb = TThreadPtr(new TThread(boost::bind(&ChannelOrStorage::threadMainPersistToDb, this)));
        
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

bool ChannelOrStorage::start() {
    return true;
}

void ChannelOrStorage::stop() {
    try {
        if ( isStopped() )
            return;
        
        terminateThreadDb();        
        flushDbCache();
        
        _isDoneFile = true;        
        
        if ( _threadFile ) {
            _threadPipeFile.stop();
            _threadFile->join();
        }
        
        flushFileCache();
        closeDbResources();
        clear();
        
        LOGGER_INFO << "Stopped ChannelOrStorage" << "\n";
        
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void ChannelOrStorage::onCommand(const tw::common::Command& command) {
    try {
        if ( command._type != tw::common::eCommandType::kChannelOrStorage )
            return;

        if ( command._subType != tw::common::eCommandSubType::kList )
            return;

        tw::common_commands::ChannelOrCme c;
        c._results = "\n";
        c._results += tw::common::THighResTime::now().toString();
        c._results += ": ============================== ChannelOrStorage STATS ==============================\n";
        c._results += "\nEntered:\n==============================\n";
        c._results += _statsEntered.header() + "\n";
        c._results += _statsEntered.toString();
        c._results += "\n\nPersistedToDb :: MaxQSize: ";
        c._results += boost::lexical_cast<std::string>(_settings._storage_max_queue_size_db);
        c._results += " <--> QSize: ";
        c._results += boost::lexical_cast<std::string>(_threadPipeDb.size());
        c._results += " <==> <==> BatchCacheSize: ";
        c._results += boost::lexical_cast<std::string>(_settings._storage_batch_cache_size);
        c._results += " <--> MaxBatchCacheSize: ";
        c._results += boost::lexical_cast<std::string>(_settings._storage_max_batch_cache_size);
        c._results += " <--> CacheSize: ";
        c._results += boost::lexical_cast<std::string>(_itemsCacheSize);        
        c._results += "\n==============================\n";
        c._results += _statsPersistedDb.header() + "\n";
        c._results += _statsPersistedDb.toString();
        c._results += "\n\nPersistedToFile :: MaxQSize: ";
        c._results += boost::lexical_cast<std::string>(_settings._storage_max_queue_size_file);
        c._results += " <--> QSize: ";
        c._results += boost::lexical_cast<std::string>(_threadPipeFile.size());
        c._results += "\n==============================\n";
        c._results += _statsPersistedFile.header() + "\n";
        c._results += _statsPersistedFile.toString();
        c._results += "\n\n";

        tw::common_strat::StrategyContainer::instance().sendToMsgBusConnection(command._connectionId, c.toString());

    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

bool ChannelOrStorage::getOpenOrders(std::vector<Order>& results) {
    try {
        Orders_GetAllOpen query;

        if ( !query.execute(_channelDb) )
            return false;
        
        results = query._o1;
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    
    return true;
}

bool ChannelOrStorage::getOpenOrdersForAccount(std::vector<Order>& results, const tw::risk::TAccountId& accId) {
    try {
        std::vector<Order> r;
        if ( !getOpenOrders(r) )
            return false;
        
        for ( std::vector<Order>::iterator iter = r.begin(); iter != r.end(); ++iter ) {
            if ( accId == (*iter)._accountId )
                results.push_back(*iter);
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

bool ChannelOrStorage::getFillsForDate(std::vector<Fill>& results, const std::string& date) {
    try {
        Fills_GetAllForDate query;

        if ( !query.execute(_channelDb, date) )
            return false;
        
        results = query._o1;
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    
    return true;
}

bool ChannelOrStorage::getFillsForAccountForDate(std::vector<Fill>& results, const tw::risk::TAccountId& accId, const std::string& date) {
    try {
        std::vector<Fill> r;
        if ( !getFillsForDate(r, date) )
            return false;
        
        for ( std::vector<Fill>::iterator iter = r.begin(); iter != r.end(); ++iter ) {
            if ( accId == (*iter)._accountId )
                results.push_back(*iter);
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

bool ChannelOrStorage::getPositions(std::vector<PosUpdate>& results) {
    try {
        Positions_GetAll query;

        if ( !query.execute(_channelDb) )
            return false;
        
        results = query._o1;
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    
    return true;
}

bool ChannelOrStorage::getPositionsForAccount(std::vector<PosUpdate>& results, const tw::risk::TAccountId& accId) {
    try {
        std::vector<PosUpdate> r;
       
        if ( !getPositions(r) )
            return false;
        
        for ( std::vector<PosUpdate>::iterator iter = r.begin(); iter != r.end(); ++iter ) {
            if ( accId == (*iter)._accountId )
                results.push_back(*iter);
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


bool ChannelOrStorage::getPositionsDropCopy(std::vector<PosUpdateDropCopy>& results) {
    try {
        PositionsDropCopy_GetAll query;

        if ( !query.execute(_channelDb) )
            return false;
        
        results = query._o1;
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    
    return true;
}  

bool ChannelOrStorage::getPositionsDropCopyForAccount(std::vector<PosUpdateDropCopy>& results, const std::string& acc) {
    try {
        std::vector<PosUpdateDropCopy> r;
       
        if ( !getPositionsDropCopy(r) )
            return false;
        
        for ( std::vector<PosUpdateDropCopy>::iterator iter = r.begin(); iter != r.end(); ++iter ) {
            if ( acc == (*iter)._accountName )
                results.push_back(*iter);
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

bool ChannelOrStorage::getMessagingForExchanges(std::vector<MessagingForExchange>& results) {
    try {
        MessagingForExchanges_GetAll query;

        if ( !query.execute(_channelDb) )
            return false;
        
        results = query._o1;
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    
    return true;
}

bool ChannelOrStorage::getFixSessionCMEStatesForSession(FixSessionCMEState& result) {
    try {
        FixSessionCMEStates_GetAll query;

        if ( !query.execute(_channelDb) )
            return false;
        
        std::vector<tw::channel_or::FixSessionCMEState>::iterator iter = query._o1.begin();
        std::vector<tw::channel_or::FixSessionCMEState>::iterator end = query._o1.end();
        for ( ; iter < end; ++iter ) {
            if ( result._senderCompID == iter->_senderCompID 
                && result._targetCompID == iter->_targetCompID 
                && result._week == iter->_week
                && result._year == iter->_year )
                result = *iter;
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

bool ChannelOrStorage::persist(const tw::common::Command& command) {
    try {
        if ( !_settings._storage_persist_commands )
            return true;
        
        TChannelOrStorageItemPtr item = getItem();
        item->set(command);
        
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

bool ChannelOrStorage::persist(const tw::channel_or::TOrderPtr& order) {
    try {
        TChannelOrStorageItemPtr item = getItem();
        item->set(*order);
        
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

bool ChannelOrStorage::persist(const tw::channel_or::TOrderPtr& order, const tw::channel_or::Reject& rej) {
    try {
        TChannelOrStorageItemPtr item = getItem();
        item->set(*order, rej);
        
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

bool ChannelOrStorage::persist(const tw::channel_or::Fill& fill) {
    try {
        TChannelOrStorageItemPtr item = getItem();
        item->set(fill);
        
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

bool ChannelOrStorage::persist(const tw::channel_or::FixSessionCMEState& state) {
    try {
        TChannelOrStorageItemPtr item = getItem();
        item->set(state);
        
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

bool ChannelOrStorage::persist(const tw::channel_or::FixSessionCMEMsg& msg) {
    try {
        TChannelOrStorageItemPtr item = getItem();
        item->set(msg);
        
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

bool ChannelOrStorage::persist(const tw::channel_or::FillDropCopy& fill) {
    try {
        TChannelOrStorageItemPtr item = getItem();
        item->set(fill);
        
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

bool ChannelOrStorage::persist(const tw::channel_or::PnLAuditTrailInfo& pnlAuditTrailInfo) {
    try {
        TChannelOrStorageItemPtr item = getItem();
        item->set(pnlAuditTrailInfo);
        
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

bool ChannelOrStorage::doPersist(const TChannelOrStorageItemPtr& item) {
    try {
        tw::common::THighResTime now = tw::common::THighResTime::now();
        
        if ( !isValid() ) {
            LOGGER_ERRO << "Can't persist item: " << item->toString() << "\n";
            return false;
        }
        
        switch( item->_type ) {
            case tw::common::eChannelOrStorageItemType::kCommand:
                ++_statsEntered._counterCommands;
                break;
            case tw::common::eChannelOrStorageItemType::kOrder:
            case tw::common::eChannelOrStorageItemType::kOrderAndRej:
                ++_statsEntered._counterOrders;
                const_cast<TChannelOrStorageItemPtr&>(item)->_order._timestamp4 = now;
                break;
            case tw::common::eChannelOrStorageItemType::kFill:
                ++_statsEntered._counterFills;
                const_cast<TChannelOrStorageItemPtr&>(item)->_fill._timestamp4 = now;
                if ( (item)->_fill._order )
                    ++_statsEntered._counterOrders;
                break;
            case tw::common::eChannelOrStorageItemType::kFixSessionCMEMsg:
                ++_statsEntered._counterFixSessionCMEMsgs;
                break;
            case tw::common::eChannelOrStorageItemType::kFixSessionCMEState:
                ++_statsEntered._counterFixSessionCMEStates;
                break;
            case tw::common::eChannelOrStorageItemType::kFillDropCopy:
                ++_statsEntered._counterFillsDropCopy;
                break;
            case tw::common::eChannelOrStorageItemType::kPnLAuditTrailInfo:
                ++_statsEntered._counterPnLAuditTrailInfos;
                break;
            default:
                LOGGER_ERRO << "Invalid item: "  << item->toString() << "\n" << "\n";
                return false;
        }
        
        if ( _isDoneDb ) {
            if ( canPersist() ) {
                _threadPipeFile.push(item);
                return true;
            }
            
            tw::channel_or::Alert alert;
            alert._type = tw::channel_or::eAlertType::kChannelDbMaxQueueSize;
            alert._text = "ChannelOrStorage_Db exceeded FILE storage's max queue size of: " + boost::lexical_cast<std::string>(_settings._storage_max_queue_size_file);
            alert._text += " ==> Can't persist to either db or file item: " + item->toString();
            tw::common_strat::ConsumerProxy::instance().onAlert(alert);
            
            return false;
        }
        
        if ( canPersistToDb() ) {
            _threadPipeDb.push(item);
            return true;
        }
        
        tw::channel_or::Alert alert;
        alert._type = tw::channel_or::eAlertType::kChannelDbMaxQueueSize;
        alert._text = "ChannelOrStorage_Db exceeded FILE storage's max queue size of: " + boost::lexical_cast<std::string>(_settings._storage_max_queue_size_file);
        tw::common_strat::ConsumerProxy::instance().onAlert(alert);
            
        _threadPipeFile.push(item);        
        startThreadPersistToFile("doPersist(): failed to persist to db");
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    
    return true;
}

bool ChannelOrStorage::flushToDb() {
    for ( size_t i = 0; i < 5; ++i ) {
        if ( doFlushToDb() )
            return true;
        
        if ( i < 4 ) {
            // Retry every 100 ms up to 5 times
            //
            tw::common_thread::sleep(100);
            LOGGER_WARN << "Failed to flushToDb() - sleeping for 100 ms for retry attempt: "  << i << "\n";
        }
    }
    
    return false;
}

bool ChannelOrStorage::doFlushToDb() {
    try {
        while ( !_itemsCache.empty() && !_stopPersistingToDb ) {
            clearQueries();
            
            // Lock for thread synchronization
            //
            tw::common_thread::LockGuard<TLock> lock(_dbLock);
            
            size_t count = std::min(static_cast<size_t>(_settings._storage_batch_cache_size), _itemsCache.size());
            TItemsCache::iterator first = _itemsCache.begin();
            TItemsCache::iterator last = first;
            for ( size_t i = 0; i < count; ++i, ++last ) {
                TChannelOrStorageItemPtr& item = _itemsCache[i];
                switch ( item->_type ) {
                    case tw::common::eChannelOrStorageItemType::kCommand:
                    {
                        tw::channel_or::CommandLog commandLog;
                        commandLog._type = item->_command._type;
                        commandLog._subType = item->_command._subType;
                        
                        if ( item->_command.has("ip") )
                             item->_command.get("ip", commandLog._ip); 
                        
                        if ( item->_command.has("user") )
                             item->_command.get("user", commandLog._user);
                        
                        commandLog._text = item->_command.toString();                                
                        _commandsLog_SaveCommandLog.add(commandLog);
                    }
                        break;
                    case tw::common::eChannelOrStorageItemType::kOrder:
                    case tw::common::eChannelOrStorageItemType::kOrderAndRej:
                        _orders_SaveOrderTransaction.add(item->_order, item->_rej);
                        _orders_SaveOrder.add(item->_order, item->_rej);
                        {
                            
                            tw::instr::InstrumentConstPtr instrument = item->_order._instrument;
                            if ( !instrument )
                                instrument = tw::instr::InstrumentManager::instance().getByKeyId(item->_order._instrumentId);
                            
                            if ( instrument )
                                addMessagingForExchanges(instrument->_symbol, instrument->_exchange, item->_order._accountId, item->_order);
                        }
                        break;
                    case tw::common::eChannelOrStorageItemType::kFill:
                        _fills_SaveFill.add(item->_fill);
                        {
                            tw::instr::InstrumentPtr instrument = tw::instr::InstrumentManager::instance().getByKeyId(item->_fill._instrumentId);

                            if ( instrument ) {
                                tw::channel_or::PosUpdate pos;                    
                                pos._accountId = item->_fill._accountId;
                                pos._strategyId = item->_fill._strategyId;
                                pos._displayName = instrument->_displayName;
                                pos._exchange = instrument->_exchange;
                                pos._avgPrice = item->_fill._avgPrice;
                                pos._pos = (item->_fill._side == tw::channel_or::eOrderSide::kBuy) ? item->_fill._qty.get() : item->_fill._qty.get() * (-1);

                                if ( tw::channel_or::eFillType::kBusted == item->_fill._type )
                                    pos._pos *= -1;

                                _positions_SavePosUpdate.add(pos);
                                
                                addMessagingForExchanges(instrument->_symbol, instrument->_exchange, item->_fill._accountId, item->_fill);
                            }
                        }

                        if ( item->_fill._order ) {
                            _orders_SaveOrderTransaction.add(item->_order, item->_rej);
                            _orders_SaveOrder.add(item->_order, item->_rej);
                        } 

                        break;
                    case tw::common::eChannelOrStorageItemType::kFixSessionCMEState:
                        _fixSessionCMEStates_SaveState.add(item->_stateCME);
                        break;
                    case tw::common::eChannelOrStorageItemType::kFixSessionCMEMsg:
                        _fixSessionCMEMsgs_SaveMsg.add(item->_msgCME);
                        break;
                    case tw::common::eChannelOrStorageItemType::kFillDropCopy:
                        _fillsDropCopy_SaveFill.add(item->_fillDropCopy);
                        {
                            tw::channel_or::PosUpdateDropCopy pos;                    
                            pos._accountName = item->_fillDropCopy._accountName;
                            pos._displayName = item->_fillDropCopy._displayName;
                            pos._exchange = item->_fillDropCopy._exchange;                            
                            pos._pos = (item->_fillDropCopy._side == tw::channel_or::eOrderSide::kBuy) ? item->_fillDropCopy._qty.get() : item->_fillDropCopy._qty.get() * (-1);
                            
                            if ( tw::channel_or::eFillType::kBusted == item->_fillDropCopy._type )
                                pos._pos *= -1;

                            _positionsDropCopy_SavePosUpdate.add(pos);
                        }
                        break;
                    case tw::common::eChannelOrStorageItemType::kPnLAuditTrailInfo:
                        _pnlAuditTrail_SavePnLAuditTrail.add(item->_pnlAuditTrailInfo);
                        break;
                    default:
                        LOGGER_ERRO << "Unknown item type "  << item->toString() << "\n" << "\n";
                        break;
                }
            }
            
            if ( _stopPersistingToDb )
                return true;
            
            _connection->setAutoCommit(false);
            
            if ( _commandsLog_SaveCommandLog._count > 0 && !_stopPersistingToDb ) {
                if ( !_commandsLog_SaveCommandLog.execute(_statement) ) {
                    _connection->rollback();
                    return false;
                }
            }
            
            if ( _orders_SaveOrderTransaction.count() > 0 && !_stopPersistingToDb ) {
                if ( !_orders_SaveOrderTransaction.execute(_statement) ) {
                     _connection->rollback();
                     return false;
                }
            }
            
            if ( _orders_SaveOrder.count() > 0 && !_stopPersistingToDb ) {
                if ( !_orders_SaveOrder.execute(_statement) ) {
                     _connection->rollback();
                     return false;
                }
            }
            
            if ( _fills_SaveFill.count() > 0 && !_stopPersistingToDb ) {
                if ( !_fills_SaveFill.execute(_statement) ) {
                     _connection->rollback();
                     return false;
                }
            }
            
            if ( _positions_SavePosUpdate.count() > 0 && !_stopPersistingToDb ) {
                if ( !_positions_SavePosUpdate.execute(_statement) ) {
                     _connection->rollback();
                     return false;
                }
            }
            
            if ( _fixSessionCMEStates_SaveState.count() > 0 && !_stopPersistingToDb ) {
                if ( !_fixSessionCMEStates_SaveState.execute(_statement) ) {
                     _connection->rollback();
                     return false;
                }
            }
            
            if ( _fixSessionCMEMsgs_SaveMsg.count() > 0 && !_stopPersistingToDb ) {
                if ( !_fixSessionCMEMsgs_SaveMsg.execute(_statement) ) {
                     _connection->rollback();
                     return false;
                }
            }
            
            if ( _fillsDropCopy_SaveFill.count() > 0 && !_stopPersistingToDb ) {
                if ( !_fillsDropCopy_SaveFill.execute(_statement) ) {
                    _connection->rollback();
                     return false;
                }
            }
            
            if ( _positionsDropCopy_SavePosUpdate.count() > 0 && !_stopPersistingToDb ) {
                if ( !_positionsDropCopy_SavePosUpdate.execute(_statement) ) {
                    _connection->rollback();
                     return false;
                }
            }
            
            if ( _messagingForExchanges_Save.count() > 0 && !_stopPersistingToDb ) {
                if ( !_messagingForExchanges_Save.execute(_statement) ) {
                    _connection->rollback();
                     return false;
                }
            }
            
            if ( _pnlAuditTrail_SavePnLAuditTrail.count() > 0 && !_stopPersistingToDb ) {
                if ( !_pnlAuditTrail_SavePnLAuditTrail.execute(_statement) ) {
                    _connection->rollback();
                     return false;
                }
            }
            
            if ( _stopPersistingToDb ) {
                _connection->rollback();
                return true;
            }
            
            _connection->commit();
            
            _statsPersistedDb._counterCommands += _commandsLog_SaveCommandLog.count();
            _statsPersistedDb._counterFills += _fills_SaveFill.count();
            _statsPersistedDb._counterOrders += _orders_SaveOrder.count();
            _statsPersistedDb._counterOrdersTransactions += _orders_SaveOrderTransaction.count();
            _statsPersistedDb._counterPosUpdates += _positions_SavePosUpdate.count();
            _statsPersistedDb._counterFixSessionCMEStates += _fixSessionCMEStates_SaveState.count();
            _statsPersistedDb._counterFixSessionCMEMsgs += _fixSessionCMEMsgs_SaveMsg.count();
            _statsPersistedDb._counterPnLAuditTrailInfos += _pnlAuditTrail_SavePnLAuditTrail.count();
            
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

void ChannelOrStorage::sendToMsgBus(const TChannelOrStorageItemPtr& item) {
    try {
        if ( item->_isSendToMsgBus )
            return;
                
        tw::common::Command cmnd;
        switch ( item->_type ) {
            case tw::common::eChannelOrStorageItemType::kOrder:
            case tw::common::eChannelOrStorageItemType::kOrderAndRej:
                cmnd = item->_order.toCommand();                
                cmnd._subType = item->_order._action;
                if ( tw::common::eChannelOrStorageItemType::kOrderAndRej == item->_type )
                    cmnd.addParams("rej", item->_rej.toString());
                break;
            case tw::common::eChannelOrStorageItemType::kFill:
                cmnd = item->_fill.toCommand();
                cmnd._subType = tw::common::eCommandSubType::kOrFill;
                break;
            default:
                return;
        }
        
        cmnd._type = tw::common::eCommandType::kChannelOr;
        tw::common_strat::StrategyContainer::instance().sendToAllMsgBusConnections(cmnd.toString()+"\n");
        item->_isSendToMsgBus = true;
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

bool ChannelOrStorage::persistToDb(const TChannelOrStorageItemPtr& item) {
    try {
        if ( !item )
            return false;
        
        sendToMsgBus(item);
        
        if ( !_connection || !_statement ) {
            LOGGER_ERRO << "_connection or _statement" << "\n" << "\n";
            return false;
        }
        
        if ( _itemsCache.size() > _settings._storage_max_batch_cache_size )
            return false;
        
        if ( tw::common::eChannelOrStorageItemType::kUnknown != item->_type ) {
            _itemsCache.push_back(item);
            _itemsCacheSize = _itemsCache.size();
        }
        
        if ( _itemsCache.size() < _settings._storage_batch_cache_size )
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

bool ChannelOrStorage::persistToFile(const TChannelOrStorageItemPtr& item) {
    try {
        if ( !item )
            return false;
        
        sendToMsgBus(item);
        
        if ( !_channelOrStorageFile.write(*item) ) 
            return false;
        
        switch ( item->_type ) {
            case tw::common::eChannelOrStorageItemType::kOrder:
            case tw::common::eChannelOrStorageItemType::kOrderAndRej:
                ++_statsPersistedFile._counterOrders;
                break;
            case tw::common::eChannelOrStorageItemType::kFill:
                ++_statsPersistedFile._counterFills;
                if ( item->_fill._order )
                    ++_statsPersistedFile._counterOrders;
                break;
            case tw::common::eChannelOrStorageItemType::kFixSessionCMEState:
                ++_statsPersistedFile._counterFixSessionCMEStates;
                break;
            case tw::common::eChannelOrStorageItemType::kFixSessionCMEMsg:
                ++_statsPersistedFile._counterFixSessionCMEMsgs;
                break;                
            case tw::common::eChannelOrStorageItemType::kCommand:
                ++_statsPersistedFile._counterCommands;
                break;
            case tw::common::eChannelOrStorageItemType::kFillDropCopy:
                ++_statsPersistedFile._counterFillsDropCopy;
                break;
            case tw::common::eChannelOrStorageItemType::kPnLAuditTrailInfo:
                ++_statsPersistedFile._counterPnLAuditTrailInfos;
                break;
            default:
                LOGGER_ERRO << "Unknown item type "  << item->toString() << "\n" << "\n";
                break;
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

void ChannelOrStorage::flushDbCache() {
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
        
        TChannelOrStorageItemPtr item;
        while ( _threadPipeDb.try_read(item) ) {
            if ( !persistToDb(item) || !flushToDb() ) {
                _threadPipeDb.push_front(item);
                LOGGER_ERRO << "Failed to flush to db item: "  << item->toString() << "\n";
                break;
            }
            
            LOGGER_INFO << "Flushed to db item: "  << item->toString() << "\n";
        }
        
        {
            TThreadPipe::TContainer items = _threadPipeDb.getAllItems();
            TThreadPipe::TContainer::iterator iter = items.begin();
            TThreadPipe::TContainer::iterator end = items.end();
            for ( ; iter != end; ++iter ) {
                _threadPipeFile.push_front(*iter);
            }
            
            _threadPipeDb.clear();
        }
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void ChannelOrStorage::flushFileCache() {
    try {
        TChannelOrStorageItemPtr item;
        while ( _threadPipeFile.try_read(item) ) {
            if ( !persistToFile(item) )
                LOGGER_ERRO << "Can't persist to file item: " << item->toString() << "\n";
        }
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void ChannelOrStorage::terminateThreadDb() {
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

void ChannelOrStorage::threadMainPersistToDb() {
    LOGGER_INFO << "Started" << "\n";
    
    try {
        TChannelOrStorageItemPtr item;
        while( !_isDoneDb ) {
            while ( _threadPipeDb.try_read(item, false) ) {
                if ( persistToDb(item) )
                   _threadPipeDb.pop();
                item.reset();
            }
            
            // Flush batch every 100 ms
            //
            tw::common_thread::sleep(100);
            if ( !flushToDb() )
                startThreadPersistToFile("flushDb() failed");
        }
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    _isDoneDbThread = true;
    LOGGER_INFO << "Finished" << "\n";
}

void ChannelOrStorage::threadMainPersistToFile() {
    LOGGER_INFO << "Started" << "\n";
    
    try {
        _stopPersistingToDb = true;
        terminateThreadDb();
        
        // Transfer all db items to file pipe
        //
        {
            TThreadPipe::TContainer items = _threadPipeDb.getAllItems();
            TThreadPipe::TContainer::reverse_iterator iter = items.rbegin();
            TThreadPipe::TContainer::reverse_iterator end = items.rend();
            for ( ; iter != end; ++iter ) {
                _threadPipeFile.push_front(*iter);
            }
            
            _threadPipeDb.clear();
        }
        
        {
            // Lock for thread synchronization
            //
            tw::common_thread::LockGuard<TLock> lock(_dbLock);
            
            TItemsCache::reverse_iterator iter = _itemsCache.rbegin();
            TItemsCache::reverse_iterator end = _itemsCache.rend();
            for ( ; iter != end; ++iter ) {
                _threadPipeFile.push_front(*iter);
            }
            _itemsCache.clear();
            _itemsCacheSize = 0;
        }
        
        while( !_isDoneFile ) {
            TChannelOrStorageItemPtr item;
            _threadPipeFile.read(item, false);
            while ( persistToFile(item) ) {
                _threadPipeFile.pop();
                if ( !_threadPipeFile.try_read(item, false) )
                    break;
            }
        }
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    LOGGER_INFO << "Finished" << "\n";
}

void ChannelOrStorage::startThreadPersistToFile(const std::string& reason) {
    try {
        {
            // Lock for thread synchronization
            //
            tw::common_thread::LockGuard<TLock> lock(_lock);
            if ( _threadFile.get() != NULL )
                return;
            
            _threadFile = TThreadPtr(new TThread(boost::bind(&ChannelOrStorage::threadMainPersistToFile, this)));
        }
        
        tw::channel_or::Alert alert;
        alert._type = tw::channel_or::eAlertType::kChannelDbMaxQueueSize;
        alert._text = reason + ":: ChannelOrStorage_Db - starting to persist to file instead of db";

        tw::common_strat::ConsumerProxy::instance().onAlert(alert);
        LOGGER_ERRO << "Alert: "  << alert.toString() << "\n" << "\n";
    } catch(const std::exception& e) {        
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

} // namespace channel_or
} // namespace tw
