#include <tw/log/defs.h>
#include <tw/generated/channel_or_defs.h>

int main(int argc, char * argv[]) {
    tw::channel_or::Order order;
    
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
    order._cancelOnAck = false;
    
    order._exTimestamp.setToNow();
    order._trTimestamp.setToNow();
    order._timestamp1.setToNow();
    order._timestamp2.setToNow();
    order._timestamp3.setToNow();
    
    tw::common::eCommandType commandType = tw::common::eCommandType::kChannelOr;
    LOGGER_INFO << "\ncommand type: " << commandType << "\n";
    LOGGER_INFO << "\norder's side: " << order._side << "\n";
    
    LOGGER_INFO << "\n" << order.toString() << "\n";
    LOGGER_INFO << "\n" << order.toStringVerbose() << "\n";
    
    tw::channel_or::Order order2;
    order2.fromString(order.toString());
    
    if ( order.toString() == order2.toString() )
        LOGGER_INFO << "order == order2" << "\n";
    else
        LOGGER_INFO << "order != order2" << "\n";        
    
    LOGGER_INFO << "\n" << order2.toString() << "\n";
    LOGGER_INFO << "\n" << order2.toStringVerbose() << "\n";
    
    tw::price::Ticks price;
    LOGGER_INFO << "invalidPrice: " << !price.isValid() << " :: " << price << "\n";
    
    tw::price::Ticks payupTicks(0);
    price = tw::price::Ticks(101) + -payupTicks;
    
    LOGGER_INFO << "price: " << !price.isValid() << " :: " << price << "\n";
    
    return 0;
}

