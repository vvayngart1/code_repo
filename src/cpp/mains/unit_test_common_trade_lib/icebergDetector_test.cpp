#include <vector>

#include <tw/common_strat/defs.h>
#include <tw/generated/common_trade_blocks.h>
#include <tw/common_trade/icebergDetector.h>
#include <tw/price/defs.h>

#include "../unit_test_price_lib/instr_helper.h"

#include <gtest/gtest.h>

namespace tw {
namespace common_trade {
    
typedef tw::price::Ticks TPrice;
typedef tw::price::Size TSize;

typedef IcebergDetector TImpl;
typedef IcebergDetectorParamsWire TParams;

typedef TImpl::TInfos TInfos;

static TParams getParams(float ratio,
                         TSize minMaxQty,
                         TSize minVolume,
                         TPrice slideStopAgainstTicks,
                         TPrice slideStopForTicks,
                         TPrice resetCountAwayTicks) {
    TParams p;
    
    p._idRatio = ratio;
    p._idMinMaxQty = minMaxQty;
    p._idMinVolume = minVolume;
    p._idSlideStopAgainstTicks = slideStopAgainstTicks;
    p._idSlideStopForTicks = slideStopForTicks;
    p._idResetCountAwayTicks = resetCountAwayTicks;
    
    return p;
}

bool checkInfo(const IcebergInfo& info, TPrice price, TSize maxQty, TSize volume, tw::channel_or::eOrderSide side) {
    EXPECT_EQ(info._price, price);
    if ( info._price != price )
        return false;
    
    EXPECT_EQ(info._maxQty, maxQty);
    if ( info._maxQty != maxQty )
        return false;
    
    EXPECT_EQ(info._volume, volume);
    if ( info._volume != volume )
        return false;
    
    EXPECT_EQ(info._side, side);
    if ( info._side != side )
        return false;
    
    return true;
}


bool checkInfo(const TInfos& infos, TPrice price, TSize maxQty, TSize volume, tw::channel_or::eOrderSide side) {
    TInfos::const_iterator iter = infos.find(price);
    EXPECT_TRUE(iter != infos.end());
    if ( iter == infos.end() )
        return false;
    
    return checkInfo(iter->second, price, maxQty, volume, side);
}

TEST(CommonTradeLibTestSuit, IcebergDetector_test)
{
    TParams params;
    
    TImpl impl(params);
    TParams& p = impl.getParams();
    p = getParams(1.5, TSize(5), TSize(20), TPrice(3), TPrice(1), TPrice(2));
    
    TInfos& infos = impl.getInfos();
    
    // Test iceberg detection
    //
    
    // Set quote
    //
    tw::price::QuoteStore::TQuote quote;
    quote.setInstrument(InstrHelper::get6CU2());
    
    impl.onQuote(quote);
    ASSERT_TRUE(infos.empty());
    ASSERT_TRUE(!p._idIsIcebergDetected);
    ASSERT_TRUE(!p._idIsLastEventTrade);    
    
    quote.setAsk(TPrice(108), TSize(2), 0);
    quote.setBid(TPrice(107), TSize(4), 0);    
    impl.onQuote(quote);
    
    ASSERT_TRUE(!p._idIsIcebergDetected);
    ASSERT_TRUE(!p._idIsLastEventTrade);
    ASSERT_EQ(p._idReason, "");
    ASSERT_EQ(infos.size(), 2);
    ASSERT_TRUE(checkInfo(infos, TPrice(108), TSize(2), TSize(), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(107), TSize(4), TSize(), tw::channel_or::eOrderSide::kBuy));
    
    quote.clearRuntime();
    quote.setTrade(TPrice(107), TSize(1));
    impl.onQuote(quote);
    
    ASSERT_TRUE(!p._idIsIcebergDetected);
    ASSERT_TRUE(p._idIsLastEventTrade);
    ASSERT_EQ(p._idReason, "");
    ASSERT_EQ(infos.size(), 2);
    ASSERT_TRUE(checkInfo(infos, TPrice(108), TSize(2), TSize(), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(107), TSize(4), TSize(1), tw::channel_or::eOrderSide::kBuy));
    ASSERT_EQ(infos.size(), 2);
    
    quote.clearRuntime();
    quote.setAsk(TPrice(107), TSize(3), 0);
    quote.setBid(TPrice(106), TSize(1), 0);    
    impl.onQuote(quote);
    
    ASSERT_TRUE(!p._idIsIcebergDetected);
    ASSERT_TRUE(!p._idIsLastEventTrade);
    ASSERT_EQ(p._idReason, "info._maxQty < _p._minMaxQty: _p._idMinMaxQty=5,info=107,3,0,Sell,quote=book[0]=0,1,106|107,3,0,trade=1,107,u,u,u,u");
    ASSERT_EQ(infos.size(), 3);
    ASSERT_TRUE(checkInfo(infos, TPrice(108), TSize(2), TSize(), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(107), TSize(3), TSize(), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(106), TSize(1), TSize(), tw::channel_or::eOrderSide::kBuy));
    
    quote.clearRuntime();
    quote.setAsk(TPrice(106), TSize(4), 0);
    quote.setBid(TPrice(105), TSize(1), 0);    
    impl.onQuote(quote);
    
    ASSERT_TRUE(!p._idIsIcebergDetected);
    ASSERT_TRUE(!p._idIsLastEventTrade);
    ASSERT_EQ(p._idReason, "");
    ASSERT_EQ(infos.size(), 4);
    ASSERT_TRUE(checkInfo(infos, TPrice(108), TSize(2), TSize(), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(107), TSize(3), TSize(), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(106), TSize(4), TSize(), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(105), TSize(1), TSize(), tw::channel_or::eOrderSide::kBuy));
    
    quote.clearRuntime();
    quote.setAsk(TPrice(105), TSize(5), 0);
    quote.setBid(TPrice(103), TSize(1), 0);    
    impl.onQuote(quote);
    
    ASSERT_TRUE(!p._idIsIcebergDetected);
    ASSERT_TRUE(!p._idIsLastEventTrade);
    ASSERT_EQ(p._idReason, "");
    ASSERT_EQ(infos.size(), 5);
    ASSERT_TRUE(checkInfo(infos, TPrice(108), TSize(), TSize(), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(107), TSize(3), TSize(), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(106), TSize(4), TSize(), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(105), TSize(5), TSize(), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(103), TSize(1), TSize(), tw::channel_or::eOrderSide::kBuy));
    
    quote.clearRuntime();
    quote.setTrade(TPrice(105), TSize(2));    
    impl.onQuote(quote);
    
    ASSERT_TRUE(!p._idIsIcebergDetected);
    ASSERT_TRUE(p._idIsLastEventTrade);
    ASSERT_EQ(p._idReason, "");
    ASSERT_EQ(infos.size(), 5);
    ASSERT_TRUE(checkInfo(infos, TPrice(108), TSize(), TSize(), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(107), TSize(3), TSize(), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(106), TSize(4), TSize(), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(105), TSize(5), TSize(2), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(103), TSize(1), TSize(), tw::channel_or::eOrderSide::kBuy));
    
    quote.clearRuntime();
    quote.setAsk(TPrice(107), TSize(15), 0);
    quote.setBid(TPrice(106), TSize(1), 0);    
    impl.onQuote(quote);
    
    ASSERT_TRUE(!p._idIsIcebergDetected);
    ASSERT_TRUE(!p._idIsLastEventTrade);
    ASSERT_EQ(p._idReason, "quote._trade._price != bbo: quote=book[0]=0,1,106|107,15,0,trade=2,105,u,u,u,u");
    ASSERT_EQ(infos.size(), 5);
    ASSERT_TRUE(checkInfo(infos, TPrice(108), TSize(), TSize(), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(107), TSize(15), TSize(), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(106), TSize(1), TSize(), tw::channel_or::eOrderSide::kBuy));
    ASSERT_TRUE(checkInfo(infos, TPrice(105), TSize(), TSize(), tw::channel_or::eOrderSide::kBuy));
    ASSERT_TRUE(checkInfo(infos, TPrice(103), TSize(), TSize(), tw::channel_or::eOrderSide::kBuy));
    
    quote.clearRuntime();
    quote.setTrade(TPrice(107), TSize(12));    
    impl.onQuote(quote);    
    
    ASSERT_TRUE(!p._idIsIcebergDetected);
    ASSERT_TRUE(p._idIsLastEventTrade);
    ASSERT_EQ(p._idReason, "");
    ASSERT_EQ(infos.size(), 5);
    ASSERT_TRUE(checkInfo(infos, TPrice(108), TSize(), TSize(), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(107), TSize(15), TSize(12), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(106), TSize(1), TSize(), tw::channel_or::eOrderSide::kBuy));
    ASSERT_TRUE(checkInfo(infos, TPrice(105), TSize(), TSize(), tw::channel_or::eOrderSide::kBuy));
    ASSERT_TRUE(checkInfo(infos, TPrice(103), TSize(), TSize(), tw::channel_or::eOrderSide::kBuy));    
    
    quote.clearRuntime();
    quote.setAsk(TPrice(107), TSize(14), 0);    
    impl.onQuote(quote);
    
    ASSERT_TRUE(!p._idIsIcebergDetected);
    ASSERT_TRUE(!p._idIsLastEventTrade);
    ASSERT_EQ(p._idReason, "info.volume < _p._idMinVolume: _p._idMinVolume=20,info=107,15,12,Sell,quote=book[0]=0,1,106|107,14,0,trade=12,107,u,u,u,u");
    ASSERT_EQ(infos.size(), 5);
    ASSERT_TRUE(checkInfo(infos, TPrice(108), TSize(), TSize(), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(107), TSize(15), TSize(12), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(106), TSize(1), TSize(), tw::channel_or::eOrderSide::kBuy));
    ASSERT_TRUE(checkInfo(infos, TPrice(105), TSize(), TSize(), tw::channel_or::eOrderSide::kBuy));
    ASSERT_TRUE(checkInfo(infos, TPrice(103), TSize(), TSize(), tw::channel_or::eOrderSide::kBuy));
    
    quote.clearRuntime();
    quote.setTrade(TPrice(107), TSize(8));    
    impl.onQuote(quote);    
    
    ASSERT_TRUE(!p._idIsIcebergDetected);
    ASSERT_TRUE(p._idIsLastEventTrade);
    ASSERT_EQ(p._idReason, "");
    ASSERT_EQ(infos.size(), 5);
    ASSERT_TRUE(checkInfo(infos, TPrice(108), TSize(), TSize(), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(107), TSize(15), TSize(20), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(106), TSize(1), TSize(), tw::channel_or::eOrderSide::kBuy));
    ASSERT_TRUE(checkInfo(infos, TPrice(105), TSize(), TSize(), tw::channel_or::eOrderSide::kBuy));
    ASSERT_TRUE(checkInfo(infos, TPrice(103), TSize(), TSize(), tw::channel_or::eOrderSide::kBuy));    
    
    quote.clearRuntime();
    quote.setAsk(TPrice(107), TSize(1), 0);    
    impl.onQuote(quote);
    
    ASSERT_TRUE(!p._idIsIcebergDetected);
    ASSERT_TRUE(!p._idIsLastEventTrade);
    ASSERT_EQ(p._idReason, "ratio < _p._idRatio: _p._idRatio=1.5,ratio=1.43,info=107,15,20,Sell,quote=book[0]=0,1,106|107,1,0,trade=8,107,u,u,u,u");
    ASSERT_EQ(infos.size(), 5);
    ASSERT_TRUE(checkInfo(infos, TPrice(108), TSize(), TSize(), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(107), TSize(15), TSize(20), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(106), TSize(1), TSize(), tw::channel_or::eOrderSide::kBuy));
    ASSERT_TRUE(checkInfo(infos, TPrice(105), TSize(), TSize(), tw::channel_or::eOrderSide::kBuy));
    ASSERT_TRUE(checkInfo(infos, TPrice(103), TSize(), TSize(), tw::channel_or::eOrderSide::kBuy));
    
    
    quote.clearRuntime();
    quote.setAsk(TPrice(107), TSize(14), 0);    
    impl.onQuote(quote);
    
    ASSERT_TRUE(!p._idIsIcebergDetected);
    ASSERT_TRUE(!p._idIsLastEventTrade);
    ASSERT_EQ(p._idReason, "");
    ASSERT_EQ(infos.size(), 5);
    ASSERT_TRUE(checkInfo(infos, TPrice(108), TSize(), TSize(), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(107), TSize(15), TSize(20), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(106), TSize(1), TSize(), tw::channel_or::eOrderSide::kBuy));
    ASSERT_TRUE(checkInfo(infos, TPrice(105), TSize(), TSize(), tw::channel_or::eOrderSide::kBuy));
    ASSERT_TRUE(checkInfo(infos, TPrice(103), TSize(), TSize(), tw::channel_or::eOrderSide::kBuy));
    
    quote.clearRuntime();
    quote.setTrade(TPrice(107), TSize(1));    
    impl.onQuote(quote);    
    
    ASSERT_TRUE(!p._idIsIcebergDetected);
    ASSERT_TRUE(checkInfo(p._idCurrIcebergInfo, TPrice(), TSize(), TSize(), tw::channel_or::eOrderSide::kUnknown));
    ASSERT_TRUE(p._idIsLastEventTrade);
    ASSERT_EQ(p._idReason, "");
    ASSERT_EQ(infos.size(), 5);
    ASSERT_TRUE(checkInfo(infos, TPrice(108), TSize(), TSize(), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(107), TSize(15), TSize(21), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(106), TSize(1), TSize(), tw::channel_or::eOrderSide::kBuy));
    ASSERT_TRUE(checkInfo(infos, TPrice(105), TSize(), TSize(), tw::channel_or::eOrderSide::kBuy));
    ASSERT_TRUE(checkInfo(infos, TPrice(103), TSize(), TSize(), tw::channel_or::eOrderSide::kBuy));
    
    
    quote.clearRuntime();
    quote.setAsk(TPrice(107), TSize(16), 0);    
    impl.onQuote(quote);
    
    ASSERT_TRUE(p._idIsIcebergDetected);
    ASSERT_TRUE(checkInfo(p._idCurrIcebergInfo, TPrice(107), TSize(16), TSize(21), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(!p._idIsLastEventTrade);
    ASSERT_EQ(p._idReason, "iceberg detected(ratio >= _p._idRatio): _p._idRatio=1.5,ratio=21,info=107,16,21,Sell,quote=book[0]=0,1,106|107,16,0,trade=1,107,u,u,u,u");
    ASSERT_EQ(infos.size(), 5);
    ASSERT_TRUE(checkInfo(infos, TPrice(108), TSize(), TSize(), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(107), TSize(16), TSize(21), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(106), TSize(1), TSize(), tw::channel_or::eOrderSide::kBuy));
    ASSERT_TRUE(checkInfo(infos, TPrice(105), TSize(), TSize(), tw::channel_or::eOrderSide::kBuy));
    ASSERT_TRUE(checkInfo(infos, TPrice(103), TSize(), TSize(), tw::channel_or::eOrderSide::kBuy));
        
    // Test stop calc
    //    
    FillInfo info;
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info));
    ASSERT_EQ(info._stop, TPrice());
    ASSERT_EQ(p._idReason, "tw::channel_or::eOrderSide::kUnknown == info._fill._side");
    
    p._idIsIcebergDetected = false;
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info));
    ASSERT_EQ(info._stop, TPrice());
    ASSERT_EQ(p._idReason, "_p._idIsIcebergDetected==false");
    
