#pragma once

#include <math.h>
#include <deque>
#include <numeric>
#include <algorithm>
#include <limits>

class StdDev {
public:
    typedef std::deque<double> TArray;
    
private:
    class VarUtil {
    public:
        VarUtil(double mean) : _mean(mean) {            
        }
        
        double operator()(double init, double value) {
            return (init + (value-_mean)*(value-_mean));
        }
        
        double _mean;
    };
    
public:
    StdDev() {
        clear();
    }
    
    void clear() {
        _values.clear();
    }
    
    TArray& getValues() {
        return _values;
    }
    
    const TArray& getValues() const {
        return _values;
    }
    
    size_t getSize() const {
        return _values.size();
    }
    
public:
    template <typename TArray>
    static double calcMean(const TArray& values) {
        if ( values.empty() )
            return 0.0;
        
        return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    }
    
    template <typename TArray>
    static double calcVar(const TArray& values) {
        if ( values.empty() )
            return 0.0;
        
        return std::accumulate(values.begin(), values.end(), 0.0, VarUtil(calcMean(values))) / values.size();
    }
    
    template <typename TArray>
    static double calcStdDev(const TArray& values) {
        return sqrt(calcVar(values));
    }
    
    template <typename TArray>
    double calcMin(const TArray& values) {
        if ( values.empty() )
            return std::numeric_limits<double>::min();
        
        return *std::min_element(values.begin(), values.end());
    }
    
    template <typename TArray>
    double calcMax(const TArray& values) {
        if ( values.empty() )
            return std::numeric_limits<double>::max();
        
        return *std::max_element(values.begin(), values.end());
    }
    
public:
    double calcMean() {
        return calcMean(_values);
    }
    
    double calcVar() {
        return calcVar(_values);
    }
    
    double calcStdDev() {
        return calcStdDev(_values);
    }
    
    double calcMin() {
        return calcMin(_values);
    }
    
    double calcMax() {
        return calcMax(_values);
    }
    
private:
    TArray _values;
};

class FinCalc {
public:
    FinCalc() {
        clear();
    }
    
    void clear() {
        _x.clear();
        _y.clear();
    }
    
    StdDev& getX() {
        return _x;
    }
    
    const StdDev& getX() const {
        return _x;
    }
    
    StdDev& getY() {
        return _y;
    }
    
    const StdDev& getY() const {
        return _y;
    }
    
    template <typename TArray>
    static double calcCov(const TArray& x_values, const TArray& y_values) {
        if ( x_values.empty() || y_values.empty() || (x_values.size() != y_values.size()) )
            return 0.0;
        
        double xmean = StdDev::calcMean(x_values);
        double ymean = StdDev::calcMean(y_values);;

        double total = 0;
        size_t n = x_values.size();
        for(size_t i = 0; i < n ; ++i) {
            total += (x_values[i] - xmean) * (y_values[i] - ymean);
        }

        return (total / n);
    }

    template <typename TArray>
    static double calcCorrel(const TArray& x_values, const TArray& y_values) {
        if ( x_values.empty() || y_values.empty() || (x_values.size() != y_values.size()) )
            return 0.0;
        
        double x_stdDev = StdDev::calcStdDev(x_values);
        double y_stdDev = StdDev::calcStdDev(y_values);
        
        if ( 0.0 == x_stdDev || 0.0 == y_stdDev)
            return 0.0;
        
        return (calcCov(x_values, y_values) / (x_stdDev * y_stdDev));
    }
    
    template <typename TArray>
    static double calcBetaXY(const TArray& x_values, const TArray& y_values) {
        if ( x_values.empty() || y_values.empty() || (x_values.size() != y_values.size()) )
            return 0.0;
        
        double var = StdDev::calcVar(y_values);
        if ( 0.0 == var )
            return 0.0;
        return (calcCov(x_values, y_values) / var);
    }
    
public:    
    double calcCov() {
        return calcCov(_x.getValues(), _y.getValues());
    }

    double calcCorrel() {
        return calcCorrel(_x.getValues(), _y.getValues());
    }
    
    double calcBetaXY() {
        if ( _x.getValues().empty() || _y.getValues().empty() || (_x.getValues().size() != _y.getValues().size()) )
            return 0.0;
        
        double var = _y.calcVar();
        
        if ( 0.0 == var )
            return 0.0;
        
        return (calcCov() / var);
    }
    
    double calcBetaYX() {
        if ( _x.getValues().empty() || _y.getValues().empty() || (_x.getValues().size() != _y.getValues().size()) )
            return 0.0;
        
        double var = _x.calcVar();
        
        if ( 0.0 == var )
            return 0.0;
        
        return (calcCov() / var);
    }
    
private:
    StdDev _x;
    StdDev _y;
};

