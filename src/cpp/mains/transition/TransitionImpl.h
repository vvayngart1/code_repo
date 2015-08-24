#pragma once

#include <tw/common/command.h>
#include <tw/common/timer_server.h>
#include <tw/common_strat/istrategy.h>
#include <tw/common_trade/wbo_manager.h>
#include <tw/generated/commands_common.h>
#include <tw/generated/enums_common.h>
#include <tw/generated/channel_or_defs.h>
#include <tw/generated/transition.h>

// for random number generation:
#include <stdlib.h>
#include <time.h>

using namespace tw::transition;

#include <tw/common_trade/activePriceProcessor.h>
#include <tw/common_trade/stopTriggerProcessor.h>
#include <tw/common_trade/mkt_vels_manager.h>

// version 1.1 (Release Date = 09 July 2013)
namespace tw {
namespace transition {
    typedef tw::common_trade::stopTriggerProcessor TStopTriggerProcessor;
    typedef boost::shared_ptr<TStopTriggerProcessor> TStopTriggerProcessorPtr;
    
    struct FillInfoExtended : public FillInfo {
        void init(TransitionParamsWireExtended& p) {
            _stopTriggerProcessor.reset(new TStopTriggerProcessor(p));
        }
        
        TStopTriggerProcessorPtr _stopTriggerProcessor;
    };
    
    struct TransitionParams : public TransitionParamsWireExtended {
        void clear() {
            TransitionParamsWireExtended::clear();
            
            _entry_fills.clear();
            _alt_fills.clear();
        }
        
        // entry fills, by type:
        std::vector<FillInfoExtended> _entry_fills;
        std::vector<FillInfoExtended> _alt_fills;
    };    
    
    template <typename TRouter>
    class TransitionImpl : public tw::common_strat::IStrategy {
    public:
        typedef tw::common_trade::ActivePriceProcessor TActivePriceProcessor;        
        typedef boost::shared_ptr<TActivePriceProcessor> TActivePriceProcessorPtr;
        typedef tw::common_trade::MktVelsManager TMktVelsManager;
        
    public:
        TransitionImpl() {
            OPENING_TRANS = "OpeningTrans";
            FLATTENING_TRANS = "FlatteningTrans";
            CLOSING_TRANS = "ClosingTrans";
            EXIT_CLOSE = "ExitClose";
        }
        
        void init(const TransitionParams& p) {
            _p = p;
            _params._name = _p._name + "_" + _p._strategy_guid;
        }
        
        // ==> unit test utility methods
        tw::common::Settings& getSettings() {
            return _settings;
        }
        
        const tw::common::Settings& getSettings() const {
            return _settings;
        }
        
        TransitionParams& getParams() {
            return _p;
        }
        
        const TransitionParams& getParams() const {
            return _p;
        }
        
        TActivePriceProcessorPtr getActiveEnter() const {
            return _activePriceProcessorEnter;
        }
        
        TActivePriceProcessorPtr getActiveExit() const {
            return _activePriceProcessorExit;
        }
        
    public:
        // ==> IStrategy interface
        virtual bool init(const tw::common::Settings& settings, const tw::risk::Strategy& strategyParams) {
            if ( !addQuoteSubscription(_p._instrument) ) 
                return false;
            
            _start = tw::common::THighResTime::parseSqlTime(_p._startTime);
            _close = tw::common::THighResTime::parseSqlTime(_p._closeTime);
            _force = tw::common::THighResTime::parseSqlTime(_p._forceTime);
            
        // ==> risk parameters
            tw::risk::Strategy& sp = const_cast<tw::risk::Strategy&> (strategyParams);
            sp._maxRealizedDrawdown = _p._maxRealizedDrawdown;
            sp._maxRealizedLoss = _p._maxRealizedLoss;
            sp._maxUnrealizedDrawdown = _p._maxUnrealizedDrawdown;
            sp._maxUnrealizedLoss = _p._maxUnrealizedLoss;
            sp._tradeEnabled = true;
            
        // ==> velocity related
            if ( 0.0 < _p._velPriceDeltaEnter ) {
                if ( !TMktVelsManager::instance().subscribe(_p._instrument->_keyId) )
                    return false;
            }
            
        // ==> additional checks
            if (!IStrategy::init(settings, strategyParams))
                return false;
            _settings = settings;
            
        // ==> common trade blocks
            _activePriceProcessorEnter.reset(new TActivePriceProcessor(static_cast<ActiveEnter&>(_p)));
            _activePriceProcessorExit.reset(new TActivePriceProcessor(static_cast<ActiveExit&>(_p)));
            resetToFlat();
                    
            return true;
        }
        
        // ==> we may want to not issue orders for levels which are in the money
        virtual bool start() {
            // see breakpoint for initiation:
            if (_settings._trading_auto_pilot)
                _p._orderMode = eOrdersMode::kNormal;
            else
                _p._orderMode = eOrdersMode::kNoOrders;
            
            _message_printed = false;
            _total_exited = 0;
            _alt_counter = 0;
            _order_placed = false;            
            _stop_triggered = false;
            _stopLossExit_count = 0;
            _opening_fill_received = false;
            _prev_state = tw::transition::eState::kNeutral;
            _slide_counter = 0;
            _mod_counter = 0;
            return true;
        }
        
        virtual bool stop() {            
            stop("Stopped strategy: " + _p.toString());
            return true;
        }

        virtual void recordExternalFill(const tw::channel_or::Fill& fill) {
            // ==> not implemented - nothing to do
        }
        
        virtual bool rebuildOrder(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej) {
            // ==> not implemented - nothing to do
            return true;
        }
        
        virtual void rebuildPos(const tw::channel_or::PosUpdate& update) {
            // ==> not implemented - nothing to do
        }

        virtual void onConnectionUp(tw::common_strat::TMsgBusConnection::native_type id, tw::common_strat::TMsgBusConnection::pointer connection) {
            if (_settings._trading_auto_pilot)
                return;
            ++_p._connectionsCount;
        }
        
        virtual void onConnectionDown(tw::common_strat::TMsgBusConnection::native_type id) {
            if (_settings._trading_auto_pilot)
                return;
            if (--_p._connectionsCount <= 0)
                stop("==> stopped strategy on disconnect of all connections");
        }  
        