    info._stop.set(103);
    p._idIsIcebergDetected = true;
    info._fill._side = tw::channel_or::eOrderSide::kBuy;
    ASSERT_TRUE(impl.isStopSlideTriggered(info));
    ASSERT_EQ(info._stop, TPrice(104));
    
    info._stop.set(105);
    ASSERT_TRUE(!impl.isStopSlideTriggered(info));
    ASSERT_EQ(info._stop, TPrice(105));
    ASSERT_EQ(p._idReason, "info._stop.isValid() && info._stop >= stop");
    
    info._stop.set(109);
    info._fill._side = tw::channel_or::eOrderSide::kSell;
    ASSERT_TRUE(impl.isStopSlideTriggered(info));
    ASSERT_EQ(info._stop, TPrice(108));
    
    info._stop.set(107);
    ASSERT_TRUE(!impl.isStopSlideTriggered(info));
    ASSERT_EQ(info._stop, TPrice(107));
    ASSERT_EQ(p._idReason, "info._stop.isValid() && info._stop <= stop");
    
    p._idCurrIcebergInfo._side = tw::channel_or::eOrderSide::kBuy;
    
    info._stop.clear();
    info._fill._side = tw::channel_or::eOrderSide::kBuy;
    ASSERT_TRUE(impl.isStopSlideTriggered(info));
    ASSERT_EQ(info._stop, TPrice(106));
    
