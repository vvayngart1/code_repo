#pragma once

#include <tw/common/defs.h>
#include <tw/price/quote.h>
#include <tw/log/defs.h>

namespace tw {
namespace common_trade {
  
class CramStopProcessor {
public:
    CramStopProcessor(CramStopLossParamsWire& p) : _p(p) {
        clear();
    }
    
public:
    CramStopLossParamsWire& getParams() {
        return _p;
    }
    
    const CramStopLossParamsWire& getParams() const {
        return _p;
    }
    
    void clear() {
        if ( _p._initialCramDisabled )
            _p._initialCramStopSet = true;
        else
            _p._initialCramStopSet = false;
        
        _p._topOfBook.clear();
    }
    
public:
    bool isEnabled() const {
        if ( 0.0 == _p._cramSlideRatio && (!_p._firmPriceQty.isValid() || 0 == _p._firmPriceQty.get()) )
            return false;
        
        return true;
    }
    
    template <typename TBars>
    bool isStopSlideWithBarsTriggered(FillInfo& info,
                                      const tw::price::Quote& quote,
                                      const TBars& bars,
                                      std::string& reason) {
        if ( !_p._minBarDispToCramTicks.isValid() || 0 == _p._minBarDispToCramTicks.get() || !_p._initialCramStopSet )
            return isStopSlideTriggered(info, quote, reason);
        
        if ( bars.size() < info._barIndex2 ) {
            reason = "CRITICAL ERROR: info._barIndex2 > bars.size()";
            return false;
        }
        
        const typename TBars::value_type& currBar = bars[bars.size()-1];
        if ( currBar._numOfTrades == 0 || !currBar._close.isValid() ) {
            reason = "currBar._numOfTrades == 0 || !currBar._close.isValid()";
            return false;
        }
        
        if ( ((tw::channel_or::eOrderSide::kBuy == info._fill._side) && (tw::common_trade::ePatternDir::kUp != currBar._dir))  || ((tw::channel_or::eOrderSide::kSell == info._fill._side) && (tw::common_trade::ePatternDir::kDown != currBar._dir)) ) {
            reason = "bar is not in the direction of trade: bar=" + currBar.toString() + ",info._fill._side=" + info._fill._side.toString(); 
            return false;
        }
        
        if ( bars.size() == info._barIndex2 ) {
            if ( tw::channel_or::eOrderSide::kBuy == info._fill._side ) {                
                if ( currBar._high < info._fill._price+_p._initialCramPopTicks ) {
                    reason = "currBar._high < info._fill._price+_p._initialCramPopTicks";
                    return false;
                }
            } else {
                if ( currBar._low > info._fill._price-_p._initialCramPopTicks ) {
                    reason = "currBar._low > info._fill._price-_p._initialCramPopTicks";
                    return false;
                }
            }
        } else {
            const typename TBars::value_type& prevBar = bars[bars.size()-2];
            
            if ( prevBar._numOfTrades == 0 || !prevBar._close.isValid() ) {
                reason = "prevBar._numOfTrades == 0 || !prevBar._close.isValid()";
                return false;
            }
            
            tw::price::Ticks range = (tw::channel_or::eOrderSide::kBuy == info._fill._side) ? (currBar._high-prevBar._close) : (prevBar._close-currBar._low);
            if ( range < _p._minBarDispToCramTicks ) {
                if ( _p._cramMinATRMult > 0 ) {
                    double limit = prevBar._atr * _p._cramMinATRMult;
                    if ( limit >= range.toDouble() ) {
                        reason = "limit >= range.toDouble()";
                        return false;
                    }
                }
                
                reason = "range < _p._minBarDispToCramTicks";
                return false;
            }
        }
        
        return isStopSlideTriggered(info, quote, reason);
    }
    
