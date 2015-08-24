#pragma once

#include <tw/common/command.h>
#include <tw/common/singleton.h>
#include <tw/common/settings.h>
#include <tw/price/quote.h>
#include <tw/price/quote_store.h>
#include <tw/common_trade/wbo.h>
#include <tw/common_trade/pnl.h>
#include <tw/common_trade/pnlComposite.h>

#include <tw/generated/channel_or_defs.h>
#include <tw/generated/risk_defs.h>

#include <map>

namespace tw {
namespace channel_or {
    
typedef tw::common_trade::PnL TPnL;
typedef tw::common_trade::PnLComposite TPnLComposite;
typedef std::pair<tw::price::Size, tw::price::Size> TLongsShorts;
    
struct StratPnL {
    typedef std::map<tw::instr::Instrument::TKeyId, TPnL> TInstrumentsPnL;
    
    StratPnL() {
        clear();
    }
    
    void clear() {
        _instrumentsPnL.clear();
        _stratPnL.clear();
        _limits.clear();
    }
    
    TPnL* getInstrumentPnL(tw::instr::Instrument::TKeyId instrumentId) {
        TInstrumentsPnL::iterator iter = _instrumentsPnL.find(instrumentId);
        if ( iter != _instrumentsPnL.end() )
            return &(iter->second);
        
        return NULL;
    }
    
    TLongsShorts getLongsShorts() const {
        TLongsShorts longsShorts;
        longsShorts.first = longsShorts.second = tw::price::Size(0);
        
        TInstrumentsPnL::const_iterator iter = _instrumentsPnL.begin();
        TInstrumentsPnL::const_iterator end = _instrumentsPnL.end();
        for ( ; iter != end; ++iter ) {
            tw::price::Size pos = iter->second.getPosition();
            if ( pos.get() > 0 )
                longsShorts.first += pos;
            else
                longsShorts.second += pos;
        }
        
        return longsShorts;
    }
    
    double getRealizedPnL() const {
        double pnl = 0.0;
        
        TInstrumentsPnL::const_iterator iter = _instrumentsPnL.begin();
        TInstrumentsPnL::const_iterator end = _instrumentsPnL.end();
        for ( ; iter != end; ++iter ) {
            pnl += iter->second.getRealizedPnL();
        }
        
        return pnl;
    }
    
    double getUnrealizedPnL() const {
        double pnl = 0.0;
        
        TInstrumentsPnL::const_iterator iter = _instrumentsPnL.begin();
        TInstrumentsPnL::const_iterator end = _instrumentsPnL.end();
        for ( ; iter != end; ++iter ) {
            pnl += iter->second.getUnrealizedPnL();
        }
        
        return pnl;
    }
    
    TInstrumentsPnL _instrumentsPnL;
    TPnLComposite _stratPnL;
    tw::risk::Strategy _limits;
};
    
class ProcessorPnL : public tw::common::Singleton<ProcessorPnL> {
public:
    typedef std::map<tw::risk::TStrategyId, StratPnL> TStratsPnL;
    
public:
    ProcessorPnL();
    ~ProcessorPnL();
    
    void clear();
    
public:
    tw::risk::Account& getAccount(){
        return _account;
    }
    
    const tw::risk::Account& getAccount() const {
        return _account;
    }
    
public:
    const TPnLComposite& getAccountPnL() const {
        return _accountPnL;
    } 
    
    const TStratsPnL& getStratsPnL() const {
        return _stratsPnL;
    }
    
    StratPnL* getStratsPnL(tw::risk::TStrategyId strategyId) {
        TStratsPnL::iterator iter = _stratsPnL.find(strategyId);
        if ( iter != _stratsPnL.end() )
            return &(iter->second);
        
        return NULL;
    }
    
    TLongsShorts getAccountLongsShorts() const {
        TLongsShorts longsShorts;
        longsShorts.first = longsShorts.second = tw::price::Size(0);
        
        TStratsPnL::const_iterator iter = _stratsPnL.begin();
        TStratsPnL::const_iterator end = _stratsPnL.end();
        for ( ; iter!=end; ++iter ) {
            TLongsShorts ls = iter->second.getLongsShorts();
            longsShorts.first += ls.first;
            longsShorts.second += ls.second;
        }
        
        return longsShorts;
    }
    
    void updatePnLForTvForStratInstrument(const tw::instr::Instrument::TKeyId& instrId);
    
public:
    bool isAccountTotalLossReached(double& pnl_amount, std::string& reason);
    
public:
    bool init(const tw::risk::Account& account, const std::vector<tw::risk::Strategy>& strats);    
    
public:
    // ProcessorOut interface
    //    
    bool init(const tw::common::Settings& settings);
    bool start();
    void stop();
    
    bool sendNew(const TOrderPtr& order, Reject& rej);    
    bool sendMod(const TOrderPtr& order, Reject& rej) { return true; }
    bool sendCxl(const TOrderPtr& order, Reject& rej) { return true; }
    void onCommand(const tw::common::Command& command);
    bool rebuildOrder(const TOrderPtr& order, Reject& rej) { return true; }
    void recordFill(const Fill& fill);    
    void rebuildPos(const PosUpdate& update);
    
public:
    // ProcessorIn interface
    //
    void onNewAck(const TOrderPtr& order) {}
    void onNewRej(const TOrderPtr& order, const Reject& rej) {}
    void onModAck(const TOrderPtr& order) {}
    void onModRej(const TOrderPtr& order, const Reject& rej) {}
    void onCxlAck(const TOrderPtr& order) {}
    void onCxlRej(const TOrderPtr& order, const Reject& rej) {}
    void onFill(const Fill& fill);
    void onAlert(const Alert& alert) {}
    void onRebuildOrderRej(const TOrderPtr& order, const Reject& rej) {}
    
public:
    bool printToString(tw::channel_or::TStrategyId strategyId, std::string& results) const;
    
private:
    Reject getRej(eRejectReason reason);
    TPnL& getOrCreatePnl(const tw::risk::TStrategyId& x,  const tw::instr::Instrument::TKeyId& y);
    StratPnL& getOrCreateStrat(const tw::risk::TStrategyId& x);
    bool isToReject(const TOrderPtr& order, int32_t pos);
    void logPnl() const;
    bool updatePnLForTv(const StratPnL& stratPnL) const;
    
private:
    bool _printPnL;
    tw::risk::Account _account;
    
    TPnLComposite _accountPnL;
    TStratsPnL _stratsPnL;
};

    
} // namespace channel_or
} // namespace tw
