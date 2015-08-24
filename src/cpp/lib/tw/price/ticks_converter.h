#pragma once

#include <tw/instr/fwdDefs.h>
#include <tw/price/defs.h>
#include <tw/functional/delegate.hpp>

namespace tw {
namespace price {

// NOTE:  this class might need to be made a base class if more than
// one kind of tick converters are needed, for example, if there are
// different tick sizes based on price levels for the same instrument
// as in the case of some Korean exchanges
//

class TicksConverter 
{
    static const double _g_epsilon;
    
public:
    typedef tw::functional::delegate1<Ticks, const double&> TConverterFrom;
    typedef tw::functional::delegate1<double, const Ticks&> TConverterTo;
    typedef tw::functional::delegate1<double, const double&> TConverterToFractional;
    typedef tw::functional::delegate1<double, const double&> TConverterFractional;
    
    // Create NULL converters
    //
    static TConverterFrom createFrom() {
        static ConverterNull _converterNull;
        return TConverterFrom::from_method<ConverterNull, &ConverterNull::fromExchangePrice>(&_converterNull);
    }    
    
    static TConverterTo createTo() {
        static ConverterNull _converterNull;
        return TConverterTo::from_method<ConverterNull, &ConverterNull::toExchangePrice>(&_converterNull);
    }
    
    static TConverterToFractional createToFractional() {
        static ConverterNull _converterNull;
        return TConverterFractional::from_method<ConverterNull, &ConverterNull::toFractionalExchangePrice>(&_converterNull);
    }
    
    static TConverterFractional createFractional() {
        static ConverterNull _converterNull;
        return TConverterFractional::from_method<ConverterNull, &ConverterNull::toFractionalTicks>(&_converterNull);
    }
        
public:
    TicksConverter(tw::instr::InstrumentConstPtr instrument);
    void resetWithNewInstrument(tw::instr::InstrumentConstPtr instrument);
    
public:
    // Conversion functions from fractional ticks
    //
    
    // returns the nearest tick to the given fractionalTicks, within a given tolerance
    //
    Ticks nearestTick(double fractionalTicks);
    
    // returns the nearest tick to the given fractionalTicks, within a given tolerance
    // Tolerance range is [-infinity, +infinity].  Semantics of calculations are:
    //     If tol > 0.0, return floor(fractionalTicks+tol)
    //     else, return ceil(fractionalTicks+tol)
    //  Look at unit tests(test/unit_tests) for possible uses with edges
    //
    Ticks nearestTick(double fractionalTicks, double tol);
    
    // returns the nearest tick above the given fractionalTicks
    // if the price is within tol of a tick, it returns that tick.  otherwise
    // it returns the tick above
    //
    Ticks nearestTickAbove(double fractionalTicks);

    // returns the nearest tick below the given fractionalTicks
    // if the price is within tol of a tick, it returns that tick.  otherwise
    // it returns the tick below
    //
    Ticks nearestTickBelow(double fractionalTicks);

    // returns the next tick above the given fractionalTicks
    //
    Ticks nextTickAbove(double fractionalTicks);

    // returns the next tick below the given fractionalTicks
    //
    Ticks nextTickBelow(double fractionalTicks);

    // returns the tick n ticks above the given fractionalTicks
    // this is equivalent to n calls to nextTickAbove
    //
    Ticks nTicksAbove(double fractionalTicks, Ticks n);

    // returns the tick n ticks below the given fractionalTicks
    // this is equivalent to n calls to nextTickBelow
    //
    Ticks nTicksBelow(double fractionalTicks, Ticks n);
    
    // Conversion functions for double prices received/send from/to exchange
    //
    
    // NOTE: assumption is made that the double prices are multiples of ticks
    // and double prices are used only to convert to ticks upon reception from
    // market feed and converted from ticks to be send to exchange in orders.
    // That means that all internal price manipulations are either done in Ticks
    // or fractional ticks represented by doubles.  E.g.:
    //          price feed price from exchange: 1237.50 (double representing exchange price for ES, tick size = 0.25)
    //          fromExchangePrice(1237.50) = 1237.50 / 0.25 = 4950 ticks
    //          calculate fair value = 4950.678 (double representing fractional ticks)
    //          send an order 1 tick below to exchange: toExchangeTicks(nTicksBelow(4950.678, 1)) = 1237.25
    //
    
    // NOTE: assumption is made - for conversion functions for double prices
    // received/send from/to exchange - that tick numerator/denominator combo
    // is the same for prices coming from price feed and prices send in orders.
    // If this assumption is incorrect, implementation needs to change to know
    // which numerator/denominator combo to use when
    //
    
