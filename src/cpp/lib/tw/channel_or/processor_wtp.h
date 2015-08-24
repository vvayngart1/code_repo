#pragma once

#include <tw/common/command.h>
#include <tw/common/settings.h>
#include <tw/common/singleton.h>
#include <tw/generated/channel_or_defs.h>

#include <list>
#include <map>

namespace tw {
namespace channel_or {

class ProcessorWTP : public tw::common::Singleton<ProcessorWTP> {
public:
    typedef std::list<TOrderPtr> TOrdersPerSide;
    typedef std::pair<TOrdersPerSide, TOrdersPerSide> TOrdersBook;
    typedef std::map<tw::instr::Instrument::TKeyId, TOrdersBook> TOrdersBooks;
    
public:
    ProcessorWTP();    
    ~ProcessorWTP();
    
    void clear();
   
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
    
    bool rebuildOrder(const TOrderPtr& order, Reject& rej) { return true; }
    void recordFill(const Fill& fill) {}
    void rebuildPos(const PosUpdate& update) {}
    
    void onAlert(const Alert& alert) {}
    void onRebuildOrderRej(const TOrderPtr& order, const Reject& rej) {}
    
    void onNewRej(const TOrderPtr& order, const Reject& rej) {}    
    void onModRej(const TOrderPtr& order, const Reject& rej) {}    
    void onCxlRej(const TOrderPtr& order, const Reject& rej) {}
    
public:
    TOrdersBook& getOrCreateOrdersBook(const tw::instr::Instrument::TKeyId& v);
    bool canSend(const tw::instr::InstrumentConstPtr& instrument, eOrderSide side, const tw::price::Ticks& price, Reject& rej);
    bool canSend(const TOrderPtr& order, const tw::price::Ticks& price, Reject& rej);

private:
    template <typename TOp>
    TOrderPtr getCrossingOrder(TOrdersPerSide& ordersPerSide, const tw::price::Ticks& price, TOp op) {
        if ( ordersPerSide.empty() )
            return TOrderPtr();
        
        TOrdersPerSide::iterator iter = ordersPerSide.begin();
        TOrdersPerSide::iterator end = ordersPerSide.end();
        
        while( iter != end ) {
            switch ( (*iter)->_state ) {
                case eOrderState::kCancelled:
                case eOrderState::kRejected:
                case eOrderState::kFilled:
                    iter = ordersPerSide.erase(iter);
                    break;
                case eOrderState::kModifying:
                    if ( (*iter)->_newPrice.isValid() && op(price, (*iter)->_newPrice) )
                        return (*iter);
                default:
                    if ( (*iter)->_price.isValid() && op(price, (*iter)->_price) )
                        return (*iter);
                    ++iter;
                    break;
            }
        }
        
        return TOrderPtr();
    }
    
private:
    Reject getRej(eRejectReason reason) {
        tw::channel_or::Reject rej;
        
        rej._rejType = eRejectType::kInternal;
        rej._rejSubType = eRejectSubType::kProcessorWTP;
        rej._rejReason = reason;
        
        return rej;
    }
    
private:
    tw::common::Settings _settings;
    TOrdersBooks _ordersBooks;
};

    
} // namespace channel_or
} // namespace tw
