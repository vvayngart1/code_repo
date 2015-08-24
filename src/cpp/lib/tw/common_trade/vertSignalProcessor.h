#pragma once

#include <tw/common/defs.h>
#include <tw/price/quote.h>
#include <tw/log/defs.h>
#include <tw/common_trade/bars_manager.h>
#include <tw/common_trade/wbo_manager.h>
#include <tw/common/timer_server.h>
#include <math.h>

namespace tw {
namespace common_trade {
    
typedef tw::common_trade::TBar TBar;
typedef tw::common_trade::TBars TBars;

class VertSignalProcessor : public tw::common::TimerClient {
public:
    VertSignalProcessor(VertSignalParamsWire& p) : _p(p) {
        clear();  
    }
    
public:
    VertSignalParamsWire& getParams() {
        return _p;
    }
    
    const VertSignalParamsWire& getParams() const {
        return _p;
    }
    
    void clear() {
        _p._alpha_new = _p._alpha;
        _p._firstrow = true;
        _p._recentmax = 0;
        _p._score = 0;
        _p._local_counter = 0;
        _p._local = 0;
        _p._local_prev = 0;
        _p._local_assigned = false;
        _p._initialized = false;
    }
    
public:   
    virtual bool onTimeout(const tw::common::TTimerId& id) {
        if (id == _p._timerId_everySecond) {
            if (_p._local_window > 0) {
                updateQueue(_p._local_scores, _p._score, _p._local_window);
                if (_p._local_window == _p._local_scores.size()) {
                    double inst_local = _p._local_scores[_p._local_scores.size() - 1] - _p._local_scores[0];
                    inst_local /= _p._local_window;
                    
                    updateQueue(_p._locals, inst_local, _p._local_window);
                    if (_p._local_window == _p._locals.size()) {
                        _p._local = std::accumulate(_p._locals.begin(), _p._locals.end(), 0.0) / _p._locals.size();
                        _p._local *= 1000;

                        if (!_p._local_assigned) {
                            _p._local_assigned = true;
                        } else {
                            if (_p._score > 0) {
                                // go WITH short term trend (vert=down)
                                if (_p._flip_mode) {
                                    if (_p._local < 0)
                                        _p._local_counter++;
                                    else
                                        _p._local_counter = 0;    
                                } else {
                                    if (_p._local > 0)
                                        _p._local_counter++;
                                    else
                                        _p._local_counter = 0;
                                }
                            } else {
                                // go WITH short-term trend (vert=up)
                                if (_p._flip_mode) {
                                    if (_p._local > 0)
                                        _p._local_counter++;
                                    else
                                        _p._local_counter = 0;    
                                } else {
                                    if (_p._local < 0)
                                        _p._local_counter++;
                                    else
                                        _p._local_counter = 0;   
                                }
                            }
                        }

                        _p._local_prev = _p._local;
                    }
                }    
            }
            
            // ==> recalculate score if window > 1, implying score mean over window score measurements
            if (_p._window > 1 && _p._scores.size() > 0) {
                double multiplier = 1;
                if (!_p._flip_mode)
                    multiplier = -1;    
                _p._score_prev = _p._score;
                _p._score = multiplier * std::accumulate(_p._scores.begin(), _p._scores.end(), 0.0) / _p._scores.size();
                _p._vel = _p._score - _p._score_prev;
                _p._vel *= 100;
                if ( (_p._score < _p._init_multiplier * _p._C1) && (_p._score > -_p._init_multiplier * _p._C1) )
                    _p._initialized = true;
                if ( _p._score > 0) {
                    if (_p._score > _p._recentmax)
                        _p._recentmax = _p._score;
                    else if (_p._score < _p._init_multiplier * _p._recentmax) {
                        _p._initialized = true;
                        _p._recentmax = _p._score;
                    }
                } else if (_p._score < 0) {
                    if (_p._score < _p._recentmax)
                        _p._recentmax = _p._score;
                    else if (_p._score > _p._init_multiplier * _p._recentmax) {
                        _p._initialized = true;
                        _p._recentmax = _p._score;
                    }
                }
            }
        } else if (id == _p._timerId_refreshUnits) {
            _p._x = _p._w * _p._alpha + _p._x * (1 - _p._alpha);
            _p._x_vel = _p._vel * (_p._vel_multiplier * _p._alpha) + _p._x_vel * (1 - (_p._vel_multiplier * _p._alpha));
        }
        
        return true;
    }
    
    template <typename T>
    void updateQueue(T& q, const typename T::value_type v, const uint32_t size) {
        q.push_back(v);
        if ( q.size() > size )
            q.pop_front();
    }
    
    // N.B. w is wbo or tradePrice, depending on _p._allquotes_mode
    void updateRuntime(const tw::price::Quote& quote, double w) {
        _p._w = w;
        
        if (_p._firstrow) {
            _p._x = _p._w;
            _p._x_vel = 0;
            _p._firstrow = false;
        }
        
        double term = _p._w - _p._x;
        if (_p._x != 0)
            term /= _p._x;
        term *= 10000;
        if (_p._window > 1) {
            updateQueue(_p._scores, term, _p._window);
        }
    }
    
