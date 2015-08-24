#pragma once

#include <stdint.h>
#include <limits>
#include <iostream>
#include <algorithm>
#include <math.h>

#include <boost/lexical_cast.hpp>

namespace tw {
namespace common {

template < typename TType, bool TTypeDefault=false>
class TypeWrap 
{
public:
    typedef TType type;
    typedef TypeWrap<TType, TTypeDefault> THIS;
    
    static type INVALID_VALUE() {
        return std::numeric_limits<type>::min();
    }
    
    static bool isValid(const type& value) {
        return value != INVALID_VALUE();
    }
    
    static THIS min(const TypeWrap& rhs, const TypeWrap& lhs) {
        return THIS(std::min(rhs.get(), lhs.get()));
    }
    
    static THIS max(const TypeWrap& rhs, const TypeWrap& lhs) {
        return THIS(std::max(rhs.get(), lhs.get()));
    }
    
public:
    explicit TypeWrap () {
        clear();
    }

    explicit TypeWrap (type value) {
        set(value);
    }        
    
    TypeWrap (const TypeWrap& rhs) {
        *this = rhs;
    }
    
    TypeWrap& operator=(const TypeWrap& rhs) {
        if ( this != &rhs )
            _value = rhs._value;
        
        return *this;
    }
    
    void clear() {
        if ( TTypeDefault )
            _value = type();
        else
            _value = INVALID_VALUE();
    }
    
    bool isValid() const {
        return _value != INVALID_VALUE();
    }
    
    bool isGapped(const TypeWrap& v, double gap) const {
        if ( !isValid() || !v.isValid() )
            return false;
        
        TypeWrap delta(_value-v._value);
        double d = (delta.abs().toDouble()/v.toDouble())*100.0;
        return (d > gap);
    }

    const type get() const {
        return _value;
    }
    
    TypeWrap abs() const {
        if ( !isValid() )
            return TypeWrap(_value);
        
        return _value > 0 ? TypeWrap(_value) : TypeWrap(-_value);
    }
    
    TypeWrap pow(double exp) const {
        if ( !isValid() )
            return TypeWrap(_value);
        
        TypeWrap v;
        v.fromDouble(::pow(toDouble(), exp));
        
        return v;
    }
    
    operator type() const {
        return get();
    }

    double toDouble() const {
        return static_cast<double>(get());
    }
    
    void fromDouble(double value) {
        set(static_cast<type>(value));
    }
    
    void set(type value) {
        _value = value;
    }
    
    std::string toString() const {
        std::string s;
        
        if ( isValid() ) {
            s = ::boost::lexical_cast<std::string>(get());
        } else {
            s = "InV";
        }
        
        return s;
    }
    
    bool fromString(const std::string& s) {
        try {
            if ( s == "InV" )
                clear();
            else
                _value = boost::lexical_cast<TType>(s);
        } catch(...) {
            return false;
        }

        return true;
        
    }
       
    // Add/subtract operators for ticks
    //
    TypeWrap operator+(const type& rhs) const {
        if ( isValid() && isValid(rhs) )
            return TypeWrap(get() + rhs); 
        
        return TypeWrap(INVALID_VALUE());
    }
    
    TypeWrap operator+(const TypeWrap& rhs) const {
        if ( isValid() && rhs.isValid() )
            return TypeWrap(get() + rhs.get()); 
        
        return TypeWrap(INVALID_VALUE());
    }    
    
    TypeWrap operator-(const type& rhs) const {
        if ( isValid() && isValid(rhs) )
            return TypeWrap(get() - rhs); 
        
        return TypeWrap(INVALID_VALUE());
    }
    
    TypeWrap operator-(const TypeWrap& rhs) const {
        if ( isValid() && rhs.isValid() )
            return TypeWrap(get() - rhs.get()); 
        
        return TypeWrap(INVALID_VALUE());
    }    

    TypeWrap& operator+=(const type& rhs) {
        if ( isValid() && isValid(rhs) )
            _value += rhs;
        else
            clear();
        
        return *this;
    }
    
    TypeWrap& operator+=(const TypeWrap& rhs) {
        if ( isValid() && rhs.isValid() )
            _value += rhs.get();
        else
            clear();
        
        return *this;
    }

    TypeWrap& operator-=(const type& rhs) {
        if ( isValid() && isValid(rhs) )
            _value -= rhs;
        else
            clear();
        
        return *this;
    }
    
    TypeWrap& operator-=(const TypeWrap& rhs) {
        if ( isValid() && rhs.isValid() )
            _value -= rhs.get();
        else
            clear();
        
        return *this;
    }
    
    // Multiply operators for ticks
    //
    TypeWrap operator*(const type& rhs) {
        if ( isValid() && isValid(rhs) )
            return TypeWrap(get() * rhs);
        
        return TypeWrap(INVALID_VALUE());
    }
    
    TypeWrap& operator*=(const type& rhs) {
        if ( isValid() && isValid(rhs) )
            _value *= rhs;
        else
            clear();
        
        return *this;
    }
    
    TypeWrap operator*(const TypeWrap& rhs) {
        if ( isValid() && rhs.isValid() )
            return TypeWrap(get() * rhs.get()); 
        
        return TypeWrap(INVALID_VALUE());
    }
    
    TypeWrap& operator*=(const TypeWrap& rhs) {
        if ( isValid() && rhs.isValid() )
            _value *= rhs.get();
        else
            clear();
        
        return *this;
    }

    // Divide operators for ticks
    //
    TypeWrap operator/(const type& rhs) {
        if ( isValid() && isValid(rhs) )
            return TypeWrap(get() / rhs); 
        
        return TypeWrap(INVALID_VALUE());
    }
    
    TypeWrap& operator/=(const type& rhs) {
        if ( isValid() && isValid(rhs) )
            _value /= rhs;
        else
            clear();
            
        return *this;
    }
    
    TypeWrap operator/(const TypeWrap& rhs) {
        if ( isValid() && rhs.isValid() )
            return TypeWrap(get() / rhs.get());
        
        return TypeWrap(INVALID_VALUE());
    }
    
    TypeWrap& operator/=(const TypeWrap& rhs) {
        if ( isValid() && rhs.isValid() )
            _value /= rhs.get();
        else
            clear();
        
        return *this;
    }

    // Increment/decrement operators
    //
    TypeWrap& operator++() {
        if ( isValid() )
            ++_value;
            
        return *this;
    }

    TypeWrap& operator--() {
        if ( isValid() )
            --_value;
        
        return *this;
    }
    
    TypeWrap operator-()const {
        if ( isValid() )
            return TypeWrap(-1 * _value);
        
        return TypeWrap(INVALID_VALUE());
    }

    bool operator==(const TypeWrap& rhs) const {
        return _value == rhs.get();
    }

    bool operator!=(const TypeWrap& rhs) const {
        return _value != rhs.get();
    }

    bool operator>(const TypeWrap& rhs) const {
        return _value > rhs.get();
    }

    bool operator<(const TypeWrap& rhs) const {
        return _value < rhs.get();
    }    
    
    friend std::ostream& operator<<(std::ostream& ostream, const TypeWrap& rhs) {
        return (ostream << rhs.toString());
    }
    
    friend bool operator>>(std::istream& istream, TypeWrap& rhs) {
        try {
            std::string s;
            istream >> s;
            
            return rhs.fromString(s);
        } catch(...) {
            return false;
        }

        return true;
    }

private:
    type _value;
};

} // namespace common
} // namespace tw