    bool isStopSlideTriggered(FillInfo& info,
                              const tw::price::Quote& quote,
                              std::string& reason) {
        static tw::price::Ticks ONE_TICK(1);
        
        reason.clear();
        
        if ( !isEnabled() ) {
            reason = "!isEnabled()";
            return false;
        }
        
        if ( !quote._book[0].isValid() ) {
            reason = "!quote._book[0].isValid()";
            return false;
        }

        tw::price::Ticks stop;
        tw::common_str_util::TFastStream s;
        switch ( info._fill._side ) {
            case tw::channel_or::eOrderSide::kBuy:
            {
                if ( !_p._initialCramStopSet ) {
                    if ( quote._book[0]._bid._price < info._fill._price+_p._initialCramStopEvalTicks ) {
                        _p._topOfBook.clear();
                        reason = "quote._book[0]._bid._price < info._fill._price+_p._initialCramStopEvalTicks";
                        return false;
                    }
                    
                    if ( !_p._topOfBook._bid._price.isValid() || _p._topOfBook._bid._price != quote._book[0]._bid._price ) {
                        _p._topOfBook.clear();
                        _p._topOfBook._bid._price = quote._book[0]._bid._price;
                        _p._topOfBook._ask._price = quote._book[0]._ask._price;
                    }
                    
                    if ( !quote.isNormalTrade() ) {
                        if ( quote._book[0]._bid._price < info._fill._price+_p._initialCramPopTicks ) {
                            reason = "quote._book[0]._bid._price < info._fill._price+_p._initialCramPopTicks";
                            return false;
                        }
                        
                        _p._initialCramStopSet = true;
                        reason = "quote._book[0]._bid._price >= info._fill._price+_p._initialCramPopTicks";
                    } else {
                        if ( quote._trade._price > _p._topOfBook._bid._price ) {
                            _p._topOfBook._ask._size += quote._trade._size;
                        } else if ( quote._trade._price == _p._topOfBook._bid._price ) {
                            _p._topOfBook._bid._size += quote._trade._size;
                        } else {
                            _p._topOfBook.clear();
                            reason = "quote._trade._price < _p._topOfBook._bid._price";
                            return false;
                        }
                        
                        if ( _p._initialCramFirmPriceQty.get() > 0 ) {
                            if ( _p._initialCramFirmPriceQty > quote._book[0]._bid._size ) {
                                reason = "initialCramFirmPriceQty > quote._book[0]._bid._size";
                                return false;
                            }
                            
                            if ( quote._book[0]._bid._price > info._fill._price+_p._initialCramStopEvalTicks ) {
                                _p._initialCramStopSet = true;
                                reason = "_p._initialCramFirmPriceQty > quote._book[0]._bid._size && quote._book[0]._bid._price > info._fill._price+_p._initialCramStopEvalTicks";
                            }
                        }
                        
                        if ( !_p._initialCramStopSet ) {
                            if ( _p._topOfBook._ask._size < _p._initialCramVolGood ) {
                                reason = "vol on ask < _p._initialCramVolGood";
                                return false;
                            }

                            double cramSlideRatio = (0 == _p._topOfBook._bid._size.get() ? _p._topOfBook._ask._size.toDouble() : (_p._topOfBook._ask._size.toDouble()/_p._topOfBook._bid._size.toDouble()));
                            if ( cramSlideRatio < _p._initialCramRatio ) {
                                reason = "cramSlideRatio < _p._initialCramRatio";
                                return false;
                            }

                            _p._initialCramStopSet = true;
                            reason = "cramSlideRatio >= _p._initialCramRatio";
                        }
                    }
                    
                    if ( _p._initialCramStopSet )
                        stop = info._fill._price - _p._initialCramOffsetTicks;
                } else {
                    if ( 0.0 == _p._cramSlideRatio ) {
                        for ( size_t i = 0; i < tw::price::Quote::SIZE; ++i ) {
                            if ( quote._book[i]._bid.isValid() && quote._book[i]._bid._size > _p._firmPriceQty ) {
                                stop = quote._book[i]._bid._price - _p._firmPriceOffsetTicks;
                                reason += "quote._book[i]._bid._size > _p._firmPriceQty :: i=" + boost::lexical_cast<std::string>(i) + ",quote[i]=" + quote._book[i].toShortString();
                                break;
                            }
                        }
                    } else {
                        for ( size_t i = 0; i < tw::price::Quote::SIZE-1; ++i ) {
                            if ( quote._book[i]._bid.isValid() && quote._book[i+1]._bid.isValid() && quote._book[i]._bid._size > _p._firmPriceQty ) {
                                tw::price::Size currLevelQty = quote._book[i]._bid._size; 
                                tw::price::Size nextLevelQty(1);
                                if ( ONE_TICK == (quote._book[i]._bid._price - quote._book[i+1]._bid._price) )
                                    nextLevelQty = quote._book[i+1]._bid._size;

                                double cramSlideRatio = currLevelQty.toDouble() / nextLevelQty.toDouble();
                                if ( cramSlideRatio > _p._cramSlideRatio ) {
                                    stop = quote._book[i]._bid._price - _p._firmPriceOffsetTicks;
                                    reason = "quote._book[i]._bid._size > _p._firmPriceQty && cramSlideRatio > _p._cramSlideRatio :: i=" + boost::lexical_cast<std::string>(i) 
                                             + ",quote[i]=" + quote._book[i].toShortString()
                                             + ",quote[i+1]=" + quote._book[i+1].toShortString();
                                    break;
                                }
                            }
                        }
                        
                        if ( !stop.isValid() )
                            reason = "quote._book[i]._bid._size <= _p._firmPriceQty || cramSlideRatio <= _p._cramSlideRatio :: quote: " + quote.toString();
                    }                                       
                }
                
                if ( !stop.isValid() ) {
                    if ( reason.empty() )
                        reason = "stop not recalculated";
                    else
                        reason = "stop not recalculated - reason: " + reason;
                    
                    return false;
                }
                
                if ( info._stop.isValid() && info._stop >= stop )  {
                    reason = "info._stop.isValid() && info._stop <= stop";
                    return false;
                }
                
                s << "isStopSlideTriggered(!info._stop.isValid() || info._stop < stop) for: "                                                 
                  << "stop=" << stop.get() << ",fill=" << info.toString() 
                  << " -- quote=" << quote._book[0].toShortString();

                info._stop = stop;
            }   
                break;
            case tw::channel_or::eOrderSide::kSell:
            {
                if ( !_p._initialCramStopSet ) {
                    if ( quote._book[0]._ask._price > info._fill._price-_p._initialCramStopEvalTicks ) {
                        _p._topOfBook.clear();
                        reason = "quote._book[0]._ask._price > info._fill._price-_p._initialCramStopEvalTicks";
                        return false;
                    }
                    
                    if ( !_p._topOfBook._ask._price.isValid() || _p._topOfBook._ask._price != quote._book[0]._ask._price ) {
                        _p._topOfBook.clear();
                        _p._topOfBook._bid._price = quote._book[0]._bid._price;
                        _p._topOfBook._ask._price = quote._book[0]._ask._price;
                    }
                    
                    if ( !quote.isNormalTrade() ) {
                        if ( quote._book[0]._ask._price > info._fill._price-_p._initialCramPopTicks ) {
                            reason = "quote._book[0]._ask._price < info._fill._price-_p._initialCramPopTicks";
                            return false;
                        }
                        
                        _p._initialCramStopSet = true;
                        reason = "quote._book[0]._ask._price <= info._fill._price-_p._initialCramPopTicks";
                    } else {
                        if ( quote._trade._price < _p._topOfBook._ask._price ) {
                            _p._topOfBook._bid._size += quote._trade._size;
                        } else if ( quote._trade._price == _p._topOfBook._ask._price ) {
                            _p._topOfBook._ask._size += quote._trade._size;
                        } else {
                            _p._topOfBook.clear();
                            reason = "quote._trade._price > _p._topOfBook._ask._price";
                            return false;
                        }
                        
                        if ( _p._initialCramFirmPriceQty.get() > 0 ) {
                            if ( _p._initialCramFirmPriceQty > quote._book[0]._ask._size ) {
                                reason = "initialCramFirmPriceQty > quote._book[0]._ask._size";
                                return false;
                            }
                            
                            if ( quote._book[0]._ask._price < info._fill._price-_p._initialCramStopEvalTicks ) {
                                _p._initialCramStopSet = true;
                                reason = "_p._initialCramFirmPriceQty > quote._book[0]._ask._size && quote._book[0]._ask._price < info._fill._price-_p._initialCramStopEvalTicks";
                            }
                        }
                        
                        if ( !_p._initialCramStopSet ) {
                            if ( _p._topOfBook._bid._size < _p._initialCramVolGood ) {
                                reason = "vol on bid < _p._initialCramVolGood";
                                return false;
                            }

                            double cramSlideRatio = (0 == _p._topOfBook._ask._size.get() ? _p._topOfBook._bid._size.toDouble() : (_p._topOfBook._bid._size.toDouble()/_p._topOfBook._ask._size.toDouble()));
                            if ( cramSlideRatio < _p._initialCramRatio ) {
                                reason = "cramSlideRatio < _p._initialCramRatio";
                                return false;
                            }

                            _p._initialCramStopSet = true;
                            reason = "cramSlideRatio >= _p._initialCramRatio";
                        }
                    }
                    
                    if ( _p._initialCramStopSet )
                        stop = info._fill._price + _p._initialCramOffsetTicks;
                } else {
                    if ( 0.0 == _p._cramSlideRatio ) {
                        for ( size_t i = 0; i < tw::price::Quote::SIZE; ++i ) {
                            if ( quote._book[i]._ask.isValid() && quote._book[i]._ask._size > _p._firmPriceQty ) {
                                stop = quote._book[i]._ask._price + _p._firmPriceOffsetTicks;
                                reason += "quote._book[i]._ask._size > _p._firmPriceQty :: i=" + boost::lexical_cast<std::string>(i) + ",quote[i]=" + quote._book[i].toShortString();
                                break;
                            }
                        }
                    } else {
                        for ( size_t i = 0; i < tw::price::Quote::SIZE-1; ++i ) {
                            if ( quote._book[i]._ask.isValid() && quote._book[i+1]._ask.isValid() && quote._book[i]._ask._size > _p._firmPriceQty ) {
                                tw::price::Size currLevelQty = quote._book[i]._ask._size; 
                                tw::price::Size nextLevelQty(1);
                                if ( ONE_TICK == (quote._book[i+1]._ask._price - quote._book[i]._ask._price) )
                                    nextLevelQty = quote._book[i+1]._ask._size;

                                double cramSlideRatio = currLevelQty.toDouble() / nextLevelQty.toDouble();
                                if ( cramSlideRatio > _p._cramSlideRatio ) {
                                    stop = quote._book[i]._ask._price + _p._firmPriceOffsetTicks;
                                    reason = "quote._book[i]._ask._size > _p._firmPriceQty && cramSlideRatio > _p._cramSlideRatio :: i=" + boost::lexical_cast<std::string>(i) 
                                             + ",quote[i]=" + quote._book[i].toShortString()
                                             + ",quote[i+1]=" + quote._book[i+1].toShortString();
                                    break;
                                }
                            }
                        }
                        
                        if ( !stop.isValid() )
                            reason = "quote._book[i]._ask._size <= _p._firmPriceQty || cramSlideRatio <= _p._cramSlideRatio :: quote: " + quote.toString(); 
                    }                                       
                }
                
                if ( !stop.isValid() ) {
                    if ( reason.empty() )
                        reason = "stop not recalculated";
                    else
                        reason = "stop not recalculated - reason: " + reason;
                    
                    return false;
                }
                
                if ( info._stop.isValid() && info._stop <= stop ) {
                    reason = "info._stop.isValid() && info._stop <= stop";
                    return false;
                }
                
                s << "isStopSlideTriggered(!info._stop.isValid() || info._stop > stop) for: "                                                 
                  << "stop=" << stop.get() << ",fill=" << info.toString() 
                  << " -- quote=" << quote._book[0].toShortString();

                info._stop = stop;
            }
                break;
            default:
                return false;
        }

        reason = s.str() + " :: " + reason;
        return true;
    }
    
private:
    CramStopLossParamsWire& _p;
};

} // common_trade
} // tw
