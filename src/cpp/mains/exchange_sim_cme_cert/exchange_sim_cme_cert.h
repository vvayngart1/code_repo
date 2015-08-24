#pragma once

#include <tw/log/defs.h>
#include <tw/instr/instrument_manager.h>
#include <tw/channel_pf_cme/settings.h>
#include <tw/common_strat/strategy_container.h>
#include <tw/common/timer_server.h>

#include <string>
#include <map>
#include <set>

// OrderEntry class, which allows to send manual orders for testing/production
//
class ExchangeSimCmeCert : public tw::common_strat::IStrategy,
                           public tw::common::TimerClient {
public:
    ExchangeSimCmeCert();
    ~ExchangeSimCmeCert();
    
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
    
    // TimerClient interface
    //
    virtual bool onTimeout(const tw::common::TTimerId& id);
    
public:
    void onQuote(const tw::price::Quote& quote);
    
public:        
    void onNewAck(const tw::channel_or::TOrderPtr& order);
    void onNewRej(const tw::channel_or::TOrderPtr& order, const tw::channel_or::Reject& rej);
    void onModAck(const tw::channel_or::TOrderPtr& order);
    void onModRej(const tw::channel_or::TOrderPtr& order, const tw::channel_or::Reject& rej);
    void onCxlAck(const tw::channel_or::TOrderPtr& order);
    void onCxlRej(const tw::channel_or::TOrderPtr& order, const tw::channel_or::Reject& rej);
    void onFill(const tw::channel_or::Fill& fill);
    
    void onRebuildOrderRej(const tw::channel_or::TOrderPtr& order, const tw::channel_or::Reject& rej);
    void onAlert(const tw::channel_or::Alert& alert);
    
private:
    tw::channel_or::TOrderPtr createOrder(const tw::channel_or::eOrderSide side,
                                          const tw::instr::InstrumentPtr instrument,
                                          const tw::price::Size qty,
                                          const tw::price::Ticks price,
                                          const std::string& reason);
    
    bool needToCancel(const tw::channel_or::TOrderPtr& order, const tw::price::Quote& quote);
    
private:
    typedef std::map<tw::instr::Instrument::TKeyId, tw::price::Quote> TMarkets;
    
    tw::risk::Account _account;
    TMarkets _markets;
};
