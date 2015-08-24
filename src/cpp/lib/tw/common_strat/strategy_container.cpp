#include <tw/common_strat/strategy_container.h>
#include <tw/common_strat/consumer_proxy.h>

#include <tw/common_thread/utils.h>

#include <tw/common/timer_server.h>
#include <tw/config/settings_config_file.h>
#include <tw/instr/instrument_manager.h>
#include <tw/common_trade/bars_storage.h>
#include <tw/common_trade/bars_manager.h>

#include <tw/channel_pf/channel.h>
#include <tw/channel_pf_cme/settings.h>

#include <tw/channel_or/translator.h>
#include <tw/channel_or/channel_or_storage.h>
#include <tw/channel_or/processor_pnl.h>
#include <tw/channel_or/pnl_audit_trail.h>
#include <tw/risk/risk_storage.h>

namespace tw {
namespace common_strat {
StrategyContainer::StrategyContainer() : _channelOrProcessorOutManager(tw::channel_or::ChannelOrManager::instance()),
                                         _channelOrProcessorOutPnL(tw::channel_or::ProcessorPnL::instance(), _channelOrProcessorOutManager),
                                         _channelOrProcessorOutRisk(tw::channel_or::ProcessorRisk::instance(), _channelOrProcessorOutPnL),
                                         _channelOrProcessorOutWTP(tw::channel_or::ProcessorWTP::instance(), _channelOrProcessorOutRisk),
                                         _channelOrProcessorOutThrottle(tw::channel_or::ProcessorThrottle::instance(), _channelOrProcessorOutWTP),
                                         _channelOrProcessorOutOrders(tw::channel_or::ProcessorOrders::instance(), _channelOrProcessorOutThrottle),
                                         _channelOrProcessorOut(tw::channel_or::ProcessorMessaging::instance(), _channelOrProcessorOutOrders),
                                         _channelOrProcessorInOrders(tw::channel_or::ProcessorOrders::instance()),
                                         _channelOrProcessorInMessaging(tw::channel_or::ProcessorMessaging::instance(), _channelOrProcessorInOrders),
                                         _channelOrProcessorIn(tw::channel_or::ProcessorPnL::instance(), _channelOrProcessorInMessaging) {
    clear();
}

StrategyContainer::~StrategyContainer() {
    clear();
}

void StrategyContainer::clear() {
    try {
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<TLock> lock(_lock);
        
        _exception.clear();
        _settings.clear();
        _state = kStateUninitialized;
        
        _strategies.clear();
        _channelsPfs.clear();
        
        _channelPfCme = TChannelPfCmePtr();
        _channelPfHistorical = TChannelPfHistoricalPtr();
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

bool StrategyContainer::init(const tw::common::Settings& settings) {
    bool status = true;
    try {
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<TLock> lock(_lock);
        _exception.clear();
        
        if ( kStateUninitialized != _state ) {
            _exception << "incorrect state: " << toString(_state) << "\n";
            throw(_exception);
        }
        
        if ( !settings.validate() ) {
            _exception << "invalid settings: " << settings.toString() << "\n";
            throw(_exception);
        }
        
        // Init timer sever
        //
        if ( !tw::common::TimerServer::instance().init(settings) ) {
            _exception << "failed to init timer server: " << settings.toString() << "\n";
            throw(_exception);
        }
        
        // Register itself with consumer_proxy for timeouts
        //
        tw::common_strat::ConsumerProxy::instance().registerCallbackTimer(this);
        
        // Load instruments
        //
        if ( !tw::instr::InstrumentManager::instance().loadInstruments(settings) ) {
            _exception << "failed to load instruments: " << settings._instruments_dataSource << "\n";
            throw(_exception);
        }
    
        // Initialize channel_pfs   
        //
        initChannelPfs(settings);
        
        // Initialize channel ors
        //
        initChannelOrs(settings);
        
        // Init bars storage
        //
        if ( !tw::common_trade::BarsStorage::instance().init(settings) ) {
            _exception << "failed to init bars storage" << "\n";
            throw(_exception);
        }
        
        // Init msg bus server
        //
        if ( !_msgBusServer.init(settings) )
            return false;
        
        _settings = settings;
        _state = kStateInitialized;
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }

    if ( !status )
        clear();

    return status;
}
    
void StrategyContainer::initChannelPfs(const tw::common::Settings& settings) {
    if ( !settings._strategy_container_channel_pf ) {
        LOGGER_WARN << "channel_pfs are turned off" << "\n" << "\n";
        return;
    }
    
    // Register itself with consumer_proxy for quotes
    //
    tw::common_strat::ConsumerProxy::instance().registerCallbackQuote(this);
    
    // Create/initialize channel_pfs related objects
    //
    // TODO: right now only channel_pf_cme_onix or channel_pf_historical (cme only for now) is supported
    //
    tw::channel_pf::IChannelImpl* channelImpl = NULL;
    if ( tw::common::eChannelPfHistoricalMode::kUnknown == settings._channel_pf_historical_mode ) {
        _channelPfCme = TChannelPfCmePtr(new tw::channel_pf_cme::ChannelPfOnix());
        channelImpl = _channelPfCme.get();
        LOGGER_INFO << "Using tw::channel_pf_cme::ChannelPfOnix() for channel_pf_impl";
    } else {
        _channelPfHistorical = TChannelPfHistoricalPtr(new tw::channel_pf_historical::ChannelPfHistorical());
        channelImpl = _channelPfHistorical.get();
        LOGGER_INFO << "Using tw::channel_pf_historical::ChannelPfHistorical() for channel_pf_impl";
    }    
    
    TChannelPfPtr channelPf = TChannelPfPtr(new tw::channel_pf::Channel());
    if ( !channelPf->init(settings, channelImpl) ) {
        _exception << "failed to initialize channel_pf" << "\n";
        throw(_exception);
    }
    
    _channelsPfs[tw::instr::eExchange::kCME] = channelPf;       
}

void StrategyContainer::initChannelOrs(const tw::common::Settings& settings) {
    // Init risk storage
    //
    if ( !tw::risk::RiskStorage::instance().init(settings) ) {
        _exception << "failed to init RiskStorage" << "\n";
        throw(_exception);
    }

    // Start risk storage
    //
    if ( !tw::risk::RiskStorage::instance().start() ) {
        _exception << "failed to start RiskStorage" << "\n";
        throw(_exception);
    }
    
    if ( !settings._strategy_container_channel_or ) {
        LOGGER_WARN << "channel_ors are turned off" << "\n" << "\n";
        return;
    }

    // Get trading account
    //
    if ( !tw::risk::RiskStorage::instance().getAccount(_account, settings._trading_account) ) {
        _exception << "NO configuration for trading account: " << settings._trading_account << "\n";
        throw(_exception);
    }

    LOGGER_INFO << "Loaded trading account: " << _account.toStringVerbose() << "\n";
    
    // Disable all strategies on start up
    //
    if ( !tw::risk::RiskStorage::instance().disableStratsForAccount(_account) ) {
        _exception << "Can't disable strategies for trading account: " << settings._trading_account << "\n";
        throw(_exception);
    }
    
    // Init channel or storage
    //
    if ( !tw::channel_or::ChannelOrStorage::instance().init(settings) ) {
        _exception << "failed to init ChannelOrStorage" << "\n";
        throw(_exception);
    }
    
    // Register itself with consumer_proxy for order acks
    //
    tw::common_strat::ConsumerProxy::instance().registerCallbackOrderResp(this);
    tw::common_strat::ConsumerProxy::instance().registerCallbackAlert(this);
    
    // Initialize channel_ors related objects
    //
    if ( !_channelOrProcessorOut.init(settings) ) {
        _exception << "failed to initialize channel_or processor out" << "\n";
        throw(_exception);
    }
}

bool StrategyContainer::doCrashRecovery() {
    try {
        // Get all open orders
        //
        {
            std::vector<tw::channel_or::Order> openOrders;
            if ( !tw::channel_or::ChannelOrStorage::instance().getOpenOrdersForAccount(openOrders, _account._id) ) {
                LOGGER_ERRO << "failed to get OpenOrders from ChannelOrStorage" << "\n";
                return false;
            }
            
            std::vector<tw::channel_or::Order>::iterator iter = openOrders.begin();
            std::vector<tw::channel_or::Order>::iterator end = openOrders.end();
            for ( ; iter != end; ++iter ) {
                // Associate order with the right strategy
                //
                IStrategy* strategy = getStrategy((*iter)._accountId, (*iter)._strategyId); 
                if ( !strategy ) {
                    LOGGER_ERRO << "Failed to find strategy for order: " << (*iter).toString() << "\n";
                    return false;
                }
                
                tw::channel_or::Reject rej;
                tw::channel_or::TOrderPtr order = tw::channel_or::ProcessorOrders::instance().createOrder(false);
                *order = *iter;
                
                if ( !strategy->rebuildOrder(order, rej) ) {
                    LOGGER_ERRO << "Failed to rebuild order: " << (*iter).toString() << " :: " << rej.toString() << "\n";
                    return false;
                }                        
                        
                if  ( !_channelOrProcessorOut.rebuildOrder(order, rej) ) {
                    LOGGER_ERRO << "Failed to rebuild order: " << (*iter).toString() << " :: " << rej.toString() << "\n";
                    return false;
                }
            }
        }
        
        // Get all positions
        //
        {
            std::vector<tw::channel_or::PosUpdate> positions;
            if ( !tw::channel_or::ChannelOrStorage::instance().getPositionsForAccount(positions, _account._id) ) {
                LOGGER_ERRO << "failed to get OpenOrders from ChannelOrStorage" << "\n";
                return false;
            }
            
            std::vector<tw::channel_or::PosUpdate>::iterator iter = positions.begin();
            std::vector<tw::channel_or::PosUpdate>::iterator end = positions.end();
            for ( ; iter != end; ++iter ) {
                if ( (*iter)._pos != 0 ) {
                    IStrategy* strategy = getStrategy((*iter)._accountId, (*iter)._strategyId); 
                    if ( !strategy ) {
                        LOGGER_ERRO << "Failed to find strategy for posUpdate: " << (*iter).toString() << "\n";
                        return false;
                    }
                    
                    strategy->rebuildPos(*iter);
                    _channelOrProcessorOut.rebuildPos(*iter);
                }
            }
        }
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }

    return true;
}

bool StrategyContainer::start() {
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
        
        if ( _settings._strategy_container_channel_pf && _channelsPfs.empty() ) {
            _exception << "channelsPfs empty" << "\n";
            throw(_exception);
        }
        
        // Start timer server
        //
        if ( !tw::common::TimerServer::instance().start() ) {
            _exception << "failed to start timer server: " << "\n";
            throw(_exception);
        }
        
        if ( _settings._strategy_container_channel_or ) {
            // Start channel or storage
            //
            if ( !tw::channel_or::ChannelOrStorage::instance().start() ) {
                _exception << "failed to start ChannelOrStorage" << "\n";
                throw(_exception);
            }

            // Perform crash recovery
            //
            if ( !doCrashRecovery() ) {
                _exception << "failed in doCrashRecovery()" << "\n";
                throw(_exception);
            }
        }
        
        // Start bars storage
        //
        if ( !tw::common_trade::BarsStorage::instance().start() ) {
            _exception << "failed to start bars storage" << "\n";
            throw(_exception);
        }
        
        // Start all strategies
        //
        {
            TStrategies::iterator iter = _strategies.begin();
            TStrategies::iterator end = _strategies.end();

            for ( ; iter != end; ++iter ) {
                if ( !iter->second->start() ) {
                    _exception << "Failed to start strategy" << iter->second->getName() << "\n";
                    throw(_exception);
                }

                LOGGER_INFO << "Started strategy: " << iter->second->getName() << "\n";
            }
        }
        
        // Start bars manager factory
        //
        if ( !tw::common_trade::BarsManagerFactory::instance().start(_settings) ) {
            _exception << "failed to start bars manager factory" << "\n";
            throw(_exception);
        }
        
        // Start all channel_pfs
        //
        if ( _settings._strategy_container_channel_pf ) {
            TChannelsPf::iterator iter = _channelsPfs.begin();
            TChannelsPf::iterator end = _channelsPfs.end();
            for ( ; iter != end; ++ iter ) {
                if ( !(iter->second->start()) ) {
                    _exception << "Failed to start channel_pf" << "\n";
                    throw(_exception);
                }
            }
        }
        
        // Start all channel_ors
        //
        if ( _settings._strategy_container_channel_or ) {
            if ( !(_channelOrProcessorOut.start()) ) {
                _exception << "Failed to start channel_or" << "\n";
                throw(_exception);
            }
        }
        
        // Start msg bus server
        //
        if ( !_msgBusServer.start(this, true) ) {
            _exception << "Failed to start msgBusServer" << "\n";
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
        stop();

    return status;
}

void  StrategyContainer::stop() {    
    try {
        if ( getState() == kStateUninitialized )
            return;
        
        LOGGER_INFO << "Stopping..." << "\n";
        
        // Stop timer server
        //
        if ( !tw::common::TimerServer::instance().stop() )
            LOGGER_ERRO << "Failed to stop timer server" << "\n";
        
        TChannelsPf channelsPfs;
        {
            // Lock for thread synchronization
            //
            tw::common_thread::LockGuard<TLock> lock(_lock);

            // Copy all channel pfs to stop outside the thread lock
            //
            if ( _settings._strategy_container_channel_pf )
                channelsPfs = _channelsPfs;
        }
        
        // Stop all channel_pfs
        //
        TChannelsPf::iterator iter = channelsPfs.begin();
        TChannelsPf::iterator end = channelsPfs.end();
        for ( ; iter != end; ++ iter ) {
            iter->second->stop();
        }
        
        {
            // Lock for thread synchronization
            //
            tw::common_thread::LockGuard<TLock> lock(_lock);

            // Stop all strategies
            //
            TStrategies::iterator iter = _strategies.begin();
            TStrategies::iterator end = _strategies.end();

            for ( ; iter != end; ++iter ) {
                if ( !iter->second || !iter->second->stop() )
                    LOGGER_ERRO << "Failed to stop strategy" << (!iter->second ? "NULL strategy" : iter->second->getName()) << "\n";
                else            
                    LOGGER_INFO << "Stopped strategy: " << iter->second->getName() << "\n";
            }
        }
        
        // Stop all channel_ors
        //
        if ( _settings._strategy_container_channel_or ) {
            // Cancel all open order
            //
            _thread = tw::common_thread::ThreadPtr(new tw::common_thread::Thread(boost::bind(&StrategyContainer::ThreadMain, this)));
            if ( _thread != NULL ) {
                _thread->join();
                _thread = tw::common_thread::ThreadPtr();
            }
            
            // Stop msg bus server
            //
            if ( !_msgBusServer.stop() )
                LOGGER_ERRO << "Failed to stop msgBusServer" << "\n";
            
            _channelOrProcessorOut.stop();
            
            // Stop channel or storage
            //
            tw::channel_or::ChannelOrStorage::instance().stop();
        } else {
            // Stop msg bus server
            //
            if ( !_msgBusServer.stop() )
                LOGGER_ERRO << "Failed to stop msgBusServer" << "\n";
        }
        
        // Stop risk storage
        //
        tw::risk::RiskStorage::instance().stop();
        
        // Stop bars storage
        //
        tw::common_trade::BarsStorage::instance().stop();
        
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<TLock> lock(_lock);
        _state = kStateUninitialized;
        
        LOGGER_INFO << "Stopped" << "\n";
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }

    clear();
}
    
void StrategyContainer::addStrategy(tw::common_strat::IStrategy* strategy) {
    if ( !strategy || strategy->getName().empty() ) {
        _exception << "strategy is NULL or not named" << "\n";
        throw(_exception);
    }
    
    TStrategies::iterator iter = _strategies.find(strategy->getName());
    if ( iter != _strategies.end() ) {
        _exception << "Can't add already existing strategy: " << strategy->getName() << "\n";
        throw(_exception);
    }
    
    tw::risk::Strategy strategyParams;
    strategyParams._name = strategy->getName();    
    
    if ( _settings._strategy_container_channel_or ) {
        strategyParams._accountId = _account._id;    
        if ( !tw::risk::RiskStorage::instance().getOrCreateStrategy(strategyParams) ) {
            _exception << "Can't getOrCreateStrategy strategy: " << strategy->getName() << "\n";
            throw(_exception);
        }
    }
    
    if ( !strategy->init(_settings, strategyParams) ) {
        _exception << "Failed to init strategy: " << strategy->getName() << "\n";
        throw(_exception);
    }
    
    strategyParams._lastStarted = tw::common::THighResTime::now().toString();
    if ( !tw::risk::RiskStorage::instance().saveStrategy(strategyParams) ) {
        _exception << "Can't saveStrategy strategy: " << strategy->getName() << "\n";
        throw(_exception);
    }
    
    _strategies.insert(TStrategies::value_type(strategy->getName(), strategy));
    
    
}

void StrategyContainer::removeStrategy(tw::common_strat::IStrategy* strategy) {
    if ( !strategy ) {
        _exception << "strategy is NULL" << "\n";
        throw(_exception);
    }
    
    TStrategies::iterator iter = _strategies.find(strategy->getName());
    if ( iter == _strategies.end() ) {
        _exception << "Can't remove non existing strategy: " << strategy->getName() << "\n";
        throw(_exception);
    }
    
    _strategies.erase(iter);
    
    LOGGER_INFO << "Removed strategy: " << strategy->getParams().toStringVerbose() << "\n";
}

inline StrategyContainer::eState StrategyContainer::getState() const {
    // Lock for thread synchronization
    //
    tw::common_thread::LockGuard<TLock> lock(const_cast<StrategyContainer*>(this)->_lock); 

    return _state;
}
    
inline void StrategyContainer::onQuote(const tw::price::QuoteStore::TQuote& quote) {
    try {
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<TLock> lock(_lock);

        quote.notifySubscribers();
        
        if ( _settings._trading_print_pnl )
            tw::channel_or::PnLAuditTrail::instance().onQuote(quote);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

inline bool StrategyContainer::onTimeout(tw::common::TimerClient* client, tw::common::TTimerId id) {
    try {
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<TLock> lock(_lock);

        if ( client )
            return client->onTimeout(id);
        
        return true;
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    return false;
}

// Outbound ChannelOr related methods
//
bool StrategyContainer::sendNew(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej) {
    bool status = true;
    try {
        tw::common::THighResTime now = tw::common::THighResTime::now();
        
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<TLock> lock(_lock);
        
        if ( !tw::channel_or::ChannelOrStorage::instance().canPersist() ) {
            rej = getRej();
            rej._text = "Can't persist request";
            return false;
        }
        
        // fix for account risk processor failure observed in ve_alpha on 2014-05-19
        status = _channelOrProcessorOut.sendNew(order, rej);
        if ( !status )
            order->_client.onNewRej(order, rej);
        
        // Mark order timers
        //
        if ( !order ) {
            LOGGER_ERRO << "order is null after reject: " << rej << "\n";
            return false;
        }
        
        order->_timestamp1 = now;
        if ( status )
            tw::channel_or::ChannelOrStorage::instance().persist(order);
        else
            tw::channel_or::ChannelOrStorage::instance().persist(order, rej);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    return status;
}

bool StrategyContainer::sendMod(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej) {
    bool status = true;
    try {
        tw::common::THighResTime now = tw::common::THighResTime::now();
        
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<TLock> lock(_lock);
        
        if ( !tw::channel_or::ChannelOrStorage::instance().canPersist() ) {
            rej = getRej();
            rej._text = "Can't persist request";
            return false;
        }        
        
        status = _channelOrProcessorOut.sendMod(order, rej);
        if ( !status ) {
            if ( tw::channel_or::eRejectSubType::kProcessorOrders == rej._rejSubType )
                tw::channel_or::ProcessorOrders::instance().onModRej(order, rej);
            
            order->_client.onModRej(order, rej);
            tw::channel_or::ProcessorOrders::instance().onModRejPost(order, rej);
        }
        
        // Mark order timers
        //
        order->_timestamp1 = now;
        if ( status )
            tw::channel_or::ChannelOrStorage::instance().persist(order);
        else
            tw::channel_or::ChannelOrStorage::instance().persist(order, rej);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    return status;
}

bool StrategyContainer::sendCxl(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej) {
    bool status = true;
    try {
        if ( tw::channel_or::eOrderState::kCancelling == order->_state )
            return true;
        
        tw::common::THighResTime now = tw::common::THighResTime::now();
        
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<TLock> lock(_lock);        
        
        status = _channelOrProcessorOut.sendCxl(order, rej);
        if ( !status )
            order->_client.onCxlRej(order, rej);
        
        // Mark order timers
        //
        order->_timestamp1 = now;
        if ( status )
            tw::channel_or::ChannelOrStorage::instance().persist(order);
        else
            tw::channel_or::ChannelOrStorage::instance().persist(order, rej);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    return status;
}

bool StrategyContainer::rebuildOrder(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej) {
    // Not implemented - nothing to do
    //
    return true;
}

// Batch ChannelOr cancels
//
void StrategyContainer::sendCxlForAccount(const tw::channel_or::TAccountId& x) {
    try {
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<TLock> lock(_lock);
        
        std::string reason = "Batch cxl for account: " + boost::lexical_cast<std::string>(x);
        sendCxlForBatch(tw::channel_or::ProcessorOrders::instance().getAllForAccount(x), reason);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void StrategyContainer::sendCxlForAccountStrategy(const tw::channel_or::TAccountId& x, const tw::channel_or::TStrategyId& y) {
    try {
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<TLock> lock(_lock);
        
        std::string reason = "Batch cxl for account :: strategy : " + boost::lexical_cast<std::string>(x) + " :: " + boost::lexical_cast<std::string>(y);
        sendCxlForBatch(tw::channel_or::ProcessorOrders::instance().getAllForAccountStrategy(x, y), reason);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void StrategyContainer::sendCxlForInstrument(const tw::instr::Instrument::TKeyId& x) {
    try {
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<TLock> lock(_lock);
        
        std::string reason = "Batch cxl for instrument: " + boost::lexical_cast<std::string>(x);
        sendCxlForBatch(tw::channel_or::ProcessorOrders::instance().getAllForInstrument(x), reason);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void StrategyContainer::sendCxlAll() {
    try {
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<TLock> lock(_lock);
        
        std::string reason = "Batch cxl for all open orders";
        sendCxlForBatch(tw::channel_or::ProcessorOrders::instance().getAll(), reason);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void StrategyContainer::sendCxlForBatch(const tw::channel_or::TOrders& orders, const std::string& reason) {
    try {
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<TLock> lock(_lock);
        
        tw::channel_or::TOrders::const_iterator iter = orders.begin();
        tw::channel_or::TOrders::const_iterator end = orders.end();
        tw::channel_or::Reject rej;
        for ( ; iter != end; ++iter ) {
            (*iter)->_stratReason = reason;
            sendCxl(*iter, rej);            
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

// channel_or ProcessorIn interface
//
inline void StrategyContainer::onOrderResp(const tw::channel_or::OrderResp& orderResp) {
    try {
       static tw::channel_or::Alert alert;
       static tw::channel_or::Reject rej;
       
       tw::common::THighResTime now = tw::common::THighResTime::now();

       // Lock for thread synchronization
       //
       tw::common_thread::LockGuard<TLock> lock(_lock);

       tw::channel_or::TOrderPtr& order = const_cast<tw::channel_or::TOrderPtr&>(orderResp._order);
       if ( !order ) {
           switch ( orderResp._type ) {
               case tw::channel_or::eOrderRespType::kNewAck:                
               case tw::channel_or::eOrderRespType::kNewRej:                
               case tw::channel_or::eOrderRespType::kModAck:                
               case tw::channel_or::eOrderRespType::kModRej:                
               case tw::channel_or::eOrderRespType::kCxlAck:                
               case tw::channel_or::eOrderRespType::kCxlRej:
                   alert._type = channel_or::eAlertType::kUntrackedOrder;
                   break;
               case tw::channel_or::eOrderRespType::kPartFill:                
               case tw::channel_or::eOrderRespType::kFill:
                   alert._type = channel_or::eAlertType::kUntrackedFill;
                   break;
               case tw::channel_or::eOrderRespType::kTradeBreak:
                   alert._type = channel_or::eAlertType::kUntrackedBustedFill;
                   break;
               default:
                   alert._type = channel_or::eAlertType::kUnsupportedOrderResp;
                   break;
           }

           alert._text = orderResp.toString();
           onAlert(alert);

           return;
       }

       bool status = true;
       switch ( orderResp._type ) {            
           case tw::channel_or::eOrderRespType::kNewAck:
               if ( (status = tw::channel_or::Translator::translateNewAck(orderResp, order)) )
                   onNewAck(order, now);
               break;
           case tw::channel_or::eOrderRespType::kNewRej:
               if ( (status = tw::channel_or::Translator::translateNewRej(orderResp, order, rej)) )
                   onNewRej(order, rej, now);
               break;
           case tw::channel_or::eOrderRespType::kModAck:
               if ( (status = tw::channel_or::Translator::translateModAck(orderResp, order)) )
                   onModAck(order, now);
               break;
           case tw::channel_or::eOrderRespType::kModRej:
               if ( (status = tw::channel_or::Translator::translateModRej(orderResp, order, rej)) )
                   onModRej(order, rej, now);
               break;
           case tw::channel_or::eOrderRespType::kCxlAck:
               if ( (status = tw::channel_or::Translator::translateCxlAck(orderResp, order)) )
                   onCxlAck(order, now);
               break;
           case tw::channel_or::eOrderRespType::kCxlRej:
               if ( (status = tw::channel_or::Translator::translateCxlRej(orderResp, order, rej)) )
                   onCxlRej(order, rej, now);
               break;
           case tw::channel_or::eOrderRespType::kPartFill:
           case tw::channel_or::eOrderRespType::kFill:
           case tw::channel_or::eOrderRespType::kTradeBreak:
           {
               tw::channel_or::Fill fill;
               fill._order = order;
               if ( (status = tw::channel_or::Translator::translateFill(orderResp, fill)) )
                   onFill(fill, order, now);
           }
               break;
           default:
               alert._type = channel_or::eAlertType::kUnsupportedOrderResp;
               alert._text = orderResp.toString();               
               onAlert(alert);               
               break;
       }
       
       if ( !status ) {
           alert._type = channel_or::eAlertType::kErrorTranslatingOrderResp;
           alert._text = orderResp.toString();               
           onAlert(alert); 
       }

    } catch(const std::exception& e) {
       LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
       LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    } 
}

inline void StrategyContainer::onAlert(const tw::channel_or::Alert& alert) {
    try {
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<TLock> lock(_lock);        
        
        switch ( alert._type ) {
            case tw::channel_or::eAlertType::kStuckOrders:
            {
                IStrategy* strat = getStrategy(alert._strategyId);
                if ( strat )
                    strat->onAlert(alert);
                else
                   LOGGER_ERRO << "Can't find strategy for alert: "  << alert.toString() << "\n" << "\n"; 
            }
                break;
            case tw::channel_or::eAlertType::kExchangeUp:
            case tw::channel_or::eAlertType::kExchangeDown:
            {
                TStrategies::iterator iter = _strategies.begin();
                TStrategies::iterator end = _strategies.end();
                for ( ; iter != end; ++iter ) {
                    iter->second->onAlert(alert);
                }
            }
                break;
            default:
                _channelOrProcessorIn.onAlert(alert);
                break;
        }
        
        tw::common::Command cmnd;
        cmnd = alert.toCommand();
        cmnd._type = tw::common::eCommandType::kChannelOr;
        cmnd._subType = tw::common::eCommandSubType::kAlert;

        tw::common_strat::StrategyContainer::instance().sendToAllMsgBusConnections(cmnd.toString()+"\n");
        LOGGER_WARN << "Send alert: "  << alert.toString() << "\n";
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

inline void StrategyContainer::onNewAck(tw::channel_or::TOrderPtr& order, const tw::common::THighResTime& now) {
    try {
        _channelOrProcessorIn.onNewAck(order);
        order->_client.onNewAck(order);
        
        {
            // Mark order timers
            //
            tw::common::THighResTimeScope orderTimer(now, order->_timestamp2, order->_timestamp3);
        }
        
        tw::channel_or::ChannelOrStorage::instance().persist(order);
        checkCancelOnAck(order);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

inline void StrategyContainer::onNewRej(tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej, const tw::common::THighResTime& now) {
    try {
        _channelOrProcessorIn.onNewRej(order, rej);
        order->_client.onNewRej(order, rej);

        {
            // Mark order timers
            //
            tw::common::THighResTimeScope orderTimer(now, order->_timestamp2, order->_timestamp3);
        }
        
        tw::channel_or::ChannelOrStorage::instance().persist(order, rej);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

inline void StrategyContainer::onModAck(tw::channel_or::TOrderPtr& order, const tw::common::THighResTime& now) {
    try {
        _channelOrProcessorIn.onModAck(order);
        order->_client.onModAck(order);
        tw::channel_or::ProcessorOrders::instance().onModAckPost(order);

        {
            // Mark order timers
            //
            tw::common::THighResTimeScope orderTimer(now, order->_timestamp2, order->_timestamp3);
        }

        tw::channel_or::ChannelOrStorage::instance().persist(order);
        checkCancelOnAck(order);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

inline void StrategyContainer::onModRej(tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej, const tw::common::THighResTime& now) {
    try {
        _channelOrProcessorIn.onModRej(order, rej);
        order->_client.onModRej(order, rej);
        tw::channel_or::ProcessorOrders::instance().onModRejPost(order, rej);
        
        {
            // Mark order timers
            //
            tw::common::THighResTimeScope orderTimer(now, order->_timestamp2, order->_timestamp3);
        }
        
        tw::channel_or::ChannelOrStorage::instance().persist(order, rej);
        checkCancelOnAck(order);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

inline void StrategyContainer::onCxlAck(tw::channel_or::TOrderPtr& order, const tw::common::THighResTime& now) {
    try {
        _channelOrProcessorIn.onCxlAck(order);
        order->_client.onCxlAck(order);
        
        {
            // Mark order timers
            //
            tw::common::THighResTimeScope orderTimer(now, order->_timestamp2, order->_timestamp3);
        }
        
        tw::channel_or::ChannelOrStorage::instance().persist(order);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

inline void StrategyContainer::onCxlRej(tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej, const tw::common::THighResTime& now) {
    try {
        _channelOrProcessorIn.onCxlRej(order, rej);
        order->_client.onCxlRej(order, rej);

        {
            // Mark order timers
            //
            tw::common::THighResTimeScope orderTimer(now, order->_timestamp2, order->_timestamp3);
        }

        tw::channel_or::ChannelOrStorage::instance().persist(order, rej);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

inline void StrategyContainer::onFill(tw::channel_or::Fill& fill, tw::channel_or::TOrderPtr& order, const tw::common::THighResTime& now) {
    try {
        _channelOrProcessorIn.onFill(fill);
        order->_client.onFill(fill);

        {
            // Mark order timers
            //
            tw::common::THighResTimeScope orderTimer(now, order->_timestamp2, order->_timestamp3);
            fill._timestamp2 = order->_timestamp2;
            fill._timestamp3 = order->_timestamp3;
        }

        tw::channel_or::ChannelOrStorage::instance().persist(fill);
        if ( _settings._trading_print_pnl )
            tw::channel_or::PnLAuditTrail::instance().onFill(fill);        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void StrategyContainer::onConnectionUp(TConnection::native_type id, TConnection::pointer connection) {
    try {
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<TLock> lock(_lock);
        
        // Notify all strategies
        //
        TStrategies::iterator iter = _strategies.begin();
        TStrategies::iterator end = _strategies.end();
        for ( ; iter != end; ++iter ) {
            iter->second->onConnectionUp(id, connection);
        }
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void StrategyContainer::onConnectionDown(TConnection::native_type id) {
    try {
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<TLock> lock(_lock);
        
        // Notify all strategies
        //
        TStrategies::iterator iter = _strategies.begin();
        TStrategies::iterator end = _strategies.end();
        for ( ; iter != end; ++iter ) {
            iter->second->onConnectionDown(id);
        }
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void StrategyContainer::onConnectionData(TConnection::native_type id, const std::string& message) {
    try {
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<TLock> lock(_lock);
        
        // TODO: right now all commands are being passed to strategy(ies) - need to
        // partition command better later
        //
        if ( _settings._trading_verbose )
            LOGGER_INFO << "raw command = " << message << "\n";
        
        tw::common::Command cmnd;
        if ( !cmnd.fromString(message) ) {
            LOGGER_ERRO << "Not a valid command: " << message << "\n";
            return;
        }
        cmnd._connectionId = id;
        
        // Persist to database
        //
        if ( _settings._strategy_container_channel_or )
            tw::channel_or::ChannelOrStorage::instance().persist(cmnd);
        
        switch ( cmnd._type ) {
            case tw::common::eCommandType::kQuoteStore:
                if ( tw::price::QuoteStore::instance().processCommand(cmnd) )
                    sendToMsgBusConnection(id, cmnd.toString()+"\n");
                break;
            case tw::common::eCommandType::kChannelPf:                
            {
                TStrategies::iterator iter = _strategies.begin();
                TStrategies::iterator end = _strategies.end();
                
                // Notify all strategies
                //                
                for ( ; iter != end; ++iter ) {
                    iter->second->onConnectionData(id, cmnd);
                }
            }
                break;
            case tw::common::eCommandType::kStrat:                
            {
                TStrategies::iterator iter = _strategies.begin();
                TStrategies::iterator end = _strategies.end();
                
                // Check if strategyId is present, then send command only
                // to specified strategy
                //
                if ( cmnd.has("strategyId") ) {
                    tw::channel_or::TStrategyId stratId;
                    cmnd.get("strategyId", stratId);
                    for ( ; iter != end; ++iter ) {
                        if ( iter->second->getStrategyId() == stratId ) {
                            iter->second->onConnectionData(id, cmnd);
                            return;
                        }
                    }
                    
                    LOGGER_ERRO << "Can't find strategyId: "  << stratId << "\n" << "\n";
                    return;
                }
                
                // Check if strategy name is present, then send command only
                // to specified strategy
                //
                iter = _strategies.begin();
                end = _strategies.end();
                
                if ( cmnd.has("strategyName") ) {
                    std::string name;
                    cmnd.get("strategyName", name);
                    for ( ; iter != end; ++iter ) {
                        if ( iter->second->getName() == name ) {
                            iter->second->onConnectionData(id, cmnd);
                            return;
                        }
                    }
                    
                    LOGGER_ERRO << "Can't find strategy: "  << name << "\n" << "\n";
                    return;
                }
                
                // Notify all strategies
                //                
                for ( ; iter != end; ++iter ) {
                    iter->second->onConnectionData(id, cmnd);
                }
            }
                break;
            case tw::common::eCommandType::kChannelOrStorage:
                tw::channel_or::ChannelOrStorage::instance().onCommand(cmnd);
                break;
            default:
                if ( tw::common::eCommandType::kChannelOr == cmnd._type && tw::common::eCommandSubType::kExtFill ==  cmnd._subType )
                    processExternalFill(cmnd);                    
                else
                    _channelOrProcessorOut.onCommand(cmnd);
                
                break;
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void StrategyContainer::onCommand(const tw::common::Command& cmnd) {
    try {
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<TLock> lock(_lock);
        
        switch ( cmnd._type ) {
            case tw::common::eCommandType::kStrat:                
            {
                TStrategies::iterator iter = _strategies.begin();
                TStrategies::iterator end = _strategies.end();
                
                // Check if strategyId is present, then send command only
                // to specified strategy
                //
                if ( cmnd.has("strategyId") ) {
                    tw::channel_or::TStrategyId stratId;
                    cmnd.get("strategyId", stratId);
                    for ( ; iter != end; ++iter ) {
                        if ( iter->second->getStrategyId() == stratId ) {
                            iter->second->onCommand(cmnd);
                            return;
                        }
                    }
                    
                    LOGGER_ERRO << "Can't find strategyId: "  << stratId << "\n" << "\n";
                    return;
                }
                
                // Check if strategy name is present, then send command only
                // to specified strategy
                //
                iter = _strategies.begin();
                end = _strategies.end();
                
                if ( cmnd.has("strategyName") ) {
                    std::string name;
                    cmnd.get("strategyName", name);
                    for ( ; iter != end; ++iter ) {
                        if ( iter->second->getName() == name ) {
                            iter->second->onCommand(cmnd);
                            return;
                        }
                    }
                    
                    LOGGER_ERRO << "Can't find strategy: "  << name << "\n" << "\n";
                    return;
                }
                
                // Notify all strategies
                //                
                for ( ; iter != end; ++iter ) {
                    iter->second->onCommand(cmnd);
                }
            }
                break;            
            default:
                break;
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void StrategyContainer::sendToMsgBusConnection(TConnection::native_type id, const std::string& message) {
    try {
        _msgBusServer.sendToConnection(id, message);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void StrategyContainer::sendToAllMsgBusConnections(const std::string& message) {
    try {
        _msgBusServer.sendToAll(message);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void StrategyContainer::ThreadMain() {
    LOGGER_INFO << "Started cancelling open orders" << "\n";
    
    try {
        // Try to cancel open orders for 5 secs max and bail out after that
        //
        tw::channel_or::TOrders orders;
        for ( int32_t i = 0; i < 5; ++i ) {
            orders = tw::channel_or::ProcessorOrders::instance().getAll();
            if ( orders.empty() )
                break;
            
            LOGGER_WARN << "Cancelling open orders on exit numbering: " << orders.size() << "\n";
            sendCxlAll();
            
            LOGGER << "." << "\n";
            
            // Sleep for 1 sec
            //
            tw::common_thread::sleep(1000);
        }
        
        if ( orders.empty() )
            LOGGER_WARN << "No open orders left" << "\n";
        else
            LOGGER_WARN << "Didn't finish cancelling open orders on exit numbering: " << orders.size() << "\n";
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    LOGGER_INFO << "Finished canceling open orders" << "\n";
}

void StrategyContainer::processExternalFill(const tw::common::Command& cmnd) {
    try {
        tw::channel_or::FillExternal fillExternal = tw::channel_or::FillExternal::fromCommand(cmnd);
       
        // Check external fill
        //
        if ( _account._name != fillExternal._account ) {
            LOGGER_ERRO << "Accounts don't match: "  << _account._name << " :: " << fillExternal._account << "\n";
            return;
        }
        
        TStrategies::iterator iter = _strategies.find(fillExternal._strategy);
        if ( iter == _strategies.end() ) {
            LOGGER_ERRO << "Can't find strategy: "  << fillExternal._strategy << "\n";
            return;
        }
        
        tw::instr::InstrumentPtr instrument = tw::instr::InstrumentManager::instance().getByDisplayName(fillExternal._displayName);
        if ( !instrument ) {
            LOGGER_ERRO << "Can't find instrument for: "  << fillExternal._displayName << "\n";
            return;
        }        
            
        tw::channel_or::Fill fill;
        fill._type = tw::channel_or::eFillType::kExternal;
        fill._subType = tw::channel_or::eFillSubType::kOutright;
        fill._accountId = _account._id;
        fill._strategyId = iter->second->getStrategyId();
        fill._instrumentId = instrument->_keyId;
        fill._orderId = fill._fillId = tw::channel_or::UuidFactory::instance().get();
        fill._side = fillExternal._side;
        fill._qty = fillExternal._qty;
        fill._price = instrument->_tc->fromExchangePrice(fillExternal._price);
        fill._liqInd = fillExternal._liqInd;
        fill._pickOff = false;
        fill._exchangeFillId = fillExternal._exchangeFillId;
        
        fill._exTimestamp = fill._timestamp1 = fill._timestamp2 = fill._timestamp3 = tw::common::THighResTime::now();
                                
        _channelOrProcessorOut.recordFill(fill);
        iter->second->recordExternalFill(fill);
        tw::channel_or::ChannelOrStorage::instance().persist(fill);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}
    

} // channel_pf
} // tw

