#include <tw/channel_or_cme_bridge/cme_route.h>
#include <tw/common_thread/utils.h>
#include <tw/common_strat/consumer_proxy.h>
#include <tw/channel_or_cme/id_factory.h>
#include <tw/generated/commands_common.h>

namespace tw {
namespace channel_or_cme_bridge {
    
CmeRoute::TSessionSchedulerPtr CmeRoute::_sessionScheduler;

class SessionSchedulerListener : public OnixS::FIX::Scheduling::SessionSchedulerListener {
public:
    virtual void onLoggingOut(const OnixS::FIX::Scheduling::SessionScheduler& scheduler,
                              OnixS::FIX::Session* session,
                              bool* allowLogout) {
        LOGGER_INFO << session->getSenderCompID() << "::" << session->getTargetCompID() << "\n";  
    }
    
    virtual void onWarning (const OnixS::FIX::Scheduling::SessionScheduler& scheduler,
                            OnixS::FIX::Session* session,
                            const std::string& reason) {
        LOGGER_WARN << session->getSenderCompID() << "::" << session->getTargetCompID()  << ": " << reason << "\n";
    }
    
    virtual void onError (const OnixS::FIX::Scheduling::SessionScheduler& scheduler,
                          OnixS::FIX::Session* session,
                          const std::string& reason) {
        LOGGER_ERRO << session->getSenderCompID() << "::" << session->getTargetCompID()  << ": " << reason << "\n";
    }
};

SessionSchedulerListener _gSessionSchedulerListener;
    
bool CmeRoute::global_init(const tw::channel_or_cme::Settings& settings) {
    bool status = true;
    try {
        // Start orderId factory
        //
        if ( !tw::channel_or_cme::IdFactory::instance().start() )
            return false;
        
        // Start global OnixS engine for all CME connections
        //
        EngineSettings engineSettings;
        engineSettings.dialect(settings._global_fixDialectDescriptionFile);
        engineSettings.licenseStore(settings._global_licenseStore);
        engineSettings.logDirectory(settings._global_logDirectory);
        engineSettings.listenPort(settings._global_exchange_sim_port);
        
        Engine::init(engineSettings);
        
        // Start global scheduler
        //
        OnixS::FIX::Scheduling::SessionSchedulerOptions schedulerOptions;
        schedulerOptions.configurationFile(settings._global_schedulerSettingsFile);
        schedulerOptions.eventListener(&_gSessionSchedulerListener);
        _sessionScheduler.reset(new OnixS::FIX::Scheduling::SessionScheduler(schedulerOptions));
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    return status;
}

void CmeRoute::global_shutdown() {
    try {
        // Stop global scheduler
        //
        _sessionScheduler.reset();
        
        // Stop global engine
        //
        Engine::shutdown();
        
        // Stop orderId factory
        //
        tw::channel_or_cme::IdFactory::instance().stop();
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    } 
}

CmeRoute::CmeRoute() {
    clear();
}

CmeRoute::~CmeRoute() {
    stop();
}
    
void CmeRoute::clear() {
    try {
        _receivedLogout = false;
        _nullSeqIdSessionInfo.first = -1;
        _nullSeqIdSessionInfo.second.first = NULL;
        _session.reset();
        _seqIdSessionInfos.clear();
        _settings.reset();
        _state = OnixS::FIX::Session::DISCONNECTED;
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

bool CmeRoute::init(const tw::channel_or_cme::TSessionSettingsPtr& settings) {
    bool status = true;
    try {
        if ( !_translator.init(settings) ) 
            return false;
        
        if ( settings->_storageType == tw::common::eChannelOrCMEStorageType::kCustom ) {
            if ( !_storage.init(settings) )
                return false;
            
            _session = TSessionPtr(new OnixS::FIX::Session(settings->_senderCompId,
                                                           settings->_targetCompId,
                                                           Translator::version(),
                                                           this,
                                                           OnixS::FIX::Session::PluggableStorage,
                                                           &_storage));
        } else {               
            _session = TSessionPtr(new OnixS::FIX::Session(settings->_senderCompId,
                                                           settings->_targetCompId,
                                                           Translator::version(),
                                                           this));
        }
        
        _session->setEncryptionMethod(OnixS::FIX::Session::NONE);
        _session->setTcpNoDelayOption();
        _session->setResendRequestMaximumRange(2500);
	_session->setSpecifyLastMsgSeqNumProcessedField(true);
        
        _session->senderSubID(settings->_senderSubId);
        _session->senderLocationID(settings->_senderLocationId);
        _session->targetSubID(settings->_targetSubId);
        
        _settings = settings;        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    return status;
}

bool CmeRoute::start() {
    bool status = true;
    try {
        if ( !isValid() )
            return false;
        
        tw::channel_or_cme::TSessionSettingsPtr s = _translator.getSettings();
        _sessionScheduler->add(_session.get(), 
                               *(_sessionScheduler->findSchedule("CME")),
                               OnixS::FIX::Scheduling::InitiatorConnectionSettings(s->_host,
                                                                                   s->_port,
                                                                                   s->_heartBeatInt,
                                                                                   false,
                                                                                   &(_translator.getMsgLogon())));
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    if ( !status )
        stop();
    
    return status;
}

void CmeRoute::stop() {
    try {
        if ( isValid() ) {
            _sessionScheduler->remove(_session.get());
            
            if ( isLoggedOn() ) {
                _receivedLogout = false;
                _session->logout("Session stopped");
                
                // Wait for up to 4 seconds to receive logout message
                //
                for ( uint32_t i = 0; i < 40 && !_receivedLogout; ++i ) {
                    tw::common_thread::sleep(100);
                }
                
                if ( _receivedLogout )
                    LOGGER_WARN << "Clean shutdown - received logout"  << "\n";
                else
                    LOGGER_ERRO << "Didn't receive logout for 4 secs"  << "\n";
            }
            
            _session->shutdown();
        }        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    clear();
}

bool CmeRoute::sendNew(const Message& source, IClient* session, std::string& error) {
    bool status = true;
    error.clear();
    try {
        if ( !isLoggedOn() ) {
            error = "ConnectionDown";
            return false;
        }
        
        TOrderCmeBridgeInfoPtr info(_pool.obtain(), _pool.getDeleter());
        Message msg(Values::MsgType::Order_Single, tw::channel_or_cme::Translator::version());
        {
            tw::common_thread::LockGuard<TLock> lock(_lock);

            if ( !_translator.translateNew(source, info) )  {
                error = "TranslationError";
                return false;
            }
            
            msg = _translator.getMsgNew();
            addSeqIdSessionInfo(source, msg, session, info);
        }
            
        _session->send(&msg);
        LOGGER_INFO << "CME_BRIDGE ==> source: " << source.toString(tw::channel_or_cme::FIX_FIELDS_DELIMITER) << " <--> dest: " << msg.toString(tw::channel_or_cme::FIX_FIELDS_DELIMITER) << "\n";
    } catch(const std::exception& e) {
        error = std::string("Exception: ") + e.what();
        LOGGER_ERRO << error << "\n" << "\n";        
        status = false;
    } catch(...) {
        error = "Exception: UNKNOWN";
        LOGGER_ERRO << error << "\n" << "\n";
        status = false;
    }
    
    return status;
}

bool CmeRoute::sendMod(const Message& source, IClient* session, std::string& error) {
    bool status = true;
    error.clear();
    try {
        if ( !isLoggedOn() ) {
            error = "ConnectionDown";
            return false;
        }
        
        Message msg(Values::MsgType::Order_Cancel_Replace_Request, tw::channel_or_cme::Translator::version());
        {
            tw::common_thread::LockGuard<TLock> lock(_lock);
            
            TSeqIdSessionInfo& sessionInfo = getSeqIdSessionInfo(source.get(Tags::OrigClOrdID));
            if ( sessionInfo.first == _nullSeqIdSessionInfo.first ) {
                error = std::string("NO seqIdSessionInfo for: ") + source.get(Tags::OrigClOrdID);
                return false;
            }
            
            if ( !_translator.translateMod(source, sessionInfo.second.second) )  {
                error = "TranslationError";
                return false;
            }
            
            msg = _translator.getMsgMod();
            addSeqIdSessionInfo(source, msg, sessionInfo.second.first, sessionInfo.second.second);
        }
            
        _session->send(&msg);
        LOGGER_INFO << "CME_BRIDGE ==> source: " << source.toString(tw::channel_or_cme::FIX_FIELDS_DELIMITER) << " <--> dest: " << msg.toString(tw::channel_or_cme::FIX_FIELDS_DELIMITER) << "\n";
    } catch(const std::exception& e) {
        error = std::string("Exception: ") + e.what();
        LOGGER_ERRO << error << "\n" << "\n";        
        status = false;
    } catch(...) {
        error = "Exception: UNKNOWN";
        LOGGER_ERRO << error << "\n" << "\n";
        status = false;
    }
    
    return status;
}

bool CmeRoute::sendCxl(const Message& source, IClient* session, std::string& error) {
    bool status = true;
    error.clear();
    try {
        if ( !isLoggedOn() ) {
            error = "ConnectionDown";
            return false;
        }
        
        Message msg(Values::MsgType::Order_Cancel_Request, tw::channel_or_cme::Translator::version());
        {
            tw::common_thread::LockGuard<TLock> lock(_lock);
            
            TSeqIdSessionInfo& sessionInfo = getSeqIdSessionInfo(source.get(Tags::OrigClOrdID));
            if ( sessionInfo.first == _nullSeqIdSessionInfo.first ) {
                error = std::string("NO seqIdSessionInfo for: ") + source.get(Tags::OrigClOrdID);
                return false;
            }

            if ( !_translator.translateCxl(source, sessionInfo.second.second) )  {
                error = "TranslationError";
                return false;
            }
            
            msg = _translator.getMsgCxl();
            addSeqIdSessionInfo(source, msg, sessionInfo.second.first, sessionInfo.second.second);
        }
            
        _session->send(&msg);
        LOGGER_INFO << "CME_BRIDGE ==> source: " << source.toString(tw::channel_or_cme::FIX_FIELDS_DELIMITER) << " <--> dest: " << msg.toString(tw::channel_or_cme::FIX_FIELDS_DELIMITER) << "\n";
    } catch(const std::exception& e) {
        error = std::string("Exception: ") + e.what();
        LOGGER_ERRO << error << "\n" << "\n";        
        status = false;
    } catch(...) {
        error = "Exception: UNKNOWN";
        LOGGER_ERRO << error << "\n" << "\n";
        status = false;
    }
    
    return status;
}

void CmeRoute::sendCxlAll(const IClient* const session) {
    try {
        if ( !isLoggedOn() )
            return;
        
        std::vector<TOrderCmeBridgeInfoPtr> orderCmeBridgeInfos;
        {
            tw::common_thread::LockGuard<TLock> lock(_lock);
            
            TSeqIdSessionInfos::iterator iter = _seqIdSessionInfos.begin();
            TSeqIdSessionInfos::iterator end = _seqIdSessionInfos.end();
            
            for ( ; iter != end; ++iter ) {
                if ( session == iter->second.second.first )
                    orderCmeBridgeInfos.push_back(iter->second.second.second);
            }            
        }
        
        if ( orderCmeBridgeInfos.empty() )
            return;
        
        std::string text = std::string("sendCxlAll for: ") + session->name() + std::string(" :: on: ") + _session->getTargetCompID();
        sendAlert(text);
        
        for ( size_t i = 0; i < orderCmeBridgeInfos.size(); ++i ) 
        {
            Message msg(Values::MsgType::Order_Cancel_Request, tw::channel_or_cme::Translator::version());
            {
                tw::common_thread::LockGuard<TLock> lock(_lock);
                
                if ( !_translator.translateCxl(orderCmeBridgeInfos[i]) ) {
                    LOGGER_ERRO << "Failed translateCxl() for: "  << orderCmeBridgeInfos[i]->toString() << "\n";
                    continue;
                }
                
                msg = _translator.getMsgCxl();
            }
            
            _session->send(&msg);
            LOGGER_INFO << "CME_BRIDGE ==> dest: " << msg.toString(tw::channel_or_cme::FIX_FIELDS_DELIMITER) << "\n";
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

// ISessionListener interface
//
void CmeRoute::onInboundApplicationMsg(const Message& source, OnixS::FIX::Session* sn) {
    try {
        tw::common::eCMEOrBridgeMsgType msgType = _translator.getInboundMessageType(source);
        tw::channel_or::eOrderRespType orderRespType = tw::channel_or::eOrderRespType::kUnknown;
        switch ( msgType ) {
            case tw::common::eCMEOrBridgeMsgType::kExRpt:
                orderRespType = _translator.getOrderRespTypeExecutionReport(source);
                break;
            case tw::common::eCMEOrBridgeMsgType::kOrderCxlRej:
                orderRespType = _translator.getOrderRespTypeCancelReject(source);
                break;
            default:
                LOGGER_ERRO << "Can't determine tw::common::eCMEOrBridgeMsgType for: " << source.toString(tw::channel_or_cme::FIX_FIELDS_DELIMITER) << "\n";
                return;
        }
        
        if ( tw::channel_or::eOrderRespType::kUnknown ) {
            LOGGER_ERRO << "Can't determine tw::channel_or::eOrderRespType for: " << source.toString(tw::channel_or_cme::FIX_FIELDS_DELIMITER) << "\n";
            return;
        }
        
        IClient* session = NULL;
        int32_t seqNum = 0;
        bool remove = false;
        {
            tw::common_thread::LockGuard<TLock> lock(_lock);
            
            TOrderCmeBridgeInfoPtr orderCmeBridgeInfoPtr;
            if ( !getSeqIdSessionInfoForMsg(source, session, orderCmeBridgeInfoPtr, seqNum) )
                return;
            
            switch (orderRespType ) {
                case tw::channel_or::eOrderRespType::kNewAck:
                case tw::channel_or::eOrderRespType::kPartFill:
                case tw::channel_or::eOrderRespType::kTradeBreak:
                case tw::channel_or::eOrderRespType::kModRej:
                case tw::channel_or::eOrderRespType::kCxlRej:
                case tw::channel_or::eOrderRespType::kModAck:
                    break;
                case tw::channel_or::eOrderRespType::kNewRej:
                case tw::channel_or::eOrderRespType::kFill:
                case tw::channel_or::eOrderRespType::kCxlAck:
                    remove = true;
                    removeOrderCmeBridgeInfo(orderCmeBridgeInfoPtr);
                    break;
                default:
                    break;
            }
            
            if ( !_translator.translateInboundApplicationMsg(source, orderCmeBridgeInfoPtr, msgType, orderRespType) ) {
                LOGGER_ERRO << "Failed to translateInboundApplicationMsg() for: " << source.toString(tw::channel_or_cme::FIX_FIELDS_DELIMITER) << "\n";
                return;
            }
        }
        
        Message& msg = (msgType == tw::common::eCMEOrBridgeMsgType::kExRpt) ? _translator.getMsgExRpt() : _translator.getMsgOrderCxlRej();
        session->onReply(msg, remove);
        LOGGER_INFO << "CME_BRIDGE <== source: " << source.toString(tw::channel_or_cme::FIX_FIELDS_DELIMITER) << " <--> dest: " << msg.toString(tw::channel_or_cme::FIX_FIELDS_DELIMITER) << "\n";
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void CmeRoute::onInboundSessionMsg(Message& source, OnixS::FIX::Session* sn) {
    try {
        tw::common::eCMEOrBridgeMsgType msgType = _translator.translateInboundSessionMsg(source);
        switch ( msgType ) {
            case tw::common::eCMEOrBridgeMsgType::kRej:
            case tw::common::eCMEOrBridgeMsgType::kBusRej:
            {
                if ( !source.contain(Tags::RefSeqNum) ) {
                    LOGGER_ERRO << "'RefSeqNum' is not set: "  << source.toString(tw::channel_or_cme::FIX_FIELDS_DELIMITER) << "\n";
                    return;
                }
                
                int32_t refSeqNum = source.getInteger(Tags::RefSeqNum);
                Message* rejMsg = _session->findSentMessage(refSeqNum);
                if ( !rejMsg ) {
                    LOGGER_ERRO << "Failed to find message for 'RefSeqNum': " << refSeqNum << " :: " << source.toString(tw::channel_or_cme::FIX_FIELDS_DELIMITER) << "\n";
                    return;
                }
                
                IClient* session = NULL;
                int32_t seqNum = 0;
                bool remove = false;
                {
                    tw::common_thread::LockGuard<TLock> lock(_lock);
                    
                    TOrderCmeBridgeInfoPtr orderCmeBridgeInfoPtr;
                    if ( !getSeqIdSessionInfoForMsg(*rejMsg, session, orderCmeBridgeInfoPtr, seqNum) )
                        return;
                    
                    // If reject for 'New' order, remove orderCmeBridgeInfoPtr map entries
                    //
                    if ( 'D' == rejMsg->getType()[0] ) {
                        remove = true;
                        removeOrderCmeBridgeInfo(orderCmeBridgeInfoPtr);
                    }
                }
                
                Message& msg = (msgType == tw::common::eCMEOrBridgeMsgType::kRej ? _translator.getMsgRej() : _translator.getMsgBusRej());
                msg.set(Tags::RefSeqNum, seqNum);
                session->onReply(msg, remove);
                LOGGER_INFO << "CME_BRIDGE <== source: " << source.toString(tw::channel_or_cme::FIX_FIELDS_DELIMITER) << " <--> dest: " << msg.toString(tw::channel_or_cme::FIX_FIELDS_DELIMITER) << "\n";
            }
                break;
            case tw::common::eCMEOrBridgeMsgType::kLogout:
                processLogout(source);
                break;
            case tw::common::eCMEOrBridgeMsgType::kHeartbeat:
                break;
            default:
                LOGGER_WARN << "Didn't determine need to send to client msg: " << source.toString(tw::channel_or_cme::FIX_FIELDS_DELIMITER) << "\n";
                return;
        }
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}
        
void CmeRoute::onStateChange(OnixS::FIX::Session::State newState, OnixS::FIX::Session::State prevState, OnixS::FIX::Session* sn) {
    try {
        _state = newState;
        
        tw::channel_or::Alert alert;
        switch ( newState ) {
            case OnixS::FIX::Session::ACTIVE:
                alert._type = tw::channel_or::eAlertType::kExchangeUp;
                break;
            case OnixS::FIX::Session::RECONNECTING:
            case OnixS::FIX::Session::DISCONNECTED:
                alert._type = tw::channel_or::eAlertType::kExchangeDown;
                break;
            default:
                alert._type = tw::channel_or::eAlertType::kExchangeStatus;
                break;
        }
        
        std::string currStateStr = OnixS::FIX::Session::state2string(newState);
        std::string prevStateStr = OnixS::FIX::Session::state2string(prevState);
        
        if ( tw::common::eChannelOrCMEStorageType::kCustom == _settings->_storageType )
            _storage.onStateChange(currStateStr, prevStateStr);
        
        alert._text = _settings->_senderCompId + " :: " + prevStateStr + std::string(" ==> ") + currStateStr;
        sendAlert(alert);
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

bool CmeRoute::onResendRequest (Message& message, OnixS::FIX::Session* session) {
    return _translator.shouldSend(message);
}

void CmeRoute::onError(OnixS::FIX::ISessionListener::ErrorReason reason, const std::string& description, OnixS::FIX::Session* sn) {
    try {
        tw::channel_or::Alert alert;
        
        alert._type = tw::channel_or::eAlertType::kExchangeStatus;
        alert._text = boost::lexical_cast<std::string>(reason);
        alert._text += " - ";
        alert._text += description;
        
        sendAlert(alert);
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void CmeRoute::onWarning(OnixS::FIX::ISessionListener::WarningReason reason, const std::string& description, OnixS::FIX::Session* sn) {
    try {
        tw::channel_or::Alert alert;
        
        alert._type = tw::channel_or::eAlertType::kExchangeStatus;
        alert._text = boost::lexical_cast<std::string>(reason) + description;
        
        sendAlert(alert);
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

// Utility functions
//
void CmeRoute::processLogout(const Message& msg) {
    int32_t nextExpectedMsgSeqNum = 0;
                           
    if ( msg.contain(OnixS::FIX::FIX44::Tags::NextExpectedMsgSeqNum) ) {    
        nextExpectedMsgSeqNum = msg.getInteger(OnixS::FIX::FIX44::Tags::NextExpectedMsgSeqNum);    
        _session->setOutSeqNum(nextExpectedMsgSeqNum);
        
        LOGGER_WARN << "Reset next outSegNum on logout to: " << nextExpectedMsgSeqNum << "\n";
    }
    
    tw::channel_or::Alert alert;
    alert._type = tw::channel_or::eAlertType::kExchangeDown;
    alert._text = msg.toString(tw::channel_or_cme::FIX_FIELDS_DELIMITER);
    
    sendAlert(alert);
    
    _receivedLogout = true;
}

void CmeRoute::sendAlert(const std::string& text) {
    tw::channel_or::Alert alert;
    alert._type = tw::channel_or::eAlertType::kExchangeStatus;
    alert._text = text;
    
    sendAlert(alert);
}

void CmeRoute::sendAlert(const tw::channel_or::Alert& alert) {
    tw::common_strat::ConsumerProxy::instance().onAlert(alert);
}

} // namespace channel_or_cme_bridge
} // namespace tw
