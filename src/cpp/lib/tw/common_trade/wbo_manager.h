#pragma once

#include <tw/common/singleton.h>
#include <tw/common_trade/wbo.h>
#include <tw/price/quote_store.h>

#include <map>

namespace tw {
namespace common_trade {
    
class WboManager : public tw::common::Singleton<WboManager> {
public:
    typedef tw::common_trade::Wbo<tw::common_trade::Wbo_calc1> TTv;
    typedef std::map<tw::instr::Instrument::TKeyId, TTv> TInstrumentsTv;
    
public:
    WboManager() {
        clear();
    }
    
    ~WboManager() {
        
    }
    
    void clear() {
        _instrumentsTv.clear();
    }
    
public:    
    void onQuote(const tw::price::Quote& quote) {
        TTv& tv = getOrCreateTv(quote._instrumentId);
        tv.onQuote(quote);
    }
    
public:    
    TTv& getOrCreateTv(const tw::instr::Instrument::TKeyId& x) {
        TInstrumentsTv::iterator iter = _instrumentsTv.find(x);
        
        if ( iter == _instrumentsTv.end() ) {
            iter = _instrumentsTv.insert(TInstrumentsTv::value_type(x, TTv())).first;
            tw::price::QuoteStore::instance().subscribeFront(x, this);
            iter->second.setInstrument(tw::instr::InstrumentManager::instance().getByKeyId(x));
        }
        
        return iter->second;
    }

private:
    TInstrumentsTv _instrumentsTv;
};
    
} // namespace common_trade
} // namespace tw
