#pragma once

#include <tw/common/command.h>
#include <tw/common/settings.h>
#include <tw/common_thread/locks.h>
#include <tw/common/timer_server.h>
#include <tw/common_strat/consumer_proxy_impl_st.h>
#include <tw/generated/channel_or_defs.h>
#include <tw/config/settings_cmnd_line.h>
#include <tw/channel_or_drop_cme/settings.h>
#include <tw/generated/channel_or_defs.h>

#include <OnixS/CME/DropCopy.h>

#include <boost/shared_ptr.hpp>

#include <map>

namespace tw {
namespace channel_or_drop_cme {
    
typedef tw::common_comm::TcpIpServerCallback TMsgBusServerCallback;
typedef tw::common_comm::TcpIpServer TMsgBusServer;
    
class ChannelOrDropOnix : public tw::common::TimerClient,
                          public TMsgBusServerCallback,
                          public OnixS::CME::DropCopy::ErrorListener,
                          public OnixS::CME::DropCopy::WarningListener,
                          public OnixS::CME::DropCopy::HandlerStateChangeListener,
                          public OnixS::CME::DropCopy::FeedStateChangeListener,
                          public OnixS::CME::DropCopy::DropCopyServiceListener {
public:
    typedef tw::channel_or::FillDropCopy TFill;
    typedef tw::channel_or::PosUpdateDropCopy TPosUpdate;
    typedef tw::channel_or::Alert TAlert;
    typedef std::map<std::string, std::string> TAccountsMappings;
    
    typedef std::map<std::string, tw::channel_or::PosUpdateDropCopy> TSymbolPositions;
    typedef std::map<std::string, TSymbolPositions> TPositions;
    
    typedef boost::shared_ptr<OnixS::CME::DropCopy::Handler> TSessionPtr;
    typedef std::pair<TSessionSettingsPtr, TSessionPtr> TSessionInfo;
    typedef std::map<std::string, TSessionInfo> TSessions;

public:
    ChannelOrDropOnix();
    ~ChannelOrDropOnix();
    
    void clear();
    
public:
    bool start(const tw::common::Settings& globalSettings);
    void stop();
    
public:
    // TimerClient interface
    //
    virtual bool onTimeout(const tw::common::TTimerId& id);
    
public:
    // TMsgBusServerCallback interface
    //
    virtual void onConnectionUp(TMsgBusServerCallback::TConnection::native_type id, TMsgBusServerCallback::TConnection::pointer connection);
    virtual void onConnectionDown(TMsgBusServerCallback::TConnection::native_type id);
    virtual void onConnectionData(TMsgBusServerCallback::TConnection::native_type id, const std::string& message);

public:
    // Callbacks from OnixS API
    //
    void onError(const OnixS::CME::DropCopy::Error& error);        
    void onWarning(const OnixS::CME::DropCopy::Warning& warning);
    void onHandlerStateChange(const OnixS::CME::DropCopy::HandlerStateChange& change);
    void onFeedStateChange(const OnixS::CME::DropCopy::FeedStateChange& change);
    void onApplFeedIdReceived(const std::string& applFeedId);
    void onApplLastSeqNumReceived(const std::string& applFeedId, int lastSeqNum);
    void onDropCopyMessageReceived(const OnixS::CME::DropCopy::Message& msg);
    
private:
    void copyField(const OnixS::CME::DropCopy::Message& source, std::string& dest, uint32_t tag) {
        if ( source.contain(tag) )
            dest = source.get(tag);
    }
    
    void onFill(tw::channel_or::FillDropCopy& fill);
    void sendPositionsToConnection(TMsgBusServerCallback::TConnection::native_type id);
    
private:
    bool getPositionsForAccounts(const std::set<std::string>& accounts);
    
private:
    typedef tw::common_thread::Lock TLock;
    
    TLock _lock;
    TPositions _positions;    
    TSessions _sessions;
    TMsgBusServer _msgBusServer;
    tw::channel_or_drop_cme::Settings _settings;
    TAccountsMappings _accountsMappings;
    
    tw::common_strat::ConsumerProxyImplSt _proxy;
};

    
} // namespace channel_or_cme
} // namespace tw
