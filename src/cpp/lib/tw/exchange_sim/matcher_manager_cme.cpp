#include <tw/exchange_sim/matcher_manager_cme.h>
#include <tw/instr/instrument_manager.h>
#include <tw/channel_or_cme/translator.h>
#include <tw/common_strat/consumer_proxy.h>
#include <tw/channel_or_cme/translator.h>
#include <tw/common_thread/utils.h>

namespace tw {
namespace exchange_sim {
    
using namespace OnixS::FIX::FIX42; 

MatcherManagerCME::MatcherManagerCME() {
    clear();
}

MatcherManagerCME::~MatcherManagerCME() {
    stop();
}

void MatcherManagerCME::clear() {    
    _done = false;
    
    _matchers.clear();
    _sessions.clear();
    
    _connectionsSubs.clear();
    _connectionsSubsIds.clear();
    
    _matcherParams.clear();    
    _channelPfCme.reset();
    _channelPf.reset();
}

bool MatcherManagerCME::start(const tw::common::Settings& settings) {
    bool status = true;
    try {
        // Parse settings
        //
        tw::channel_or_cme::Settings cme_settings;
        if ( !cme_settings.parse(settings._channel_or_cme_dataSource) ) {
            LOGGER_ERRO << "failed to parse settings: " << settings.toString() << "\n";
            return false;
        }
        
        // Load instruments
        //
        if ( !tw::instr::InstrumentManager::instance().loadInstruments(settings) ) {
            LOGGER_ERRO << "failed to load instruments: " << settings._instruments_dataSource << "\n";
            return false;
        }
        
        // Register single threaded proxy, since thread synchronization is done at matcher's level
        //
        if ( !tw::common_strat::ConsumerProxy::instance().registerCallbackQuote(&_proxy) ) {
            LOGGER_ERRO << "failed to register callback for quotes" << "\n";
            return false;
        }
        
        if ( !tw::common_strat::ConsumerProxy::instance().registerCallbackTimer(&_proxy) ) {
            LOGGER_ERRO << "failed to register callback for timeouts" << "\n";
            return false;
        }
        
        // Init msgBus
        //
        if ( !_msgBus.init(settings) ) {
            LOGGER_ERRO << "failed to start msgBus" << "\n";
            return false;
        }
        
        // Init channel pf
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
        
        _channelPf = TChannelPfPtr(new tw::channel_pf::Channel());
        if ( !_channelPf->init(settings, channelImpl) ) {
            LOGGER_ERRO << "failed to initialize channel_pf or channel_pf_cme" << "\n";
            return false;
        }
        
        // Initialize matchers for all instruments
        //
        tw::instr::InstrumentManager::TInstruments instruments = tw::instr::InstrumentManager::instance().getByExchange(tw::instr::eExchange::kCME);
        if ( instruments.empty() ) {
            LOGGER_ERRO << "No instruments configured for CME" << "\n" << "\n";        
            return false;
        }        
        
        _matcherParams = cme_settings._exchange_sim_params;
        LOGGER_INFO << "Matcher params: " << _matcherParams.toString() << "\n";
        
        {
            tw::instr::InstrumentManager::TInstruments::iterator iter = instruments.begin();
            tw::instr::InstrumentManager::TInstruments::iterator end = instruments.end();
            
            for ( ; iter != end; ++iter ) {
                TMatcherPtr matcher = TMatcherPtr(new Matcher(*this, (*iter)));
                if ( !matcher->init(_matcherParams) )
                    return false;
                
                if ( !_channelPf->subscribe(*iter, matcher.get()) ) {
                    LOGGER_ERRO << "Can't subscribe to channel_pf for: " << (*iter)->toString() << "\n";
                    return false;
                }
                _matchers[(*iter)->_displayName] = matcher;
                LOGGER_ERRO << "Added matcher for: " << (*iter)->_displayName << "::" << (*iter)->toString() << "\n";
            }
        }
    
        // Start global OnixS engine for all CME connections
        //
        OnixS::FIX::EngineSettings engineSettings;
        engineSettings.dialect(cme_settings._global_fixDialectDescriptionFile);
        engineSettings.licenseStore(cme_settings._global_licenseStore);
        engineSettings.logDirectory(cme_settings._global_logDirectory);
        engineSettings.listenPort(cme_settings._global_exchange_sim_port);
        
        OnixS::FIX::Engine::init(engineSettings);
        
        // Start all configured sessions
        //
        tw::channel_or_cme::Settings::TSessions::const_iterator iter = cme_settings._sessions.begin();
        tw::channel_or_cme::Settings::TSessions::const_iterator end = cme_settings._sessions.end();
        for ( ; iter != end; ++iter ) {
            tw::channel_or_cme::TSessionSettingsPtr s = iter->second;
            
            TSessionPtr session = TSessionPtr(new OnixS::FIX::Session(s->_senderCompId,
                                                                      s->_targetCompId,
                                                                      OnixS::FIX::FIX_42,
                                                                      this));
            
            session->setEncryptionMethod(OnixS::FIX::Session::NONE);
            session->setTcpNoDelayOption();
            session->setResendRequestMaximumRange(2500);
            session->setSpecifyLastMsgSeqNumProcessedField(true);
            
            session->logonAsAcceptor();
            
            _sessions[s->_targetCompId] = session;
        }
        
        
        // Start channel pf
        //
        if ( !_channelPf->start() ) {
            LOGGER_ERRO << "failed to start channel_pf or channel_pf_cme" << "\n";
            return false;
        }
        
        // Start msgBus
        //
        if ( !_msgBus.start(this, true) ) {
            LOGGER_ERRO << "failed to start msgBus" << "\n";
            return false;
        }
        
        // Start subs thread
        //
        _done = false;        
        _thread = tw::common_thread::ThreadPtr(new tw::common_thread::Thread(boost::bind(&MatcherManagerCME::ThreadMain, this)));
        
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

void MatcherManagerCME::stop() {
    try {
        // Stop channel pf
        //
        if ( _channelPf ) {
            _channelPf->stop();
            _channelPf.reset();
            _channelPfCme.reset();
            _channelPfHistorical.reset();
        }
        
        // Disconnect all FIX sessions
        //
        TSessions::iterator iter = _sessions.begin();
        TSessions::iterator end = _sessions.begin();
        
        for ( ; iter != end; ++iter ) {
            iter->second->logout();
            iter->second->shutdown();
        }
        
        // Stop global engine
        //
        OnixS::FIX::Engine::shutdown();
        
        
        // Stop subs thread
        //
        _done = true;
        if ( _thread != NULL ) {
            _threadPipe.stop();
            _thread->join();
            _thread = tw::common_thread::ThreadPtr();
        }
        
        // Stop msgBus
        //
        if ( !_msgBus.stop() )
            LOGGER_ERRO << "failed to stop msgBus" << "\n";
        
        clear();
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void MatcherManagerCME::onNewAck(const TOrderPtr& order) {
    // Not implement - nothing to do
    //
}

void MatcherManagerCME::onModAck(const TOrderPtr& order) {
    // Not implement - nothing to do
    //
}

void MatcherManagerCME::onCxlAck(const TOrderPtr& order) {
    try {
        if ( !order->_internalCxl )
            return;
        
        TSessionPtr session = getSession(order->_targetCompId);
        if ( !session ) {
            LOGGER_ERRO << "Can't find session for: "  << order->_targetCompId << "\n" << "\n";
            return;
        }
        
        OnixS::FIX::Message ack("8", OnixS::FIX::FIX_42);
        
        ack.set(Tags::OrderID, order->_exOrderId);
        ack.set(Tags::ClOrdID, order->_clOrderId);
        ack.set(Tags::OrigClOrdID, order->_origClOrderId);
        ack.set(tw::channel_or_cme::CustomTags::CorrelationClOrdID, order->_corrClOrderId);
        
        if ( order->_instrument )
            ack.set(Tags::Symbol, order->_instrument->_symbol);
        
        ack.set(Tags::Side, (tw::channel_or::eOrderSide::kBuy == order->_side ? Values::Side::Buy : Values::Side::Sell));
        
        ack.set(Tags::ExecType, "4");
        ack.set(Tags::ExecTransType, "1"); // Cxl
        ack.set(Tags::OrdStatus,  "4"); // Cxl
        
        session->send(&ack);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void MatcherManagerCME::onFill(const TFill& fill) {
    static int32_t execId = 0;
    
    try {
        TOrderPtr order = fill._order;
        tw::price::Ticks price = fill._price;
        tw::price::Size qty = fill._qty;
        
        TSessionPtr session = getSession(order->_targetCompId);
        if ( !session ) {
            LOGGER_ERRO << "Can't find session for: "  << order->_targetCompId << "\n" << "\n";
            return;
        }
        
        std::string fillType = "2";
        if ( (order->_qty - order->_cumQty - qty) > 0 )
            fillType = "1";
        
        OnixS::FIX::Message ack("8", OnixS::FIX::FIX_42);
        
        ack.set(Tags::OrderID, order->_exOrderId);
        ack.set(Tags::ClOrdID, order->_clOrderId);
        ack.set(Tags::OrigClOrdID, order->_origClOrderId);
        ack.set(tw::channel_or_cme::CustomTags::CorrelationClOrdID, order->_corrClOrderId);
        
        ack.set(Tags::ExecType, fillType);
        ack.set(Tags::ExecTransType, "0");   // New
        ack.set(Tags::OrdStatus,  fillType); // fill/partial fill
        
        ack.set(Tags::LastShares, qty.get());
        ack.set(Tags::LastPx, order->_instrument->_tc->toExchangePrice(price), order->_instrument->_precision);
        ack.set(Tags::ExecID, ++execId);
        ack.set(Tags::ExecRefID, execId);
        
        switch ( fill._liqInd ) {
            case tw::channel_or::eLiqInd::kAdd:
                ack.set(tw::channel_or_cme::CustomTags::AggressorIndicator, 'N');
                break;
            case tw::channel_or::eLiqInd::kRem:
                ack.set(tw::channel_or_cme::CustomTags::AggressorIndicator, 'Y');
                break;
            default:
                break;
        }
                
        
        session->send(&ack);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void MatcherManagerCME::onEvent(const Matcher* matcher) {
    try {
        if ( !matcher )
            return;
        
        _threadPipe.push(matcher);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void MatcherManagerCME::sendAck(const OnixS::FIX::Message& msg,
                                OnixS::FIX::Session* sn) {
    try {
        OnixS::FIX::Message ack("8", OnixS::FIX::FIX_42);
        
        ack.set(Tags::ClOrdID, msg.get(Tags::ClOrdID));
        ack.set(Tags::OrigClOrdID, msg.get(Tags::ClOrdID));        
        
        ack.set(Tags::Symbol, msg.get(Tags::Symbol));
        ack.set(Tags::Side, msg.get (Tags::Side));
                
        char msgType = msg.getType()[0];
        switch ( msgType ) {
            case 'D':   // New
                ack.set(Tags::OrderID, msg.get(Tags::ClOrdID));
                ack.set(tw::channel_or_cme::CustomTags::CorrelationClOrdID, msg.get(Tags::ClOrdID));
                
                ack.set(Tags::ExecType, "0");
                ack.set(Tags::ExecTransType, "0"); // New
                ack.set(Tags::OrdStatus,  "0"); // New
                
                ++_matcherParams._new_ack_seq_counter;
                break;
            case 'G':   // Mod
                ack.set(Tags::OrderID, msg.get(Tags::OrderID));
                ack.set(tw::channel_or_cme::CustomTags::CorrelationClOrdID, msg.get(Tags::OrderID));
                
                ack.set(Tags::ExecType, "5");
                ack.set(Tags::ExecTransType, "0"); // New
                ack.set(Tags::OrdStatus,  "5"); // Mod
                
                ack.set(Tags::Price, msg.get(Tags::Price));
                
                ++_matcherParams._mod_ack_seq_counter;
                break;                
            case 'F':   // Cxl
                ack.set(Tags::OrderID, msg.get(Tags::OrderID));
                ack.set(tw::channel_or_cme::CustomTags::CorrelationClOrdID, msg.get(Tags::OrderID));
                
                ack.set(Tags::ExecType, "4");
                ack.set(Tags::ExecTransType, "1"); // Cxl
                ack.set(Tags::OrdStatus,  "4"); // Cxl
                
                ++_matcherParams._cxl_ack_seq_counter;
                break;
            default:
                return;
        }
        
        if ( _matcherParams._ack_delay_ms > 0 )
            tw::common_thread::sleep(_matcherParams._ack_delay_ms);
        
        sn->send(&ack);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void MatcherManagerCME::sendRej(const OnixS::FIX::Message& msg,
                                OnixS::FIX::Session* sn,
                                const std::string& reason) {
    try {
        if ( _matcherParams._rej_as_bussiness_rej ) {
            OnixS::FIX::Message ack("3", OnixS::FIX::FIX_42);
            
            ack.set(Tags::LastMsgSeqNumProcessed, msg.getSeqNum());
            ack.set(Tags::RefSeqNum, msg.getSeqNum());
            
            sn->send(&ack);
            
            return;
        }        
        
        char msgType = msg.getType()[0];
        switch ( msgType ) {
            case 'D':   // New
            {
                OnixS::FIX::Message ack("8", OnixS::FIX::FIX_42);
        
                ack.set(Tags::OrderID, msg.get(Tags::ClOrdID));
                ack.set(Tags::ClOrdID, msg.get(Tags::ClOrdID));
                ack.set(Tags::OrigClOrdID, msg.get(Tags::ClOrdID));
                ack.set(tw::channel_or_cme::CustomTags::CorrelationClOrdID, msg.get(Tags::ClOrdID));
                
                ack.set(Tags::ExecType, "8");
                ack.set(Tags::ExecTransType, "1"); // Cxl
                ack.set(Tags::OrdStatus,  "8"); // Rej

                ack.set(Tags::Symbol, msg.get(Tags::Symbol));
                ack.set(Tags::Side, msg.get (Tags::Side));
                
                ack.set(Tags::OrdRejReason, "Exchange_sim rej");
                ack.set(Tags::Text, reason);
                
                sn->send(&ack);
                
                ++_matcherParams._new_rej_seq_counter;
            }
                break;
            case 'G':   // Mod                
            case 'F':   // Cxl
            {
                OnixS::FIX::Message ack("9", OnixS::FIX::FIX_42);
                
                ack.set(Tags::OrderID, msg.get(Tags::OrderID));
                ack.set(Tags::ClOrdID, msg.get(Tags::OrigClOrdID));
                ack.set(Tags::OrigClOrdID, msg.get(Tags::OrigClOrdID));
                ack.set(tw::channel_or_cme::CustomTags::CorrelationClOrdID, msg.get(Tags::OrderID));
                
                if ( 'G' == msgType ) {
                    ack.set(Tags::CxlRejResponseTo, "2");
                    ++_matcherParams._mod_ack_seq_counter;
                } else {
                    ack.set(Tags::CxlRejResponseTo, "1");
                    ++_matcherParams._cxl_ack_seq_counter;
                }
                
                ack.set(Tags::CxlRejReason, "Exchange_sim rej");
                ack.set(Tags::Text, reason);
                
                sn->send(&ack);
            }
                break;
            default:
                return;
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void MatcherManagerCME::onInboundApplicationMsg(const OnixS::FIX::Message& msg,
                                                OnixS::FIX::Session* sn) {
     try {
        char msgType = msg.getType()[0];
        switch ( msgType ) {
            case 'D':   // New
            {
                TMatcherPtr matcher = getMatcher(msg.get(Tags::SecurityDesc));
                if ( !matcher ) {
                    sendRej(msg, sn, std::string("Invalid symbol: ") + msg.get(Tags::SecurityDesc));
                    return;
                }
                
                // Right now supporting only limit orders
                //
                if ( msg.get(Tags::OrdType) != Values::OrdType::Limit ) {
                    sendRej(msg, sn, std::string("Accepting only limit orders"));
                    return;
                }
                
                // Check if any 'new' orders counters are exceeded
                //
                if ( _matcherParams._new_ack_seq_count > -1 && _matcherParams._new_rej_seq_count > -1 ) {
                    LOGGER_INFO << "'new' counters: " << _matcherParams.toString() << "\n";
                    if ( _matcherParams._new_ack_seq_counter > _matcherParams._new_ack_seq_count ) {
                        if ( _matcherParams._new_rej_seq_counter < _matcherParams._new_rej_seq_count ) {
                            sendRej(msg, sn, std::string("new_ack_seq_counter > new_ack_seq_count"));
                            LOGGER_INFO << "sending new reject: " << _matcherParams.toString() << "\n";
                            return;
                        } else {
                            _matcherParams._new_ack_seq_count = 0;
                            _matcherParams._new_rej_seq_count = 0;
                            LOGGER_INFO << "resetting 'new' counters: " << _matcherParams.toString() << "\n";
                        }
                    }
                }
                
                TOrderPtr order = matcher->createOrder();
                order->_type = tw::channel_or::eOrderType::kLimit;
                order->_targetCompId = sn->getTargetCompID();
                
                order->_clOrderId = msg.get(Tags::ClOrdID);
                order->_origClOrderId = msg.get(Tags::ClOrdID);
                order->_corrClOrderId = msg.get(Tags::ClOrdID);
                
                order->_qty.set(msg.getInteger(Tags::OrderQty));
                order->_cumQty.set(0);
                order->_instrument = matcher->getInstrument();
                order->_price = order->_instrument->_tc->fromExchangePrice(msg.getDouble(Tags::Price));
                if ( msg.get(Tags::Side) == Values::Side::Buy )
                    order->_side = tw::channel_or::eOrderSide::kBuy;
                else
                    order->_side = tw::channel_or::eOrderSide::kSell;
                
                if ( msg.contain(Tags::TimeInForce) && msg.get(Tags::TimeInForce) == Values::TimeInForce::Immediate_or_Cancel )
                    order->_timeInForce =  tw::channel_or::eTimeInForce::kIOC;
                
                // Lock matcher for thread synchronization
                //
                tw::common_thread::LockGuard<tw::common_thread::Lock> lock(matcher->getLock());
                    
                sendAck(msg, sn);
                matcher->sendNew(order);
            }
                break;
            case 'G':   // Mod
            case 'F':   // Cxl
            {
                TMatcherPtr matcher = getMatcher(msg.get(Tags::SecurityDesc));
                if ( !matcher ) {
                    sendRej(msg, sn, std::string("Invalid symbol: ") + msg.get(Tags::SecurityDesc));
                    return;
                }                
                
                if ( 'G' == msgType ) {
                    // Check if any 'mod' orders counters are exceeded
                    //
                    if ( _matcherParams._mod_ack_seq_count > -1 && _matcherParams._mod_rej_seq_count > -1 ) {
                        LOGGER_INFO << "'mod' counters: " << _matcherParams.toString() << "\n";
                        if ( _matcherParams._mod_ack_seq_counter > _matcherParams._mod_ack_seq_count ) {
                            if ( _matcherParams._mod_rej_seq_counter < _matcherParams._mod_rej_seq_count ) {
                                sendRej(msg, sn, std::string("mod_ack_seq_counter > mod_ack_seq_count"));
                                LOGGER_INFO << "sending mod reject: " << _matcherParams.toString() << "\n";
                                return;
                            } else {
                                _matcherParams._mod_ack_seq_count = 0;
                                _matcherParams._mod_rej_seq_count = 0;
                                LOGGER_INFO << "resetting 'mod' counters: " << _matcherParams.toString() << "\n";
                            }
                        }
                    }
                } else {
                    // Check if any 'cxl' orders counters are exceeded
                    //
                    if ( _matcherParams._cxl_ack_seq_count > -1 && _matcherParams._cxl_rej_seq_count > -1 ) {
                        LOGGER_INFO << "'cxl' counters: " << _matcherParams.toString() << "\n";
                        if ( _matcherParams._cxl_ack_seq_counter > _matcherParams._cxl_ack_seq_count ) {
                            if ( _matcherParams._cxl_rej_seq_counter < _matcherParams._cxl_rej_seq_count ) {
                                sendRej(msg, sn, std::string("cxl_ack_seq_counter > cxl_ack_seq_count"));
                                LOGGER_INFO << "sending cxl reject: " << _matcherParams.toString() << "\n";
                                return;
                            } else {
                                _matcherParams._cxl_ack_seq_count = 0;
                                _matcherParams._cxl_rej_seq_count = 0;
                                LOGGER_INFO << "resetting 'cxl' counters: " << _matcherParams.toString() << "\n";
                            }
                        }
                    } 
                }
                
                bool status = true;
                do {
                    // Lock matcher for thread synchronization
                    //
                    tw::common_thread::LockGuard<tw::common_thread::Lock> lock(matcher->getLock());

                    TOrderPtr order = matcher->getOrder(msg.get(Tags::OrderID));
                    if ( !order) {
                        status = false;
                        break;
                    }
                    
                    order->_clOrderId = msg.get(Tags::ClOrdID);
                    order->_origClOrderId = msg.get(Tags::ClOrdID);
                    sendAck(msg, sn);
                    
                    if ( 'G' == msgType ) {
                        order->_newPrice = matcher->getInstrument()->_tc->fromExchangePrice(msg.getDouble(Tags::Price));
                        matcher->sendMod(order);
                    } else {
                        matcher->sendCxl(order);
                    }
                } while (false);
                
                if ( !status )
                    sendRej(msg, sn, std::string("Order doesn't exist"));
                
            }
                break;
            default:
                return;
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }    
}

void MatcherManagerCME::onInboundSessionMsg(OnixS::FIX::Message& msg,
                                            OnixS::FIX::Session* sn) {
    //LOGGER_INFO << "\nIncoming session-level message:\n" << msg << "\n";
}

void MatcherManagerCME::onStateChange(OnixS::FIX::Session::State newState,
                                      OnixS::FIX::Session::State prevState,
                                      OnixS::FIX::Session* sn) {
    LOGGER_INFO << "\nSession's state is changed, prevState=" << OnixS::FIX::Session::state2string(prevState)
                << ", newState=" << OnixS::FIX::Session::state2string(newState) << "\n";
    
}

bool MatcherManagerCME::onResendRequest(OnixS::FIX::Message& message,
                                        OnixS::FIX::Session* session) {
    return true;
}

void MatcherManagerCME::onError(OnixS::FIX::ISessionListener::ErrorReason reason,
                                const std::string& description,
                                OnixS::FIX::Session* sn) {
    LOGGER_INFO << "\nSession-level error:" << description << "\n";    
}

void MatcherManagerCME::onWarning(OnixS::FIX::ISessionListener::WarningReason reason,
                                  const std::string& description,
                                  OnixS::FIX::Session* sn) {
    LOGGER_INFO << "\nSession-level warning:" << description << "\n";    
}


void MatcherManagerCME::onConnectionUp(TConnection::native_type id, TConnection::pointer connection) {
    try {
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<tw::common_thread::Lock> lock(_lock);
        
        _connectionsSubsIds[id] = connection;
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void MatcherManagerCME::onConnectionDown(TConnection::native_type id) {
    try {
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<tw::common_thread::Lock> lock(_lock);
        {
            TConnectionsSubsIds::iterator iter = _connectionsSubsIds.find(id);
            if ( iter != _connectionsSubsIds.end() )
                _connectionsSubsIds.erase(iter);
        }
        
        {
            TConnectionsSubs::iterator iterSubs = _connectionsSubs.begin();
            TConnectionsSubs::iterator endSubs = _connectionsSubs.end();

            for ( ; iterSubs != endSubs; ++iterSubs ) {
                TConnections::iterator iter = iterSubs->second.begin();
                while ( iter != iterSubs->second.end() ) {
                    if ( (*iter)->id() == id ) {
                        iterSubs->second.erase(iter);
                        iter = iterSubs->second.begin();
                    } else {
                        ++iter;
                    }
                }
            }
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void MatcherManagerCME::onConnectionData(TConnection::native_type id, const std::string& message) {
    try {
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<tw::common_thread::Lock> lock(_lock);
        
        TConnectionsSubsIds::iterator iter = _connectionsSubsIds.find(id);
        if ( iter == _connectionsSubsIds.end() ) {
            LOGGER_ERRO << "Can't find connection: "  << id << " :: " << message << "\n" << "\n";
            return;
        }
        
        tw::common::Command cmnd;
        if ( !cmnd.fromString(message) ) {
            LOGGER_ERRO << "Not a valid cmnd: " << message << "\n";
            return;
        }
        
        if ( tw::common::eCommandType::kExchangeSim != cmnd._type ) {
            LOGGER_ERRO << "Unsupported cmnd: " << cmnd.toString() << " :: " << message << "\n";
            return;
        }
        
        std::string displayName;
        if ( !cmnd.get("displayName", displayName) ) {
            LOGGER_ERRO << "No 'displayName' in cmnd: " << cmnd.toString() << " :: " << message << "\n";
            return;
        }
        
        if ( _matchers.find(displayName) == _matchers.end() ) {
            LOGGER_ERRO << "No matcher for 'displayName': " << displayName << "\n";
            return;
        }
        
        TConnectionsSubs::iterator iterSubs = _connectionsSubs.find(displayName);
        if ( iterSubs == _connectionsSubs.end() ) {
            if ( tw::common::eCommandSubType::kUnsub == cmnd._subType )
                return;
            
            iterSubs = _connectionsSubs.insert(TConnectionsSubs::value_type(displayName, TConnections())).first;                
        }
        
        TConnections& connections = iterSubs->second;
        TConnections::iterator iterCons = connections.begin();
        TConnections::iterator endCons = connections.end();
        
        if ( tw::common::eCommandSubType::kSub  == cmnd._subType ) {
            bool exists = false;
            
            for ( ; iterCons != endCons && !exists; ++iterCons ) {
                if ( (*iterCons).get() == iter->second.get() )
                    exists = true;
            }
            
            if ( !exists )
                connections.push_back(iter->second);
        } else if ( tw::common::eCommandSubType::kUnsub  == cmnd._subType ) {
            for ( ; iterCons != endCons; ++iterCons ) {
                if ( (*iterCons).get() == iter->second.get() ) {
                    connections.erase(iterCons);
                    break;
                }
            }
        } else {
            LOGGER_ERRO << "Unsupported cmnd subtype: " << cmnd.toString() << " :: " << message << "\n";
            return;
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void MatcherManagerCME::processSubs(const Matcher* item) {
    try {
        // Lock for thread synchronization
        //
        tw::common_thread::LockGuard<tw::common_thread::Lock> lock(_lock);
        
        if ( !item )
            return;
        
        TConnectionsSubs::iterator iter = _connectionsSubs.find(item->getInstrument()->_displayName);
        if ( iter == _connectionsSubs.end() || iter->second.empty() )
            return;
        
        std::string msg = item->toString();
        
        TConnections::iterator iterCons = iter->second.begin();
        TConnections::iterator endCons = iter->second.end();

        for ( ; iterCons != endCons; ++iterCons ) {
            (*iterCons)->send(msg);
        }
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
} 

void MatcherManagerCME::ThreadMain() {
    LOGGER_INFO << "Started" << "\n";
    
    try {
        while ( !_done ) {            
            const Matcher* item = NULL;
            _threadPipe.read(item);
            processSubs(item);
            
            while ( !_done && _threadPipe.try_read(item) ) {
                processSubs(item);
            }
        }        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    LOGGER_INFO << "Finished" << "\n";
}

} // namespace channel_or_cme
} // namespace tw
