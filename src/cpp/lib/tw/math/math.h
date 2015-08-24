#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <math.h>

namespace tw {
namespace math {

// Uitility functions to compare numbers wihtin tolerance
//
template <typename TType, typename TTolType> inline int compare(TType a, TType b, TTolType tol) {
    TTolType diff = a - b;

    if ( diff > tol )
        return 1;
    
    if ( diff < -tol )
        return -1;
    
    return 0;
}

template <typename TTolType> inline bool isZero(TTolType a, TTolType tol) { return (a < tol && a > -tol); }
template <typename TType, typename TTolType> inline bool eq(TType a, TType b, TTolType tol)     { return isZero<TTolType>(a - b, tol); }
template <typename TType, typename TTolType> inline bool neq(TType a, TType b, TTolType tol)    { return !eq(a, b, tol); }
template <typename TType, typename TTolType> inline bool lt(TType a, TType b, TTolType tol)     { return (a + tol < b); }
template <typename TType, typename TTolType> inline bool gt(TType a, TType b, TTolType tol)     { return (a - tol > b); }
template <typename TType, typename TTolType> inline bool lte(TType a, TType b, TTolType tol)    { return (a + tol <= b); }
template <typename TType, typename TTolType> inline bool gte(TType a, TType b, TTolType tol)    { return (a - tol >= b); }

// Utility functions to have uniformed interface for rounding numbers
//
template <typename T> inline T abs(T value) { return value; }
template <> inline int           abs(int           value) { return ::abs(value); }
template <> inline long int      abs(long int      value) { return ::labs(value); }
template <> inline long long int abs(long long int value) { return ::llabs(value); }
template <> inline float       abs(float       value) { return ::fabsf(value); }
template <> inline double      abs(double      value) { return ::fabs (value); }
template <> inline long double abs(long double value) { return ::fabsl(value); }

template <typename T> inline T floor(T value) { return value; }
template <> inline float       floor(float       value) { return ::floorf(value); }
template <> inline double      floor(double      value) { return ::floor (value); }
template <> inline long double floor(long double value) { return ::floorl(value); }

template <typename T> inline T ceil(T value) { return value; }
template <> inline float       ceil(float       value) { return ::ceilf(value); }
template <> inline double      ceil(double      value) { return ::ceil (value); }
template <> inline long double ceil(long double value) { return ::ceill(value); }

template <typename T> inline T rint(T value) { return value; }
template <> inline float       rint(float       value) { return ::rintf(value); }
template <> inline double      rint(double      value) { return ::rint (value); }
template <> inline long double rint(long double value) { return ::rintl(value); }

template <typename T> inline long int lrint(T value) { return value; }
template <> inline long int lrint(float       value) { return ::lrintf(value); }
template <> inline long int lrint(double      value) { return ::lrint (value); }
template <> inline long int lrint(long double value) { return ::lrintl(value); }

template <typename T> inline long long int llrint(T value) { return value; }
template <> inline long long int llrint(float       value) { return ::llrintf(value); }
template <> inline long long int llrint(double      value) { return ::llrint (value); }
template <> inline long long int llrint(long double value) { return ::llrintl(value); }

template <typename T> inline T seqfloor(T value) { return value; }
template <> inline float       seqfloor(float       value) { return value<0 ? math::ceil(value) : math::floor(value); }
template <> inline double      seqfloor(double      value) { return value<0 ? math::ceil(value) : math::floor(value); }
template <> inline long double seqfloor(long double value) { return value<0 ? math::ceil(value) : math::floor(value); }

template <typename T> inline T seqceil(T value) { return value; }
template <> inline float       seqceil(float       value) { return value<0 ? math::floor(value) : math::ceil(value); }
template <> inline double      seqceil(double      value) { return value<0 ? math::floor(value) : math::ceil(value); }
template <> inline long double seqceil(long double value) { return value<0 ? math::floor(value) : math::ceil(value); }

template <typename T> inline T div(T a, T b) { return a/b; }
template <> inline float       div(float       a, float       b) { return math::seqfloor(a/b); }
template <> inline double      div(double      a, double      b) { return math::seqfloor(a/b); }
template <> inline long double div(long double a, long double b) { return math::seqfloor(a/b); }

template <typename T> inline T seqdiv(T a, T b) { return (a<0)?(a-b+1)/b:a/b; }
template <> inline float       seqdiv(float       a, float       b) { return math::floor(a/b); }
template <> inline double      seqdiv(double      a, double      b) { return math::floor(a/b); }
template <> inline long double seqdiv(long double a, long double b) { return math::floor(a/b); }

template <typename T> inline T mod(T a, T b) { return a%b; }
template <> inline float       mod(float       a, float       b) { return a-b*math::div(a,b); }
template <> inline double      mod(double      a, double      b) { return a-b*math::div(a,b); }
template <> inline long double mod(long double a, long double b) { return a-b*math::div(a,b); }

template <typename T> inline T seqmod(T a, T b) { return (a<0)?mod(a+1,b)+(b-1):mod(a,b); }
template <> inline float       seqmod(float       a, float       b) { return a-b*math::seqdiv(a,b); }
template <> inline double      seqmod(double      a, double      b) { return a-b*math::seqdiv(a,b); }
template <> inline long double seqmod(long double a, long double b) { return a-b*math::seqdiv(a,b); }

template <typename T, typename U> inline T roundunit(T value, U unit) { return math::rint(value / unit) * unit; }
template <typename T, typename G> inline T roundgrain(T value, G grain) { return math::rint(value * grain) / grain; }
template <typename T, typename U> inline T ceilunit(T value, U unit) { return math::ceil(value / unit) * unit; }
template <typename T, typename G> inline T ceilgrain(T value, G grain) { return math::ceil(value * grain) / grain; }
template <typename T, typename U> inline T floorunit(T value, U unit) { return math::floor(value / unit) * unit; }
template <typename T, typename G> inline T floorgrain(T value, G grain) { return math::floor(value * grain) / grain; }

// Utility functions to have uniformed interface to rounding numbers within given tolerance
//
template <typename T> inline T ceil(T a, T tol) {
    const T ra = math::rint(a);
    const T d = a - ra;
    if ( 0 <= d && d < tol )
        return ra;
    else
        return math::ceil(a);
}

template <typename T> inline T floor(T a, T tol) {
    const T ra = math::rint(a);
    const T d = ra - a;
    if ( 0 <= d && d < tol )
        return ra;
    else
        return math::floor(a);
}

template <typename T, typename G> inline T ceilgrain(T a, G grain, T tol) {
    return math::ceil(a * grain, tol * grain) / grain;
}

template <typename T, typename G> inline T floorgrain(T a, G grain, T tol) {
    return math::floor(a * grain, tol * grain) / grain;
}

template <typename T, typename U> inline T ceilunit(T a, U unit, T tol) {
    U invUnit = static_cast<U>(1) / unit;
    return math::ceil(a * invUnit, tol * invUnit) * unit;
}

template <typename T, typename U> inline T floorunit(T a, U unit, T tol) {
    U invUnit = static_cast<U>(1) / unit;
    return math::floor(a * invUnit, tol * invUnit) * unit;
}

    
} // namespace math
} // namespace tw
