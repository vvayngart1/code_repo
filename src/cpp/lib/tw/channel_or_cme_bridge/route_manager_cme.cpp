#include <tw/channel_or_cme_bridge/route_manager_cme.h>
#include <tw/common_strat/consumer_proxy.h>
#include <tw/channel_or/channel_or_storage.h>

namespace tw {
namespace channel_or_cme_bridge {
    
using namespace OnixS::FIX::FIX42; 

RouteManagerCME::RouteManagerCME() {
    clear();
}

RouteManagerCME::~RouteManagerCME() {
    stop();
}

void RouteManagerCME::clear() {
    _sessions.clear();
    _routeSessions.clear();
    _lastRouteSessionUsed = 0;
    
    _routeSessions.clear();
}

bool RouteManagerCME::start(const tw::common::Settings& settings) {
    bool status = true;
    try {
        // Parse settings
        //
        tw::channel_or_cme::Settings cme_settings;
        if ( !cme_settings.parse(settings._channel_or_cme_dataSource) ) {
            LOGGER_ERRO << "failed to parse settings: " << settings.toString() << "\n";
            return false;
        }
        
        // Register single threaded proxy, since thread synchronization is done at routes level
        //
        if ( !tw::common_strat::ConsumerProxy::instance().registerCallbackQuote(&_proxy) ) {
            LOGGER_ERRO << "failed to register callback for quotes" << "\n";
            return false;
        }
        
        if ( !tw::common_strat::ConsumerProxy::instance().registerCallbackTimer(&_proxy) ) {
            LOGGER_ERRO << "failed to register callback for timeouts" << "\n";
            return false;
        }
        
        // Init channel or storage
        //
        if ( !tw::channel_or::ChannelOrStorage::instance().init(settings) ) {
            LOGGER_ERRO << "failed to init ChannelOrStorage" << "\n";
            return false;
        }
        
        // Start channel or storage
        //
        if ( !tw::channel_or::ChannelOrStorage::instance().start() ) {
            LOGGER_ERRO << "failed to stop ChannelOrStorage" << "\n";
            return false;
        }
        
        // Init msgBus
        //
        if ( !_msgBus.init(settings) ) {
            LOGGER_ERRO << "failed to start msgBus" << "\n";
            return false;
        }                        
    
        // Start global OnixS engine for all CME connections
        //
        if ( !CmeRoute::global_init(cme_settings) )
            return false;
        
        // Start all configured sessions
        //
        tw::channel_or_cme::Settings::TSessions::const_iterator iter = cme_settings._sessions.begin();
        tw::channel_or_cme::Settings::TSessions::const_iterator end = cme_settings._sessions.end();
        for ( ; iter != end; ++iter ) {
            tw::channel_or_cme::TSessionSettingsPtr s = iter->second;
            if ( s->_isAcceptor ) {
                TRouteClientPtr routeClient = TRouteClientPtr(new RouteClientCME());
                if ( !routeClient->start(s, this) )
                    return false;
                
                _sessions[routeClient->name()] = routeClient;
            } else {
                TRouteSessionPtr session = TRouteSessionPtr(new CmeRoute());
                if ( !session->init(s) || !session->start() )
                    return false;
                
                _routeSessions.push_back(session);
            }
        }
        
        // Start msgBus
        //
        if ( !_msgBus.start(this, true) ) {
            LOGGER_ERRO << "failed to start msgBus" << "\n";
            return false;
        }
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

void RouteManagerCME::stop() {
    try {
        // Disconnect all FIX sessions
        //
        
        // Disconnect all routes sessions
        //
        {
            TRouteSessions::iterator iter = _routeSessions.begin();
            TRouteSessions::iterator end = _routeSessions.begin();

            for ( ; iter != end; ++iter ) {
                (*iter)->stop();
            }
        }
        
        // Disconnect all client sessions
        //
        {
            TSessions::iterator iter = _sessions.begin();
            TSessions::iterator end = _sessions.begin();

            for ( ; iter != end; ++iter ) {
                iter->second->stop();
            }
        }
        
        // Stop global OnixS engine for all CME connections
        //
        CmeRoute::global_shutdown();
        
        // Stop msgBus
        //
        if ( !_msgBus.stop() )
            LOGGER_ERRO << "failed to stop msgBus" << "\n";
        
        // Stop channel or storage
        //
        tw::channel_or::ChannelOrStorage::instance().stop();
        
        clear();
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void RouteManagerCME::sendRej(const OnixS::FIX::Message& msg,
                              OnixS::FIX::Session* sn,
                              const std::string& reason) {
    try {
        OnixS::FIX::Message ack("3", OnixS::FIX::FIX_42);

        ack.set(Tags::LastMsgSeqNumProcessed, msg.getSeqNum());
        ack.set(Tags::RefSeqNum, msg.getSeqNum());
        if ( !reason.empty() )
            ack.set(Tags::Text, reason);

        sn->send(&ack);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void RouteManagerCME::onInboundApplicationMsg(const OnixS::FIX::Message& msg,
                                              OnixS::FIX::Session* sn) {
    try {
        std::string error;
        
        const TRouteClientPtr& routeClient = getSession(sn->getTargetCompID());
        if ( !routeClient ) {
            LOGGER_ERRO << "Can't find routeClient for: " << sn->getTargetCompID() << " :: " << msg.toString(tw::channel_or_cme::FIX_FIELDS_DELIMITER) << "\n";
            return;
        }
        
        char msgType = msg.getType()[0];
        switch ( msgType ) {
            case 'D':   // New
            {
                TRouteSessionPtr& routeSession = getNextRouteSession();
                if ( !routeSession ) {
                    error = "getNextRouteSession() returned NULL";
                    sendRej(msg, sn, error);
                    return;
                }
                
                if ( !routeClient->sendNew(msg, routeSession, error) )
                    sendRej(msg, sn, error);
            }
                break;
            case 'G':   // Mod            
                if ( !routeClient->sendMod(msg, error) )
                    sendRej(msg, sn, error);            
                break;
            case 'F':   // Cxl            
                if ( !routeClient->sendCxl(msg, error) )
                    sendRej(msg, sn, error);            
                break;
            default:
                return;
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }    
}

void RouteManagerCME::onInboundSessionMsg(OnixS::FIX::Message& msg,
                                          OnixS::FIX::Session* sn) {
    //LOGGER_INFO << "\nIncoming session-level message:\n" << msg << "\n";
}

void RouteManagerCME::onStateChange(OnixS::FIX::Session::State newState,
                                    OnixS::FIX::Session::State prevState,
                                    OnixS::FIX::Session* sn) {
    LOGGER_INFO << "\nSession's state is changed, prevState=" << OnixS::FIX::Session::state2string(prevState)
                << ", newState=" << OnixS::FIX::Session::state2string(newState) << "\n";
    
    const TRouteClientPtr& routeClient = getSession(sn->getTargetCompID());
    if ( !routeClient ) {
        LOGGER_ERRO << "Can't find routeClient for: " << sn->getTargetCompID() << "\n";
        return;
    }
    
    routeClient->onStateChange(newState, prevState);
    
    // Cancel all open orders for disconnected session
    //
    if ( OnixS::FIX::Session::DISCONNECTED == newState ) {
        TRouteSessions::iterator iter = _routeSessions.begin();
        TRouteSessions::iterator end = _routeSessions.end();
        for ( ; iter != end; ++iter ) {
            routeClient->sendCxlAll(*iter);
        }
    }
}

bool RouteManagerCME::onResendRequest(OnixS::FIX::Message& message,
                                      OnixS::FIX::Session* session) {
    return true;
}

void RouteManagerCME::onError(OnixS::FIX::ISessionListener::ErrorReason reason,
                              const std::string& description,
                              OnixS::FIX::Session* sn) {
    LOGGER_INFO << "\nSession-level error:" << description << "\n";    
}

void RouteManagerCME::onWarning(OnixS::FIX::ISessionListener::WarningReason reason,
                                const std::string& description,
                                OnixS::FIX::Session* sn) {
    LOGGER_INFO << "\nSession-level warning:" << description << "\n";    
}


void RouteManagerCME::onConnectionUp(TConnection::native_type id, TConnection::pointer connection) {
}

void RouteManagerCME::onConnectionDown(TConnection::native_type id) {
}

void RouteManagerCME::onConnectionData(TConnection::native_type id, const std::string& message) {
    try {
        tw::common::Command cmnd;
        if ( !cmnd.fromString(message) ) {
            LOGGER_ERRO << "Not a valid cmnd: " << message << "\n";
            return;
        }
        
        TRouteSessions::iterator iter = _routeSessions.begin();
        TRouteSessions::iterator end = _routeSessions.begin();
        for ( ; iter != end; ++iter ) {
            std::string s = (*iter)->onCommand(cmnd);
            if ( !s.empty() )
                _msgBus.sendToConnection(id, s+"\n");
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

} // namespace channel_or_cme_bridge
} // namespace tw