    bool considerEnter(tw::channel_or::eOrderSide& side, std::string& reason) {
        bool result = false;
        
        tw::common_str_util::TFastStream s;
        s.setPrecision(2);
        
        double cutoff_pos = _p._C1;
        double cutoff_neg = -_p._C1;
        if (_p._score > cutoff_pos) {
            if (_p._score > _p._max_enter_multiplier * cutoff_pos) {
                _p._local_counter = 0;
                _p._initialized = false;
                s << " max enter violated (SELL OPEN): " << _p._score << " > " << _p._max_enter_multiplier << " * " << cutoff_pos;
                reason = s.str();
                return result;
            }
            
            if (_p._local_counter < _p._local_min_counter) {
                s << " local_min_counter violated (SELL OPEN): " << _p._local_counter << " < " << _p._local_min_counter << ", _p._score=" << _p._score;
                reason = s.str();
                return result;
            }
            
            if (_p._x_vel > _p._speed_upper) {
               s << "velocity violated (SELL OPEN): _p._x_vel=" << _p._x_vel << " > " << _p._speed_upper << ", _p._vel=" << _p._vel;            
               reason = s.str();
               return result;
            } 
            
            // sell here
            side = tw::channel_or::eOrderSide::kSell;
            result = true;
        }
        else if (_p._score < cutoff_neg) {
            if (_p._score < _p._max_enter_multiplier * cutoff_neg) {
                _p._local_counter = 0;
                _p._initialized=false;
                s << " max enter violated (BUY OPEN): " << _p._score << " < " << _p._max_enter_multiplier << " * " << cutoff_neg;
                reason = s.str();
                return result;
            }
            
            if (_p._local_counter < _p._local_min_counter) {
                s << " local_min_counter violated (BUY OPEN): " << _p._local_counter << " < " << _p._local_min_counter;
                reason = s.str();
                return result;
            }
            
            if (_p._x_vel < _p._speed_lower) {
               s << "velocity violated (BUY OPEN): _p._x_vel=" << _p._x_vel << " < " << _p._speed_lower << ", _p._vel=" << _p._vel;
               reason = s.str();
               return result;
            }
            
            // buy here
            side = tw::channel_or::eOrderSide::kBuy;
            result = true;
        } else {
            s << " no trigger with _p._score=" << _p._score << ", _p._local_counter=" << _p._local_counter << ", _p._initialized=" << _p._initialized;
            reason = s.str();
        }
        
        return result;
    }
    
    bool isSignalTriggered(bool trysignal, const TBars& bars, tw::channel_or::eOrderSide& side, const tw::price::Quote& quote, double tradePrice, double wbo, std::string& reason) {
        side = tw::channel_or::eOrderSide::kUnknown;
        reason.clear();  
        
        tw::common_str_util::TFastStream s;
        s.setPrecision(2);
        
        // ==> prelim. need 2 bars to proceed
        if ( bars.size() < 2 ) {
            s << "bars.size() < 2 -- bars.size()=" << bars.size();
            reason = s.str();
            return false;
        }
        
        // ensure wbo and quotes are available
        const TBar& bar = bars[bars.size()-1];
        if (!bar._close.isValid()) {
            s << "bar not valid; exiting";
            reason = s.str();
            return false;
        }
        
        if (_p._allquotes_mode)
            updateRuntime(quote, wbo);
        else {
            if ( quote._trade._price.isValid() && quote.isNormalTrade() && quote._trade._size.get() >= 1) {
                _p._alpha_new = ( 1 + _p._alpha_multiplier * (log10(quote._trade._size.get())) ) * _p._alpha;
                _p._x = tradePrice * _p._alpha_new + _p._x * (1 - _p._alpha_new);
            }
            
            updateRuntime(quote, tradePrice);
        }
        
        if (!trysignal)
            return false;
        
        if (!_p._initialized) {
            // verbose logging:
            s << "init";
            reason = s.str();
            return false;
        }
        
        std::string ce_reason;
        considerEnter(side, ce_reason);
        
        if ( tw::channel_or::eOrderSide::kUnknown == side ) {
            s << "side not set: "
            << ",ce_reason=" << ce_reason
            << ",_p._alpha_new=" << _p._alpha_new
            << ",_p._score=" << _p._score
            << ",_p._recentmax=" << _p._recentmax
            << ",_p._vel=" << _p._vel
            << ",_p._x_vel=" << _p._x_vel
            << ",w=" << _p._w
            << ",x=" << _p._x
            << ",local_counter=" << _p._local_counter
            << ",initialized=" << _p._initialized
            << ",quote._trade._price=" << quote._trade._price
            << ",bid=" << quote._book[0]._bid._price
            << ",ask=" << quote._book[0]._ask._price;
            
            reason = s.str();
            return false;
        }
        
        s << "VertSignalProcessor::isSignalTriggered()==true"
          << ",side=" << side.toString()
          << ",_p._alpha_new=" << _p._alpha_new
          << ",_p._score=" << _p._score
          << ",_p._recentmax=" << _p._recentmax
          << ",_p._vel=" << _p._vel
          << ",_p._x_vel=" << _p._x_vel
          << ",w=" << _p._w
          << ",x=" << _p._x
          << ",local_counter=" << _p._local_counter
          << ",initialized=" << _p._initialized
          << ",quote._trade._price=" << quote._trade._price
          << ",bid=" << quote._book[0]._bid._price
          << ",ask=" << quote._book[0]._ask._price;
        
        reason = s.str();
        _p._local_counter = 0;
        _p._initialized=false;
        return true;
    }
    
private:
    VertSignalParamsWire& _p;
};

} // common_trade
} // tw


