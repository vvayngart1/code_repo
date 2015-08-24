#include <tw/generated/instrument.h>
#include <tw/generated/channel_or_defs.h>
#include <tw/channel_or_cme/id_factory.h>
#include <tw/exchange_sim/matcher.h>

#include "../unit_test_price_lib/instr_helper.h"

struct OrderHelper {
    static tw::channel_or::TOrderPtr getEmpty(tw::instr::InstrumentPtr instrument = InstrHelper::getNQH2() ) {
        tw::channel_or::TOrderPtr order(new tw::channel_or::Order());
        
        order->_instrument = instrument;
        order->_instrumentId = instrument->_keyId;
        
        tw::instr::InstrumentManager::instance().addInstrument(instrument);
        
        return order;
    }
    
    static tw::channel_or::TOrderPtr getEmptyWithOrderId(tw::instr::InstrumentPtr instrument = InstrHelper::getNQH2() ) {
        tw::channel_or::TOrderPtr order = getEmpty(instrument);
        
        order->_orderId = tw::common::generateUuid();        
        return order;
    }
    
    static tw::channel_or::TOrderPtr getBuyLimit(int32_t price,
                                                 uint32_t qty,
                                                 tw::instr::InstrumentPtr instrument = InstrHelper::getNQH2() ) {
        tw::channel_or::TOrderPtr order = getEmptyWithOrderId(instrument);
        
        order->_type = tw::channel_or::eOrderType::kLimit;
        order->_side = tw::channel_or::eOrderSide::kBuy;
        order->_price.set(price);
        order->_qty.set(qty);
        
        return order;
    }
    
    static tw::channel_or::TOrderPtr getSellLimit(int32_t price,
                                                  uint32_t qty,
                                                  tw::instr::InstrumentPtr instrument = InstrHelper::getNQH2() ) {
        tw::channel_or::TOrderPtr order = getEmptyWithOrderId(instrument);
        
        order->_type = tw::channel_or::eOrderType::kLimit;
        order->_side = tw::channel_or::eOrderSide::kSell;
        order->_price.set(price);
        order->_qty.set(qty);
        
        return order;
    }
    
    static tw::channel_or::TOrderPtr getBuyLimitForAccountStrategy(int32_t price,
                                                                   uint32_t qty,
                                                                   tw::channel_or::TAccountId accountId,
                                                                   tw::channel_or::TStrategyId strategyId,
                                                                   tw::instr::InstrumentPtr instrument = InstrHelper::getNQH2() ) {
        tw::channel_or::TOrderPtr order = getBuyLimit(price, qty, instrument);
        order->_accountId = accountId;
        order->_strategyId = strategyId;
        
        return order;
    }
    
    static tw::channel_or::TOrderPtr getSellLimitForAccountStrategy(int32_t price,
                                                                   uint32_t qty,
                                                                   tw::channel_or::TAccountId accountId,
                                                                   tw::channel_or::TStrategyId strategyId,
                                                                   tw::instr::InstrumentPtr instrument = InstrHelper::getNQH2() ) {
        tw::channel_or::TOrderPtr order = getSellLimit(price, qty, instrument);
        order->_accountId = accountId;
        order->_strategyId = strategyId;
        
        return order;
    }
    
    static tw::channel_or::TOrderPtr getBuyLimitWithClOrderIds(int32_t price,
                                                               uint32_t qty,
                                                               tw::instr::InstrumentPtr instrument = InstrHelper::getNQH2() ) {
        tw::channel_or::TOrderPtr order = getBuyLimit(price, qty, instrument);
        order->_origClOrderId = order->_clOrderId = order->_corrClOrderId = tw::channel_or_cme::IdFactory::instance().get().c_str();
        
        return order;
    }
    
    static tw::channel_or::TOrderPtr getSellLimitWithClOrderIds(int32_t price,
                                                                uint32_t qty,
                                                                tw::instr::InstrumentPtr instrument = InstrHelper::getNQH2() ) {
        tw::channel_or::TOrderPtr order = getSellLimit(price, qty, instrument);
        order->_origClOrderId = order->_clOrderId = order->_corrClOrderId = tw::channel_or_cme::IdFactory::instance().get().c_str();
        
        return order;
    }
    
    
    static tw::exchange_sim::TOrderPtr getSimBuyLimitWithClOrderIds(int32_t price,
                                                                  uint32_t qty,
                                                                  tw::instr::InstrumentPtr instrument = InstrHelper::getNQH2() ) {
        tw::channel_or::TOrderPtr order = getBuyLimit(price, qty, instrument);
        order->_origClOrderId = order->_clOrderId = order->_corrClOrderId = tw::channel_or_cme::IdFactory::instance().get().c_str();
        
        tw::exchange_sim::TOrderPtr orderSim(new tw::exchange_sim::TOrder());        
        static_cast<tw::channel_or::Order&>(*orderSim) = *order;
        
        return orderSim;
    }
    
    static tw::exchange_sim::TOrderPtr getSimSellLimitWithClOrderIds(int32_t price,
                                                                   uint32_t qty,
                                                                   tw::instr::InstrumentPtr instrument = InstrHelper::getNQH2() ) {
        tw::channel_or::TOrderPtr order = getSellLimit(price, qty, instrument);
        order->_origClOrderId = order->_clOrderId = order->_corrClOrderId = tw::channel_or_cme::IdFactory::instance().get().c_str();
        
        tw::exchange_sim::TOrderPtr orderSim(new tw::exchange_sim::TOrder());        
        static_cast<tw::channel_or::Order&>(*orderSim) = *order;
        
        return orderSim;
    }
    
