#pragma once

#include <tw/common/defs.h>
#include <tw/price/defs.h>

#include <numeric>
#include <deque>

namespace tw {
namespace common_trade {
    
class Atr {
public:
    typedef tw::price::Ticks TTicks;
    typedef std::deque<TTicks> TRanges;
    
public:
    Atr() {
        clear();
    }
    
    void clear() {
        _atr = 0.0;
        _size = 0.0;
        _ranges.clear();
        _dirty = false;
    }
    
public:
    void setNumOfPeriods(const uint16_t size) {
        if ( 0 == size )
            return;
        
        if ( _ranges.size() > size ) {
            _ranges.erase(_ranges.begin(), _ranges.begin()+(_ranges.size() - size));
            recalculate();
        }
        
        _size = size;
    }
    
    uint16_t getNumOfPeriods() const {
        return _size;
    }
    
    double getAtr() {
        if ( _dirty )
            recalculate();
        
        return _atr;
    }
    
    void addRange(const TTicks& r) {
        if ( 0 == _size )
            return;
        
        if ( _size == _ranges.size() )
            _ranges.pop_front();
        
        _ranges.push_back(r);
        _dirty = true;
    }
    
private:
    void recalculate() {
        if ( _ranges.empty() ) {
            _atr = 0.0;
            return;
        }
        
        _atr = std::accumulate(_ranges.begin(), _ranges.end(), TTicks(0)).toDouble() / _ranges.size();
    }
    
private:
    double _atr;
    uint16_t _size;
    TRanges _ranges;
    bool _dirty;
};

} // common_trade
} // tw