    info._stop.clear();
    info._fill._side = tw::channel_or::eOrderSide::kSell;
    ASSERT_TRUE(impl.isStopSlideTriggered(info));
    ASSERT_EQ(info._stop, TPrice(110));
    
    p._idCurrIcebergInfo.clear();
    info._stop.clear();
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info));
    ASSERT_EQ(info._stop, TPrice());
    ASSERT_EQ(p._idReason, "tw::channel_or::eOrderSide::kUnknown == _p._idCurrIcebergInfo._icebergSide");
    
    p._idCurrIcebergInfo._side = tw::channel_or::eOrderSide::kBuy;
    
    ASSERT_TRUE(!impl.isStopSlideTriggered(info));
    ASSERT_EQ(info._stop, TPrice());
    ASSERT_EQ(p._idReason, "_p._idCurrIcebergInfo._price.isValid()==false");
    
    // Test resetting iceberg
    //
    p._idIsLastEventTrade = true;
    quote.clearRuntime();
    quote.setAsk(TPrice(107), TSize(16), 0);    
    impl.onQuote(quote);
    
    ASSERT_TRUE(p._idIsIcebergDetected);
    ASSERT_TRUE(checkInfo(p._idCurrIcebergInfo, TPrice(107), TSize(16), TSize(21), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(!p._idIsLastEventTrade);
    ASSERT_EQ(p._idReason, "iceberg detected(ratio >= _p._idRatio): _p._idRatio=1.5,ratio=21,info=107,16,21,Sell,quote=book[0]=0,1,106|107,16,0,trade=1,107,u,u,u,u");
    ASSERT_EQ(infos.size(), 5);
    ASSERT_TRUE(checkInfo(infos, TPrice(108), TSize(), TSize(), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(107), TSize(16), TSize(21), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(106), TSize(1), TSize(), tw::channel_or::eOrderSide::kBuy));
    ASSERT_TRUE(checkInfo(infos, TPrice(105), TSize(), TSize(), tw::channel_or::eOrderSide::kBuy));
    ASSERT_TRUE(checkInfo(infos, TPrice(103), TSize(), TSize(), tw::channel_or::eOrderSide::kBuy));
    
    impl.resetIceberg();
    
    ASSERT_TRUE(!p._idIsIcebergDetected);
    ASSERT_TRUE(checkInfo(p._idCurrIcebergInfo, TPrice(), TSize(), TSize(), tw::channel_or::eOrderSide::kUnknown));
    ASSERT_TRUE(!p._idIsLastEventTrade);
    ASSERT_EQ(p._idReason, "");
    ASSERT_EQ(infos.size(), 5);
    ASSERT_TRUE(checkInfo(infos, TPrice(108), TSize(), TSize(), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(107), TSize(), TSize(), tw::channel_or::eOrderSide::kSell));
    ASSERT_TRUE(checkInfo(infos, TPrice(106), TSize(1), TSize(), tw::channel_or::eOrderSide::kBuy));
    ASSERT_TRUE(checkInfo(infos, TPrice(105), TSize(), TSize(), tw::channel_or::eOrderSide::kBuy));
    ASSERT_TRUE(checkInfo(infos, TPrice(103), TSize(), TSize(), tw::channel_or::eOrderSide::kBuy));
    
}

} // common_trade
} // tw

