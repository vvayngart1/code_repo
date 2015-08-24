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
#include <list>

namespace tw {
namespace channel_or {
    
class ProcessorMessaging : public tw::common::Singleton<ProcessorMessaging> {
public:
    ProcessorMessaging();
    ~ProcessorMessaging();
    
    void clear();
    
public:
    bool init(const std::vector<tw::channel_or::MessagingForExchange>& v);

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
    tw::channel_or::MessagingForExchange* getInfo(const tw::instr::Instrument::TKeyId& v, const TAccountId& w);
    
private:
    typedef std::list<tw::channel_or::MessagingForExchange> TMessagingInfos;
    
    typedef std::pair<tw::instr::Instrument::TKeyId, TAccountId> TKey;
    typedef std::map<TKey, tw::channel_or::MessagingForExchange*> TIndexToMessagingInfos;
    
    TMessagingInfos _infos;
    TIndexToMessagingInfos _index;
};

} // namespace channel_or
} // namespace tw
