#pragma once

#include <tw/common/defs.h>
#include <tw/price/quote.h>
#include <tw/log/defs.h>

namespace tw {
namespace common_trade {
  
class InitialEdgeDefenseStopProcessor {
public:
    InitialEdgeDefenseStopProcessor(InitialEdgeDefenseStopLossParamsWire& p) : _p(p) {
    }
    
public:
    InitialEdgeDefenseStopLossParamsWire& getParams() {
        return _p;
    }
    
    const InitialEdgeDefenseStopLossParamsWire& getParams() const {
        return _p;
    }
    
public:
    bool isStopSlideTriggered(FillInfo& info,
                              const tw::price::Quote& quote,
                              std::string& reason) {
        reason.clear();
        
        if ( !quote._book[0].isValid() ) {
            reason = "!quote._book[0].isValid()";
            return false;
        }

        tw::price::Ticks stop;
        tw::common_str_util::TFastStream s;
        switch ( info._fill._side ) {
            case tw::channel_or::eOrderSide::kBuy:
            {
                if ( info._stop.isValid() && info._stop >= info._fill._price - _p._slideStopPayupTicks2 ) {
                    reason = "info._stop.isValid() && info._stop >= info._fill._price - _p._slideStopPayupTicks2";
                    return false;
                }
                
                if ( info._fill._price + _p._slideStopOnProfitTicks1 > quote._book[0]._bid._price )  {
                    reason = "info._fill._price + _p._slideStopOnProfitTicks1 > quote._book[0]._bid._price";
                    return false;
                }

                if ( info._fill._price + _p._slideStopOnProfitTicks2 <= quote._book[0]._bid._price ) {
                    stop = info._fill._price - _p._slideStopPayupTicks2;
                    reason = "info._fill._price + _p._slideStopOnProfitTicks2 <= quote._book[0]._bid._price";
                } else if ( info._fill._price + _p._slideStopOnProfitTicks1 <= quote._book[0]._bid._price ) {
                    stop = info._fill._price - _p._slideStopPayupTicks1;
                    reason = "info._fill._price + _p._slideStopOnProfitTicks1 <= quote._book[0]._bid._price";
                }
                
                if ( !stop.isValid() ) {
                    if ( reason.empty() )
                        reason = "stop not recalculated";
                    else
                        reason = "stop not recalculated - reason: " + reason;
                    
                    return false;
                }
                
                if ( info._stop.isValid() && info._stop >= stop )  {
                    reason = "info._stop.isValid() && info._stop >= stop";
                    return false;
                }
                
                s << "isStopSlideTriggered(!info._stop.isValid() || info._stop < stop) for: " << "fill=" << info.toString() << " -- quote=" << quote._book[0].toShortString();
                info._stop = stop;
            }   
                break;
            case tw::channel_or::eOrderSide::kSell:
            {
                if ( info._stop.isValid() && info._stop <= info._fill._price + _p._slideStopPayupTicks2 ) {
                    reason = "info._stop.isValid() && info._stop <= info._fill._price + _p._slideStopPayupTicks2";
                    return false;
                }
                
                if ( info._fill._price - _p._slideStopOnProfitTicks1 < quote._book[0]._ask._price )  {
                    reason = "info._fill._price - _p._slideStopOnProfitTicks1 < quote._book[0]._ask._price";
                    return false;
                }

                if ( info._fill._price - _p._slideStopOnProfitTicks2 >= quote._book[0]._ask._price ) {
                    stop = info._fill._price + _p._slideStopPayupTicks2;
                    reason = "info._fill._price - _p._slideStopOnProfitTicks2 >= quote._book[0]._ask._price";
                } else if ( info._fill._price - _p._slideStopOnProfitTicks1 >= quote._book[0]._ask._price ) {
                    stop = info._fill._price + _p._slideStopPayupTicks1;
                    reason = "info._fill._price - _p._slideStopOnProfitTicks1 >= quote._book[0]._ask._price";
                }
                
                if ( !stop.isValid() ) {
                    if ( reason.empty() )
                        reason = "stop not recalculated";
                    else
                        reason = "stop not recalculated - reason: " + reason;
                    
                    return false;
                }
                
                if ( info._stop.isValid() && info._stop <= stop )  {
                    reason = "info._stop.isValid() && info._stop <= stop";
                    return false;
                }
                
                s << "isStopSlideTriggered(!info._stop.isValid() || info._stop > stop) for: " << "fill=" << info.toString() << " -- quote=" << quote._book[0].toShortString();
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
    InitialEdgeDefenseStopLossParamsWire _p;
};

} // common_trade
} // tw
