#pragma once

#include <tw/common/defs.h>
#include <tw/common/settings.h>
#include <tw/common/exception.h>
#include <tw/common/singleton.h>
#include <tw/common/timer_server.h>
#include <tw/common_thread/locks.h>
#include <tw/common_thread/thread.h>
#include <tw/common_strat/istrategy.h>
#include <tw/log/defs.h>
#include <tw/config/settings_cmnd_line.h>
#include <tw/channel_pf/channel.h>
#include <tw/channel_pf_cme/channel_pf_onix.h>
#include <tw/channel_pf_historical/channel_pf_historical.h>
#include <tw/channel_or/processor.h>
#include <tw/channel_or/processor_orders.h>
#include <tw/channel_or/processor_wtp.h>
#include <tw/channel_or/processor_risk.h>
#include <tw/channel_or/processor_throttle.h>
#include <tw/channel_or/processor_pnl.h>
#include <tw/channel_or/processor_messaging.h>
#include <tw/channel_or/channel_or_manager.h>

#include <map>
#include <vector>

namespace tw {
namespace common_strat {
    
// TODO: for now, strategy container is thread safe by 'locking' all methods
// calls - might need to produce other flavors, e.g. no locking, worker threads, etc.
//
class StrategyContainer : public tw::common::Singleton<StrategyContainer>,
                          public TMsgBusServerCallback {
    typedef std::map<std::string, IStrategy*> TStrategies;    
    typedef boost::shared_ptr<tw::channel_pf::Channel> TChannelPfPtr;
    typedef std::map<tw::instr::eExchange::_ENUM, TChannelPfPtr> TChannelsPf;        
    typedef boost::shared_ptr<tw::channel_pf_cme::ChannelPfOnix> TChannelPfCmePtr;
    typedef boost::shared_ptr<tw::channel_pf_historical::ChannelPfHistorical> TChannelPfHistoricalPtr;
    
public:
    enum eState {
        kStateUninitialized,
        kStateInitialized,
        kStateStarted
    };
    
    static const char* toString(eState state) {
        switch ( state ) {
            case kStateUninitialized:   return "kStateUninitialized";
            case kStateInitialized:     return "kStateInitialized";
            case kStateStarted:         return "kStateStarted";
            default:                    return "kDefault";
        }
    }
    
public:
    StrategyContainer();
    ~StrategyContainer();
    
public:
    bool init(const tw::common::Settings& settings);
    bool start();
    void stop();
    
    const tw::risk::Account& getAccount() const {
        return _account;
    }
    
public:
    // TMsgBusServerCallback (implemented as TcpIpServer server for now) interface
    //
    virtual void onConnectionUp(TConnection::native_type id, TConnection::pointer connection);
    virtual void onConnectionDown(TConnection::native_type id);    
    virtual void onConnectionData(TConnection::native_type id, const std::string& message);
    
    void sendToMsgBusConnection(TConnection::native_type id, const std::string& message);    
    void sendToAllMsgBusConnections(const std::string& message);

public:
    // Method to communicate commands between strategies
    //
    void onCommand(const tw::common::Command& cmnd);
    
public:
    // NOTE: add/remove adds IStrategy strategies to strategy
    // container. In order to add pf subscriptions, strategies should
    // call addSubscriptionsPf()/removeSubscriptionsPf() methods
    //
    bool add(IStrategy* strategy) {
        bool status = true;
        try {
            // Lock for thread synchronization
            //
            tw::common_thread::LockGuard<TLock> lock(_lock);
            _exception.clear();
        
            if ( kStateInitialized != _state ) {
                _exception << "incorrect state: " << toString(_state) << "\n";
                throw(_exception);
            }

            if ( !strategy ) {
                _exception << "strategy is NULL" << "\n";
                throw(_exception);
            }
            
            addStrategy(strategy);
            
            LOGGER_INFO << "Added strategy: " << strategy->getName() << " w/params: " << strategy->getParams().toStringVerbose() << "\n";
        } catch(const std::exception& e) {
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
            status = false;
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
            status = false;
        }
        
        if ( !status )
            remove(strategy);
        
        return status;
    }    
    
    void remove(IStrategy* client) {
        try {
            // Lock for thread synchronization
            //
            tw::common_thread::LockGuard<TLock> lock(_lock);
            
            if ( kStateUninitialized != _state ) {
                _exception << "incorrect state: " << toString(_state) << "\n";
                throw(_exception);
            }                                 

            if ( !client ) {
                LOGGER_ERRO << "strategy is NULL" << "\n";
                return;
            }
            
            removeStrategy(client);
            
            LOGGER_INFO << "Removed strategy: " << client->getName() << "\n";
        } catch(const std::exception& e) {
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        }
    }
    
    template <typename TStrategy>
    bool addSubscriptionsPf(TStrategy* client) {
        bool status = true;
        try {
            // Lock for thread synchronization
            //
            IStrategy::TQuoteSubscriptions::const_iterator iter = client->getQuoteSubscriptions().begin();
            IStrategy::TQuoteSubscriptions::const_iterator end = client->getQuoteSubscriptions().end();

            for ( ; iter != end;  ++iter ) {
                if ( !subscribePf(*iter, client) )
                    throw(_exception);
            }        
        } catch(const std::exception& e) {
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
            status = false;
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
            status = false;
        }
        
        if ( !status )
            removeSubscriptionsPf(client);
        
        return status;
    }
    
    template <typename TStrategy>
    void removeSubscriptionsPf(TStrategy* client) {
        try {
            // Lock for thread synchronization
            //
            IStrategy::TQuoteSubscriptions::const_iterator iter = client->getQuoteSubscriptions().begin();
            IStrategy::TQuoteSubscriptions::const_iterator end = client->getQuoteSubscriptions().end();

            for ( ; iter != end;  ++iter ) {
                unsubscribePf(*iter, client);
            }
        } catch(const std::exception& e) {
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        }
    }
    
public:    
    // The following 2 methods allows strategies to subscribe/unsubscribe to/from
    // exchange's quote stream.
    // NOTE: quote subscription/unsubsription is thread safe AS LONG as quotes were
    // created before the first client was subscribed to it.  In order to ensure that,
    // set 'instruments.createQuotesOnLoad' config file value to 'true' (which is
    // default)!
    //
    template <typename TStrategy>
    bool subscribePf(tw::instr::InstrumentConstPtr instrument, TStrategy* client) {
        bool status = true;
        try {
            // Lock for thread synchronization
            //
            tw::common_thread::LockGuard<TLock> lock(_lock);
            
            _exception.clear();

            TChannelPfPtr channelPf = getChannelPf(instrument);
            if ( !channelPf || !channelPf->subscribe(instrument, client) ) {
                _exception << "Can't find or subscribe to channel_pf for: " << client->getName() << " :: " << instrument->toString() << "\n";
                throw(_exception);
            }
            
            LOGGER_INFO << "Subscribed: " << client->getName() << " :: " << instrument->toString() << "\n";
            
        } catch(const std::exception& e) {
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
            status = false;
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
            status = false;
        }

        return status;
    }
    
    template <typename TStrategy>
    bool unsubscribePf(tw::instr::InstrumentConstPtr instrument, TStrategy* client) {
        bool status = true;
        try {
            // Lock for thread synchronization
            //
            tw::common_thread::LockGuard<TLock> lock(_lock);
            
            _exception.clear();

            TChannelPfPtr channelPf = getChannelPf(instrument);
            if ( !channelPf || !channelPf->unsubscribe(instrument, client) ) {
                _exception << "Can't find or unsubscribe from channel_pf for: " << client->getName() << " :: " << instrument->toString() << "\n";
                throw(_exception);
            }
            
            LOGGER_INFO << "Unsubscribed: " << client->getName() << " :: " << instrument->toString() << "\n";
            
        } catch(const std::exception& e) {
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
            status = false;
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
            status = false;
        }

        return status;
    }    
    
public:
    typedef tw::common_thread::Lock TLock;
        
    TLock& getLock() {
        return _lock;
    }
    
    eState getState() const;
    // Inbound ChannelPf methods
    //
    void onQuote(const tw::price::QuoteStore::TQuote& quote);
    
public:
    // Timer server callback method through tw::common_strat::ConsumerProxy
    //
    bool onTimeout(tw::common::TimerClient* client, tw::common::TTimerId id);
    
public:
    // Outbound ChannelOr related methods
    //
    bool sendNew(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej);    
    bool sendMod(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej);    
    bool sendCxl(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej);
    
    bool rebuildOrder(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej);
    
public:
    // Batch ChannelOr cancels
    //
    void sendCxlForAccount(const tw::channel_or::TAccountId& x);
    void sendCxlForAccountStrategy(const tw::channel_or::TAccountId& x, const tw::channel_or::TStrategyId& y);
    void sendCxlForInstrument(const tw::instr::Instrument::TKeyId& x);
    void sendCxlAll();
    void sendCxlForBatch(const tw::channel_or::TOrders& orders, const std::string& reason);
    
public:
    // Inbound ChannelOr methods
    //
    void onOrderResp(const tw::channel_or::OrderResp& orderResp);
    void onAlert(const tw::channel_or::Alert& alert);
    
    void onNewAck(tw::channel_or::TOrderPtr& order, const tw::common::THighResTime& now);
    void onNewRej(tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej, const tw::common::THighResTime& now);
    void onModAck(tw::channel_or::TOrderPtr& order, const tw::common::THighResTime& now);
    void onModRej(tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej, const tw::common::THighResTime& now);
    void onCxlAck(tw::channel_or::TOrderPtr& order, const tw::common::THighResTime& now);
    void onCxlRej(tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej, const tw::common::THighResTime& now);
    void onFill(tw::channel_or::Fill& fill, tw::channel_or::TOrderPtr& order, const tw::common::THighResTime& now);
    
private:
    void clear();
    void initChannelPfs(const tw::common::Settings& settings);
    void initChannelOrs(const tw::common::Settings& settings);
    
    void addStrategy(IStrategy* client);
    void removeStrategy(IStrategy* client);
    
private:
    TChannelPfPtr getChannelPf(tw::instr::InstrumentConstPtr instrument) {
        TChannelPfPtr channelPf;
        
        TChannelsPf::iterator iter = _channelsPfs.find(instrument->_exchange);
        if ( iter != _channelsPfs.end() ) 
            channelPf = iter->second;
        
        return channelPf;
    }
    
    IStrategy* getStrategy(const tw::channel_or::TAccountId& accountId, const tw::channel_or::TStrategyId& strategyId) const {
        TStrategies::const_iterator iter = _strategies.begin();
        TStrategies::const_iterator end = _strategies.end();
        
        for ( ; iter != end; ++ iter ) {
            if ( (iter->second->getAccountId() == accountId) && (iter->second->getStrategyId() == strategyId) )
                return iter->second;
        }
        
        return NULL;
    }
    
    void checkCancelOnAck(const tw::channel_or::TOrderPtr& order) {
        if ( order->_cancelOnAck ) {
            order->_cancelOnAck = false;
            
            tw::channel_or::Reject rej;            
            if ( !sendCxl(order, rej) ) {
                tw::channel_or::Alert alert;
                
                alert._type = tw::channel_or::eAlertType::kOrderRej;
                alert._text = rej.toStringVerbose();
                
                onAlert(alert);
            }
        }
    }
    
    tw::channel_or::Reject getRej() {
        tw::channel_or::Reject rej;
        
        rej._rejType = tw::channel_or::eRejectType::kInternal;
        rej._rejSubType = tw::channel_or::eRejectSubType::kStrategyContainer;
        rej._rejReason = tw::channel_or::eRejectReason::kSystemError;
        
        return rej;
    }
    
    bool doCrashRecovery();
    
private:
    void ThreadMain();
    void processExternalFill(const tw::common::Command& cmnd);
    
    IStrategy* getStrategy(const tw::channel_or::TStrategyId& strategyId) const {
        TStrategies::const_iterator iter = _strategies.begin();
        TStrategies::const_iterator end = _strategies.end();
        
        for ( ; iter != end; ++iter ) {
            if ( iter->second && iter->second->getStrategyId() == strategyId )
                return iter->second;
        }
        
        return NULL;
    }
    
private:
    // NOTE: the order of template initialization is from last channel_or_processor
    // to first one
    //
    typedef tw::channel_or::ProcessorOut<tw::channel_or::ChannelOrManager> TProcessorOutChannelOrManager;
    typedef tw::channel_or::ProcessorOut<tw::channel_or::ProcessorPnL, TProcessorOutChannelOrManager> TProcessorOutPnL;
    typedef tw::channel_or::ProcessorOut<tw::channel_or::ProcessorRisk, TProcessorOutPnL> TProcessorOutRisk;
    typedef tw::channel_or::ProcessorOut<tw::channel_or::ProcessorWTP, TProcessorOutRisk> TProcessorOutWTP;
    typedef tw::channel_or::ProcessorOut<tw::channel_or::ProcessorThrottle, TProcessorOutWTP> TProcessorOutThrottle;
    typedef tw::channel_or::ProcessorOut<tw::channel_or::ProcessorOrders, TProcessorOutThrottle> TProcessorOutOrders;
    typedef tw::channel_or::ProcessorOut<tw::channel_or::ProcessorMessaging, TProcessorOutOrders> TProcessorOut;
    
    typedef tw::channel_or::ProcessorIn<tw::channel_or::ProcessorOrders> TProcessorInOrders;
    typedef tw::channel_or::ProcessorIn<tw::channel_or::ProcessorMessaging, TProcessorInOrders> TProcessorInMessaging;
    typedef tw::channel_or::ProcessorIn<tw::channel_or::ProcessorPnL, TProcessorInMessaging> TProcessorIn;
    
private:    
    TLock _lock;
    tw::common::Exception _exception;
    
    eState _state;
    tw::common::Settings _settings;
    tw::risk::Account _account;
    TMsgBusServer _msgBusServer;
    
    TChannelPfCmePtr _channelPfCme;
    TChannelPfHistoricalPtr _channelPfHistorical;
    TStrategies _strategies;
    TChannelsPf _channelsPfs;
    
    TProcessorOutChannelOrManager _channelOrProcessorOutManager;    
    TProcessorOutPnL _channelOrProcessorOutPnL;
    TProcessorOutRisk _channelOrProcessorOutRisk;
    TProcessorOutWTP _channelOrProcessorOutWTP;
    TProcessorOutThrottle _channelOrProcessorOutThrottle;
    TProcessorOutOrders _channelOrProcessorOutOrders;
    TProcessorOut _channelOrProcessorOut;
    
    TProcessorInOrders _channelOrProcessorInOrders;
    TProcessorInMessaging _channelOrProcessorInMessaging;
    TProcessorIn _channelOrProcessorIn;
    
    tw::common_thread::ThreadPtr _thread;
};
	
} // channel_pf
} // tw

