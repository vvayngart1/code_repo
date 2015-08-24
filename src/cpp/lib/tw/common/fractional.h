#pragma once

#include <tw/common/defs.h>
#include <stdio.h>

namespace tw {
namespace common {
    // Very CRUDE implementation of convertion from fraction
    // to num/denom - precision is limited by the value of CM!
    //
    class Fractional {
    public:
        typedef std::pair<uint32_t, uint32_t> TFraction;
        static const uint64_t CM = 1000000000;
        
        static uint64_t GCD(uint64_t numer, uint64_t denom) {
            while(true) {
                int rem = denom%numer;
                if(rem == 0) return numer;
                denom = numer;
                numer = rem;
            }
        }
        
        static TFraction getFraction(double d) {
            TFraction fraction;
            
            uint64_t num = (uint64_t)(d * CM);
            uint64_t den = CM;
            uint64_t gcd = GCD(num,den);
            num /= gcd;
            den /= gcd;
            
            fraction.first = static_cast<TFraction::first_type>(num);
            fraction.second = static_cast<TFraction::second_type>(den);
            
            return fraction;
        }
        
        // TODO: right now precision is between 0 and 7 (per CME, maximum
        // precision can be up to 7 characters)
        //
        static uint32_t getPrecision(uint64_t numer, uint64_t denom) {
            static double EPSILON = 0.000000005;
            uint32_t precision = 0;
            
            if ( denom > 1) {
                double d = static_cast<double>(numer)/static_cast<double>(denom);
                char buffer[256];
                sprintf(buffer, "%.7f", d+EPSILON);
                char* iter = buffer + ::strlen(buffer) - 1;
                char* end = iter - 7;
                
                precision = 7;
                for ( ; iter != end; --iter ) {
                    if ( *iter != '0' )
                        break;
                    --precision;
                }                
            }
            
            return precision;
        }
    };

} // namespace common
} // namespace tw