    // Use this method to convert prices received from exchange
    // (e.g.: from price feed or matching engine) to ticks
    //
    Ticks fromExchangePrice(const double& price) {
        return _convertFrom(price);
    }
    
    // Use this method to convert ticks to prices to send to exchange
    // (e.g.: matching engine)
    //
    double toExchangePrice(const Ticks& ticks) {
        return _convertTo(ticks);
    }
    
    // Use this method to convert fractional ticks to prices in exchange format
    // (e.g.: matching engine)
    //
    double toFractionalExchangePrice(const double& price) {
        return _convertToFractional(price);
    }
    
    // Use this method to calculate fractional number
    // of ticks (e.g. for avgPrice - fill._price)
    //
    double fractionalTicks(const double& price) {
        return _convertFractional(price);
    }
    
public:
    const TConverterFrom& getConverterFrom() const {
        return _convertFrom;
    }
    
    const TConverterTo& getConverterTo() const {
        return _convertTo;
    }
    
    const TConverterToFractional& getConverterToFractional() const {
        return _convertToFractional;
    }
    
private:
    // Optimized converters for different numerator/denominator values
    //
    class ConverterNull {
    public:
        Ticks fromExchangePrice(const double& price) {
            return Ticks(static_cast<Ticks::type>(price));
        }
        
        double toExchangePrice(const Ticks& ticks) {
            return ticks.toDouble();
        }
        
        double toFractionalExchangePrice(const double& price) {
            return price;
        }
        
        double toFractionalTicks(const double& price) {
            return price;
        }
    };

    class ConverterNum {
    public:
        ConverterNum() {            
            set(1);
        }
        
        void set(uint32_t value) {
            _num = value;
        }
        
    public:
        Ticks fromExchangePrice(const double& price) {
            return Ticks(static_cast<Ticks::type>(toFractionalTicks(price)+_g_epsilon));
        }
        
        double toExchangePrice(const Ticks& ticks) {
            return ticks*_num;
        }
        
        double toFractionalExchangePrice(const double& price) {
            return price*_num;
        }
        
        double toFractionalTicks(const double& price) {
            return price/_num;
        }
        
    private:
        uint32_t _num;
    };
    
    class ConverterDenom {
    public:
        ConverterDenom() {            
            set(1);
        }
        
        void set(uint32_t value) {
            _denom = value;
        }
        
    public:
        Ticks fromExchangePrice(const double& price) {
            return Ticks(static_cast<Ticks::type>(toFractionalTicks(price)+_g_epsilon));
        }
        
        double toExchangePrice(const Ticks& ticks) {
            return static_cast<double>(ticks)/_denom;
        }
        
        double toFractionalExchangePrice(const double& price) {
            return price/_denom;
        }
        
        double toFractionalTicks(const double& price) {
            return (price*_denom);
        }
        
    private:
        uint32_t _denom;
    };
    
    class ConverterFull {
    public:
        ConverterFull() {
            set(1, 1);
        }
                      
        void set(uint32_t num, uint32_t denom) {
            _num = num;
            _denom = denom;
        }
        
    public:
        Ticks fromExchangePrice(const double& price) {
            return Ticks(static_cast<Ticks::type>(toFractionalTicks(price)+_g_epsilon));
        }
        
        double toExchangePrice(const Ticks& ticks) {
            return static_cast<double>(ticks)*_num/_denom;
        }
        
        double toFractionalExchangePrice(const double& price) {
            return price*_num/_denom;
        }
        
        double toFractionalTicks(const double& price) {
            return ((price*_denom)/_num);
        }
        
    private:
        uint32_t _num;
        uint32_t _denom;
    };
    
private:
    template<typename TClient>
    TConverterFrom createFrom(TClient* client) {
        return TConverterFrom::from_method<TClient, &TClient::fromExchangePrice>(client);
    }
    
    template<typename TClient>
    TConverterTo createTo(TClient* client) {
        return TConverterTo::from_method<TClient, &TClient::toExchangePrice>(client);
    }
    
    template<typename TClient>
    TConverterToFractional createToFractional(TClient* client) {
        return TConverterToFractional::from_method<TClient, &TClient::toFractionalExchangePrice>(client);
    }
    
    template<typename TClient>
    TConverterFractional createFractional(TClient* client) {
        return TConverterFractional::from_method<TClient, &TClient::toFractionalTicks>(client);
    }

private:    
    ConverterNum _converterNum;
    ConverterDenom _converterDenom;
    ConverterFull _converterFull;
    
    TConverterFrom _convertFrom;
    TConverterTo _convertTo;
    TConverterToFractional _convertToFractional;
    TConverterFractional _convertFractional;
};

} // namespace price
} // namesapce tw
