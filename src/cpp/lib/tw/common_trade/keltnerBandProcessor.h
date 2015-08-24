#pragma once

#include <tw/common/defs.h>
#include <tw/price/quote.h>
#include <tw/log/defs.h>

#include <tw/common_trade/bars_manager.h>

namespace tw {
namespace common_trade {
    
typedef tw::common_trade::TBar TBar;
typedef tw::common_trade::TBars TBars;
typedef tw::common_trade::BarPattern TBarPattern;

class KeltnerBandProcessor {
public:
    KeltnerBandProcessor(KeltnerBandProcessorParamsWire& p) : _p(p) {
        _p._kbBarPositionToKeltnerBand = eBarPositionToKeltnerBand::kUnknown;
    }
    
    void clear() {
        _p._kbReason.clear();
        _s.clear(); 
    }
    
public:
    KeltnerBandProcessorParamsWire& getParams() {
        return _p;
    }
    
    const KeltnerBandProcessorParamsWire& getParams() const {
        return _p;
    }
    
public:
    bool isEnabled() const {
        if ( 1 > _p._kbMANumOfPeriods )
            return false;
        
        return true;
    }
    
    bool isStateChanged(const TBars& bars) {
        if ( !isEnabled() )
            return false;
        
        clear();                
        if ( bars.size() < 2 )
            return false;
        
        const TBar& bar = bars[bars.size()-2];
        if ( bar._index == _p._kbProcessdBarIndex )
            return false;
        
         _p._kbProcessdBarIndex = bar._index;

        if ( !calculate(bars) )
            return false;
        
        KeltnerBandStateInfo stateInfo;
        
        const KeltnerBandInfo& info = _p._kbInfos.back();
        if ( bar._high.toDouble() > info._upperValue && ePatternDir::kDown != bar._dir ) {
            stateInfo._bias = eTradeBias::kLong;
            stateInfo._extremeBarHighLowPrice = bar._high;
        } else if ( bar._low.toDouble() < info._lowerValue && ePatternDir::kUp != bar._dir ) {
            stateInfo._bias = eTradeBias::kShort;
            stateInfo._extremeBarHighLowPrice = bar._low;
        } else {
            _s << "KeltnerBandProcessor: NO STATE CHANGE - bar is inside keltner band ==> "
               << "bias=" << _p._kbBias.toString()
               << ",info=" <<  info.toString()
               << ",bar=" <<  bar.toString();
            _p._kbReason = _s.str();
            _p._kbBarPositionToKeltnerBand = eBarPositionToKeltnerBand::kMiddle;
            return false;
        }        
        
        switch ( _p._kbStateInfos.size() ) {
            case 0:
            case 1:
                _p._kbBias = stateInfo._bias;
                if ( 0 == _p._kbStateInfos.size() ) {
                    _p._kbStateInfos.push_back(stateInfo);
                } else {
                    if ( _p._kbStateInfos.back()._bias == stateInfo._bias ) {
                        checkIsSameBreak(stateInfo);
                        
                        _s << "KeltnerBandProcessor: NO STATE CHANGE - same trading bias ==> stateInfo=" << stateInfo.toString()
                           << ",bias=" << _p._kbBias.toString()
                           << ",info=" <<  info.toString()
                           << ",bar=" <<  bar.toString();
                        _p._kbReason = _s.str();
                        return false;
                    }
                    
                    _p._kbStateInfos.push_back(stateInfo);
                }
                
                _s << "KeltnerBandProcessor: STATE CHANGE ==> stateInfo=" << stateInfo.toString()
                   << ",bias=" << _p._kbBias.toString()
                   << ",info=" <<  info.toString()
                   << ",bar=" <<  bar.toString();
                
                _p._kbReason = _s.str();
                break;
            case 2:
                if ( _p._kbStateInfos.back()._bias == stateInfo._bias ) {
                    if ( checkIsSameBreak(stateInfo) ) {
                        _s << "KeltnerBandProcessor: NO STATE CHANGE (checkIsSameBreak()==true) ==> "
                           << "bias=" << _p._kbBias.toString()
                           << ",info=" <<  info.toString()
                           << ",stateInfo=" <<  stateInfo.toString()
                           << ",bar=" <<  bar.toString();    
                                
                        _p._kbReason = _s.str();
                        return false;
                    }
                    _p._kbStateInfos.clear();
                    _p._kbStateInfos.push_back(stateInfo);
                } else {
                    _p._kbBias = eTradeBias::kNoTrades;
                    _p._kbStateInfos.push_back(stateInfo);
                    _s << "KeltnerBandProcessor: STATE CHANGE to kNoTrades ==> stateInfo=" << stateInfo.toString()
                       << ",bias=" << _p._kbBias.toString()
                       << ",info=" <<  info.toString()
                       << ",bar=" <<  bar.toString();
                    
                    _p._kbReason = _s.str();
                }
                break;
            default:
            {
                if ( _p._kbStateInfos.back()._bias == stateInfo._bias ) {
                    if ( checkIsSameBreak(stateInfo) ) {
                        _s << "KeltnerBandProcessor: NO STATE CHANGE (checkIsSameBreak()==true) from kNoTrades ==> "
                           << "bias=" << _p._kbBias.toString()
                           << ",info=" <<  info.toString()
                           << ",stateInfo=" <<  stateInfo.toString()
                           << ",bar=" <<  bar.toString();    
                                
                        _p._kbReason = _s.str();
                        return false;
                    }
                    
                    const KeltnerBandStateInfo& stateInfoOld = ((eTradeBias::kLong == stateInfo._bias) ? ((_p._kbStateInfos[0]._extremeBarHighLowPrice > _p._kbStateInfos[2]._extremeBarHighLowPrice) ? _p._kbStateInfos[0] : _p._kbStateInfos[2]) : ((_p._kbStateInfos[0]._extremeBarHighLowPrice < _p._kbStateInfos[2]._extremeBarHighLowPrice) ? _p._kbStateInfos[0] : _p._kbStateInfos[2]));
                    if ( !checkStateChangeForPattern(bar, stateInfoOld, stateInfo, info) )
                        return false;
                } else {                    
                    if ( !checkStateChangeForPattern(bar, _p._kbStateInfos[1], stateInfo, info) ) 
                        return false;
                }
            }
                break;
        }
        
        _p._kbBarPositionToKeltnerBand = (eTradeBias::kLong == stateInfo._bias)  ? eBarPositionToKeltnerBand::kHigh : eBarPositionToKeltnerBand::kLow;
        return true;
    }
    
private:
    bool checkIsSameBreak(const KeltnerBandStateInfo& stateInfo) {
        if ( ((eTradeBias::kLong == stateInfo._bias) && (eBarPositionToKeltnerBand::kHigh == _p._kbBarPositionToKeltnerBand) && stateInfo._extremeBarHighLowPrice > _p._kbStateInfos.back()._extremeBarHighLowPrice)
             || ((eTradeBias::kShort == stateInfo._bias) && (eBarPositionToKeltnerBand::kLow == _p._kbBarPositionToKeltnerBand) && stateInfo._extremeBarHighLowPrice < _p._kbStateInfos.back()._extremeBarHighLowPrice) ) {
            _p._kbStateInfos.back()._extremeBarHighLowPrice = stateInfo._extremeBarHighLowPrice;
            return true;
        }
        
        return false;
    }
    
