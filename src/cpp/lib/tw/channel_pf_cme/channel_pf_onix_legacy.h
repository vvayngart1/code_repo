#pragma once

#include <tw/common/high_res_time.h>
#include <tw/channel_pf_cme/settings.h>
#include <tw/channel_pf/ichannel_impl.h>
#include <tw/price/quote.h>

#include <OnixS/CME/MarketData.h>

#include <boost/shared_ptr.hpp>
#include <map>

#include <tr1/unordered_map>

// NOTE: no thread synchornization provisions are implemented in this class
// The following assumptions need to be valid to avoid race conditions:
//      1. ALL calls to subscribe()/unsubscribe() methods are done AFTER init() 
//      but BEFORE start() or AFTER stop()
//      2. All tw::price::TQuotes objects are created BEFORE init() call and destroyed
//      AFTER stop() call 
//      3. Any thread syncronizations needed for managing tw::price::TQuotes are done
//      OUTSIDE this class
//

// NOTE:  the following mapping is used for onix's events to tw's quote statuses:
// Onix status                                    // Tw's status
//
//kCMEDirectBooksOutOfDate,                       // kStaleQuote      
//kCMETradingHaltOrStopSkipe,                     // kTradingHaltedOrStopped     
//kCMEResumeOrOpen,                               // kTradingResumeOrOpen
//kCMEEndOfTradingSession,                        // kTradingSessionEnd
//kCMEPauseInTrading,                             // kTradingPause
//kCMEStatisticsReset,                            // kHLOReset
//kCMEStatistics,                                 // not needed
//kCMEHandlerStopped,                             // kExchangeDown
//kCMEHandlerStarted,                             // kExchangeUp
//kCMEHandlerSecurityDefinitionsRecoveryStarted,  // --NOT IMPLEMENTED
//kCMEHandlerSecurityDefinitionsRecoveryFinished, // NOT IMPLEMENTED
//kCMEHandlerBooksResynchronizationStarted,       // kDataRecoveryStart
//kCMEHandlerBooksResynchronizationFinished,      // kDataRecoveryFinish
//kCMEHandlerTcpReplayStarted,                    // --NOT IMPLEMENTED
//kCMEHandlerTcpReplayFinished,                   // --NOT IMPLEMENTED
//kCMEOnReplayError,                              // kReplayError
//kCMEOnReplayFinished,                           // kReplayFinished
//kCMEErrorGeneric,                               // kConnectionHandlerError
//kCMEErrorNotLicensed,                           // kConnectionHandlerError
//kCMEErrorBadConfiguration,                      // kConnectionHandlerError
//kCMEErrorBadPacket,                             // kConnectionHandlerError
//kCMEErrorBadMessage                             // kConnectionHandlerError
//

namespace tw {
namespace channel_pf_cme {    
    
