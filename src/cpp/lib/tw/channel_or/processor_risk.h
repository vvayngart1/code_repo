#pragma once

#include <tw/common/command.h>
#include <tw/common/singleton.h>
#include <tw/common/settings.h>

#include <tw/generated/channel_or_defs.h>
#include <tw/generated/risk_defs.h>

#include <map>

namespace tw {
namespace channel_or {
    
class ProcessorRisk : public tw::common::Singleton<ProcessorRisk> {
public:
    ProcessorRisk();
    ~ProcessorRisk();
    
    void clear();
    
    bool init(const tw::risk::Account& account, const std::vector<tw::risk::AccountRiskParams>& params);
    
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
    
    void onAlert(const Alert& alert) {}
    void onRebuildOrderRej(const TOrderPtr& order, const Reject& rej) {}
    
    void onNewRej(const TOrderPtr& order, const Reject& rej) {}    
    void onModRej(const TOrderPtr& order, const Reject& rej) {}    
    void onCxlRej(const TOrderPtr& order, const Reject& rej) {}
    
public:
    bool canSend(const TOrderPtr& order);
    bool canSend(const TAccountId& accountId, tw::instr::Instrument::TKeyId instrumentId, tw::price::Size qty, eOrderSide side);
    bool isTradeable(tw::instr::Instrument::TKeyId id);
    
private:
    Reject getRej(eRejectReason reason) {
        tw::channel_or::Reject rej;
        
        rej._rejType = eRejectType::kInternal;
        rej._rejSubType = eRejectSubType::kProcessorRisk;
        rej._rejReason = reason;
        
        return rej;
    }
    
private:
    typedef std::map<tw::instr::Instrument::TKeyId, tw::risk::AccountRiskParams> TInstrAccountRiskParams;
    
    tw::risk::Account _account;
    TInstrAccountRiskParams _params;
};

    
} // namespace channel_or
} // namespace tw
