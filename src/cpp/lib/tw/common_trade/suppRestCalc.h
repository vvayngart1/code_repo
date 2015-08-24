#pragma once

#include <tw/common/defs.h>
#include <tw/price/defs.h>

namespace tw {
namespace common_trade {
    
class SuppRestCalc {
public:
    
    typedef tw::price::Ticks TTicks;
    
    static bool calcPP(const TTicks& h, const TTicks& l, const double& c, double& x) {
        x = 0.0;
        if ( !h.isValid() || !l.isValid() )
            return false;
        
        x = (h.toDouble()+l.toDouble()+c) / 3.0;
        return true;
    }
    
    static bool calcS1(const TTicks& h, const double& pp, double& x) {
        return calcS1R1(h, pp, x);
    }
    
    static bool calcR1(const TTicks& l, const double& pp, double& x) {
        return calcS1R1(l, pp, x);
    }
    
    static bool calcS2(const TTicks& h, const TTicks& l, const double& pp, double& x) {
        return calcS2R2(h, l, pp, -1, x);
    }
    
    static bool calcR2(const TTicks& h, const TTicks& l, const double& pp, double& x) {
        return calcS2R2(h, l, pp, 1, x);
    }
    
private:
    static bool calcS1R1(const TTicks& hl, const double& pp, double& x) {
        x = 0.0;
        if ( !hl.isValid() || 0.0 == pp )
            return false;
        
        x = 2.0 * pp - hl.toDouble();
        return true;
    }
    
    static bool calcS2R2(const TTicks& h, const TTicks& l, const double& pp, short sign, double& x) {
        x = 0.0;
        if ( !h.isValid() || !l.isValid() || 0.0 == pp )
            return false;
        
        x = pp + sign*(h.toDouble() - l.toDouble());
        return true;
    }
};

} // common_trade
} // tw
