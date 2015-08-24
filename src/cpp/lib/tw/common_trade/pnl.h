#pragma once

#include <tw/common/defs.h>
#include <tw/price/defs.h>
#include <tw/price/ticks_converter.h>
#include <tw/log/defs.h>
#include <tw/instr/instrument_manager.h>
#include <tw/common_trade/ipnl.h>
#include <tw/generated/channel_or_defs.h>

namespace tw {
namespace common_trade {
    
class PnL : public IPnL {
    typedef IPnL Parent;
    
public:
    PnL() {
        clear();
    }
    
    ~PnL() {
        clear();
    }
    
    PnL(const PnL& rhs) {
        *this = rhs;
    }
    
    PnL& operator=(const PnL& rhs) {
        if ( this != &rhs ) {
             static_cast<Parent&>(*this) = static_cast<const Parent&>(rhs);
             _instrument = rhs._instrument;
             _isTVSet = rhs._isTVSet;
             _tv = rhs._tv;
             _avgPrice = rhs._avgPrice;
        }
        
        return *this;
    }
    
    void clear() {
        Parent::clear();
        _instrument.reset();
        _isTVSet = false;
        _tv = 0.0;
        _avgPrice = 0.0;
    }
    
public:
    tw::instr::InstrumentConstPtr getInstrument() const {
        return _instrument;
    }
    
    double getAvgPrice() const {
        return _avgPrice;
    }
    
    bool setInstrument(const tw::instr::Instrument::TKeyId& instrumentId) {
        if ( !_instrument ) {
            _instrument = tw::instr::InstrumentManager::instance().getByKeyId(instrumentId);
            if ( !_instrument || !_instrument->isValid() ) {
                LOGGER_ERRO << "Severe error - unknown or invalid instrument for id: " << instrumentId << "\n";
                _instrument.reset();
                return false;
            }
        }
        
        return true;
    }
    
public:
    // IPnL interface
    //
    virtual bool isValid() const {
        if ( !_instrument || !_instrument->isValid() )
            return false;
        
        return true;
    }
    
    virtual bool isForMe(const tw::instr::Instrument::TKeyId& instrumentId) const {
        if ( !isValid() )
            return false;
        
        return (_instrument->_keyId == instrumentId);
    }
    
    virtual void onTv(const ITv& tv) {
        if ( !tv.isValid() )
            return;
        
        if ( !setInstrument(tv.getInstrumentId()) )
            return;
        
        setTv(tv.getTv());
    }
    
    virtual void onFill(const tw::channel_or::Fill& fill) {
        processFill(fill, false);
    }
    
    void rebuildPos(const tw::channel_or::PosUpdate& update) {
        if ( update._pos == 0 )
            return;
        
        tw::instr::InstrumentConstPtr instrument = tw::instr::InstrumentManager::instance().getByDisplayName(update._displayName);
        if ( !instrument || !instrument->isValid() ) {
            LOGGER_ERRO << "Can't rebuild position for unknown or invalid instrument: " << update._displayName << "\n";            
            return;
        }
        
        tw::channel_or::Fill fill;
        
        fill._instrumentId = instrument->_keyId;        
        fill._avgPrice = update._avgPrice;
        if ( update._pos > 0 ) {
            fill._qty.set(update._pos);
            fill._side = tw::channel_or::eOrderSide::kBuy;
        } else {
            fill._qty.set(-update._pos);
            fill._side = tw::channel_or::eOrderSide::kSell;
        }
        fill._liqInd = tw::channel_or::eLiqInd::kUnknown;
        
        processFill(fill, true);
    }
    
private:
    double calculatePnL(const tw::price::Size& qty, double price, int32_t sign) const {
        if ( !isValid() )
            return 0.0;
        
        return _instrument->_tc->fractionalTicks(_avgPrice-price) * _instrument->_tickValue * qty * -sign;
    }
    
    void updateRealizedPnL(const tw::price::Size& qty, double price, const tw::price::Size& position, int32_t sign) {
        _pnl._realizedPnL += calculatePnL(tw::price::Size(std::min(sign*_pnl._position.get(), qty.get())), price, sign);
        if ( 0 >= (sign*position.get()) )
            _avgPrice = price * (0 != position.get());
        
        Parent::updateRealizedPnL();
    }
    
    void updateUnrealizedPnL() {
        if ( _pnl._position == 0 ) {
            _pnl._unrealizedPnL = _pnl._maxUnrealizedPnL = _pnl._maxUnrealizedDrawdown = 0.0;
            return;
        }
        
        int32_t sign = 1;
        if ( _pnl._position > 0 )
            sign = -1;
        
        _pnl._unrealizedPnL = calculatePnL(tw::price::Size(sign*_pnl._position.get()), _tv, sign);
        
        Parent::updateUnrealizedPnL();
    }
    
    void updateFees(const tw::price::Size& qty, tw::channel_or::eLiqInd liqInd) {
        switch ( liqInd ) {
            case tw::channel_or::eLiqInd::kAdd:
                _fees._feeExLiqAdd += _instrument->_feeExLiqAdd * qty;
                break;
            case tw::channel_or::eLiqInd::kRem:
                _fees._feeExLiqRem += _instrument->_feeExLiqRem * qty;
                break;
            default:
                return;
        }
        
        _fees._feeExClearing += _instrument->_feeExClearing * qty;
        _fees._feeBrokerage += _instrument->_feeBrokerage * qty;
        _fees._feePerTrade += _instrument->_feePerTrade;
    }
    
    void updatePnL(const tw::price::Size& qty, double price, tw::channel_or::eOrderSide side, tw::channel_or::eLiqInd liqInd) {
        tw::price::Size position = (tw::channel_or::eOrderSide::kBuy == side) ? (_pnl._position+qty) : (_pnl._position-qty);

        if ( _pnl._position > 0 && tw::channel_or::eOrderSide::kSell == side ) {
            updateRealizedPnL(qty, price, position, 1);
        } else if ( _pnl._position < 0 && tw::channel_or::eOrderSide::kBuy == side ) {
            updateRealizedPnL(qty, price, position, -1);
        } else {
            _avgPrice = (_avgPrice * ::labs(_pnl._position.get()) + qty.get() * price) / ::labs(position.get());
        }
        
        _pnl._position = position;
        updateUnrealizedPnL();
        updateFees(qty, liqInd);
    }
    
    void processFill(const tw::channel_or::Fill& fill, bool useFillAvgPrice) {
        if ( !setInstrument(fill._instrumentId) )
            return;
        
        if ( _observer )
            _observer->preProcess(*this);
        
        double price = useFillAvgPrice ? fill._avgPrice : _instrument->_tc->toExchangePrice(fill._price);
        if ( !_isTVSet )
            setTv(price);
        
        updatePnL(fill._qty, price, fill._side, fill._liqInd);
        if ( !useFillAvgPrice )
            const_cast<tw::channel_or::Fill&>(fill)._avgPrice = _avgPrice;
        
        if ( _observer )
            _observer->postProcess(*this);
    }
    
private:        
    void setTv(double tv) {
        if ( _observer )
            _observer->preProcess(*this);
        
        _tv = tv;
        _isTVSet = true;
        updateUnrealizedPnL();
        
        if ( _observer )
            _observer->postProcess(*this);
    }
    
private:
    tw::instr::InstrumentConstPtr _instrument;
    bool _isTVSet;
    double _tv;
    double _avgPrice;
};

} // common_trade
} // tw
