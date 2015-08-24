#pragma once

#include <tw/common/command.h>
#include <tw/common/settings.h>
#include <tw/common_thread/locks.h>
#include <tw/config/settings_cmnd_line.h>
#include <tw/generated/channel_or_defs.h>
#include <tw/channel_or/orders_table.h>
#include <tw/channel_or_cme/translator.h>
#include <tw/channel_or_cme/channel_or_onix_storage.h>

#include <OnixS/FIXEngine.h>

#include <boost/shared_ptr.hpp>

namespace tw {
namespace channel_or_cme {
    
class ChannelOrOnix : public OnixS::FIX::ISessionListener {
public:
    typedef tw::channel_or::TOrderPtr TOrderPtr;
    typedef tw::channel_or::Reject Reject;
    typedef tw::channel_or::Fill Fill;
    typedef tw::channel_or::PosUpdate PosUpdate;
    typedef tw::channel_or::Alert Alert;    
    
    typedef boost::shared_ptr<OnixS::FIX::Session> TSessionPtr;

public:
    ChannelOrOnix();
    ~ChannelOrOnix();
    
    void clear();
    
public:
    static bool global_init(const tw::channel_or_cme::Settings& settings);
    static void global_shutdown();
    
public:
    bool init(const TSessionSettingsPtr& settings);
    bool start();    
    void stop();
    
    TSessionPtr getSession() const {
        return _session;
    }
    
public:
    // ProcessorOut interface
    //
    bool sendNew(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej);    
    bool sendMod(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej);    
    bool sendCxl(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej);    
    void onCommand(const tw::common::Command& command);
    bool rebuildOrder(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej);
    void recordFill(const tw::channel_or::Fill& fill);    
    void rebuildPos(const tw::channel_or::PosUpdate& update);
    
    void onNewRej(const TOrderPtr& order, const Reject& rej);
    void onModRej(const TOrderPtr& order, const Reject& rej);
    void onCxlRej(const TOrderPtr& order, const Reject& rej);
    
public:
    // ISessionListener interface
    //
    virtual void onInboundApplicationMsg(const OnixS::FIX::Message& msg, OnixS::FIX::Session* sn);
    virtual void onInboundSessionMsg(OnixS::FIX::Message& msg, OnixS::FIX::Session* sn);
    virtual void onStateChange(OnixS::FIX::Session::State newState, OnixS::FIX::Session::State prevState, OnixS::FIX::Session* sn);
    virtual bool onResendRequest (OnixS::FIX::Message& message, OnixS::FIX::Session* session);
    virtual void onError(OnixS::FIX::ISessionListener::ErrorReason reason, const std::string& description, OnixS::FIX::Session* sn);
    virtual void onWarning(OnixS::FIX::ISessionListener::WarningReason reason, const std::string& description, OnixS::FIX::Session* sn);
   
private:
    bool isValid() {
        if ( !_session )
            return false;
        
        return true;
    }
    
    bool isLoggedOn() {
        if ( !isValid() )
            return false;
        
        return ( OnixS::FIX::Session::ACTIVE == _session->getState() );
    }
    
private:    
    tw::channel_or::Reject getRej(tw::channel_or::eRejectReason reason) {
        tw::channel_or::Reject rej;
        
        rej._rejType = tw::channel_or::eRejectType::kInternal;
        rej._rejSubType = tw::channel_or::eRejectSubType::kConnection;
        rej._rejReason = reason;
        
        return rej;
    }
    
    void addOrder(const TOrderPtr& order) {
        tw::common_thread::LockGuard<TLock> lock(_lock);
        
        _orders.add(order->_clOrderId, order);
    }
    
    void addOrderForRebuild(const TOrderPtr& order) {
        tw::common_thread::LockGuard<TLock> lock(_lock);        
        
        _orders.add(order->_clOrderId, order);
        _orders.add(order->_origClOrderId, order);
    }
    
    void remOrder(const tw::channel_or::OrderResp& orderResp) {
        tw::common_thread::LockGuard<TLock> lock(_lock);
        
        _orders.rem(orderResp._clOrderId);
        _orders.rem(orderResp._origClOrderId);
    }
    
    void remOrder(const std::string& orderId) {
        tw::common_thread::LockGuard<TLock> lock(_lock);
        
        _orders.rem(orderId);
    }
    
    void getOrder(tw::channel_or::OrderResp& orderResp) {
        tw::common_thread::LockGuard<TLock> lock(_lock);
        
        orderResp._order = _orders.get(orderResp._clOrderId);
        if ( !orderResp._order )
            orderResp._order = _orders.get(orderResp._origClOrderId);
    }
    
private:    
    void processReject(const OnixS::FIX::Message& msg);
    void processLogout(const OnixS::FIX::Message& msg);
    
    void processOrderResp(tw::channel_or::OrderResp& orderResp);
    
private:
    typedef tw::common_thread::Lock TLock;
    typedef tw::channel_or::OrderTable<std::string> TOrderTable;
    typedef boost::shared_ptr<OnixS::FIX::Scheduling::SessionScheduler> TSessionSchedulerPtr;
    
    static TSessionSchedulerPtr _sessionScheduler;
    
    TLock _lock;
    bool _receivedLogout;
    TSessionPtr _session;
    Translator _translator;
    TOrderTable _orders;
    TSessionSettingsPtr _settings;
    ChannelOrOnixStorage _storage;
    OnixS::FIX::Session::State _state;
};

    
} // namespace channel_or_cme
} // namespace tw
