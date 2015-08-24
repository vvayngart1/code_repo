#pragma once

#include <tw/common/command.h>
#include <tw/common/pool.h>
#include <tw/common/singleton.h>
#include <tw/generated/channel_or_defs.h>
#include <tw/channel_or/uuid_factory.h>
#include <tw/channel_or/orders_table.h>
#include <tw/common/settings.h>
#include <tw/common/timer_server.h>

#include <map>

namespace tw {
namespace channel_or {
    
class ProcessorOrders : public tw::common::Singleton<ProcessorOrders>,
                        public tw::common::TimerClient {
public:
    typedef std::map<tw::instr::Instrument::TKeyId, AccountInstrOpenOrdersPos> TInstrOpenOrdersPos;
    typedef std::map<tw::channel_or::TAccountId, TInstrOpenOrdersPos> TAccountInstrOpenOrdersPos;
    
    typedef std::map<tw::instr::Instrument::TKeyId, StrategyInstrOpenOrdersPos> TInstrStratOpenOrdersPos;
    typedef std::map<tw::channel_or::TStrategyId, TInstrStratOpenOrdersPos> TStrategyInstrOpenOrdersPos;
    
public:
    ProcessorOrders();
    ~ProcessorOrders();
    
    void clear();

public:
    // Factory method for creating orders - returns shared_ptr
    // with custom deleter, which returns it back to the pool
    //
    TOrderPtr createOrder(bool createOrderId = true);
    
public:
    TOrderPtr get(const TOrderId& id);
    TOrders getAll() const;
    TOrders getAllForAccount(const TAccountId& x) const;
    TOrders getAllForAccountStrategy(const TAccountId& x, const TStrategyId& y) const;
    TOrders getAllForAccountStrategyInstr(const TAccountId& x, const TStrategyId& y, const tw::instr::Instrument::TKeyId& z) const;
    TOrders getAllForInstrument(const tw::instr::Instrument::TKeyId& x) const;
    
public:
    const TInstrOpenOrdersPos& getAllInstrOpenOrdersPosForAccount(const TAccountId& x);
    const AccountInstrOpenOrdersPos& getInstrOpenOrdersPosForAccountInstr(const TAccountId& x, const tw::instr::Instrument::TKeyId& y);
    
    const TInstrStratOpenOrdersPos& getAllInstrOpenOrdersPosForStrategy(const TStrategyId& x);
    const StrategyInstrOpenOrdersPos& getInstrOpenOrdersPosForStrategyInstr(const TStrategyId& x, const tw::instr::Instrument::TKeyId& y);
    
public:
    bool isOrderLive(const TOrderPtr& order);
    
public:
    // ProcessorOut interface
    //
    bool init(const tw::common::Settings& settings);
    bool start();
    void stop();
    
    bool sendNew(const TOrderPtr& order, Reject& rej);    
    bool sendMod(const TOrderPtr& order, Reject& rej);    
    bool sendCxl(const TOrderPtr& order, Reject& rej);    
    void onCommand(const tw::common::Command& command);
    bool rebuildOrder(const TOrderPtr& order, Reject& rej);
    void recordFill(const Fill& fill);    
    void rebuildPos(const PosUpdate& update);
    
public:
    // ProcessorIn interface
    //
    void onNewAck(const TOrderPtr& order);    
    void onNewRej(const TOrderPtr& order, const Reject& rej);
    void onModAck(const TOrderPtr& order);
    void onModRej(const TOrderPtr& order, const Reject& rej);
    void onCxlAck(const TOrderPtr& order);    
    void onCxlRej(const TOrderPtr& order, const Reject& rej);    
    void onFill(const Fill& fill);    
    void onAlert(const Alert& alert);
    void onRebuildOrderRej(const TOrderPtr& order, const Reject& rej);
    
public:
    // 'Post' processing methods for 'modAck/modRej'
    //
    void onModAckPost(const TOrderPtr& order);
    void onModRejPost(const TOrderPtr& order, const Reject& rej);
    void doOnModPost(const TOrderPtr& order, bool updatePrice);
    
public:
    // TimerClient interface
    //
    virtual bool onTimeout(const tw::common::TTimerId& id);
    
private:
    void remove(const TOrderPtr& order);
    void changeOpenOrdersCounts(const TOrderPtr& order, int8_t multiplier);
    void changePosCounts(const Fill& fill);
    
    Reject getRej(eRejectReason reason) {
        tw::channel_or::Reject rej;
        
        rej._rejType = eRejectType::kInternal;
        rej._rejSubType = eRejectSubType::kProcessorOrders;
        rej._rejReason = reason;
        
        return rej;
    }
    
private:    
    typedef tw::common::Pool<Order> TPool;
    typedef tw::channel_or::OrderTable<> TOrderTable;
    
private:
    tw::common::Settings _settings;
    TPool _pool;
    TOrderTable _table;
    
    TAccountInstrOpenOrdersPos _accountInstrOpenOrdersPos;
    TStrategyInstrOpenOrdersPos _strategyInstrOpenOrdersPos;
};

    
} // namespace channel_or
} // namespace tw
