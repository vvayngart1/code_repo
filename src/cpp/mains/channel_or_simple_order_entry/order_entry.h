#pragma once

#include <tw/log/defs.h>
#include <tw/instr/instrument_manager.h>
#include <tw/channel_pf_cme/settings.h>
#include <tw/common_strat/strategy_container.h>

#include <string>
#include <map>
#include <set>

// OrderEntry class, which allows to send manual orders for testing/production
//
class OrderEntry : public tw::common_strat::IStrategy {
public:
    OrderEntry();
    ~OrderEntry();
    
    void clear();
    
public:
    // IStrategy interface
    //
    virtual bool init(const tw::common::Settings& settings, const tw::risk::Strategy& strategyParams);    
    virtual bool start();    
    virtual bool stop();
    
    virtual void recordExternalFill(const tw::channel_or::Fill& fill);
    virtual bool rebuildOrder(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej);
    virtual void rebuildPos(const tw::channel_or::PosUpdate& update);

    virtual void onConnectionUp(tw::common_strat::TMsgBusConnection::native_type id, tw::common_strat::TMsgBusConnection::pointer connection);
    virtual void onConnectionDown(tw::common_strat::TMsgBusConnection::native_type id);    
    virtual void onConnectionData(tw::common_strat::TMsgBusConnection::native_type id, const tw::common::Command& cmnd);
    
public:        
    void onNewAck(const tw::channel_or::TOrderPtr& order);
    void onNewRej(const tw::channel_or::TOrderPtr& order, const tw::channel_or::Reject& rej);
    void onModAck(const tw::channel_or::TOrderPtr& order);
    void onModRej(const tw::channel_or::TOrderPtr& order, const tw::channel_or::Reject& rej);
    void onCxlAck(const tw::channel_or::TOrderPtr& order);
    void onCxlRej(const tw::channel_or::TOrderPtr& order, const tw::channel_or::Reject& rej);
    void onFill(const tw::channel_or::Fill& fill);
    
    void onAlert(const tw::channel_or::Alert& alert);
    void onRebuildOrderRej(const tw::channel_or::TOrderPtr& order, const tw::channel_or::Reject& rej);
    
private:
    void sendAlert(const std::string& text);
    void sendToMsgBus(const tw::common::Command& cmnd);
    
    tw::channel_or::Reject getReject(const std::string& text) {
        tw::channel_or::Reject rej;
        
        rej._rejType = tw::channel_or::eRejectType::kInternal;
        rej._rejSubType = tw::channel_or::eRejectSubType::kTranslator;
        rej._text = text;
        
        return rej;
    }
};