    static tw::channel_or::TOrderPtr getBuyMarket(uint32_t qty,
                                                  tw::instr::InstrumentPtr instrument = InstrHelper::getNQH2() ) {
        tw::channel_or::TOrderPtr order(new tw::channel_or::Order());
        
        order->_type = tw::channel_or::eOrderType::kMarket;
        order->_side = tw::channel_or::eOrderSide::kBuy;
        order->_qty.set(qty);
        order->_instrument = instrument;
        
        return order;
    }
    
    static tw::channel_or::TOrderPtr getSellMarket(uint32_t qty,
                                                   tw::instr::InstrumentPtr instrument = InstrHelper::getNQH2() ) {
        tw::channel_or::TOrderPtr order(new tw::channel_or::Order());
        
        order->_type = tw::channel_or::eOrderType::kMarket;
        order->_side = tw::channel_or::eOrderSide::kSell;
        order->_qty.set(qty);
        order->_instrument = instrument;
        
        return order;
    }
    
    static void getOrder(tw::channel_or::Order& order) {
        order._state = tw::channel_or::eOrderState::kWorking;
        order._type = tw::channel_or::eOrderType::kLimit;
        order._side = tw::channel_or::eOrderSide::kBuy;
        order._timeInForce = tw::channel_or::eTimeInForce::kDay;
        order._accountId = 1111;
        order._strategyId = 2222;
        order._instrumentId = 3333;
        order._orderId = tw::common::generateUuid();
        order._qty = tw::price::Size(1);
        order._price = tw::price::Ticks(12567);
        order._newPrice = tw::price::Ticks(12568);
        order._manual = true;
        order._cancelOnAck = false;

        order._exTimestamp.setToNow();
        order._trTimestamp.setToNow();
        order._timestamp1.setToNow();
        order._timestamp2.setToNow();
        order._timestamp3.setToNow();
        order._timestamp4.setToNow();
    }

    static void getFill(tw::channel_or::Fill& fill, tw::channel_or::PosUpdate& pos) {
        fill._type = tw::channel_or::eFillType::kNormal;
        fill._subType = tw::channel_or::eFillSubType::kOutright;
        fill._accountId = 1111;
        fill._strategyId = 2222;
        fill._instrumentId = 3333;
        fill._fillId = tw::common::generateUuid();
        fill._side = tw::channel_or::eOrderSide::kBuy;
        fill._qty = tw::price::Size(1);
        fill._price = tw::price::Ticks(12567);

        fill._exTimestamp.setToNow();
        fill._timestamp1.setToNow();
        fill._timestamp2.setToNow();
        fill._timestamp3.setToNow();
        fill._timestamp4.setToNow();
        fill._exchangeFillId = "12345678";

        pos._accountId = fill._accountId;
        pos._strategyId = fill._strategyId;
        pos._displayName = "NQH2";
        pos._exchange = tw::instr::eExchange::kCME;
        pos._pos = fill._qty;
    }

    static void getRej(tw::channel_or::Reject& rej) {
        rej._rejType = tw::channel_or::eRejectType::kInternal;
        rej._rejSubType = tw::channel_or::eRejectSubType::kConnection;
        rej._rejReason = tw::channel_or::eRejectReason::kConnectionDown;
        rej._text = "Test 'ConnectionDown' reject";
    }
    
    static void getStateCME(tw::channel_or::FixSessionCMEState& state) {
        state._inSeqNum = 1;
        state._outSeqNum = 2;
        state._senderCompID = "AMR";
        state._targetCompID = "CME";
        state._week = 5;
        state._year = 2012;
    }
    
    static void getMsgCME(tw::channel_or::FixSessionCMEMsg& msg) {
        msg._seqNum = 1;
        msg._senderCompID = "AMR";
        msg._targetCompID = "CME";
        msg._week = 5;
        msg._year = 2012;
        msg._direction = tw::channel_or::eDirection::kOutbound;
        msg._message = "8=FIX.4.2|9=251|35=D|49=7E59Z1N|56=CME|34=1444|50=AMR_TW|142=US,IL|57=G|52=20120302-17:02:49.092|369=1435|11=S57BPXW|1=82409802|21=1|55=NQ|167=FUT|107=NQM2|54=1|60=20120302-17:02:49.092|38=1|40=2|44=257775|204=1|9702=2|1603=RosenthalSPOC|1604=1.0|1605=AMRSPOC|1028=Y|10=192|";
    }
       
    static void getFillDropCopy(tw::channel_or::FillDropCopy& fill) {
        fill._type = tw::channel_or::eFillType::kNormal;
        fill._subType = tw::channel_or::eFillSubType::kOutright;
        fill._accountName = "TestAcccount";
        fill._displayName = "NQM2";
        fill._exchange = tw::instr::eExchange::kCME;
        fill._side = tw::channel_or::eOrderSide::kBuy;
        fill._qty = tw::price::Size(1);
        fill._price = 1620.75;
        
        fill._origClOrderId = "1";
        fill._clOrderId = "2";
        fill._exOrderId = "4";        
        fill._exFillId = "12345678";
        fill._exFillRefId = "12345678.1";
        
        fill._liqInd = tw::channel_or::eLiqInd::kAdd;
        fill._sourceSession = "TestSession";
        fill._date = "20100506";
        fill._exTimestamp = "20100506-12:30:42.576000";
        fill._recvTimestamp = "20100506-12:30:43.576000";
    }
};