    inline int channelIdToInt(OnixS::CME::MarketData::ChannelId channelId) {
        return atoi(channelId.c_str());
    }

class ChannelPfOnix : public tw::channel_pf::IChannelImpl,
                        OnixS::CME::MarketData::SecurityDefinitionListener,
                        OnixS::CME::MarketData::DirectBookUpdateListener,
                        OnixS::CME::MarketData::TradeListener,
                        OnixS::CME::MarketData::NewsListener,
                        OnixS::CME::MarketData::SecurityStatusListener,
                        OnixS::CME::MarketData::StatisticsListener,
                        OnixS::CME::MarketData::MessageProcessingListener,
                        OnixS::CME::MarketData::HandlerStateChangeListener,
                        OnixS::CME::MarketData::ReplayListener,
                        OnixS::CME::MarketData::ErrorListener {
public:
    ChannelPfOnix();
    virtual ~ChannelPfOnix();

public:
    // tw::channel_pf::IChannelImpl interface
    //
    virtual bool init(const tw::common::Settings& settings);
    virtual bool start();    
    virtual void stop();
    
    virtual bool subscribe(tw::instr::InstrumentConstPtr instrument);
    virtual bool unsubscribe(tw::instr::InstrumentConstPtr instrument);
    
public:
    // Provides ability to get security definitions for CME's instrument
    // loader
    //
    bool start(OnixS::CME::MarketData::SecurityDefinitionListener* handler);
    
    const tw::channel_pf_cme::Settings& getSettings() const {
        return _settings;
    }
    
public:
    // OnixS::CME::MarketData::SecurityDefinitionListener interface
    //
    virtual void onSecurityDefinition(const OnixS::CME::MarketData::SecurityDefinition& securityDefinition, const OnixS::CME::MarketData::ChannelId& channelId);
    virtual void onSecurityDefinitionDeleted(OnixS::CME::MarketData::SecurityId securityId, const OnixS::CME::MarketData::ChannelId& channelId);
    virtual void onSecurityDefinitionsRecoveryStarted(const OnixS::CME::MarketData::ChannelId& channelId);
    virtual void onSecurityDefinitionsRecoveryFinished(const OnixS::CME::MarketData::ChannelId& channelId);
        
    // OnixS::CME::MarketData::DirectBookUpdateListener interface
    //
    virtual void onDirectBooksOutOfDate(const OnixS::CME::MarketData::ChannelId& channelId);
    virtual void onDirectBookUpdated(const OnixS::CME::MarketData::Book& book, const OnixS::CME::MarketData::ChannelId& channelId);
    
    // OnixS::CME::MarketData::TradeListener interface
    //
    virtual void onTrade(const OnixS::CME::MarketData::Trade& trade, const OnixS::CME::MarketData::ChannelId& channelId);
    
    // OnixS::CME::MarketData::NewsListener interface
    //
    virtual void onNews(const OnixS::CME::MarketData::News& news, const OnixS::CME::MarketData::ChannelId& channelId);

    // OnixS::CME::MarketData::SecurityStatusListener interface
    //
    virtual void onSecurityStatus(const OnixS::CME::MarketData::SecurityStatus& status, const OnixS::CME::MarketData::ChannelId& channelId);
    
    // OnixS::CME::MarketData::StatisticsListener interface
    //
    virtual void onStatisticsReset(OnixS::CME::MarketData::SecurityId securityId, const OnixS::CME::MarketData::ChannelId& channelId);
    virtual void onStatisticsReset(const OnixS::CME::MarketData::Symbol& symbol, const OnixS::CME::MarketData::ChannelId& channelId);
    virtual void onStatistics(const OnixS::CME::MarketData::Statistics& statistics, const OnixS::CME::MarketData::ChannelId& channelId);
    
    // OnixS::CME::MarketData::MessageProcessingListener interface
    //
    virtual void onProcessingBegin(const OnixS::CME::MarketData::Message& message, const OnixS::CME::MarketData::ChannelId& channelId);
    virtual void onProcessingEnd(const OnixS::CME::MarketData::Message &message, const OnixS::CME::MarketData::ChannelId& channelId);
    
    // OnixS::CME::MarketData::HandlerStateChangeListener interface
    //
    virtual void onHandlerStateChange(const OnixS::CME::MarketData::HandlerStateChange& change, const OnixS::CME::MarketData::ChannelId& channelId);
    
    // OnixS::CME::MarketData::ReplayListener interface
    //
    virtual void onReplayError (const OnixS::CME::MarketData::ChannelId& channelId, const std::string& errorDescription);
    virtual void onReplayFinished (const OnixS::CME::MarketData::ChannelId& channelId);
    
    // OnixS::CME::MarketData::ErrorListener
    //
    virtual void onError(const OnixS::CME::MarketData::Error& error, const OnixS::CME::MarketData::ChannelId& channelId);
    
private:
    typedef OnixS::CME::MarketData::Handler THandler;
    typedef boost::shared_ptr<THandler> THandlerPtr;
    typedef std::tr1::unordered_map<int, THandlerPtr> THandlers;
    typedef std::tr1::unordered_map<int, tw::common::THighResTime> THandlersHighResolutionTime;
    
private:
    bool doInit(const tw::channel_pf_cme::Settings& config);
    
    THandlerPtr addHandler(std::string channel_id);
    void registerWithHandler(THandlerPtr handler, bool topOfBookOnly);
    
    void checkExchangeSlow(const OnixS::CME::MarketData::ChannelId& channelId,
                           tw::price::Quote& quote,
                           uint64_t maxDelta,
                           const std::string& source);
    
private:
    tw::channel_pf_cme::Settings _settings;
    OnixS::CME::MarketData::SecurityDefinitionListener* _handlerSecurityDefinitions;
    THandlers _handlers;
    THandlersHighResolutionTime _handlersHighResolutionTime;
};
	
} // channel_pf_cme
} // tw

