#include <stdexcept>

#include <tw/math/math.h>
#include <tw/generated/instrument.h>
#include <tw/price/ticks_converter.h>

namespace tw {
namespace price {

// Allows precision to 4th decimal place
//
const double TicksConverter::_g_epsilon = 0.0001;

TicksConverter::TicksConverter(tw::instr::InstrumentConstPtr instrument) {
    resetWithNewInstrument(instrument);
}

void TicksConverter::resetWithNewInstrument(tw::instr::InstrumentConstPtr instrument) {
    if ( NULL == instrument )
        throw std::invalid_argument("invalid instrument: NULL");
                
    if ( !instrument->isValid() )
        throw std::invalid_argument("invalid instrument: " + instrument->toString());
    
    if ( instrument->_tickNumerator == instrument->_tickDenominator ) {
        _convertFrom = createFrom();
        _convertTo = createTo();
        _convertToFractional = createToFractional();
        _convertFractional = createFractional();
    } else if ( 1 == instrument->_tickDenominator ) {
        _converterNum.set(instrument->_tickNumerator);
        _convertFrom = createFrom(&_converterNum);
        _convertTo = createTo(&_converterNum);
        _convertToFractional = createToFractional(&_converterNum);
        _convertFractional = createFractional(&_converterNum);
    } else if ( 1 == instrument->_tickNumerator ) {
        _converterDenom.set(instrument->_tickDenominator);
        _convertFrom = createFrom(&_converterDenom);
        _convertTo = createTo(&_converterDenom);
        _convertToFractional = createToFractional(&_converterDenom);
        _convertFractional = createFractional(&_converterDenom);
    } else {
        _converterFull.set(instrument->_tickNumerator, instrument->_tickDenominator);
        _convertFrom = createFrom(&_converterFull);
        _convertTo = createTo(&_converterFull);
        _convertToFractional = createToFractional(&_converterFull);
        _convertFractional = createFractional(&_converterFull);
    }
}
 
Ticks TicksConverter::nearestTick(double fractionalTicks) {
    return Ticks(static_cast<Ticks::type>(tw::math::rint(fractionalTicks))); 
} 

Ticks TicksConverter::nearestTick(double fractionalTicks, double tol) {
    if ( tol > 0.0 )
        return nearestTickAbove(fractionalTicks+tol);

    return nearestTickBelow(fractionalTicks+tol);
}
    
Ticks TicksConverter::nearestTickAbove(double fractionalTicks) {
    return Ticks(static_cast<Ticks::type>(tw::math::ceil(fractionalTicks-_g_epsilon)));
}

Ticks TicksConverter::nearestTickBelow(double fractionalTicks) {
    return Ticks(static_cast<Ticks::type>(tw::math::floor(fractionalTicks+_g_epsilon)));
}

Ticks TicksConverter::nextTickAbove(double fractionalTicks) {
    return ++(nearestTickAbove(fractionalTicks));
}

Ticks TicksConverter::nextTickBelow(double fractionalTicks) {
    return --(nearestTickBelow(fractionalTicks));
}

Ticks TicksConverter::nTicksAbove(double fractionalTicks, Ticks n) {
    if ( n < 0 )
        return nTicksBelow(fractionalTicks, -n);
    
    return nearestTickAbove(fractionalTicks)+n;
}

Ticks TicksConverter::nTicksBelow(double fractionalTicks, Ticks n) {
    if ( n < 0 )
        return nTicksAbove(fractionalTicks, -n);
    
    return nearestTickBelow(fractionalTicks)-n;
}


} // namespace price
} // namespace tw
