#pragma once

#include <tw/channel_or_cme_bridge/cme_route.h>

#include <tw/common_comm/tcp_ip_server.h>
#include <tw/common_thread/thread.h>
#include <tw/common_thread/thread_pipe.h>
#include <tw/common_strat/consumer_proxy_impl_st.h>

#include <OnixS/FIXEngine.h>

#include <boost/shared_ptr.hpp>

#include <map>
#include <tr1/unordered_map>

namespace tw {
namespace channel_or_cme_bridge {
    
typedef boost::shared_ptr<OnixS::FIX::Session> TSessionPtr;
typedef boost::shared_ptr<CmeRoute> TRouteSessionPtr;
typedef tw::common_thread::Lock TLock;

class RouteClientCME : public IClient {
public:
    RouteClientCME() {
        clear();
    }
    
    ~RouteClientCME() {
    }
    
    void clear() {
        _session.reset();
        _routeSessionInfos.clear();
    }
    
public:
    TSessionPtr& getSession() {
        return _session;
    }
    
    tw::channel_or_cme::ChannelOrOnixStorage& getStorage() {
        return _storage;
    }
    
public:
    bool start(const tw::channel_or_cme::TSessionSettingsPtr& settings, OnixS::FIX::ISessionListener* sessionListener) {
        bool status = true;
        try {
            if ( settings->_storageType == tw::common::eChannelOrCMEStorageType::kCustom ) {
                if ( !_storage.init(settings) )
                    return false;
                
                _session = TSessionPtr(new OnixS::FIX::Session(settings->_senderCompId,
                                                               settings->_targetCompId,
                                                               Translator::version(),
                                                               sessionListener,
                                                               OnixS::FIX::Session::PluggableStorage,
                                                               &_storage));
            } else {               
                _session = TSessionPtr(new OnixS::FIX::Session(settings->_senderCompId,
                                                               settings->_targetCompId,
                                                               Translator::version(),
                                                               sessionListener));
            }
            
            _session->setEncryptionMethod(OnixS::FIX::Session::NONE);
            _session->setTcpNoDelayOption();
            _session->setResendRequestMaximumRange(2500);
            _session->setSpecifyLastMsgSeqNumProcessedField(true);

            _session->logonAsAcceptor();

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
    
    void stop() {
        try {
            _session->logout();
            _session->shutdown();
        } catch(const std::exception& e) {
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        }
    }
    
    void onStateChange(OnixS::FIX::Session::State newState, OnixS::FIX::Session::State prevState) {
        try {
            if ( tw::common::eChannelOrCMEStorageType::kCustom == _settings->_storageType )
                _storage.onStateChange(OnixS::FIX::Session::state2string(newState), OnixS::FIX::Session::state2string(prevState));
        } catch(const std::exception& e) {
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        }
    }
    
public:
    // IClient
    //
    virtual void onReply(Message& msg, bool remove) {
        try {
            _session->send(&msg);
            if ( remove )
                remRouteSessionInfo(msg);
            else
                addRouteSessionInfo(msg);
        } catch(const std::exception& e) {
            LOGGER_ERRO << "Exception: " << e.what() << "\n" << "\n";        
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        }
    }
    
    virtual const std::string& name() const {
        static std::string unknown = "Unknown";
        if ( !_session )
            return unknown;
            
        const std::string& name = _session->getTargetCompID();
        return name;
    }
    
public:
    bool sendNew(const Message& msg, TRouteSessionPtr& routeSession, std::string& error) {
        bool status = true;
        try {
            addRouteSessionInfo(msg, routeSession);
            return routeSession->sendNew(msg, this, error);
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
    
    bool sendMod(const Message& msg, std::string& error) {
        bool status = true;
        try {
            TRouteSessionPtr routeSession = getRouteSessionInfo(msg);
            if ( !routeSession ) {            
                error = "Can't find routeSession";
                return false;
            }

            return routeSession->sendMod(msg, this, error);
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
    
    bool sendCxl(const Message& msg, std::string& error) {
        bool status = true;
        try {
            TRouteSessionPtr routeSession = getRouteSessionInfo(msg);
            if ( !routeSession ) {            
                error = "Can't find routeSession";
                return false;
            }

            return routeSession->sendCxl(msg, this, error);
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
    
    void sendCxlAll(TRouteSessionPtr& routeSession) {
        try {
            routeSession->sendCxlAll(this);
        } catch(const std::exception& e) {
            LOGGER_ERRO << "Exception: " << e.what() << "\n" << "\n";        
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        }
    }
    
private:
    void addRouteSessionInfo(const Message& msg, TRouteSessionPtr& routeSession) {
        tw::common_thread::LockGuard<TLock> lock(_lock);
                
        _routeSessionInfos[msg.get(Tags::ClOrdID)] = routeSession;
        LOGGER_INFO << "Added routeSession for: " << msg.get(Tags::ClOrdID) << "\n";
    }
    
    void addRouteSessionInfo(const Message& msg) {
        tw::common_thread::LockGuard<TLock> lock(_lock);
        
        if ( !msg.contain(Tags::OrderID) )
            return;
        
        TRouteSessionInfos::iterator iter = _routeSessionInfos.find(msg.get(Tags::OrderID));
        if ( iter != _routeSessionInfos.end() )
            return;
                
        iter = _routeSessionInfos.find(msg.get(Tags::ClOrdID));
        if ( iter == _routeSessionInfos.end() ) {
             LOGGER_ERRO << "Can't find routeSession for: " << msg.get(Tags::ClOrdID) << "\n";
             return;
        }
            
        _routeSessionInfos[msg.get(Tags::OrderID)] = iter->second;
        LOGGER_INFO << "Added routeSession for: " << msg.get(Tags::OrderID) << "\n";
    }
    
    TRouteSessionPtr getRouteSessionInfo(const Message& msg) {
        TRouteSessionPtr routeSession;
        
        tw::common_thread::LockGuard<TLock> lock(_lock);
        
        TRouteSessionInfos::iterator iter = msg.contain(Tags::OrderID) ? _routeSessionInfos.find(msg.get(Tags::OrderID)) : _routeSessionInfos.end();
        if ( iter != _routeSessionInfos.end() )
            routeSession = iter->second;
        
        return routeSession;
    }
    
    void remRouteSessionInfo(const Message& msg) {
        tw::common_thread::LockGuard<TLock> lock(_lock);
        
        if ( !msg.contain(tw::channel_or_cme::CustomTags::CorrelationClOrdID) )
            return;
        
        TRouteSessionInfos::iterator iter = _routeSessionInfos.find(msg.get(tw::channel_or_cme::CustomTags::CorrelationClOrdID));
        if ( iter != _routeSessionInfos.end() ) {
            _routeSessionInfos.erase(iter);
            LOGGER_INFO << "Removed routeSession for: " << msg.get(tw::channel_or_cme::CustomTags::CorrelationClOrdID) << " -- size: " << _routeSessionInfos.size() << "\n";
        } else {
            LOGGER_WARN << "Can't find routeSession for: " << msg.get(tw::channel_or_cme::CustomTags::CorrelationClOrdID) << "\n";
        }
        
        if ( !msg.contain(Tags::OrderID) )
            return;
        
        iter = _routeSessionInfos.find(msg.get(Tags::OrderID));
        if ( iter != _routeSessionInfos.end() ) {
            _routeSessionInfos.erase(iter);
            LOGGER_INFO << "Removed routeSession for: " << msg.get(Tags::OrderID) << " -- size: " << _routeSessionInfos.size() << "\n";
        } else {
            LOGGER_WARN << "Can't find routeSession for: " << msg.get(Tags::OrderID) << "\n";
        }
    }
    
private:
    typedef std::tr1::unordered_map<std::string, TRouteSessionPtr> TRouteSessionInfos; 
    
    TLock _lock;
    tw::channel_or_cme::TSessionSettingsPtr _settings;
    tw::channel_or_cme::ChannelOrOnixStorage _storage;
    TSessionPtr _session;
    TRouteSessionInfos _routeSessionInfos;
};
    
class RouteManagerCME : public OnixS::FIX::ISessionListener,
                        public tw::common_comm::TcpIpServerCallback {
public:
    typedef tw::channel_or::Reject Reject;
    typedef boost::shared_ptr<RouteClientCME> TRouteClientPtr;

public:
    RouteManagerCME();
    ~RouteManagerCME();
    
    void clear();
    
public:
    bool start(const tw::common::Settings& settings);
    void stop();
   
public:
    // ISessionListener interface
    //
    virtual void onInboundApplicationMsg(const OnixS::FIX::Message& msg, OnixS::FIX::Session* sn);
    virtual void onInboundSessionMsg(OnixS::FIX::Message& msg, OnixS::FIX::Session* sn);
    virtual void onStateChange(OnixS::FIX::Session::State newState, OnixS::FIX::Session::State prevState, OnixS::FIX::Session* sn);
    virtual bool onResendRequest (OnixS::FIX::Message& message, OnixS::FIX::Session* session);
    virtual void onError(OnixS::FIX::ISessionListener::ErrorReason reason, const std::string& description, OnixS::FIX::Session* sn);
    virtual void onWarning(OnixS::FIX::ISessionListener::WarningReason reason, const std::string& description, OnixS::FIX::Session* sn);
    
public:
    // TcpIpServerCallback interface
    //
    virtual void onConnectionUp(TConnection::native_type id, TConnection::pointer connection);
    virtual void onConnectionDown(TConnection::native_type id);
    virtual void onConnectionData(TConnection::native_type id, const std::string& message);
    
private:
    void sendRej(const OnixS::FIX::Message& msg, OnixS::FIX::Session* sn, const std::string& reason);
    
private:
    const TRouteClientPtr& getSession(const std::string& targetCompId) const {
        static TRouteClientPtr nullSession;
        
        TSessions::const_iterator iter = _sessions.find(targetCompId);
        if ( iter != _sessions.end() )
            return iter->second;
        
        return nullSession;
    }
    
    TRouteSessionPtr& getNextRouteSession() {
        static TRouteSessionPtr nullSession;
        
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<tw::common_thread::Lock> lock(_lock);
        
        if ( _routeSessions.empty() )
            return nullSession;
        
        if ( _lastRouteSessionUsed >= _routeSessions.size() )
            _lastRouteSessionUsed = 0;
            
        return _routeSessions[_lastRouteSessionUsed++];
    }
    
    void onAlert(const tw::channel_or::Alert& alert) {
        try {
            tw::common::Command cmnd;
            cmnd = alert.toCommand();
            cmnd._type = tw::common::eCommandType::kChannelOr;
            cmnd._subType = tw::common::eCommandSubType::kAlert;

            _msgBus.sendToAll(cmnd.toString()+"\n");
            LOGGER_WARN << "Send alert: "  << alert.toString() << "\n";
        } catch(const std::exception& e) {
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        }
    }
   
private:
    typedef std::map<std::string, TRouteClientPtr> TSessions;
    typedef std::vector<TRouteSessionPtr> TRouteSessions;
    
    typedef tw::common_comm::TcpIpServer TMsgBus;
    
    typedef std::vector<TConnection::pointer> TConnections;
    typedef std::map<std::string, TConnections> TConnectionsSubs;
    
    typedef std::map<TConnection::native_type, TConnection::pointer> TConnectionsSubsIds;
    
private:
    TLock _lock;
    
    TSessions _sessions;
    TRouteSessions _routeSessions;
    uint32_t _lastRouteSessionUsed;
    
    tw::common_strat::ConsumerProxyImplSt _proxy;
    TMsgBus _msgBus;
};

    
} // namespace channel_or_cme_bridge
} // namespace tw
