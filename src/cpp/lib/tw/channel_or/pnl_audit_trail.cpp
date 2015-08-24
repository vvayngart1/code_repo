#include <tw/channel_or/pnl_audit_trail.h>
#include <tw/channel_or/processor_pnl.h>
#include <tw/channel_or/channel_or_storage.h>

namespace tw {
namespace channel_or {
    
static bool isMinMaxPnLChanged(PnLAuditTrailInfo& info) {
    if ( info._unrealizedInDollars > info._maxInDollars ) {
        info._maxInDollars = info._unrealizedInDollars;
        return true;
    }
    
    if ( info._unrealizedInDollars < info._minInDollars ) {
        info._minInDollars = info._unrealizedInDollars;
        return true;
    }
    
    return false;
}

PnLAuditTrail::PnLAuditTrail() {
    clear();    
}

PnLAuditTrail::~PnLAuditTrail() {
}
    
void PnLAuditTrail::clear() {
    _stratsPnLAuditTrails.clear();
    _instrumentsPnLAuditTrails.clear();
}

void PnLAuditTrail::onFill(const Fill& fill) {
    // Process accountPnL
    //
    if ( _accountPnLAuditTrailInfo._displayName1.empty() ) {
        _accountPnLAuditTrailInfo._accountId = fill._accountId;        
        _accountPnLAuditTrailInfo._displayName1 = "ACCOUNT_PNL";
        _accountPnLAuditTrailInfo._parentType = "ACCOUNT";
    }
    
    const TPnLComposite& accountPnL = ProcessorPnL::instance().getAccountPnL();
    _accountPnLAuditTrailInfo._pos1 = accountPnL.getPosition();
    _accountPnLAuditTrailInfo._realizedInDollars = accountPnL.getRealizedPnL();
    
    TLongsShorts longsShortsAccount = ProcessorPnL::instance().getAccountLongsShorts();
    _accountPnLAuditTrailInfo._posLong = longsShortsAccount.first;
    _accountPnLAuditTrailInfo._posShort = longsShortsAccount.second;
    if ( 0 == _accountPnLAuditTrailInfo._posLong.get() && 0 == _accountPnLAuditTrailInfo._posShort.get() ) {
        _accountPnLAuditTrailInfo._maxInDollars = 0.0;
        _accountPnLAuditTrailInfo._minInDollars = 0.0;
        _accountPnLAuditTrailInfo._unrealizedInDollars = 0.0;
    }

    // Process stratPnL
    //
    PnLAuditTrailInfo* info = getOrCreatePnLAuditTrailInfo(fill);
    if ( !info ) {
        LOGGER_ERRO << "Can't get info for: " << fill.toString() << "\n";
        return;
    }
    
    StratPnL* stratPnL = ProcessorPnL::instance().getStratsPnL(info->_strategyId);
    if ( !stratPnL ) {
        LOGGER_ERRO << "Can't find stratPnL for: " << fill.toString() << "\n";
        return;
    }
    
    TPnL* instrumentPnL = stratPnL->getInstrumentPnL(fill._instrumentId);
    if ( !instrumentPnL ) {
        LOGGER_ERRO << "Can't find instrumentPnL for: " << fill.toString() << "\n";
        return;
    }
    
    tw::price::Size pos = instrumentPnL->getPosition();
    if ( info->_instrument1 && fill._instrumentId == info->_instrument1->_keyId ) {
        info->_pos1 = pos;
        info->_avgPriceInTicks1 = instrumentPnL->getInstrument()->_tc->fractionalTicks(instrumentPnL->getAvgPrice());
        info->_realizedInDollars1 = instrumentPnL->getRealizedPnL();         
    } else if ( info->_instrument2 && fill._instrumentId == info->_instrument2->_keyId ) {
        info->_pos2 = pos;
        info->_avgPriceInTicks2 = instrumentPnL->getInstrument()->_tc->fractionalTicks(instrumentPnL->getAvgPrice());
        info->_realizedInDollars2 = instrumentPnL->getRealizedPnL();         
    } else {
        LOGGER_ERRO << "info's and fill's instruments don't match - info: "  << info->toString() << " <--> fill: " << fill.toString() << "\n";
        return;
    }
    
    TLongsShorts longsShorts = stratPnL->getLongsShorts();
    info->_posLong = longsShorts.first;
    info->_posShort = longsShorts.second;
    info->_realizedInDollars = stratPnL->getRealizedPnL();
    
    if ( 0 == info->_pos1 ) {
        info->_maxInTicks1 = 0.0;
        info->_minInTicks1 = 0.0;
        info->_unrealizedInTicks1 = 0.0;
    }
    
    if ( 0 == info->_pos2 ) {
        info->_maxInTicks2 = 0.0;
        info->_minInTicks2 = 0.0;
        info->_unrealizedInTicks2 = 0.0;
    }
    
    if ( 0 == info->_pos1 && 0 == info->_pos2 ) {
        info->_maxInDollars = 0.0;
        info->_minInDollars = 0.0;
        info->_unrealizedInDollars = 0.0;
    }
    
    outputAllToStorage();
}

void PnLAuditTrail::onQuote(const tw::price::QuoteStore::TQuote& quote) {
    if ( !quote.isLevelUpdate(0) || !quote._book[0].isValid() )
        return;
    
    TInstrumentsPnLAuditTrailInfos::iterator iter = _instrumentsPnLAuditTrails.find(quote._instrumentId);
    if ( _instrumentsPnLAuditTrails.end() == iter )
        return;
    
    ProcessorPnL::instance().updatePnLForTvForStratInstrument(quote._instrumentId);
    
    TPnLAuditTrailInfos::iterator iter2 = iter->second.begin();
    TPnLAuditTrailInfos::iterator end2 = iter->second.end();
    bool changed = false;
    for ( ; iter2 != end2; ++iter2 ) {
        PnLAuditTrailInfo& info = *(*iter2);
        if ( updateOnQuote(quote, info) )
            changed = true;
        
        StratPnL* stratPnL = ProcessorPnL::instance().getStratsPnL(info._strategyId);
        if ( stratPnL ) {
            info._unrealizedInDollars = stratPnL->getUnrealizedPnL();
            if ( isMinMaxPnLChanged(info) )
                changed = true;
        }
    }
    
    // Update account
    //    
    _accountPnLAuditTrailInfo._unrealizedInDollars = ProcessorPnL::instance().getAccountPnL().getUnrealizedPnL();
    if ( isMinMaxPnLChanged(_accountPnLAuditTrailInfo) )
        changed = true;
    
    if ( changed )
        outputAllToStorage();
}

bool PnLAuditTrail::updateOnQuote(const tw::price::QuoteStore::TQuote& quote, PnLAuditTrailInfo& info) {
    bool changed = false;
    if ( info._instrument1 && quote._instrumentId == info._instrument1->_keyId ) {
        if ( 0 == info._pos1 )
            return false;
        
        if ( 0 < info._pos1 )
            info._unrealizedInTicks1 = quote._book[0]._bid._price.toDouble() - info._avgPriceInTicks1;
        else
            info._unrealizedInTicks1 = info._avgPriceInTicks1 - quote._book[0]._ask._price.toDouble();
        
        if ( info._unrealizedInTicks1 > info._maxInTicks1 ) {
            info._maxInTicks1 = info._unrealizedInTicks1;
            changed = true;
        } else if ( info._unrealizedInTicks1 < info._minInTicks1 ) {
            info._minInTicks1 = info._unrealizedInTicks1;
            changed = true;
        }
    } else if ( info._instrument2 && quote._instrumentId == info._instrument2->_keyId ) {
        if ( 0 == info._pos2 )
            return false;
        
        if ( 0 < info._pos2 )
            info._unrealizedInTicks2 = quote._book[0]._bid._price.toDouble() - info._avgPriceInTicks2;
        else
            info._unrealizedInTicks2 = info._avgPriceInTicks2 - quote._book[0]._ask._price.toDouble();
        
        if ( info._unrealizedInTicks2 > info._maxInTicks2 ) {
            info._maxInTicks2 = info._unrealizedInTicks2;
            changed = true;
        } else if ( info._unrealizedInTicks2 < info._minInTicks2 ) {
            info._minInTicks2 = info._unrealizedInTicks2;
            changed = true;
        }
    } else {
        LOGGER_ERRO << "info's and quote's instruments don't match - info: "  << info.toString() << " <--> fill: " << quote.toString() << "\n";
        return false;
    }
    
    return changed;
}

PnLAuditTrailInfo* PnLAuditTrail::getOrCreatePnLAuditTrailInfo(const Fill& fill) {
    tw::instr::InstrumentPtr instrument = tw::instr::InstrumentManager::instance().getByKeyId(fill._instrumentId);
    if ( !instrument ) {
        LOGGER_ERRO << "Can't get instrument for: " << fill.toString() << "\n";
        return NULL;
    }
    
    PnLAuditTrailInfo* info = NULL;
    TStratsPnLAuditTrailInfos::iterator iter = _stratsPnLAuditTrails.find(fill._strategyId);
    if ( iter == _stratsPnLAuditTrails.end() ) {
        iter = _stratsPnLAuditTrails.insert(TStratsPnLAuditTrailInfos::value_type(fill._strategyId, PnLAuditTrailInfo())).first;
        info = &(iter->second);
        
        info->_parentType = "STRATEGY";
        info->_accountId = fill._accountId;
        info->_strategyId = fill._strategyId;
        info->_instrument1 = instrument;
        info->_displayName1 = info->_instrument1->_displayName;
    } else {    
        info = &(iter->second);
        if ( info->_instrument1 && fill._instrumentId != info->_instrument1->_keyId && !info->_instrument2 ) {
            info->_instrument2 = instrument;
            info->_displayName2 = info->_instrument2->_displayName;
        }
    }
    
    TInstrumentsPnLAuditTrailInfos::iterator iter2 = _instrumentsPnLAuditTrails.find(fill._instrumentId);
    if ( iter2 == _instrumentsPnLAuditTrails.end() )
        iter2 = _instrumentsPnLAuditTrails.insert(TInstrumentsPnLAuditTrailInfos::value_type(fill._instrumentId, TPnLAuditTrailInfos())).first;
    
    TPnLAuditTrailInfos::iterator iter3 = iter2->second.find(info);
    if ( iter3 == iter2->second.end() )
        iter2->second.insert(info);
    
    return info;
}

void PnLAuditTrail::outputToStorage(PnLAuditTrailInfo& info, const tw::common::THighResTime& timestamp) {
    if ( 0 == info._posLong.get() && 0 == info._posShort.get() && 0.0 == info._realizedInDollars && 0.0 == info._realizedInDollars1 && 0.0 == info._realizedInDollars2 )
        return;
    
    info._eventTimestamp = timestamp;
    ChannelOrStorage::instance().persist(info);
}

void PnLAuditTrail::outputAllToStorage() {
    tw::common::THighResTime timestamp = tw::common::THighResTime::now();
    
    TStratsPnLAuditTrailInfos::iterator iter = _stratsPnLAuditTrails.begin();
    TStratsPnLAuditTrailInfos::iterator end = _stratsPnLAuditTrails.end();
    for ( ; iter != end; ++iter ) {
        outputToStorage(iter->second, timestamp);
    }
    
    outputToStorage(_accountPnLAuditTrailInfo, timestamp);
}
    
} // namespace channel_or
} // namespace tw
