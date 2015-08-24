#pragma once

#include <tw/common/defs.h>
#include <tw/common/exception.h>
#include <tw/common/timer_server.h>
#include <tw/price/quote.h>
#include <tw/log/defs.h>

namespace tw {
namespace common_trade {
    
class ITimerBasedStopProcessorClient {
public:
    ITimerBasedStopProcessorClient() {
    }
    
    virtual ~ITimerBasedStopProcessorClient() {
    }
    
public:
    virtual void processCycleTimer(bool isLBound) = 0;
};


template <typename TRouter>   
class TimerBasedStopProcessor : public tw::common::TimerClient {
public:
    typedef std::pair<uint32_t, uint32_t> TimersInfo;
    
public:
    TimerBasedStopProcessor(TimerStopLossParamsWire& p, ITimerBasedStopProcessorClient& client) : _p(p),
                                                                                                  _client(client) {
        if ( !isEnabled() )
            return;
        
        //user sets seconds but we want ms
        _p._cycleLength *= 1000; 
        _p._cycleLBound *= 1000;
        _p._cycleUBound *= 1000;
        
        if ( !registerCycleTimeout() ) {
            tw::common::Exception exception;
            exception << "Can't registerCycleTimeout() for: " << _p.toString() << "\n";
            throw(exception);
        }
    }
    
public:
    TimerStopLossParamsWire& getParams() {
        return _p;
    }
    
    const TimerStopLossParamsWire& getParams() const {
        return _p;
    }

public:
    // TimerClient interface
    //
    virtual bool onTimeout(const tw::common::TTimerId& id) {
        bool processed = false;
        
        if ( id == _p._cycleLBoundTimerId ) {
            processCycleTimer(true);
            processed = true;
        } else if ( id == _p._cycleUBoundTimerId ) {
            processCycleTimer(false);
            processed = true;
        }
        
        if ( !processed )
            LOGGER_WARN << _p._strategyInfo << " -- didn't process timerId: " << id << "\n";
        
        return false;
    }
    
public:
    // Methods to be used by clients
    //
    bool isEnabled() const {
        if ( 0 == _p._cycleLength )
            return false;
        
        return true;
    }
    
    tw::price::Ticks calcStopSlide(const uint32_t timerCounter, bool isLBound) const {
        tw::price::Ticks stopSlide;
        if ( eStopLossCycleMode::kAnchor == _p._stopLossCycleMode )
            stopSlide = isLBound ? _p._stopLossCycleLBound : _p._stopLossCycleUBound;                
        else
            stopSlide = ( 1 == (timerCounter%2) ) ? _p._stopLossCycleLBound : _p._stopLossCycleUBound;

        return stopSlide;
    }
    
    bool isTakeProfitTriggered(const FillInfo& info,
                               const tw::price::Quote& quote,
                               const tw::price::Ticks& takeProfitPayupTicks,
                               bool isLBound,
                               tw::price::Ticks& price,
                               std::string& reason) const {
         tw::price::Ticks takeProfitTics = isLBound ? _p._takeProfitCycleLBound : _p._takeProfitCycleUBound;

         if ( takeProfitTics.get() < 1 )
             return false;

         tw::common_str_util::TFastStream s;
         switch ( info._fill._side ) {
             case tw::channel_or::eOrderSide::kBuy:
                 if ( (info._fill._price + takeProfitTics) > quote._book[0]._bid._price )
                     return false;

                 price = quote._book[0]._bid._price - takeProfitPayupTicks;
                 s << "takeProfitExit((info._fill._price + takeProfitTics) <= quote._book[0]._bid._price) for: " 
                   << (isLBound ? "LBound" : "UBound")
                   << ",price=" << price.get() << ",fill=" << info.toString()
                   << " -- quote=" << quote._book[0].toShortString();

                 break;
             case tw::channel_or::eOrderSide::kSell:
                 if ( (info._fill._price - takeProfitTics) < quote._book[0]._ask._price )
                     return false;

                 price = quote._book[0]._ask._price + takeProfitPayupTicks;
                 s << "takeProfitExit((info._fill._price - takeProfitTics) >= quote._book[0]._ask._price) for: " 
                   << (isLBound ? "LBound" : "UBound")
                   << ",price=" << price.get() << ",fill=" << info.toString()
                   << " -- quote=" << quote._book[0].toShortString();
                 break;
             default:
                 return false;
         }

         reason = s.str();
         return true;
    }
    
public:
    // Utility methods made public for unit tests only
    //
    TimersInfo calcCycleTimeouts() {
        TimersInfo timers;

        uint32_t msecFromMidnight = TRouter::getMSecFromMidnight();
        uint32_t msecInCycle = msecFromMidnight % _p._cycleLength;
        uint32_t timerMsLBound = 0;
        uint32_t timerMsUBound = 0;

        if ( msecInCycle > _p._cycleUBound ) {
            timerMsLBound = ((_p._cycleLength-msecInCycle)+_p._cycleLBound);
            timerMsUBound = ((_p._cycleLength-msecInCycle)+_p._cycleUBound);
        } else if ( msecInCycle < _p._cycleLBound ) {
            timerMsLBound = (_p._cycleLBound-msecInCycle);
            timerMsUBound = (_p._cycleUBound-msecInCycle);
        } else {
            timerMsUBound = (_p._cycleUBound-msecInCycle);
            timerMsLBound = timerMsUBound+((_p._cycleLength-_p._cycleUBound)+_p._cycleLBound);
            
            if ( 0 == timerMsUBound )
                timerMsUBound = _p._cycleLength;
        }

        timers.first = timerMsLBound;
        timers.second = timerMsUBound;

        return timers;
    }

    void processCycleTimer(bool isLBound) {
        _client.processCycleTimer(isLBound);

        std::pair<uint32_t, uint32_t> timers = calcCycleTimeouts();
        if ( isLBound ) {
            if ( 0 == timers.first )
                timers.first = _p._cycleLength;
            TRouter::registerTimerClient(this, timers.first, true, _p._cycleLBoundTimerId);
        } else {
            if ( 0 == timers.second )
                timers.second = _p._cycleLength;
            TRouter::registerTimerClient(this, timers.second, true, _p._cycleUBoundTimerId);
        }
    }

    bool registerCycleTimeout() {
        if ( 0 == _p._cycleLength )
            return false;

        TimersInfo timers = calcCycleTimeouts();
        if ( !TRouter::registerTimerClient(this, timers.first, true, _p._cycleLBoundTimerId) )
            return false;                
        
        if ( !TRouter::registerTimerClient(this, timers.second, true, _p._cycleUBoundTimerId) )
            return false;
        
        LOGGER_INFO << "Registered cycle for timer (msecInCycle > cycleUBound): "
                    << "timerMsLBound: " << timers.first 
                    << ",timerMsUBound: " << timers.second
                    << ",_cycleLength (ms)=" << _p._cycleLength
                    << ",_cycleLBound (ms)=" << _p._cycleLBound
                    << ",_cycleUBound (ms)=" << _p._cycleUBound
                    << ",_cycleLBoundTimerId=" << _p._cycleLBoundTimerId
                    << ",_cycleUBoundTimerId=" << _p._cycleUBoundTimerId
                    << "\n";

        return true;
    }
    
private:
    TimerStopLossParamsWire& _p;
    ITimerBasedStopProcessorClient& _client;
};

} // common_trade
} // tw
