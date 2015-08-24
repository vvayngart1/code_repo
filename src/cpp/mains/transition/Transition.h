#pragma once

#include <tw/common_strat/strategy_container.h>
#include <tw/common_trade/bars_manager.h>
#include "TransitionImpl.h"

namespace tw {
namespace transition {
    class Transition {
    public:
        typedef tw::channel_or::TOrders TOrders;
        typedef TransitionImpl<Transition> TImpl;
        typedef tw::common_trade::BarsManager TBarsManager;
        typedef tw::common_trade::TBar TBar;
        typedef tw::common_trade::TBars TBars;
        typedef tw::common_trade::BarsManagerFactory TBarsManagerFactory;
        
    public:
        TImpl& getImpl() {
            return _impl;
        }
        
        const TImpl& getImpl() const {
            return _impl;
        }
        
    public:
        static tw::instr:: InstrumentPtr getInstrByDisplayName(const std::string& x) {
            return tw::instr::InstrumentManager::instance().getByDisplayName(x);
        }
        
        static tw::channel_or::TOrderPtr createOrder(bool createOrderId = true) {
            return tw::channel_or::ProcessorOrders::instance().createOrder(createOrderId);
        }
        
        static tw::channel_or::TOrderPtr getOrder(const tw::channel_or::TOrderId& x) {
            return tw::channel_or::ProcessorOrders::instance().get(x);
        }
        
        static bool canSend(const tw::channel_or::TOrderPtr& order, const tw::price::Ticks& price, tw::channel_or::Reject& rej) {
            return tw::channel_or::ProcessorWTP::instance().canSend(order, price, rej);
        }
        
        static bool sendNew(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej) {
            return tw::common_strat::StrategyContainer::instance().sendNew(order, rej);
        }
        
        static bool sendMod(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej) {
            return tw::common_strat::StrategyContainer::instance().sendMod(order, rej);
        }
        
        static bool sendCxl(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej) {
            return tw::common_strat::StrategyContainer::instance().sendCxl(order, rej);
        }
        
        static bool isOrderLive(const tw::channel_or::TOrderPtr& order) {
            return tw::channel_or::ProcessorOrders::instance().isOrderLive(order);
        }
        
        static bool hasOpenOrders(const tw::channel_or::TAccountId& x, const tw::channel_or::TStrategyId& y) {
            return (!tw::channel_or::ProcessorOrders::instance().getAllForAccountStrategy(x, y).empty());
        }

        static bool hasPosition(const tw::channel_or::TStrategyId& x, tw::instr::InstrumentPtr& instrument) {
            tw::channel_or::StrategyInstrOpenOrdersPos infos = tw::channel_or::ProcessorOrders::instance().getInstrOpenOrdersPosForStrategyInstr(x, instrument->_keyId);
            if ( 0 != infos._pos )
                return true;

            return false;
        }
        
        static int32_t getPosition(const tw::channel_or::TStrategyId& x, tw::instr::InstrumentPtr& instrument) {
            return tw::channel_or::ProcessorOrders::instance().getInstrOpenOrdersPosForStrategyInstr(x, instrument->_keyId)._pos;
        }
        
        static TOrders getOpenOrders(const tw::channel_or::TAccountId& x, const tw::channel_or::TStrategyId& y) {
            return tw::channel_or::ProcessorOrders::instance().getAllForAccountStrategy(x, y);
        }
        
        static TOrders getOpenOrders(const tw::channel_or::TAccountId& x, const tw::channel_or::TStrategyId& y, const tw::instr::Instrument::TKeyId& z) {
            return tw::channel_or::ProcessorOrders::instance().getAllForAccountStrategyInstr(x, y, z);
        }
        
        static bool checkMaxPos(const tw::channel_or::TAccountId& x, const tw::channel_or::TStrategyId& y, tw::instr::InstrumentConstPtr& instrument, const tw::channel_or::eOrderSide side, tw::price::Size qty) {
            return tw::channel_or::ProcessorRisk::instance().canSend(x, instrument->_keyId, tw::price::Size(qty), side);
        }
        
        static void sendToMsgBusConnection(tw::common_strat::TMsgBusConnection::native_type id, const std::string& m) {
            tw::common_strat::StrategyContainer::instance().sendToMsgBusConnection(id, m);
        }
        
        template <typename TClient>
        static bool registerTimerClient(TClient* client, const uint32_t msecs, const bool once, tw::common::TTimerId& id) {
            return tw::common::TimerServer::instance().registerClient(client, msecs, once, id);
        }
    
        static void sendAlert(const std::string& m) {
            tw::common_strat::StrategyContainer::instance().sendToAllMsgBusConnections(m + "\n");
        }
        
        static uint32_t getSecFromMidnight() {
            return tw::common::THighResTime::now().getUsecsFromMidnight() / 1000000;
        }
        
    private:
        TImpl _impl;
    };
}
}
