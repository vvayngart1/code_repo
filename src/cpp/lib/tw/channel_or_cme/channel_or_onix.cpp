#include <tw/channel_or_cme/channel_or_onix.h>

#include <tw/common_strat/consumer_proxy.h>
#include <tw/common_strat/strategy_container.h>
#include <tw/common_thread/utils.h>
#include <tw/channel_or_cme/id_factory.h>
#include <tw/generated/channel_or_defs.h>
#include <tw/generated/commands_common.h>

namespace tw {
namespace channel_or_cme {
    
ChannelOrOnix::TSessionSchedulerPtr ChannelOrOnix::_sessionScheduler;

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
    
bool ChannelOrOnix::global_init(const tw::channel_or_cme::Settings& settings) {
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

void ChannelOrOnix::global_shutdown() {
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

ChannelOrOnix::ChannelOrOnix() {
    clear();
}

ChannelOrOnix::~ChannelOrOnix() {
    stop();
}
    
void ChannelOrOnix::clear() {
    try {
        _receivedLogout = false;
        _session.reset();
        _orders.clear();
        _settings.reset();
        _state = OnixS::FIX::Session::DISCONNECTED;
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

bool ChannelOrOnix::init(const TSessionSettingsPtr& settings) {
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

bool ChannelOrOnix::start() {
    bool status = true;
    try {
        if ( !isValid() )
            return false;
        
        TSessionSettingsPtr s = _translator.getSettings();
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

void ChannelOrOnix::stop() {
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

// ProcessorOut interface
//
bool ChannelOrOnix::sendNew(const TOrderPtr& order, Reject& rej) {
    bool status = true;
    try {
        // Translate first new orders, so that exSessionName is added to order
        //
        if ( !_translator.translateNew(order, rej) )
            return false;
        
        if ( !isLoggedOn() ) {
            rej = getRej(tw::channel_or::eRejectReason::kConnectionDown);
            return false;
        }
        
        addOrder(order);        
        {
            // Mark order timers
            //
            tw::common::THighResTimeScope orderTimer(order->_timestamp2, order->_timestamp3);
            
            _session->send(&(_translator.getMsgNew()));
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

bool ChannelOrOnix::sendMod(const TOrderPtr& order, Reject& rej) {
    bool status = true;
    try {
        if ( !isLoggedOn() ) {
            rej = getRej(tw::channel_or::eRejectReason::kConnectionDown);
            return false;
        }
        
        if ( !_translator.translateMod(order, rej) )
            return false;
        
        addOrder(order);
        {
            // Mark order timers
            //
            tw::common::THighResTimeScope orderTimer(order->_timestamp2, order->_timestamp3);
            
            _session->send(&(_translator.getMsgMod()));
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

bool ChannelOrOnix::sendCxl(const TOrderPtr& order, Reject& rej) {
    bool status = true;
    try {
        if ( !isLoggedOn() ) {
            rej = getRej(tw::channel_or::eRejectReason::kConnectionDown);
            return false;
        }
        
        if ( order->_cancelOnAck ) {
            LOGGER_INFO << "delaying cancel until ack for orderId: "  << order->_orderId.toString() << "\n";
            return true;
        }
        
        if ( !_translator.translateCxl(order, rej) )
            return false;        
        
        addOrder(order);
        {
            // Mark order timers
            //
            tw::common::THighResTimeScope orderTimer(order->_timestamp2, order->_timestamp3);

            _session->send(&(_translator.getMsgCxl()));
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

void ChannelOrOnix::onCommand(const tw::common::Command& command) {
    if ( command._type != tw::common::eCommandType::kChannelOrCme )
        return;
    
    tw::common_commands::ChannelOrCme data;
    switch (command._subType) {
        case tw::common::eCommandSubType::kList:
            data._results = _orders.toString();
            break;
        case tw::common::eCommandSubType::kStatus:
            data._results = OnixS::FIX::Session::state2string(_state);
            break;
        default:
            return;
    }
    
    if ( _settings )
        data._exSessionName = _settings->_senderCompId;
    
    tw::common::Command c = data.toCommand();
    c._connectionId = command._connectionId;
    c._type = command._type;
    c._subType = command._subType;
    tw::common_strat::StrategyContainer::instance().sendToMsgBusConnection(c._connectionId, c.toString()+"\n");
}

bool ChannelOrOnix::rebuildOrder(const TOrderPtr& order, Reject& rej) {
    bool status = true;
    try {
        switch (order->_state) {
            case tw::channel_or::eOrderState::kPending:
            case tw::channel_or::eOrderState::kWorking:
                addOrder(order);
                break;
            case tw::channel_or::eOrderState::kModifying:
            case tw::channel_or::eOrderState::kCancelling:
                addOrderForRebuild(order);
                break;
            default:
                rej = getRej(tw::channel_or::eRejectReason::kInvalidOrderState);
                return false;
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

void ChannelOrOnix::recordFill(const Fill& fill) {
    // Not implemented - nothing to do
    //
}

void ChannelOrOnix::rebuildPos(const PosUpdate& update) {
    // Not implemented - nothing to do
    //
}

void ChannelOrOnix::onNewRej(const TOrderPtr& order, const Reject& rej) {
    // Not implemented - nothing to do
    //
}

void ChannelOrOnix::onModRej(const TOrderPtr& order, const Reject& rej) {
    // Not implemented - nothing to do
    //
}

void ChannelOrOnix::onCxlRej(const TOrderPtr& order, const Reject& rej) {
    // Not implemented - nothing to do
    //
}

// Processing of inbound messages
//
void ChannelOrOnix::processReject(const OnixS::FIX::Message& msg) {
   int32_t refSeqNum = 0;
    if ( !msg.contain(Tags::RefSeqNum) ) {
        LOGGER_ERRO << "'RefSeqNum' is not set: "  << msg.toString(FIX_FIELDS_DELIMITER) << "\n";
        return;
    }
    
    refSeqNum = msg.getInteger(Tags::RefSeqNum);
    OnixS::FIX::Message* rejMsg = _session->findSentMessage(refSeqNum);
    if ( !rejMsg ) {
        LOGGER_ERRO << "Failed to find message for 'RefSeqNum': " << refSeqNum << " :: " << msg.toString(FIX_FIELDS_DELIMITER) << "\n";
        return;
    }
    
    TMessagePtr rejAckMsg;
    if ( !_translator.translateMsgRej(msg, *rejMsg, rejAckMsg) ) {
        LOGGER_ERRO << "Failed to translate rejAckMsg message for: " << msg.toString(FIX_FIELDS_DELIMITER) << " :: " << rejMsg->toString(FIX_FIELDS_DELIMITER) << "\n";
        return;
    }
    
    onInboundApplicationMsg(*rejAckMsg, _session.get());
}

void ChannelOrOnix::processLogout(const OnixS::FIX::Message& msg) {
    int32_t nextExpectedMsgSeqNum = 0;
                           
    if ( msg.contain(OnixS::FIX::FIX44::Tags::NextExpectedMsgSeqNum) ) {    
        nextExpectedMsgSeqNum = msg.getInteger(OnixS::FIX::FIX44::Tags::NextExpectedMsgSeqNum);    
        _session->setOutSeqNum(nextExpectedMsgSeqNum);
        
        LOGGER_WARN << "Reset next outSegNum on logout to: " << nextExpectedMsgSeqNum << "\n";
    }
    
    tw::channel_or::Alert alert;
    alert._type = tw::channel_or::eAlertType::kExchangeDown;
    alert._text = msg.toString(FIX_FIELDS_DELIMITER);
    
    LOGGER_WARN << alert._text << "\n";
    
    tw::common_strat::ConsumerProxy::instance().onAlert(alert);
    
    _receivedLogout = true;
}

void ChannelOrOnix::processOrderResp(tw::channel_or::OrderResp& orderResp) {
    getOrder(orderResp);
    switch ( orderResp._type ) {
        case tw::channel_or::eOrderRespType::kNewAck:
            break;
        case tw::channel_or::eOrderRespType::kNewRej:
        case tw::channel_or::eOrderRespType::kModRej:
        case tw::channel_or::eOrderRespType::kCxlRej:
            if ( orderResp._clOrderId != orderResp._origClOrderId)
                remOrder(orderResp._clOrderId);
            break;
        case tw::channel_or::eOrderRespType::kModAck:
            if ( orderResp._clOrderId != orderResp._origClOrderId)
                remOrder(orderResp._origClOrderId);
            if ( orderResp._order && orderResp._order->_origClOrderId != orderResp._clOrderId )
                remOrder(orderResp._order->_origClOrderId);
            break;        
        case tw::channel_or::eOrderRespType::kCxlAck:
            remOrder(orderResp);
            break;
        case tw::channel_or::eOrderRespType::kPartFill:
            // TODO: need to determine if any processing required
            //
            break;
        case tw::channel_or::eOrderRespType::kFill:
            // TODO: need to accommodate for spreads
            //            
            remOrder(orderResp);
            break;
        case tw::channel_or::eOrderRespType::kTradeBreak:           
            // TODO: need to determine if any processing required
            //
            break;
        default:
            // TODO: need to determine if any processing required
            //
            break;
    }
}

// ISessionListener interface
//
void ChannelOrOnix::onInboundApplicationMsg(const OnixS::FIX::Message& msg, OnixS::FIX::Session* sn) {
    try {
        tw::channel_or::OrderResp orderResp;
        orderResp._timestamp1.setToNow();
        
        bool status = true;
        
        char msgType = msg.getType()[0];
        switch ( msgType ) {
            case '8':
                status = _translator.translateExecutionReport(msg, orderResp);
                break;
            case '9':
                status = _translator.translateOrderCancelReject(msg, orderResp);
                break;
            default:
                return;
        }
        
        if ( !status ) {
            tw::channel_or::Alert alert;
            alert._type = tw::channel_or::eAlertType::kErrorTranslatingOrderResp;
            alert._text = msg.toString(FIX_FIELDS_DELIMITER);
            tw::common_strat::ConsumerProxy::instance().onAlert(alert);
            return;
        }
        
        processOrderResp(orderResp);
        tw::common_strat::ConsumerProxy::instance().onOrderResp(orderResp);
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void ChannelOrOnix::onInboundSessionMsg(OnixS::FIX::Message& msg, OnixS::FIX::Session* sn) {
    try {
        char msgType = msg.getType()[0];
        switch ( msgType ) {
            case '3':
            case 'j':
                processReject(msg);
                break;
            case '5':
                processLogout(msg);
                break;
        }
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}
        
void ChannelOrOnix::onStateChange(OnixS::FIX::Session::State newState, OnixS::FIX::Session::State prevState, OnixS::FIX::Session* sn) {
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
        tw::common_strat::ConsumerProxy::instance().onAlert(alert);
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

bool ChannelOrOnix::onResendRequest (OnixS::FIX::Message& message, OnixS::FIX::Session* session) {
    return _translator.shouldSend(message);
}

void ChannelOrOnix::onError(OnixS::FIX::ISessionListener::ErrorReason reason, const std::string& description, OnixS::FIX::Session* sn) {
    try {
        tw::channel_or::Alert alert;
        
        alert._type = tw::channel_or::eAlertType::kExchangeStatus;
        alert._text = boost::lexical_cast<std::string>(reason);
        alert._text += " - ";
        alert._text += description;
        
        tw::common_strat::ConsumerProxy::instance().onAlert(alert);
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void ChannelOrOnix::onWarning(OnixS::FIX::ISessionListener::WarningReason reason, const std::string& description, OnixS::FIX::Session* sn) {
    try {
        tw::channel_or::Alert alert;
        
        alert._type = tw::channel_or::eAlertType::kExchangeStatus;
        alert._text = boost::lexical_cast<std::string>(reason) + description;
        
        tw::common_strat::ConsumerProxy::instance().onAlert(alert);
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

} // namespace channel_or_cme
} // namespace tw
