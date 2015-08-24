#pragma once

#include <tw/common/singleton.h>
#include <tw/price/quote.h>
#include <tw/price/quote_store.h>
#include <tw/common_trade/pnl.h>
#include <tw/common_trade/pnlComposite.h>

#include <tw/generated/channel_or_defs.h>

#include <map>
#include <set>

namespace tw {
namespace channel_or {

class PnLAuditTrail : public tw::common::Singleton<PnLAuditTrail> {
public:
    PnLAuditTrail();
    ~PnLAuditTrail();
    
    void clear();
    
public:
    void onFill(const Fill& fill);
    void onQuote(const tw::price::QuoteStore::TQuote& quote);
    
private:
    bool updateOnQuote(const tw::price::QuoteStore::TQuote& quote, PnLAuditTrailInfo& info);
    PnLAuditTrailInfo* getOrCreatePnLAuditTrailInfo(const Fill& fill);
    void outputToStorage(PnLAuditTrailInfo& info, const tw::common::THighResTime& timestamp);
    void outputAllToStorage();
        
private:
    typedef std::map<TStrategyId, PnLAuditTrailInfo> TStratsPnLAuditTrailInfos;
    typedef std::set<PnLAuditTrailInfo*> TPnLAuditTrailInfos;
    typedef std::map<tw::instr::Instrument::TKeyId, TPnLAuditTrailInfos> TInstrumentsPnLAuditTrailInfos;
    
    TStratsPnLAuditTrailInfos _stratsPnLAuditTrails;
    TInstrumentsPnLAuditTrailInfos _instrumentsPnLAuditTrails;
    PnLAuditTrailInfo _accountPnLAuditTrailInfo;
};

} // namespace channel_or
} // namespace tw
