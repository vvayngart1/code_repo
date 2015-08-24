#pragma once

#include <tw/exchange_sim/matcher.h>
#include <tw/channel_or_cme/settings.h>
#include <tw/common_comm/tcp_ip_server.h>
#include <tw/common_thread/thread.h>
#include <tw/common_thread/thread_pipe.h>
#include <tw/common_strat/consumer_proxy_impl_st.h>
#include <tw/channel_pf/channel.h>
#include <tw/channel_pf_cme/channel_pf_onix.h>
#include <tw/channel_pf_historical/channel_pf_historical.h>

#include <OnixS/FIXEngine.h>

#include <boost/shared_ptr.hpp>

#include <map>

namespace tw {
namespace exchange_sim {
    
class MatcherManagerCME : public MatcherEventListener,
                          public OnixS::FIX::ISessionListener,
                          public tw::common_comm::TcpIpServerCallback {
public:
    typedef tw::channel_or::Reject Reject;
    typedef boost::shared_ptr<OnixS::FIX::Session> TSessionPtr;    

public:
    MatcherManagerCME();
    ~MatcherManagerCME();
    
    void clear();
    
public:
    bool start(const tw::common::Settings& settings);
    void stop();
    
public:
    // MatcherEventListener interface
    //
    virtual void onNewAck(const TOrderPtr& order);    
    virtual void onModAck(const TOrderPtr& order);    
    virtual void onCxlAck(const TOrderPtr& order);
    virtual void onFill(const TFill& fill);
    virtual void onEvent(const Matcher* matcher);
    
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
    void sendAck(const OnixS::FIX::Message& msg, OnixS::FIX::Session* sn);
    void sendRej(const OnixS::FIX::Message& msg, OnixS::FIX::Session* sn, const std::string& reason);
    
    const TMatcherPtr& getMatcher(const std::string& displayName) const {
        static TMatcherPtr nullMatcher;
        
        TMatchers::const_iterator iter = _matchers.find(displayName);
        if ( iter != _matchers.end() )
            return iter->second;
        
        return nullMatcher;
    }
    
    const TSessionPtr& getSession(const std::string& senderCompId) const {
        static TSessionPtr nullSession;
        
        TSessions::const_iterator iter = _sessions.find(senderCompId);
        if ( iter != _sessions.end() )
            return iter->second;
        
        return nullSession;
    }
    
    void processSubs(const Matcher* item);
    void ThreadMain();
   
private:
    typedef tw::common_thread::Lock TLock;
    typedef std::map<std::string, TMatcherPtr> TMatchers;    
    typedef std::map<std::string, TSessionPtr> TSessions;
    
    typedef boost::shared_ptr<tw::channel_pf::Channel> TChannelPfPtr;
    typedef boost::shared_ptr<tw::channel_pf_cme::ChannelPfOnix> TChannelPfCmePtr;
    typedef boost::shared_ptr<tw::channel_pf_historical::ChannelPfHistorical> TChannelPfHistoricalPtr;
    
    typedef tw::common_comm::TcpIpServer TMsgBus;
    
    typedef std::vector<TConnection::pointer> TConnections;
    typedef std::map<std::string, TConnections> TConnectionsSubs;
    
    typedef std::map<TConnection::native_type, TConnection::pointer> TConnectionsSubsIds;
    
private:
    TLock _lock;
    bool _done;
    tw::common_thread::ThreadPtr _thread;
    tw::common_thread::ThreadPipe<const Matcher*> _threadPipe;
    
    TMatchers _matchers;
    TSessions _sessions;
    
    TConnectionsSubs _connectionsSubs;
    TConnectionsSubsIds _connectionsSubsIds;
    
    tw::channel_or::SimulatorMatcherParams _matcherParams;
    tw::common_strat::ConsumerProxyImplSt _proxy;
    TChannelPfCmePtr _channelPfCme;
    TChannelPfHistoricalPtr _channelPfHistorical;
    TChannelPfPtr _channelPf;
    TMsgBus _msgBus;
};

    
} // namespace channel_or_cme
} // namespace tw