    bool checkStateChangeForPattern(const TBar& bar, const KeltnerBandStateInfo& stateInfoOld, const KeltnerBandStateInfo& stateInfoNew, const KeltnerBandInfo& info) {
        bool status = false;
        if ( (eTradeBias::kLong == stateInfoOld._bias && stateInfoOld._extremeBarHighLowPrice < bar._close) || (eTradeBias::kShort == stateInfoOld._bias && stateInfoOld._extremeBarHighLowPrice > bar._close) ) {
            _p._kbBias = stateInfoNew._bias;
            _p._kbStateInfos.clear();
            _p._kbStateInfos.push_back(stateInfoNew);
            _s << "KeltnerBandProcessor: STATE CHANGE from kNoTrades ==> ";
            
            status = true;
        } else {
            _s << "KeltnerBandProcessor: NO STATE CHANGE from kNoTrades ==> ";
        }
        
        _s << "bias=" << _p._kbBias.toString()
           << ",info=" <<  info.toString()
           << ",stateInfoOld=" <<  stateInfoOld.toString()
           << ",stateInfoNew=" <<  stateInfoNew.toString()
           << ",bar=" <<  bar.toString();
        
        _p._kbReason = _s.str();
        return status;
    }
    
    bool calculate(const TBars& bars) {
        size_t size = bars.size()-1;
        if ( size < _p._kbMANumOfPeriods ) {
            _p._kbReason = "KeltnerBandProcessor: size < _p._kbMANumOfPeriods";
            return false;
        }

        const TBar& bar = bars[size-1];
        if ( !bar._close.isValid() ) {
            _p._kbReason = "KeltnerBandProcessor: !bars.back()._close.isValid()";
            return false;
        }
        
        KeltnerBandInfo info;
        info._barIndex = bar._index;
        
        size_t count = 0;
        for ( size_t i = (size-_p._kbMANumOfPeriods); i < size; ++i ) {
            const TBar& bar = bars[i];
            if ( bar._close.isValid() ) {
                ++count;
                info._middleValue += bar._close.toDouble();
            }
        }
        
        if ( 0 == count ) {
            _p._kbReason = "KeltnerBandProcessor: NO valid bars found to calculate";
            return false;
        }
        
        info._middleValue /= count;
        info._atr = calculateAtr(bars, _p._kbAtrNumOfPeriods, false);
        info._upperValue = info._middleValue + _p._kbAtrMult*info._atr;
        info._lowerValue = info._middleValue - _p._kbAtrMult*info._atr;
        
        _p._kbInfos.push_back(info);
        
        return true;
    }
    
private:
    KeltnerBandProcessorParamsWire& _p;
    tw::common_str_util::TFastStream _s;
};

} // common_trade
} // tw
