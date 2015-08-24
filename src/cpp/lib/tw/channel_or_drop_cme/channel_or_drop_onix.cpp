#include <tw/channel_or_drop_cme/channel_or_drop_onix.h>
#include <tw/channel_or/channel_or_storage.h>
#include <tw/common_strat/consumer_proxy.h>

#include <tw/generated/commands_common.h>

#include <set>

namespace tw {
namespace channel_or_drop_cme {
    
static const char FIX_FIELDS_DELIMITER = '|';    

ChannelOrDropOnix::ChannelOrDropOnix() {
    clear();
}

ChannelOrDropOnix::~ChannelOrDropOnix() {
    stop();
}
    
void ChannelOrDropOnix::clear() {
    try {        
        _sessions.clear();
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

bool ChannelOrDropOnix::getPositionsForAccounts(const std::set<std::string>& accounts) {
    // Get all open positions (if any)
    //
    for ( std::set<std::string>::iterator iter = accounts.begin(); iter != accounts.end(); ++iter ) {
        std::string account = *iter;
        std::vector<tw::channel_or::PosUpdateDropCopy> positions;
        if ( !tw::channel_or::ChannelOrStorage::instance().getPositionsDropCopyForAccount(positions, account) )
            return false;

        TSymbolPositions& symbolPositions = _positions[account];

        for ( size_t i = 0; i < positions.size(); ++i ) {
            symbolPositions[positions[i]._displayName] = positions[i];
        }
    }
    
    return true;
}

bool ChannelOrDropOnix::start(const tw::common::Settings& globalSettings) {
    bool status = true;
    try {
        // Check if settings are valid and at least one session is configured
        //
        if ( !_settings.parse(globalSettings._channel_or_drop_cme_dataSource) ) {
            LOGGER_ERRO << "Failed to parse channel_or_drop_cme settings" << "\n";
            return false;
        }
        
        if ( _settings._sessions.empty() ) {
            LOGGER_ERRO << "No sessions configured" << "\n";
            return false;
        }
        
        // Start dependencies
        //
        
        // ChannelOrStorage
        //
        if ( !tw::channel_or::ChannelOrStorage::instance().init(globalSettings) )
            return false;
        
        if ( !tw::channel_or::ChannelOrStorage::instance().start() )
            return false;
        
        // MsgBusServer (TCP/IP server)
        //
        if ( !_msgBusServer.init(globalSettings) )
            return false;
        
        if ( !_msgBusServer.start(this, true) )
            return false;
        
        std::set<std::string> accounts;
        if ( _settings._global_simulate ) {
            LOGGER_WARN << "running in SIMULATION mode" << "\n";
            
            accounts.insert(_settings._global_account);
            if ( !getPositionsForAccounts(accounts) )
                return false;

            if ( _settings._global_simSymbol.empty() ) {
                LOGGER_ERRO << "simSymbol is empty for simulation" << "\n";
                return false;
            }

            if ( _settings._global_simTimeout < 100 ) {
                LOGGER_ERRO << "simTimeout is less than 100 ms" << "\n";
                return false;
            }

            if ( _settings._global_simFillsBatch < 1 ) {
                LOGGER_ERRO << "simFillsBatch is less than 1" << "\n";
                return false;
            }

            if ( !tw::common_strat::ConsumerProxy::instance().registerCallbackTimer(&_proxy) )
                return false;

            if ( !tw::common::TimerServer::instance().init(globalSettings) )
                return false;

            if ( !tw::common::TimerServer::instance().start() )
                return false;

            tw::common::TTimerId id;
            if ( !tw::common::TimerServer::instance().registerClient(this, _settings._global_simTimeout, false, id) )
                return false;
        } else {
            // Determine accounts mappings
            //
            if ( _settings._global_accounts_mappings.empty() ) {
                LOGGER_ERRO << "No accounts mappings configured" << "\n";
                return false;
            }

            std::vector<std::string> values;
            boost::split(values, _settings._global_accounts_mappings, boost::algorithm::is_any_of(","));

            if ( values.empty() ) {
                LOGGER_ERRO << "No accounts mappings configured" << "\n";
                return false;
            }
            
            for ( size_t i = 0; i < values.size(); ++i ) {
                std::vector<std::string> pair;
                boost::split(pair, values[i], boost::algorithm::is_any_of("="));
                if ( pair.size() != 2 ) {
                    LOGGER_ERRO << "Corrupted accounts mappings configured: " << i << "::" << values[0] <<  _settings._global_accounts_mappings << "\n";
                    return false;
                }

                std::for_each(pair.begin(), pair.end(), boost::bind(boost::algorithm::trim<std::string>, _1, std::locale()));
                
                accounts.insert(pair[1]);
                _accountsMappings[pair[0]] = pair[1];

                LOGGER_INFO << "Added account mapping: " << pair[0] << "=" << pair[1] << "\n";
            }
            
            if ( !getPositionsForAccounts(accounts) )
                return false;
            
            // Global one time initialization
            //
            OnixS::CME::DropCopy::InitializationSettings initSettings;

            initSettings.logDirectory = _settings._global_logDirectory; 
            initSettings.licenseStore = _settings._global_licenseStore;
            initSettings.fixDialectDescriptionFile = _settings._global_fixDialectDescriptionFile;
            OnixS::CME::DropCopy::initialize(initSettings);

            tw::channel_or_drop_cme::Settings::TSessions::iterator iter = _settings._sessions.begin();
            tw::channel_or_drop_cme::Settings::TSessions::iterator end = _settings._sessions.end();
            for ( ; iter != end; ++iter ) {
                OnixS::CME::DropCopy::HandlerSettings handlerSettings;
                handlerSettings.logDirectory = _settings._global_logDirectory+"/"+(*iter).second->_senderCompId;
                handlerSettings.senderCompId = (*iter).second->_senderCompId;
                handlerSettings.senderSubId = (*iter).second->_senderSubId;
                handlerSettings.senderLocationId = (*iter).second->_senderLocationId;
                handlerSettings.targetCompId = (*iter).second->_targetCompId;
                handlerSettings.targetSubId = (*iter).second->_targetSubId;
                handlerSettings.logMode = OnixS::CME::DropCopy::LogModes::Debug;
                handlerSettings.advancedLogOptions = OnixS::CME::DropCopy::AdvancedLogOptions::LogEverything;

                TSessionPtr session;
                session.reset(new OnixS::CME::DropCopy::Handler(handlerSettings));

                // Register callbacks
                //
                session->registerErrorListener(this);
                session->registerWarningListener(this);
                session->registerHandlerStateChangeListener(this);
                session->registerFeedStateChangeListener(this);
                session->registerDropCopyServiceListener(this);

                LOGGER_INFO << "Logging on to: " 
                            << (*iter).second->_host << " :: "
                            << (*iter).second->_port << " :: "
                            << (*iter).second->_password << "\n";

                _sessions[(*iter).second->_senderCompId] = TSessionInfo((*iter).second, session);

                session->logon((*iter).second->_host, (*iter).second->_port, (*iter).second->_password);
                session->requestApplFeedIds();
            }
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    return status;
}

void ChannelOrDropOnix::stop() {
    try {
        TSessions::iterator iter = _sessions.begin();
        TSessions::iterator end = _sessions.end();
        
        for ( ; iter != end; ++iter ) {
            if ( iter->second.second )
                iter->second.second->logout();
        }
        
        if ( _settings._global_simulate )
            tw::common::TimerServer::instance().stop();
        
        _msgBusServer.stop();
        
        tw::channel_or::ChannelOrStorage::instance().stop();
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    clear();
}

bool ChannelOrDropOnix::onTimeout(const tw::common::TTimerId& id) {
    try {
        for ( uint32_t i = 0; i < _settings._global_simFillsBatch; ++i ) {
            tw::channel_or::FillDropCopy fill;

            fill._type = tw::channel_or::eFillType::kNormal;
            fill._subType = tw::channel_or::eFillSubType::kOutright;
            fill._exchange = tw::instr::eExchange::kCME;

            fill._accountName = _settings._global_account;
            fill._liqInd = tw::channel_or::eLiqInd::kRem;        
            fill._side = tw::channel_or::eOrderSide::kBuy;        
            fill._qty.set(1);
            fill._price = 1258;        
            fill._displayName = _settings._global_simSymbol;

            fill._origClOrderId = tw::common::TUuidBuffer(tw::common::generateUuid()).toString();
            fill._clOrderId = fill._origClOrderId;
            fill._exOrderId = fill._origClOrderId;

            fill._exFillId = fill._origClOrderId;
            fill._exFillRefId= fill._origClOrderId;

            fill._sourceSession = "SimSession";
            fill._exTimestamp = tw::common::THighResTime::now().toString();

            onFill(fill);
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

void ChannelOrDropOnix::sendPositionsToConnection(TMsgBusServerCallback::TConnection::native_type id) {
    tw::common_thread::LockGuard<TLock> lock(_lock);
    
    TPositions::iterator iter = _positions.begin();
    TPositions::iterator end = _positions.end();
    for ( ; iter != end; ++iter ) {
        TSymbolPositions& symbolPositions = iter->second;
        TSymbolPositions::iterator iterSymbol = symbolPositions.begin();
        TSymbolPositions::iterator endSymbol = symbolPositions.end();
        for ( ; iterSymbol != endSymbol; ++iterSymbol ) {
            tw::common::Command c = iterSymbol->second.toCommand();
            c._type = tw::common::eCommandType::kChannelOrDropCme;
            c._subType = tw::common::eCommandSubType::kPosUpdateDropCopy;
            _msgBusServer.sendToConnection(id, c.toString()+"\n");
            LOGGER_INFO << "Send status: " << c.toString() << "\n";
        }
    }
}

void ChannelOrDropOnix::onConnectionUp(TMsgBusServerCallback::TConnection::native_type id, TMsgBusServerCallback::TConnection::pointer connection) {
    sendPositionsToConnection(id);
}

void ChannelOrDropOnix::onConnectionDown(TMsgBusServerCallback::TConnection::native_type id) {
    
}

void ChannelOrDropOnix::onConnectionData(TMsgBusServerCallback::TConnection::native_type id, const std::string& message) {
    tw::common::Command cmnd;
    if ( !cmnd.fromString(message) ) {
        LOGGER_ERRO << "Not a valid command: " << message << "\n";
        return;
    }
    
    if ( cmnd._type != tw::common::eCommandType::kChannelOrDropCme )
        return;
    
    if ( cmnd._subType != tw::common::eCommandSubType::kList )
        return;
    
    LOGGER_INFO << "Processing command: " << message << "\n";
    sendPositionsToConnection(id);
}

void ChannelOrDropOnix::onError(const OnixS::CME::DropCopy::Error& error) {
    LOGGER_ERRO << error.toString() << "\n" << "\n";
}

void ChannelOrDropOnix::onWarning(const OnixS::CME::DropCopy::Warning& warning) {
    LOGGER_WARN << warning.toString() << "\n" << "\n";
}

void ChannelOrDropOnix::onHandlerStateChange(const OnixS::CME::DropCopy::HandlerStateChange& change) {
    LOGGER_INFO << "Handler state changed to " << OnixS::CME::DropCopy::HandlerStates::toString(change.newState()) << "\n";
}

void ChannelOrDropOnix::onFeedStateChange(const OnixS::CME::DropCopy::FeedStateChange& change) {
    LOGGER_INFO << "Feed state changed to " << OnixS::CME::DropCopy::FeedStates::toString(change.newState() ) << "." << "\n";
}

void ChannelOrDropOnix::onApplFeedIdReceived(const std::string& applFeedId) {
    LOGGER_INFO << "Application Feed Id received: '" << applFeedId << "'." << "\n";
}

void ChannelOrDropOnix::onApplLastSeqNumReceived(const std::string& applFeedId,
                                                 int lastSeqNum) {
    LOGGER_INFO << "Application last sequence number received: " 
                << lastSeqNum << " for feed '" << applFeedId << "'" << "\n";
}

void ChannelOrDropOnix::onDropCopyMessageReceived(const OnixS::CME::DropCopy::Message& msg) {
    LOGGER_INFO << "Drop Copy Message received: '" << msg.toString(FIX_FIELDS_DELIMITER) << "'" << "\n";
        
    try {
        tw::channel_or::FillDropCopy fill;
        const char execType = msg.get(OnixS::CME::DropCopy::Tags::ExecType)[0];
        switch ( execType ) {            
            case '1':   // Fill
            case '2':   // PartialFill
                fill._type = tw::channel_or::eFillType::kNormal;
                break;
            case 'H':   // TradeBreak
                fill._type = tw::channel_or::eFillType::kBusted;
                break;            
            default:
                LOGGER_ERRO << "Unsupported execType: "  << msg.toString(FIX_FIELDS_DELIMITER) << "\n";
                return;
        }
        
        // Apply filter
        //
        TSessions::iterator iter = _sessions.find(msg.getTargetCompID());
        if ( iter == _sessions.end() ) {
            LOGGER_ERRO << "Can't find session for: " << msg.getTargetCompID() << " :: " << msg.toString(FIX_FIELDS_DELIMITER) << "\n";
            return;
        }
        
        if ( !msg.contain(OnixS::CME::DropCopy::Tags::ApplFeedId) ) {
            LOGGER_ERRO << "Can't find ApplFeedId for: " << msg.toString(FIX_FIELDS_DELIMITER) << "\n";
            return;
        }
            
        if ( !iter->second.first->hasApplFeedId(msg.get(OnixS::CME::DropCopy::Tags::ApplFeedId)) ) {
            LOGGER_WARN << "Filtered out fill for ApplFeedId: " << msg.get(OnixS::CME::DropCopy::Tags::ApplFeedId) << "\n";
            return;
        }
        
        // Populate fill fields
        //
        fill._subType = tw::channel_or::eFillSubType::kOutright;
        fill._exchange = tw::instr::eExchange::kCME;        
        
        std::string account = _settings._global_account;
        if ( msg.contain(OnixS::CME::DropCopy::Tags::TargetSubID) ) {
            std::string appTag50 = msg.get(OnixS::CME::DropCopy::Tags::TargetSubID);
            TAccountsMappings::iterator iter = _accountsMappings.find(appTag50);
            if ( iter != _accountsMappings.end() )
                account = iter->second;
            else
                LOGGER_ERRO << "No entry in accounts mappings for: " << appTag50 << " -- defaulting to: " << account << "\n";
        }
        
        fill._accountName = account;        
        
        if ( msg.contain(OnixS::CME::DropCopy::Tags::AggressorIndicator) ) {
            switch ( msg.get(OnixS::CME::DropCopy::Tags::AggressorIndicator)[0]) {
                case 'Y':
                case 'y':
                    fill._liqInd = tw::channel_or::eLiqInd::kRem;
                    break;
                case 'N':
                case 'n':
                    fill._liqInd = tw::channel_or::eLiqInd::kAdd;
                    break;
                default:
                    fill._liqInd = tw::channel_or::eLiqInd::kUnknown;
                    break;
            }
        } else {
            fill._liqInd = tw::channel_or::eLiqInd::kUnknown;
        }
        
        if ( msg.contain(OnixS::CME::DropCopy::Tags::Side) ) {
            switch ( msg.get(OnixS::CME::DropCopy::Tags::Side)[0]) {
                case '1':
                case '3':
                    fill._side = tw::channel_or::eOrderSide::kBuy;
                    break;
                case '2':
                case '4':
                case '5':
                case '6':
                    fill._side = tw::channel_or::eOrderSide::kSell;
                    break;
                default:
                    fill._side = tw::channel_or::eOrderSide::kUnknown;
                    break;
            }
        } else {
            fill._side = tw::channel_or::eOrderSide::kUnknown;
        }
        
        if ( msg.contain(OnixS::CME::DropCopy::Tags::LastQty) )
            fill._qty.set(msg.getInteger(OnixS::CME::DropCopy::Tags::LastQty));
        
        if ( msg.contain(OnixS::CME::DropCopy::Tags::LastPx) )
            fill._price = msg.getDouble(OnixS::CME::DropCopy::Tags::LastPx);
        
        copyField(msg, fill._displayName, OnixS::CME::DropCopy::Tags::SecurityDesc);
        
        copyField(msg, fill._origClOrderId, OnixS::CME::DropCopy::Tags::OrigClOrdID);
        copyField(msg, fill._clOrderId, OnixS::CME::DropCopy::Tags::ClOrdID);
        copyField(msg, fill._exOrderId, OnixS::CME::DropCopy::Tags::OrderID);        
        
        copyField(msg, fill._exFillId, OnixS::CME::DropCopy::Tags::ExecID);
        copyField(msg, fill._exFillRefId, OnixS::CME::DropCopy::Tags::ExecRefID);
        
        copyField(msg, fill._sourceSession, OnixS::CME::DropCopy::Tags::ApplFeedId);
        
        if ( msg.contain(OnixS::CME::DropCopy::Tags::TransactTime) )
            fill._exTimestamp = tw::common::THighResTime::parse(msg.get(OnixS::CME::DropCopy::Tags::TransactTime)).toString();
        else if ( msg.contain(OnixS::CME::DropCopy::Tags::SendingTime) )
            fill._exTimestamp = tw::common::THighResTime::parse(msg.get(OnixS::CME::DropCopy::Tags::SendingTime)).toString();
        else
            fill._exTimestamp = tw::common::THighResTime::now().toString();
        
        onFill(fill);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
}

void ChannelOrDropOnix::onFill(tw::channel_or::FillDropCopy& fill) {
    fill._recvTimestamp = tw::common::THighResTime::now().toString();
    fill._date = fill._recvTimestamp.substr(0, 8);
    {
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<TLock> lock(_lock);
        TSymbolPositions& symbolPositions = _positions[fill._accountName];

        TSymbolPositions::iterator iter = symbolPositions.find(fill._displayName);
        if ( iter == symbolPositions.end() ) {
            iter = symbolPositions.insert(TSymbolPositions::value_type(fill._displayName, tw::channel_or::PosUpdateDropCopy())).first;
            iter->second._accountName = fill._accountName;
            iter->second._displayName = fill._displayName;
            iter->second._exchange = fill._exchange;
        }

        switch ( fill._type ) {
            case tw::channel_or::eFillType::kNormal:
                iter->second._pos += (tw::channel_or::eOrderSide::kBuy == fill._side) ? fill._qty.get() : (-1 * fill._qty.get());
                break;
            case tw::channel_or::eFillType::kBusted:
                iter->second._pos += (tw::channel_or::eOrderSide::kBuy == fill._side) ? (-1 * fill._qty.get()) : fill._qty.get();
                break;
            default:
                LOGGER_ERRO << "Unsupported fill type: "  << fill.toString() << "\n";
                return;
        }
        fill._posAccount.set(iter->second._pos);
        
        tw::common::Command c;
        c = fill.toCommand();
        c._type = tw::common::eCommandType::kChannelOrDropCme;
        c._subType = tw::common::eCommandSubType::kFillDropCopy;

        _msgBusServer.sendToAll(c.toString()+"\n");
    }
    
    tw::channel_or::ChannelOrStorage::instance().persist(fill);
}


} // namespace channel_or_cme
} // namespace tw