        virtual void onConnectionData(tw::common_strat::TMsgBusConnection::native_type id, const tw::common::Command& cmnd) {
            try {
                if (tw::common::eCommandType::kStrat != cmnd._type)
                    return;
                LOGGER_INFO << "==> cmnd @ = " << id << " :: " << cmnd.toString() << "\n";
                tw::channel_or::TOrderPtr order;
                std::string text;

                switch (cmnd._subType) {
                    case tw::common::eCommandSubType::kOrNew:
                    {
                        tw::channel_or::OrderWireOrderEntry orderWire = tw::channel_or::OrderWireOrderEntry::fromCommand(cmnd);
                        if (orderWire._orderId.isValid()) {
                            order = TRouter::createOrder(false);
                            order->_orderId = orderWire._orderId;
                        } else {
                            order = TRouter::createOrder();
                            orderWire._orderId = order->_orderId;
                        }
                        if (orderWire._strategyId != getStrategyId()) {
                            text = "StrategyIds don't match for: " + getStrategyInfo() + orderWire.toString();
                            sendAlert(text);
                            return;
                        }
                        reinterpret_cast<tw::channel_or::OrderWire&> (*order) = orderWire;
                        order->_instrument = TRouter::getInstrByDisplayName(orderWire._displayName);
                        order->_manual = true;                    
                        if ( NULL != order->_instrument )
                            order->_price = order->_instrument->_tc->fromExchangePrice(orderWire._exPrice);
                        sendNew(order, "manual_new");
                    }
                        break;
                    case tw::common::eCommandSubType::kOrMod:
                    {
                        tw::channel_or::OrderWireOrderEntry orderWire = tw::channel_or::OrderWireOrderEntry::fromCommand(cmnd);
                        if (orderWire._orderId.isValid())
                            order = TRouter::getOrder(orderWire._orderId);
                        if (!order) {
                            text = "Can't find order for id: " + orderWire._orderId.toString();
                            sendAlert(text);
                            return;
                        }
                        if (order->_strategyId != getStrategyId()) {
                            text = "StrategyIds don't match for: " + getStrategyInfo() + order->toString();
                            sendAlert(text);
                            return;
                        }
                        if (tw::channel_or::eOrderState::kWorking == order->_state) {
                            tw::price::Ticks newPrice = order->_instrument->_tc->fromExchangePrice(orderWire._exPrice);
                            if (newPrice.isValid())
                                order->_newPrice = newPrice;
                            else
                                order->_newPrice = order->_price;
                            order->_manual = true;
                            sendMod(order, "manual_mod");
                        } else {
                            text = "Can't modify order: " + reinterpret_cast<tw::channel_or::OrderWire&> (*order).toString();
                            sendAlert(text);
                        }
                    }
                        break;
                    case tw::common::eCommandSubType::kOrCxl:
                    {
                        tw::channel_or::OrderWire orderWire = tw::channel_or::OrderWire::fromCommand(cmnd);
                        order = TRouter::getOrder(orderWire._orderId);
                        if (!order) {
                            text = "Can't find order for id: " + orderWire._orderId.toString();
                            sendAlert(text);
                            return;
                        }
                        if (order->_strategyId != getStrategyId()) {
                            text = "StrategyIds don't match for: " + getStrategyInfo() + order->toString();
                            sendAlert(text);
                            return;
                        }
                        order->_manual = true;
                        sendCxl(order, "manual_cxl");
                    }
                        break;
                    case tw::common::eCommandSubType::kParams:
                    {
                        bool resetMode = true;
                        TransitionParamsWireExtended p = TransitionParamsWireExtended::fromCommand(cmnd);
                        if (_params._id == static_cast<uint32_t> (p._strategyId)) {                        
                            switch( p._action ) {
                                case eStratAction::kChange:
                                    if ( eStratMode::kExit == _p._mode ) {
                                        if ( hasPosition() ) {
                                            createAndSendAlert(tw::channel_or::eAlertType::kStratAlert, std::string("==> can't change from kExit because not flat: ") + cmnd.toString());
                                            return;
                                        }
                                        if ( hasOpenOrders() ) {
                                            createAndSendAlert(tw::channel_or::eAlertType::kStratAlert, std::string("==> can't change from kExit because has open order: ") + cmnd.toString());
                                            return;
                                        }
                                        _p._new_rej_counter = 0;
                                        _p._cxl_rej_counter = 0;
                                        resetToFlat();
                                    } else if ( (eStratMode::kLong == _p._mode || eStratMode::kShort == _p._mode) && eStratMode::kFlat == p._mode ) {
                                        eState prevState = _p._state;
                                        _p._state = eState::kFlattening;
                                        sendStateChangeAlert(prevState);
                                        resetMode = false;
                                    }
                                    
                                    _p._action = p._action;
                                    if ( resetMode )
                                        _p._mode = p._mode;
                                    _p._orderMode = p._orderMode;
                                    processStratStates();
                                    createAndSendAlert(tw::channel_or::eAlertType::kStratRegimeChange, std::string("==> changed: ") + cmnd.toString());
                                    break;
                                case eStratAction::kChangeConfig:
                                {
                                    TransitionParamsWire paramsBefore = static_cast<TransitionParamsWire&>(_p);                                    
                                    static_cast<TransitionParamsWire&>(_p) =  static_cast<TransitionParamsWire&>(p);
                                    processStratStates();
                                    createAndSendAlert(tw::channel_or::eAlertType::kStratAlert, std::string("==> config changed: ") + cmnd.toString());
                                    LOGGER_INFO << "StratAction::kChangeConfig for: " << getStrategyInfo() 
                                        << " before: \n" << paramsBefore.toStringVerbose()
                                        << " after: \n" << static_cast<TransitionParamsWire&>(_p).toStringVerbose();
                                }
                                    break;
                                case eStratAction::kStatus:
                                {
                                    _p._strategyId = _params._id;
                                    tw::common::Command c = _p.toCommand();
                                    c._connectionId = cmnd._connectionId;
                                    c._type = cmnd._type;
                                    c._subType = cmnd._subType;
                                    std::string m = c.toString();
                                    m += "\n";                                    
                                    TRouter::sendToMsgBusConnection(id, m);
                                    LOGGER_INFO << "Replied to eStratAction::kStatus command w/= " << m;
                                }
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                        break;
                    default:
                        break;
                }
            } catch (const std::exception& e) {
                LOGGER_ERRO << "onConnectionData() :: Exception: " << e.what() << "\n" << "\n";
            } catch (...) {
                LOGGER_ERRO << "onConnectionData() :: Exception: UNKNOWN" << "\n" << "\n";
            }
        }
        
    public:
        // ==> basic quote processing method without higher logic
        void onQuote(const tw::price::Quote& quote) {
            try {
                if ( !quote.isNormalTrade() && !quote.isLevelUpdate(0) )
                    return;
                if ( quote.isTrade() && !quote.isNormalTrade() )
                    return;
                if ( !quote._book[0].isValid() )
                    return;            
                
                switch (quote._status) {
                    case tw::price::Quote::kSuccess:
                    case tw::price::Quote::kExchangeSlow:
                        break;
                    case tw::price::Quote::kExchangeDown:
                    case tw::price::Quote::kDropRate:
                    case tw::price::Quote::kTradingHaltedOrStopped:
                    case tw::price::Quote::kTradingPause:
                    case tw::price::Quote::kConnectionHandlerError:
                    case tw::price::Quote::kPriceGap:
                        // ==> critical error quotes - going into 'NoOrders' mode
                        if ( eOrdersMode::kNormal == _p._orderMode )
                            stop(std::string("ErrorQuote w/status: ") + tw::price::Quote::statusToString(quote._status));
                        return;
                    default:
                        LOGGER_WARN << "\n\t" << "!= kSuccess && !=kExchangeSlow; status = " << tw::price::Quote::statusToString(quote._status) << " and " << quote._instrument->_displayName << " @ " << quote._timestamp1.toString() << "\n";
                        return;
                }
                
                // we got through, now set the wbo with this acceptable quote
                tw::common_trade::WboManager::TTv& wbo = tw::common_trade::WboManager::instance().getOrCreateTv(_p._instrument->_keyId);
                if (!wbo.isValid())
                    return;
                _p._wbo = wbo.getTv();
                
                _activePriceProcessorEnter->setQuote(_p._quote);
                _activePriceProcessorExit->setQuote(_p._quote);
                
                _p._quote = quote;
                processQuote(quote);
                _p._quote.clearFlag();
            } catch (const std::exception& e) {
                LOGGER_ERRO << "OnQuote() :: Exception: " << e.what() << "\n" << "\n";
            } catch (...) {
                LOGGER_ERRO << "OnQuote() :: Exception: UNKNOWN" << "\n" << "\n";
            }
        }
        
    public:
        // ==> order acknowledgment-related methods
        void onNewAck(const tw::channel_or::TOrderPtr& order) {
            _p._new_rej_counter = 0;
        }
        
        void onNewRej(const tw::channel_or::TOrderPtr& order, const tw::channel_or::Reject& rej) {
            // ==> circuit breakers to prevent network storm of new requests
            if ( tw::channel_or::eRejectSubType::kProcessorPnL == rej._rejSubType ) {
                stop(rej.toString() + "eRejectSubType::kProcessorPnL");
                return;
            }
            if ( tw::channel_or::eRejectType::kInternal == rej._rejType 
                && tw::channel_or::eRejectSubType::kConnection == rej._rejSubType 
                && tw::channel_or::eRejectReason::kConnectionDown == rej._rejReason ) {
                return;
            }
            if ( ++_p._new_rej_counter > 5 ) {
                createAndSendAlert(tw::channel_or::eAlertType::kMaxNewRejCounter, std::string("MaxNewRejCounter of 5 exceeded - going into no_orders_mode"));
                setNoOrdersMode("caused by new_rej_counter>5");
                return;
            }
        }
        
        void onModAck(const tw::channel_or::TOrderPtr& order) {
            // if a velocity mod, then should log it here
            // 
        }
        
        void onModRej(const tw::channel_or::TOrderPtr& order, const tw::channel_or::Reject& rej) {
            // ==> not implemented - nothing to do
        }        
        
        void onCxlAck(const tw::channel_or::TOrderPtr& order) {            
            _p._cxl_rej_counter = 0;
            
            // do we end alt period here
            // 
        }
    
        void onCxlRej(const tw::channel_or::TOrderPtr& order, const tw::channel_or::Reject& rej) {
            // ==> circuit breaker to prevent network storm of cxl requests
            if ( eOrdersMode::kNoOrders != _p._orderMode ) {
                if (++_p._cxl_rej_counter > 20) {
                    createAndSendAlert(tw::channel_or::eAlertType::kMaxCxlRejCounter, std::string("MaxCxlRejCounter of 20 exceeded - going into no_orders_mode"));
                    setNoOrdersMode("caused by onCxlRej>20");
                }
            } else {
                // ==> need to do something to prevent further activity... beyond NO b/c working in the regime
                if (++_p._cxl_rej_counter > 5) {
                    createAndSendAlert(tw::channel_or::eAlertType::kMaxCxlRejCounter, std::string("MaxCxlRejCounter of 5 after _no_orders_mode entered has been exceeded"));
                }
            }
        }
        
        void onFill(const tw::channel_or::Fill& fill) {
        // ==> for now, we are using a single _stop_price rather than per resting order
            LOGGER_INFO << getExtendedStrategyInfo() << fill.toString() << "\n";
            LOGGER_INFO << getExtendedStrategyInfo() 
                << "fillPrice=" << _p._instrument->_tc->toExchangePrice(fill._price)
                << ",fillSide=" << fill._side.toString()
                << ",fillInfo=" << fill.toString() << "\n";
            
            _opening_fill_received = false;
            bool send_exit_order = true;
            typename TRouter::TOrders open_orders = TRouter::getOpenOrders(_params._accountId, _params._id);
            
        // ==> fill for stopLossOrder (not usual case, here is a "fresh" stop loss order)
            if ( _p._stopLossOrder && _p._stopLossOrder->_orderId == fill._orderId ) {
                _stopLossExit_count = 0;
                LOGGER_INFO << getExtendedStrategyInfo() << " stopLossExit fill received" << getStateInfo() << "\n";
                
                // do we have to do anything here?
                // 
            }
            
        // ==> fill for alt order
            if ( _p._altOrder && _p._altOrder->_orderId == fill._orderId ) {
                _opening_fill_received = true;
                _slide_counter = 0;
                LOGGER_INFO << getExtendedStrategyInfo() << " :: alt fill received" << getStateInfo() << "\n";
                FillInfoExtended fillInfo;
                fillInfo.init(_p);
                fillInfo._fill = fill;
                
                if ( tw::channel_or::eOrderSide::kBuy == fill._side ) {
                    _p._mode = tw::transition::eStratMode::kLong;
                    _stop_price = fill._price - _p._stopTicks;
                } else {
                    _p._mode = tw::transition::eStratMode::kShort;
                    _stop_price = fill._price + _p._stopTicks;
                }
                
                _opening_price = fill._price;
                _opening_side = fill._side;
                LOGGER_INFO << getExtendedStrategyInfo() << " ==> alt stop price set " << getStopInfo(fillInfo) << "\n";
                _p._alt_fills.push_back(fillInfo);
                
                for (uint32_t i = 0; i < open_orders.size(); ++i) {
                    tw::channel_or::TOrderPtr& order = open_orders[i];
                    if ( TRouter::isOrderLive(order) && (order->_stratReason == "exit") )
                        send_exit_order = false;   
                }
                
                // N.B. this is only called after alt order in alt sequence
                if (send_exit_order)
                    processOrders();
                else {
                    LOGGER_INFO << getExtendedStrategyInfo() << " not sending exit" << "\n";
                }
            }
            
        // ==> fill for opening order
            if ( _p._order && _p._order->_orderId == fill._orderId ) {
                _opening_fill_received = true;
                _slide_counter = 0;
                LOGGER_INFO << getExtendedStrategyInfo() << " :: opening fill received" << getStateInfo() << "\n";
                FillInfoExtended fillInfo;
                fillInfo.init(_p);
                fillInfo._fill = fill;
                
                if ( tw::channel_or::eOrderSide::kBuy == fill._side ) {
                    _p._mode = tw::transition::eStratMode::kLong;
                    _stop_price = fill._price - _p._stopTicks;
                } else {
                    _p._mode = tw::transition::eStratMode::kShort;
                    _stop_price = fill._price + _p._stopTicks;
                }
                
                _opening_price = fill._price;
                _opening_side = fill._side;
                LOGGER_INFO << getExtendedStrategyInfo() << " ==> initial stop price set " << getStopInfo(fillInfo) << "\n";
                _p._entry_fills.push_back(fillInfo);
                
                // here as a safety we do not send exit order if an exit order 
                for (uint32_t i = 0; i < open_orders.size(); ++i) {
                    tw::channel_or::TOrderPtr& order = open_orders[i];
                    if ( TRouter::isOrderLive(order) && (order->_stratReason == "exit") )
                        send_exit_order = false;   
                }
                
                // N.B. this is only called after the very initial order submitted before the event
                if (send_exit_order)
                    processOrders();
                else {
                    LOGGER_INFO << getExtendedStrategyInfo() << " not sending exit" << "\n";
                }
            }
            
            if (!_opening_fill_received) {
                // ==> closing
                processExitFill(fill);
                verifyReset();
            } else {                
                _stop_triggered = false;
                LOGGER_INFO << getExtendedStrategyInfo() << " :: opening fill received and set (_opening_price,stop,_opening_side) = (" << _p._instrument->_tc->toExchangePrice(_opening_price) << "," << _p._instrument->_tc->toExchangePrice(_stop_price) << "," << _opening_side << ") :: " << "\n";
            }
        }
        
        void onAlert(const tw::channel_or::Alert& alert) {
            switch ( alert._type ) {
                case tw::channel_or::eAlertType::kExchangeUp:
                    _p._isExchangeUp = true;
                    break;
                case tw::channel_or::eAlertType::kExchangeDown:
                    _p._isExchangeUp = false;
                    break;
                default:
                    break;
            }
            LOGGER_WARN << "Received alert: " << alert.toString() << "\n";
        }
        
        void onRebuildOrderRej(const tw::channel_or::TOrderPtr& order, const tw::channel_or::Reject& rej) {
            // ==> not implemented - nothing to do
        }
        
    public:                
        // ==> exit- and stop- related methods        
        bool checkEquity(tw::channel_or::TOrderPtr& order, const tw::price::Quote& quote) {
            _excursion = 0.0;
            tw::channel_or::StratPnL* PnL = tw::channel_or::ProcessorPnL::instance().getStratsPnL(getStrategyId());
            
            // here rely on order to verify instrument id
            if ((NULL != order) && (order->_instrument->_keyId == quote._instrumentId) && PnL) {
                _excursion = PnL->_stratPnL.getUnrealizedPnL();
                _excursion /= ( order->_instrument->_tickValue );
            } else {
                return false;
            }
            
            return true;
        }
        
        bool checkEquity(const tw::price::Quote& quote) {
            _excursion = 0.0;
            tw::channel_or::StratPnL* PnL = tw::channel_or::ProcessorPnL::instance().getStratsPnL(getStrategyId());
            
            // here rely on _p._instrument to verify instrument id for trend order only
            if ( (_p._order->_instrument->_keyId == quote._instrumentId) && PnL) {
                _excursion = PnL->_stratPnL.getUnrealizedPnL();
                _excursion /= ( _p._order->_instrument->_tickValue );
            } else {
                return false;
            }
            
            return true;
        }
        
        // currently not utilized:
        void calcStopPrice(FillInfo& fillInfo) {
            const tw::channel_or::Fill& fill = fillInfo._fill;
            tw::price::Ticks& stop = fillInfo._stop;
            
            switch ( fill._side ) {
                case tw::channel_or::eOrderSide::kBuy:
                    stop = fill._price - _p._stopTicks;
                    fillInfo._exitOrderSide = tw::channel_or::eOrderSide::kSell;
                    break;
                case tw::channel_or::eOrderSide::kSell:
                    stop = fill._price + _p._stopTicks;
                    fillInfo._exitOrderSide = tw::channel_or::eOrderSide::kBuy;
                    break;
                default:
                    break;
            }
        }
        
    public:       
        std::string getStateInfo() const {
            tw::common_str_util::TFastStream s;
            s << " :: _p._mode= " << _p._mode.toString();
            
            return s.str();
        }
        
        std::string getBookInfo() const {
            tw::common_str_util::TFastStream s;
            s << " :: _p._wbo = " << _p._wbo
                << ", book = (" << _p._instrument->_tc->toExchangePrice(_p._quote._book[0]._bid._price)
                << ","
                << _p._instrument->_tc->toExchangePrice(_p._quote._book[0]._ask._price)
                << ")";
            return s.str();
        }

        std::string getStopInfo(FillInfoExtended& info) const {
            tw::common_str_util::TFastStream s;
            const tw::channel_or::Fill& f = info._fill;
            s << " :: _stop_price = " << _p._instrument->_tc->toExchangePrice(info._stop)
                << ",_opening_price = " << _p._instrument->_tc->toExchangePrice(f._price)
                << ",_opening_side = " << f._side.toString()
                << ",_p._quote._trade._price = " << _p._instrument->_tc->toExchangePrice(_p._quote._trade._price);
            return s.str();
        }
        
        std::string getStopInfo() const {
            tw::common_str_util::TFastStream s;
            s << " :: _stop_price = " << _p._instrument->_tc->toExchangePrice(_stop_price)
                << ",_opening_price = " << _p._instrument->_tc->toExchangePrice(_opening_price)
                << ",_opening_side = " << _opening_side.toString()
                << ",_p._quote._trade._price = " << _p._instrument->_tc->toExchangePrice(_p._quote._trade._price);
            return s.str();
        }
        
    private:
        std::string getStrategyInfo() const {
            tw::common_str_util::TFastStream s;
            s << getStrategyId() << "::" << getName() << " -- ";
            return s.str();
        }

        std::string getExtendedStrategyInfo() const {
            tw::common_str_util::TFastStream s;
            s << getStrategyInfo() << "trigger_guid=" << _p._trigger_guid << ": ";
            return s.str();
        }        
        
        void stop(const std::string& reason) {            
            if ( eStratMode::kExit == _p._mode )
                return;
            _p._mode = eStratMode::kExit;
            resetStateVariables();
            createAndSendAlert(tw::channel_or::eAlertType::kStratRegimeChange, getStrategyInfo() + EXIT_CLOSE + " on stop(): " + reason);
            processOrders(reason);
        }
        
    private:
        // ==> utility methods related to order routing
        void sendAlert(const std::string& text) {
            try {
                tw::channel_or::Alert alert;
                alert._type = tw::channel_or::eAlertType::kStratAlert;
                alert._strategyId = getStrategyId();
                alert._text = text;
                sendAlert(alert);
            } catch (const std::exception& e) {
                LOGGER_ERRO << "sendAlert(text) :: Exception: " << e.what() << "\n" << "\n";
            } catch (...) {
                LOGGER_ERRO << "sendAlert(text) :: Exception: UNKNOWN" << "\n" << "\n";
            }
        }
        
        void sendAlert(const tw::channel_or::Alert& alert) {
            try {
                tw::common::Command cmnd;
                cmnd = alert.toCommand();
                cmnd._type = tw::common::eCommandType::kChannelOr;
                cmnd._subType = tw::common::eCommandSubType::kAlert;
                TRouter::sendAlert(cmnd.toString());
                LOGGER_WARN << alert.toString() << "\n";
            } catch (const std::exception& e) {
                LOGGER_ERRO << "sendAlert(alert) :: Exception: " << e.what() << "\n" << "\n";
            } catch (...) {
                LOGGER_ERRO << "sendAlert(alert) :: Exception: UNKNOWN" << "\n" << "\n";
            }
        }
        
        bool sendNew(tw::channel_or::TOrderPtr& order, const std::string& reason="") {
            try {
                if ( !_p._isExchangeUp ) {
                    LOGGER_WARN << getExtendedStrategyInfo() << "!_p._isExchangeUp" << "\n";
                    return false;
                }
                if ( !order->_manual && eOrdersMode::kNoOrders == _p._orderMode ) {
                    LOGGER_WARN << getExtendedStrategyInfo() << "eOrdersMode::kNoOrders == _p._orderMode" << "\n";
                    return false;
                }
                tw::channel_or::Reject rej;
                order->_client.registerClient(this);
                order->_accountId = _params._accountId;
                order->_strategyId = _params._id;
                if (NULL == order->_instrument) {
                    sendAlert("Can't find instrument for order: " + order->toString());
                    return false;
                }
                order->_instrumentId = order->_instrument->_keyId;            
                if ( !reason.empty() )
                    order->_stratReason = reason;
                if ( !order->_manual ) {
                    if ( !checkMaxPos(order->_instrument, order->_side, order->_qty) )
                        return false;
                    if ( !TRouter::canSend(order, order->_price, rej) ) {
                        LOGGER_WARN << getExtendedStrategyInfo() << " :: can't send because 'WTP' would be triggered for order: " << order->toString() << " :: reject: " << rej.toString() << "\n";
                        return false;
                    }
                }            
                order->_trTimestamp.setToNow();
                if ( !TRouter::sendNew(order, rej) ) {
                    LOGGER_ERRO << getExtendedStrategyInfo() << " :: sendNew failure for order: " << order->toString() << " and reject: " << rej.toString() << "\n";
                    return false;
                }
                LOGGER_INFO << order->toString() << "\n";
                return true;
            } catch (const std::exception& e) {
                LOGGER_ERRO << getExtendedStrategyInfo() << " :: sendNew() Exception: " << e.what() << "\n" << "\n";
            } catch (...) {
                LOGGER_ERRO << getExtendedStrategyInfo() << " :: sendNew() Exception: UNKNOWN" << "\n" << "\n";
            }

            return false;
        }
        
        bool sendMod(tw::channel_or::TOrderPtr& order, const std::string& reason) {
            try {
                if ( !_p._isExchangeUp )
                    return false;
                if ( !order->_manual && eOrdersMode::kNoOrders == _p._orderMode )
                    return false;
                if ( !order->_newPrice.isValid() )
                    return false;
                if ( order->_price == order->_newPrice )
                    return false;
                tw::channel_or::Reject rej;
                if (!TRouter::canSend(order, order->_newPrice, rej)) {
                    LOGGER_WARN << "Can't send because 'WTP' would be triggered for order: " << order->toString() << " :: reject: " << rej.toString() << "\n";
                    order->_newPrice.clear();
                    return false;
                }
                order->_stratReasonMod = reason;
                order->_trTimestamp.setToNow();
                if ( !TRouter::sendMod(order, rej) ) {
                    LOGGER_ERRO << "sendMod failure for order: " << order->toString() << " and reject: " << rej.toString() << "\n";
                    return false;
                }
                LOGGER_INFO << order->toString() << "\n";
                return true;                
            } catch (const std::exception& e) {
                LOGGER_ERRO << "sendMod() :: Exception: " << e.what() << "\n" << "\n";
            } catch (...) {
                LOGGER_ERRO << "sendMod() :: Exception: UNKNOWN" << "\n" << "\n";
            }

            return false;
        }
        
        bool sendCxl(tw::channel_or::TOrderPtr& order, const std::string& reason) {
            try {
                if ( !TRouter::isOrderLive(order) )
                    return false;
                if ( !_p._isExchangeUp )
                    return false;
                if ( tw::channel_or::eOrderState::kCancelling == order->_state )
                    return true;
                if ( eOrdersMode::kNoOrders == _p._orderMode && _p._cxl_rej_counter > 5 )
                    return false;
                tw::channel_or::Reject rej;
                order->_stratReasonCxl = reason;
                order->_trTimestamp.setToNow();
                if ( !TRouter::sendCxl(order, rej) ) {
                    LOGGER_ERRO << "sendCxl failure for order: " << order->toString() << " and reject: " << rej.toString() << "\n";
                    return false;
                }

                LOGGER_INFO << order->toString() << "\n";
                return true;                
            } catch (const std::exception& e) {
                LOGGER_ERRO << "sendCxl() :: Exception: " << e.what() << "\n" << "\n";
            } catch (...) {
                LOGGER_ERRO << "sendCxl() :: Exception: UNKNOWN" << "\n" << "\n";
            }

            return false;
        }
               
        bool checkMaxPos(tw::instr::InstrumentConstPtr& instrument, const tw::channel_or::eOrderSide side, tw::price::Size qty) {
            if ( !TRouter::checkMaxPos(_params._accountId, _params._id, instrument, side, qty) ) {
                LOGGER_WARN << getExtendedStrategyInfo() << "Account SymbolMaxPos: " << qty.get() << " :: for: " << instrument->_displayName << "[" << side << "]" << "\n";
                return false;
            }
            return true;
        }
        
        void cancelOpenEntryOrders(const std::string& reason) {
            sendCxl(_p._order, reason);
            // N.B. do NOT cancel exit order here:
            // sendCxl(_p._exitOrder, reason);
            
            LOGGER_INFO << getExtendedStrategyInfo() << " :: open orders cancelled" << "\n";
        }
        
        void cancelOpenOrders(const std::string& reason) {
            // N.B. should cancel ALL here, including exit orders
            // 
            
            LOGGER_INFO << getExtendedStrategyInfo() << " :: calling cancelOpenOrders" << "\n";
            typename TRouter::TOrders open_orders = TRouter::getOpenOrders(_params._accountId, _params._id);
            for (uint32_t i = 0; i < open_orders.size(); ++i) {
                tw::channel_or::TOrderPtr& order = open_orders[i];
                if ( TRouter::isOrderLive(order) && !order->_manual )
                    sendCxl(order, reason);                
            }
            if ( !open_orders.empty() )
                createAndSendAlert(tw::channel_or::eAlertType::kStratRegimeChange, reason);
        }
        
    private:      
        // ==> order- and position- related methods
        bool formatCommonFields(tw::channel_or::TOrderPtr& order) {            
            order = TRouter::createOrder();
            if ( !order ) {
                LOGGER_ERRO << "order is NULL" << "\n";
                return false;
            }
            order->_client.registerClient(this);
            order->_accountId = _params._accountId;
            order->_strategyId = _params._id;
            order->_type = tw::channel_or::eOrderType::kLimit;
            return true;
        }
        
        bool createOrder(tw::channel_or::TOrderPtr& order, const tw::channel_or::eOrderSide side, const tw::price::Size& qty, const std::string& reason, const tw::price::Ticks& stop = tw::price::Ticks()) {
            if ( !formatCommonFields(order) )
                return false;
            order->_state = tw::channel_or::eOrderState::kUnknown;
            order->_instrument = _p._instrument;
            order->_instrumentId = _p._instrument->_keyId;
            order->_side = side;
            order->_qty = qty;
            order->_stratReason = reason;
            order->_manual = false;
            
            LOGGER_INFO << "createOrder called with side = " << side << ", qty = " << qty << ", reason = " << reason << " :: " << getExtendedStrategyInfo() << "\n";
            if ( OPENING_TRANS == reason || CLOSING_TRANS == reason ) {
                order->_memo = "trigger_guid=" + _p._trigger_guid;
                if ( stop.isValid() )
                    order->_memo += ",stop=" + stop.toString();
                order->_stratReason += ",curr_quote=" + _p._quote._book[0].toShortString();
            } else {
                LOGGER_INFO << "createOrder completed without a memo" << "\n";
            }
            
            return true;
        }
        
        bool createAndSendOrder(tw::channel_or::TOrderPtr& order, const tw::channel_or::eOrderSide side, const tw::price::Size& qty, const tw::price::Ticks& price, const std::string& reason) {
            if ( !createOrder(order, side, qty, reason) ) {
                LOGGER_INFO << getExtendedStrategyInfo() << "createAndSendOrder failure" << "\n";
                return false;
            }
            order->_price = price;
            return sendNew(order);
        }        
                
        bool hasOpenOrders() {
            return TRouter::hasOpenOrders(_params._accountId, _params._id);
        }        
        
        bool hasPosition() {
            if ( hasPosition(_p._instrument) )
                return true;
            return false;
        }                
        
        bool hasPosition(tw::instr::InstrumentPtr& instrument) {
            return TRouter::hasPosition(_params._id, instrument);
        }
        
        int32_t getPosition() {
            return getPosition(_p._instrument);
        }
        
        int32_t getPosition(tw::instr::InstrumentPtr& instrument) {
            return TRouter::getPosition(_params._id, instrument);
        }
        
        void setNoOrdersMode(const std::string& reason) {
            _p._orderMode = eOrdersMode::kNoOrders;
            _p._cxl_rej_counter = 0;
            processOrders();
        }
        
        void createAndSendAlert(const tw::channel_or::eAlertType type, const std::string& text) {
            tw::channel_or::Alert alert;
            alert._type = type;
            alert._strategyId = getStrategyId();
            alert._text = text;
            sendAlert(alert);
        }
        
    private:
        // ==> state-related methods
        void sendStateChangeAlert(eState prevState, const tw::price::Size& avgVol = tw::price::Size(), const std::string& reason = "") {
            tw::common_str_util::TFastStream s;
            s << getExtendedStrategyInfo() << prevState.toString() << " ==> " << _p._state.toString() << " :: " << _p._mode.toString();
            if ( tw::price::Size() != avgVol )
                s << ",avgVol=" << avgVol.get();
            if ( !reason.empty() )
                s << ",reason=" << reason;
            createAndSendAlert(tw::channel_or::eAlertType::kStratAlert, s.str());
        }                
           
        bool isStopLossTriggered(const FillInfoExtended& info, tw::price::Ticks& price, std::string& reason) {
            // uses info._stop instead of _stop_price (for vanilla) .. is this necessary?
            bool isTriggered = false;
            if ( info._stopTriggerProcessor.get() != NULL && info._stopTriggerProcessor->isEnabled() ) { 
                if ( !info._stopTriggerProcessor->isStopTriggered(info, _p._quote, reason) )
                    return false;
                isTriggered = true;
            }
            
            tw::common_str_util::TFastStream s;
            switch ( info._fill._side ) {
                case tw::channel_or::eOrderSide::kSell:
                    if ( !isTriggered ) {
                        if ( _p._quote.isNormalTrade() && _p._quote._trade.isValid() && info._stop <= _p._quote._trade._price ) {
                            s << "stopLossExit(stop <= trade._price)";
                            isTriggered = true;
                        }
                    } else {
                        s << reason;
                    }
                    
                    if (isTriggered )
                        price = info._stop + _p._stopLossPayupTicks;
                    break;
                case tw::channel_or::eOrderSide::kBuy:
                    if ( !isTriggered ) {
                        if ( _p._quote.isNormalTrade() && _p._quote._trade.isValid() && info._stop >= _p._quote._trade._price ) {
                            s << "stopLossExit(stop >= trade._price)";                        
                            isTriggered = true;
                        }
                    } else {
                        s << reason;
                    }
                    
                    if (isTriggered )
                        price = info._stop - _p._stopLossPayupTicks;
                    break;
                default:
                    break;
            }
            
            if ( isTriggered ) {
                s << " for: "
                  << "fill=" << info.toString()                               
                  << ", stopLossPayupTicks=" << _p._stopLossPayupTicks
                  << ", price=" << price
                  << " -- " << _p._quote._book[0].toShortString() << "::" << _p._quote._trade.toShortString();
                const_cast<FillInfoExtended&>(info)._stop = price;
                reason = s.str();
            }
            
            return isTriggered;
        }
        
        void checkPriceVelEnter(std::string& reason) {
            reason.clear();
            if ( _p._isVelPriceDeltaEnterTriggered )
                return;
            if ( !isCheckPriceEnterEnabled() ) {
                _p._isVelPriceDeltaEnterTriggered = true;
                return;
            }
            
            bool isOrderLive = false;
            bool orderatlevel = false;
            TMktVelsManager::TResult r = TMktVelsManager::instance().getVelDelta(_p._instrument->_keyId, _p._velPriceDeltaTimeMsEnter, 0);
            if ( r.first ) {
                isOrderLive = TRouter::isOrderLive(_p._order);
                if ( isOrderLive && (tw::channel_or::eOrderState::kWorking == _p._order->_state))
                    orderatlevel = true;
                // prospective long position
                if ( r.second < -_p._velPriceDeltaEnter && r.second < _vel ) {
                    _p._isVelPriceDeltaEnterTriggered = true;
                    _vel = r.second;
                }
                
                if (_p._isVelPriceDeltaEnterTriggered && orderatlevel) {
                    tw::common_str_util::TFastStream s;
                    s << "_p._isVelPriceDeltaEnterTriggered=" << (_p._isVelPriceDeltaEnterTriggered ? "true" : "false")
                      << ",wbo_price_delta=" << r.second;
                    reason = s.str();  
                }
            }            
        }
        
        bool isCheckPriceEnterEnabled() const {
            return (0 < _p._velPriceDeltaTimeMsEnter);
        }
              
        void handleStopLoss(FillInfoExtended& info) {
            // ==> vanilla stop loss evaluation
            if (!_stop_triggered) {
                switch (_p._mode) {
                    case eStratMode::kLong:
                        if ( info._stop >= _p._quote._trade._price ) {
                            _stop_triggered = true;
                        }
                        break;
                    case eStratMode::kShort:
                        if ( info._stop <= _p._quote._trade._price ) {
                            _stop_triggered = true;
                        }
                        break;
                    default:
                        // do nothing
                        break;
                }

                if ( _p._quote.isNormalTrade() && _p._quote._trade.isValid() && _stop_triggered ) {
                    LOGGER_INFO << getExtendedStrategyInfo() << " :: vanilla stopLossExit triggered" << getStopInfo(info) << getBookInfo() << getStateInfo() << "\n";
                    cancelOpenEntryOrders(EXIT_CLOSE+" stopLossExit");
                } 
            }
        }
        
        void modifyRestingOrder() {
            std::string vreason;
            checkPriceVelEnter(vreason);
            
            if ( !vreason.empty() ) {
                // verbose logging:
                LOGGER_INFO << getExtendedStrategyInfo() << getStateInfo() << vreason << "\n";
            }
            
            if ( _p._isVelPriceDeltaEnterTriggered ) 
            {
                bool isOrderLive = false;
                isOrderLive = TRouter::isOrderLive(_p._order);
                if ( isOrderLive && tw::channel_or::eOrderState::kWorking == _p._order->_state && (_mod_counter < 2) ) {
                    _p._order->_newPrice = _p._order->_price - 1;
                    if (sendMod(_p._order, "velocity")) {
                        _mod_counter++;
                        LOGGER_INFO << getExtendedStrategyInfo() << " ==> sendMod due to velocity with _mod_counter = " << _mod_counter << "\n";
                    } else {
                        LOGGER_INFO << getExtendedStrategyInfo() << " ==> sendMod fails for with _mod_counter = " << _mod_counter << "\n";
                    }
                }
            }

            _p._isVelPriceDeltaEnterTriggered = false;
        }
        
        bool doStates() {
            if ( eState::kNeutral == _p._state )
                return false;
            
            // _p._state values:
            // Neutral
            // BuyOpen
            // SellOpen
            // SellExit
            // BuyExit
            // Alternate
            // Flattening
            
            switch (_p._state) {
                case tw::transition::eState::kNeutral:
                    // use random number generator to decide:
                    // 
                    // 
                    _prevState = _p._state;
                    _p._state = tw::transition::eState::kBuyOpen;
                    // _p._state = tw::transition::eState::kSellOpen;
                    _p._trigger_guid = tw::channel_or::UuidFactory::instance().get().toString();                
                    
                    // because we are coming from Neutral
                    sendStateChangeAlert(_prevState);
                    break;
                case tw::transition::eState::kBuyOpen:
                    // do nothing, this is decided onFill
                    break;
                case tw::transition::eState::kSellOpen:
                    // do nothing, this is decided onFill
                    break;
                case tw::transition::eState::kSellExit:
                    // do alternation here
                    _p._state = tw::transition::eState::kSellOpen;
                    
                    break;
                case tw::transition::eState::kBuyExit:
                    // do alternation here
                    _p._state = tw::transition::eState::kBuyOpen;
                    
                    break;
                case tw::transition::eState::kAlternate:
                    break;
                case tw::transition::eState::kFlattening:
                    break;
                default:
                    // do nothing
                    break;
            }
            
            return true;
        }
        
        // contains random number generation for _alt_counter = 0
        bool verifyEntryOrder(const tw::price::Quote& quote) {
            bool result = false;
            if ( tw::transition::eState::kNeutral == _p._state) {
                return result;
            }
            
            std::string reason;
            bool isOrderLive = TRouter::isOrderLive(_p._order);
            if ( isOrderLive && tw::channel_or::eOrderState::kWorking != _p._order->_state ) {
                LOGGER_INFO << getExtendedStrategyInfo() << " :: not sending order due to isOrderLive && tw::channel_or::eOrderState::kWorking != _p._order->_state = " << _p._order->_state.toString() << "\n";
                return result; 
            }
            if ( isOrderLive )
                return result;
            
            tw::channel_or::eOrderSide side = tw::channel_or::eOrderSide::kUnknown;
            // use random number generation to determine side
            if (_alt_counter == 0) {
                // size = unit size
                
                srand (time(NULL));
                // here perform multiple random number generations:
                int sumrand = 0;
                for ( uint32_t i = 0; i < 10; ++i ) {
                    LOGGER_INFO << "dice_roll: " << i << "--" << (rand()%6+1) << "\n";
                    sumrand += i;
                }
                
                if (sumrand < 30) {
                    side = tw::channel_or::eOrderSide::kBuy;
                    LOGGER_INFO << getExtendedStrategyInfo() << "==> sumrand = " << sumrand << " doing BUY OPEN" << "\n";
                } else {
                    side = tw::channel_or::eOrderSide::kSell;
                    LOGGER_INFO << getExtendedStrategyInfo() << "==> sumrand = " << sumrand << " doing SELL OPEN" << "\n";
                }
                
            } else {
                // do nothing, this is handled by info._exitOrder
                // size = double unit size
            }

            
            tw::common_trade::ActivePriceProcessor::TInfo activeEnterInfo;
            if (!_message_printed) {
                _message_printed = true;
                LOGGER_INFO << getExtendedStrategyInfo() << " :: no order because already placed" << "\n";
            }

            // ==> check if should proceed to order generation:
            if (!result)
                return result;
            
            switch (side) {
                case tw::channel_or::eOrderSide::kBuy:
                    activeEnterInfo = _activePriceProcessorEnter->calcPrice(quote, true, reason);
                    LOGGER_INFO << getExtendedStrategyInfo()
                        << "active_price=" << activeEnterInfo.first.toString()
                        << ",side=" << side.toString()
                        << " -- " << reason
                        << "\n"; 
                    break;
                default:
                    activeEnterInfo = _activePriceProcessorEnter->calcPrice(quote, false, reason);
                    LOGGER_INFO << getExtendedStrategyInfo()
                        << "active_price=" << activeEnterInfo.first.toString()
                        << ",side=" << side.toString()
                        << " -- " << reason
                        << "\n";
                    break;
            }

            tw::price::Ticks priceInTicks = activeEnterInfo.first;
            LOGGER_INFO << "about to send order :: " << getExtendedStrategyInfo() << "\n";
            if ( createAndSendOrder(_p._order, side, _p._betSize, priceInTicks, OPENING_TRANS) ) {
                result = true;
                // verbose logging:
                LOGGER_INFO << getExtendedStrategyInfo() << " :: verifyEntryOrder() places order w/ side =  " << side << ", _p._betSize = " << _p._betSize << ", price = " << _p._instrument->_tc->toExchangePrice(priceInTicks) << "\n";
                _order_placed = true;
            } else {
                LOGGER_INFO << getExtendedStrategyInfo() << " :: verifyEntryOrder() order fails w/ side =  " << side << ", _p._betSize = " << _p._betSize << ", price = " << _p._instrument->_tc->toExchangePrice(priceInTicks) << "\n";
            }
            
            return result;
        }
        
        void processQuote(const tw::price::Quote& quote) {
            if ( quote._instrumentId != _p._instrument->_keyId )
                return;
            
            // _p._state values:
            // Neutral
            // Alternate
            // BuyOpen
            // SellOpen
            // SellExit
            // BuyExit
            // Flattening
                        
            // N.B. if Flatten command issued, the _p._mode = eStratMode::kExit:
            if ( eState::kFlattening == _p._state || eStratMode::kExit == _p._mode || eOrdersMode::kNoOrders == _p._orderMode )
                return;
            
            // critical part: decide on time to submit orders for commencing alternation
            if (afterStartTime(quote._timestamp2)) {
                if ( tw::transition::eState::kNeutral == _p._state ) {
                    _p._state = tw::transition::eState::kAlternate;
                }
                
                if (beforeCloseTime(quote._timestamp2)) {
                    // no more alternation: stop or distro out of working order
                    LOGGER_INFO << getExtendedStrategyInfo() << " ==> starting alternation " << "\n";
                    
                }
                
                if (afterForceTime(quote._timestamp2)) {
                    // now force out of trade
                    LOGGER_INFO << getExtendedStrategyInfo() << quote._timestamp2.toString() << " ==> force time " << "\n";
                }
            }
            
            try
            {
                if ( !_order_placed && (eOrdersMode::kNormal == _p._orderMode) ) {
                    // start alternation
                    //
                    
                    
                    
                } else if ( _order_placed && !_opening_fill_received && (eOrdersMode::kNormal == _p._orderMode) ) {
                    modifyRestingOrder();
                }
            } catch (const std::exception& e) {
                LOGGER_ERRO << "PART B Exception: " << e.what() << "\n" << "\n";
            } catch (...) {
                LOGGER_ERRO << "PART B Exception: UNKNOWN" << "\n" << "\n";
            }
            
            try
            {
                // handle state machine here
                // 
                
            } catch (const std::exception& e) {
                LOGGER_ERRO << "PART C Exception: " << e.what() << "\n" << "\n";
            } catch (...) {
                LOGGER_ERRO << "PART C Exception: UNKNOWN" << "\n" << "\n";
            } 
        
            // N.B. below we actually send the trend retouch order (if conditions satisfied)
            try 
            {
                // here attempts to begin the alternation sequence:
                // 
                verifyEntryOrder(quote);
            }  catch (const std::exception& e) {
                LOGGER_ERRO << "PART D Exception: " << e.what() << "\n" << "\n";
            } catch (...) {
                LOGGER_ERRO << "PART D Exception: UNKNOWN" << "\n" << "\n";
            }   
        }
        
        void resetStateVariables() {
        // might do this when signal goes away
            _prev_state = _p._state;
           _p._state = eState::kNeutral;
           
           // _p._state values:
           // Neutral
           // BuyOpen
           // SellOpen
           // SellExit
           // BuyExit
           // Alternate
           // Flattening
                  
           _p._trigger_guid.clear();
           // velocity related
           _p._pulsePriceCounter = 0;
           _p._pulseVolTimeCounter = 0;
           _p._isVelPriceDeltaEnterTriggered = false;
        }
        
        void resetToFlat() {
            _p._order = tw::channel_or::TOrderPtr();
            
            // under certain circumstances, if we use a fresh stop loss:
            // _p._stopLossOrder = tw::channel_or::TOrderPtr();
            
            if ( eStratMode::kExit != _p._mode )
                _p._mode = eStratMode::kFlat;
            resetStateVariables();
        }
        
        void processStopLossExit(FillInfoExtended& info) {
            tw::price::Ticks newPrice;
            switch (info._exitOrderSide) {
                case tw::channel_or::eOrderSide::kBuy:
                    newPrice = _p._quote._book[0]._bid._price + _p._stopLossPayupTicks;
                    break;
                case tw::channel_or::eOrderSide::kSell:
                    newPrice = _p._quote._book[0]._ask._price - _p._stopLossPayupTicks;
                    break;
                default:
                    LOGGER_INFO << getExtendedStrategyInfo() << " :: processStopLossExit() exit order side undefined" << "\n";
                    break;
            }

            if (newPrice.isValid())
                info._exitOrder->_newPrice = newPrice;
            else
                info._exitOrder->_newPrice = info._exitOrder->_price;
            if (!sendMod(info._exitOrder, "stopLossExit")) {
                std::string text = "Can't modify exit order";
                sendAlert(text);
            } else {
                LOGGER_INFO << getExtendedStrategyInfo() << " :: processStopLossExit() invoked" << "\n";
            }         
        }
                
        void processSlides(FillInfoExtended& info) {
            const tw::channel_or::Fill& f = info._fill;
            tw::price::Ticks sub = tw::price::Ticks(0);
            switch (info._exitOrderSide) {
                case tw::channel_or::eOrderSide::kBuy:
                    // sold open ... have to buy ask to exit and lower the stopping level
                    while ( (f._price - (sub + tw::price::Ticks(1)) * _p._slideStopOnProfitTicks) >= _p._quote._book[0]._ask._price ) {
                        // verbose logging:
                        // LOGGER_INFO << getExtendedStrategyInfo() << " :: slide down approved, with (fill, ask , counter) = (" << _p._instrument->_tc->toExchangePrice(f._price) << "," << _p._instrument->_tc->toExchangePrice(_p._quote._book[0]._ask._price) << "," << sub << ")" << "\n";
                        ++sub;
                    }
                    
                    // the satisfying counter is one below the last incremented value:
                    if (sub > tw::price::Ticks(0))
                        --sub;
                    if ( (f._price - (sub + tw::price::Ticks(1)) * _p._slideStopOnProfitTicks) >= _p._quote._book[0]._ask._price ) {
                        if ( info._stop > (f._price - (sub * _p._slideStopOnProfitTicks) - _p._slideStopPayupTicks) ) {
                            
                            // verbose before logging:
                            // LOGGER_INFO << getExtendedStrategyInfo() << "BEFORE STOP SLIDE DOWN: info._stop = " << info._stop << " > (" << f._price << " - (" << sub << " * " << _p._slideStopOnProfitTicks << ") - " << _p._slideStopPayupTicks << ")" << "\n";
                                    
                            info._stop = f._price - (sub * _p._slideStopOnProfitTicks) - _p._slideStopPayupTicks;
                            
                            // verbose after logging:
                            // LOGGER_INFO << getExtendedStrategyInfo() << "AFTER STOP SLIDE DOWN: info._stop = " << info._stop << " > (" << f._price << " - (" << sub << " * " << _p._slideStopOnProfitTicks << ") - " << _p._slideStopPayupTicks << ")" << "\n";
                            
                            LOGGER_INFO << getExtendedStrategyInfo() << " ==> slide down" << getStopInfo(info) << getStateInfo() << " for [" << sub << "]" << "\n";
                        } else {
                            // verbose logging:
                            // LOGGER_INFO << getExtendedStrategyInfo() << " :: no new slide down" << getStopInfo(info) << getStateInfo() << " for [" << sub << "]" << "\n";
                        }
                    }
                    break;
                case tw::channel_or::eOrderSide::kSell:
                    // bought open ... have to sell bid to exit and raise the stopping level
                    while ( (f._price + (sub + tw::price::Ticks(1)) * _p._slideStopOnProfitTicks) <= _p._quote._book[0]._bid._price ) {
                        // verbose logging:
                        // LOGGER_INFO << getExtendedStrategyInfo() << " :: slide up approved, with (fill, bid , counter) = (" << _p._instrument->_tc->toExchangePrice(f._price) << "," << _p._instrument->_tc->toExchangePrice(_p._quote._book[0]._bid._price) << "," << sub << ")" << "\n";
                        ++sub;
                    }
                    
                    // the satisfying counter is one below the last incremented value:
                    if (sub > tw::price::Ticks(0))
                        --sub;
                    if ( (f._price + (sub + tw::price::Ticks(1)) * _p._slideStopOnProfitTicks) <= _p._quote._book[0]._bid._price ) {
                        if ( info._stop < (f._price + (sub * _p._slideStopOnProfitTicks) + _p._slideStopPayupTicks) ) {
                            
                            // verbose before logging:
                            // LOGGER_INFO << getExtendedStrategyInfo() << "BEFORE STOP SLIDE UP: info._stop = " << info._stop << " < (" << f._price << " + (" << sub << " * " << _p._slideStopOnProfitTicks << ") + " << _p._slideStopPayupTicks << ")" << "\n";
                            
                            info._stop = f._price + (sub * _p._slideStopOnProfitTicks) + _p._slideStopPayupTicks;
                            
                            // verbose after logging:
                            // LOGGER_INFO << getExtendedStrategyInfo() << "AFTER STOP SLIDE UP: info._stop = " << info._stop << " < (" << f._price << " + (" << sub << " * " << _p._slideStopOnProfitTicks << ") + " << _p._slideStopPayupTicks << ")" << "\n";
                            
                            LOGGER_INFO << getExtendedStrategyInfo() << " ==> slide up" << getStopInfo(info) << getStateInfo() << " for [" << sub << "]" << "\n";
                        } else {
                            // verbose logging:
                            // LOGGER_INFO << getExtendedStrategyInfo() << " :: no new slide up" << getStopInfo(info) << getStateInfo() << " for [ " << sub << "]" << "\n";
                        }
                    }
                    break;
                default:
                    LOGGER_WARN << getExtendedStrategyInfo() << " :: processSlides inputs unknown _exitOrderSide" << "\n";
                    break;
            }
            
            // other stop loss evaluation
            tw::price::Ticks price;
            std::string reason2;
            if ( isStopLossTriggered(info, price, reason2) ) {
                if ( info._exitOrder->_price != price ) {
                    info._exitOrder->_newPrice = price;
                    info._exitOrder->_stopLoss = true;
                    sendMod(info._exitOrder, reason2);
                }
            }
        }
        
        // ==> this order is designed to reap profit (could be the last order in sequence)
        // ==> generate exit order from filling on entry (see OnFill)
        void processExitOrders() {
            if (_stop_triggered) {
                // cannot produce new order after stop is already working
                LOGGER_INFO << getExtendedStrategyInfo() << " cannot produce exit order because _stop_triggered = " << _stop_triggered << "\n";
                return;
            }
            
            // N.B. with the verifyReset logic, ensure correct operation when Flattening
            // when the exit order is in the process of filling, and other edge situations
            tw::channel_or::eOrderSide side;
            switch (_p._mode) {
                case eStratMode::kLong:
                    side = tw::channel_or::eOrderSide::kSell;
                    break;
                default:
                    side = tw::channel_or::eOrderSide::kBuy;
                    break;
            }
            
            // N.B. cycle through entry fills, utilize the attached exit orders
            for ( size_t i = 0; i < _p._entry_fills.size(); ++i ) {
                FillInfoExtended& info = _p._entry_fills[i];
                const tw::channel_or::Fill& f = info._fill;
                tw::channel_or::TOrderPtr& order = info._exitOrder;
                if ( !TRouter::isOrderLive(info._exitOrder) ) {
                    if ( createOrder(order, info._exitOrderSide, f._qty, CLOSING_TRANS, info._stop) ) {
                        order->_price = (tw::channel_or::eOrderSide::kBuy == info._exitOrderSide) ? f._price - _p._profitTicks : f._price + _p._profitTicks;
                        if (sendNew(order,"exit")) {
                            LOGGER_INFO << getExtendedStrategyInfo() << "==> exit order OK for entry fill of qty = " << f._qty << "\n";
                        } else {
                            LOGGER_WARN << getExtendedStrategyInfo() << "==> exit order fails for entry fill of qty = " << f._qty << "\n";
                        }
                    }
                    continue;
                }

                if ( tw::channel_or::eOrderState::kWorking != order->_state )
                    continue;
                
                // deal with stop and slide per entry fill:
                handleStopLoss(info);
                if (!_stop_triggered)
                    processSlides(info);
                else
                    processStopLossExit(info);
            }
        }        
        
        // for use with Flatten and Stop Loss
        void processStratExitOrders(const std::string& reason, int scenario) {
            tw::price::Size position(getPosition());
            
            // here, _p._exitOrder is assuming role of flattener
            if ( 0 == position.get() )
                return;
            
            switch (scenario) {
                case 0:
                    if (_stopLossExit_count > 0) {
                        LOGGER_INFO << " :: return from processStratExitOrders because cannot send more than one stop loss order" << "\n";
                        return;
                    }
                    
                    // here we send a new stopLossExit order:
                    _p._stopLossOrder = tw::channel_or::TOrderPtr();
                    LOGGER_INFO << getExtendedStrategyInfo() << " :: sending fresh stop loss order" << getStopInfo() << getBookInfo() << "\n";

                    if ( position.get() > 0 ) {
                        // here we are zeroing out a PLUS position, so take qty = position
                        createAndSendOrder(_p._stopLossOrder, tw::channel_or::eOrderSide::kSell, position, _p._quote._book[0]._ask._price - _p._stopLossPayupTicks, reason);
                    } else {
                        // here we are zeroing out a MINUS position, so take qty = -position
                        createAndSendOrder(_p._stopLossOrder, tw::channel_or::eOrderSide::kBuy, -position, _p._quote._book[0]._bid._price + _p._stopLossPayupTicks, reason);
                    }

                    if (!sendNew(_p._stopLossOrder))
                        LOGGER_INFO << getExtendedStrategyInfo() << " :: processStratExitOrders() :: sendNew() failure with _stopLossExit_count = " << _stopLossExit_count << " for stop order @ " << _p._instrument->_tc->toExchangePrice(_p._stopLossOrder->_price) << "\n";
                    else {
                        _stopLossExit_count++;
                        LOGGER_INFO << getExtendedStrategyInfo() << " :: processStratExitOrders() :: sendNew() success with _stopLossExit_count = " << _stopLossExit_count << " for stop order @ " << _p._instrument->_tc->toExchangePrice(_p._stopLossOrder->_price) << "\n";
                    }
                    break;
                default:
                    // do nothing, we do this in processExitOrders now
                    break;
            }
        }
        
        void processOrders(const std::string& reason="") {
            if ( eOrdersMode::kNoOrders == _p._orderMode ) {
                cancelOpenOrders(std::string("==> kNoOrders ") + reason);
                return;
            }
            
            switch (_p._mode) {
                case eStratMode::kFlat:
                case eStratMode::kLong:
                case eStratMode::kShort:
                    processExitOrders();                    
                    break;
                case eStratMode::kExit:
                    cancelOpenEntryOrders(EXIT_CLOSE+reason);
                    processStratExitOrders(EXIT_CLOSE+reason, 0);
                    break;
                default:
                    break;
            }
        }
        
        // involved in Flatten
        void processStratStates() {
            switch ( _p._mode ) {
                case eStratMode::kLong:
                case eStratMode::kShort:
                case eStratMode::kExit:
                    {
                        bool canGoToFlat = true;
                        if ( TRouter::isOrderLive(_p._order) )
                            canGoToFlat = false;
                        else if ( hasPosition(_p._instrument) )
                            canGoToFlat = false;
                        
                        if ( canGoToFlat ) {
                            resetToFlat();
                            createAndSendAlert(tw::channel_or::eAlertType::kStratRegimeChange, std::string("==> changed to kFlat mode"));
                        }
                    }
                    break;
                default:
                    break;
            }
            
            processOrders();
        }
        
        void verifyReset() {
            // checking zero position is NOT the true measure to avoid machine-gun orders
            bool reset_allowed = false;
            bool effective_reset_allowed = false;
            int effective_size = 0;
            switch (_p._actual) {
                case tw::transition::eActualState::kBUYOPEN:
                    if ( _total_exited == _p._betSize )
                        reset_allowed = true;
                    else if ( _resting_cancelled && (_total_exited == _effective_size) ) {
                        reset_allowed = true;
                        effective_reset_allowed = true;
                        effective_size = _effective_size;
                    }
                    break;
                case tw::transition::eActualState::kSELLOPEN:
                    if ( _total_exited == _p._betSize)
                        reset_allowed = true;
                    else if ( _resting_cancelled && (_total_exited == _effective_size) ) {
                        reset_allowed = true;
                        effective_reset_allowed = true;
                        effective_size = _effective_size;
                    }
                    break;
                case tw::transition::eActualState::kSELLEXIT:
                    if ( _total_exited == _p._betSize )
                        reset_allowed = true;
                    else if ( _resting_cancelled && (_total_exited == _effective_size) ) {
                        reset_allowed = true;
                        effective_reset_allowed = true;
                        effective_size = _effective_size;
                    }
                    break;
                case tw::transition::eActualState::kBUYEXIT:
                    if ( _total_exited == _p._betSize )
                        reset_allowed = true;
                    else if ( _resting_cancelled && (_total_exited == _effective_size) ) {
                        reset_allowed = true;
                        effective_reset_allowed = true;
                        effective_size = _effective_size;
                    }
                    break;
                default:
                    // do nothing
                    break;
            }
            
            if (reset_allowed) {
                // reset _p._state in what manner here:
                LOGGER_INFO << getExtendedStrategyInfo() << " ==> reset to Neutral from _p._state = " << _p._state << "\n";
                
                if (effective_reset_allowed) {
                    LOGGER_INFO << ", comparing to effective size = " << effective_size << "\n";
                }
                
                // now send next alt order if we did not slide the previous order
                //
                
                // alternatively, send next alt order if previous order was a loser
                // 
                
                _p._mode = tw::transition::eStratMode::kFlat;
                _p._state = tw::transition::eState::kNeutral;
                _p._actual = tw::transition::eActualState::kNEUTRAL;
            }
        }
        
        // ==> N.B. here handle order submission of successive alt
        void processExitFill(const tw::channel_or::Fill& fill) {
            // N.B. do not change state based on zero position (could still have working entry orders)
            LOGGER_INFO << getExtendedStrategyInfo() << " :: exit fill recorded"<< "\n";
            
            // true opening fills
            std::vector<FillInfoExtended>::iterator iter = _p._entry_fills.begin();
            std::vector<FillInfoExtended>::iterator end = _p._entry_fills.end();
            for ( ; iter != end; ++iter ) {
                tw::channel_or::TOrderPtr order = (*iter)._exitOrder;
                if ( order && order->_orderId == fill._orderId ) {
                    _total_exited += fill._qty;
                    LOGGER_INFO << getExtendedStrategyInfo() << " :: exit fill, _total_exited = " << _total_exited << "\n";
                    if ( tw::channel_or::eOrderState::kFilled == order->_state )
                        _p._entry_fills.erase(iter);
                    return;
                }
            }

            // alt fills: e.g. sequence is [+1]->[-2]->[+2]->[-2]...
            iter = _p._alt_fills.begin();
            end = _p._alt_fills.end();
            for ( ; iter != end; ++iter ) {
                tw::channel_or::TOrderPtr order = (*iter)._exitOrder;
                if ( order && order->_orderId == fill._orderId ) {
                    _total_exited += fill._qty;
                    LOGGER_INFO << getExtendedStrategyInfo() << " :: alt fill, _total_exited = " << _total_exited << "\n";
                    if ( tw::channel_or::eOrderState::kFilled == order->_state )
                        _p._alt_fills.erase(iter);
                    return;
                }
            }
            
            // ==> should check verifyReset before sending new "entry" order in alt sequence
            // ==> now consider sending another _p._order (this might be cleanest way)
            if (_alt_counter < _p._altMax) {
                verifyEntryOrder(_p._quote);
                // consider a verifyAltOrder here
                _alt_counter++;
            } else {
                LOGGER_INFO << getExtendedStrategyInfo() << " ==> no more alt b/c _alt_counter = " << _alt_counter << " reaches _altMax = " << _p._altMax << "\n";
            }
        }
               
        bool afterStartTime(const tw::common::THighResTime& now = tw::common::THighResTime::now()) const {
            return ((now - _start) > 0);
        }
        
        bool beforeCloseTime(const tw::common::THighResTime& now = tw::common::THighResTime::now()) const {
            return ((_close - now) > 0);
        }
        
        bool afterForceTime(const tw::common::THighResTime& now = tw::common::THighResTime::now()) const {
            return ((now - _force) > 0);
        }
        
    private:
        double _excursion;
        bool _order_placed;
        bool _opening_fill_received;
        bool _stop_triggered;
        int _stopLossExit_count;
        tw::transition::eState _prev_state;
        bool _exit_placed;
        bool _message_printed;
        
        tw::common::THighResTime _start;
        tw::common::THighResTime _close;
        tw::common::THighResTime _force;
            
        // for tracking vanilla stop sliding:
        int _slide_counter;
        int _mod_counter;
        double _vel;
        int _total_exited;
        bool _resting_cancelled;
        int _effective_size;
        int _alt_counter;
        eState _prevState;
                
        tw::channel_or::eOrderSide _opening_side;
        tw::price::Ticks _opening_price;
        tw::price::Ticks _stop_price;
        
        std::string OPENING_TRANS;
        std::string FLATTENING_TRANS;
        std::string CLOSING_TRANS;
        std::string EXIT_CLOSE;
        
        tw::common::Settings _settings;
        TransitionParams _p;
        
        // ==> common trading blocks
        TActivePriceProcessorPtr _activePriceProcessorEnter;
        TActivePriceProcessorPtr _activePriceProcessorExit;
    };
}
}
