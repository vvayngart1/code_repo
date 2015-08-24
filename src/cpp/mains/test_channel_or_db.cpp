#include <tw/log/defs.h>
#include <tw/channel_db/channel_db.h>
#include <tw/generated/channel_or_defs.h>

#include "./unit_test_channel_or_lib/order_helper.h"

struct  Orders_SaveOrderTransaction_test {
    bool execute(tw::channel_db::ChannelDb::TStatementPtr statement, const std::vector<tw::channel_or::Order*> orders)
    {
        bool status = true;
        std::stringstream sql;
        try {
        
		sql << "INSERT INTO OrdersTransactions (route_out, route_in, state, type, side, timeInForce, accountId, strategyId, instrumentId, orderId, qty, cumQty, price, newPrice, manual, cancelOnAck, modCounter, exTimestamp, trTimestamp, timestamp1, timestamp2, timestamp3, timestamp4, origClOrderId, clOrderId, corrClOrderId, exOrderId, stratReason) VALUES";
                std::vector<tw::channel_or::Order*>::const_iterator iter = orders.begin();
                std::vector<tw::channel_or::Order*>::const_iterator beg = orders.begin();
                std::vector<tw::channel_or::Order*>::const_iterator end = orders.end();
                
                for ( ; iter != end; ++ iter ) {
                    tw::channel_or::Order& p1 = *(*iter);
                    if ( iter != beg )
                        sql << ",";
                    
                    sql << "("
			 << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._newMsgs), "'", "\\'") << "'" << ","
			 << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._modMsgs), "'", "\\'") << "'" << ","
                         << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._cxlMsgs), "'", "\\'") << "'" << ","
                         << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._totalVolume), "'", "\\'") << "'" << ","
			 << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._state), "'", "\\'") << "'" << ","
			 << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._type), "'", "\\'") << "'" << ","
			 << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._side), "'", "\\'") << "'" << ","
			 << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._timeInForce), "'", "\\'") << "'" << ","
			 << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._accountId), "'", "\\'") << "'" << ","
			 << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._strategyId), "'", "\\'") << "'" << ","
			 << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._instrumentId), "'", "\\'") << "'" << ","
			 << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._orderId), "'", "\\'") << "'" << ","
			 << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._qty), "'", "\\'") << "'" << ","
			 << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._cumQty), "'", "\\'") << "'" << ","
			 << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._price), "'", "\\'") << "'" << ","
			 << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._newPrice), "'", "\\'") << "'" << ","
			 << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._manual), "'", "\\'") << "'" << ","
			 << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._cancelOnAck), "'", "\\'") << "'" << ","
			 << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._modCounter), "'", "\\'") << "'" << ","
			 << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._exTimestamp), "'", "\\'") << "'" << ","
			 << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._trTimestamp), "'", "\\'") << "'" << ","
			 << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._timestamp1), "'", "\\'") << "'" << ","
			 << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._timestamp2), "'", "\\'") << "'" << ","
			 << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._timestamp3), "'", "\\'") << "'" << ","
			 << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._timestamp4), "'", "\\'") << "'" << ","
			 << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._origClOrderId), "'", "\\'") << "'" << ","
			 << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._clOrderId), "'", "\\'") << "'" << ","
			 << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._corrClOrderId), "'", "\\'") << "'" << ","
			 << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._exOrderId), "'", "\\'") << "'" << ","
			 << "'" << std::replace_all(boost::lexical_cast<std::string>(p1._stratReason), "'", "\\'") << "'"
			 <<")";
                }
                
                std::string s = sql.str();
                
                
		statement->execute(sql.str());
        } catch(const std::exception& e) {            
            status = false;
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        } catch(...) {
            status = false;
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";        
        }
        
        if ( !status ) {
            try {
                LOGGER_ERRO << "Failed to execute sql: " << sql.str() << "\n" << "\n";
            } catch(...) {
            }
        }
        
        return status;
    }
    
};

int main(int argc, char * argv[]) {
    try {
        tw::channel_or::Order order;
        tw::channel_or::Fill fill;
        tw::channel_or::Reject rej;
        tw::channel_or::Reject rej2;
        tw::channel_or::PosUpdate pos;
        
        tw::channel_or::Orders_SaveOrderTransaction orders_SaveOrderTransaction;        
        tw::channel_or::Orders_SaveOrder orders_SaveOrder;
        
        tw::channel_or::Fills_SaveFill fills_SaveFill;
        tw::channel_or::Positions_SavePosUpdate positions_SavePosUpdate;
        
        tw::channel_or::Orders_GetAll orders_GetAll;
        tw::channel_or::Orders_GetAllOpen orders_GetAllOpen;
        tw::channel_or::Positions_GetAll positions_GetAll;
        
        std::string connectionString;
        connectionString = "tcp://172.20.10.158:3306;root;tw_test_vlad";
        
        tw::channel_db::ChannelDb channelDb;
        if ( !channelDb.init(connectionString) )
            return -1;
        
        OrderHelper::getOrder(order);
        OrderHelper::getRej(rej);
        
        tw::channel_db::ChannelDb::TConnectionPtr connection = channelDb.getConnection();
        tw::channel_db::ChannelDb::TStatementPtr statement = channelDb.getStatement(connection);
        
        for ( int32_t i = 0; i < 10; ++i ) {
            if ( i % 2 ) {
                order._state = tw::channel_or::eOrderState::kCancelling;
                orders_SaveOrderTransaction.clear();
                orders_SaveOrder.clear();
                
                orders_SaveOrderTransaction.add(order, rej);
                orders_SaveOrder.add(order, rej);
                
                if ( !orders_SaveOrderTransaction.execute(statement) ) {
                    LOGGER_ERRO << "Failed to insert order: "  << order.toString() << "\n" << "\n";
                }
                
                if ( !orders_SaveOrder.execute(statement) ) {
                    LOGGER_ERRO << "Failed to update order: "  << order.toString() << "\n" << "\n";
                }
            } else {
                order._state = tw::channel_or::eOrderState::kWorking;
                order._clOrderId = boost::lexical_cast<std::string>(i+1);
                
                orders_SaveOrderTransaction.add(order, rej2);
                orders_SaveOrder.add(order, rej2);
                
                if ( !orders_SaveOrderTransaction.execute(statement) ) {
                    LOGGER_ERRO << "Failed to insert order: "  << order.toString() << "\n" << "\n";
                }
                
                if ( !orders_SaveOrder.execute(statement) ) {
                    LOGGER_ERRO << "Failed to update order: "  << order.toString() << "\n" << "\n";
                }
            }
        }
        
        OrderHelper::getFill(fill, pos);
        fill._orderId = order._orderId;
        
        fills_SaveFill.add(fill);
        if ( !fills_SaveFill.execute(statement) ) {
            LOGGER_ERRO << "Failed to insert fill: "  << fill.toString() << "\n" << "\n";
        }
        
        positions_SavePosUpdate.add(pos);
        if ( !positions_SavePosUpdate.execute(statement) ) {
            LOGGER_ERRO << "Failed to insert fill: "  << pos.toString() << "\n" << "\n";
        }
        
        
        if ( !orders_GetAll.execute(channelDb) ) {
            LOGGER_ERRO << "Failed in orders_GetAll.execute()"  << "\n" << "\n";
        } else {
            for ( size_t i = 0; i < orders_GetAll._o1.size(); ++i ) {
                LOGGER_INFO << i << " <==> " << orders_GetAll._o1[i].toString() << "\n";
            }
        }
        
        if ( !orders_GetAllOpen.execute(channelDb) ) {
            LOGGER_ERRO << "Failed in orders_GetAllOpen.execute()"  << "\n" << "\n";
        } else {
            for ( size_t i = 0; i < orders_GetAllOpen._o1.size(); ++i ) {
                LOGGER_INFO << i << " <==> " << orders_GetAllOpen._o1[i].toString() << "\n";
            }
        }        
        
        if ( !positions_GetAll.execute(channelDb) ) {
            LOGGER_ERRO << "Failed in positions_GetAll.execute()"  << "\n" << "\n";
        } else {
            for ( size_t i = 0; i < positions_GetAll._o1.size(); ++i ) {
                LOGGER_INFO << i << " <==> " << positions_GetAll._o1[i].toString() << "\n";
            }
        }
        
        
        std::vector<tw::channel_or::Order> orders;
        std::vector<tw::channel_or::Order*> ordersPtrs;
        Orders_SaveOrderTransaction_test test;
        
        tw::common::THighResTime t1;
        tw::common::THighResTime t2;
        
        size_t i = 0;
        
        for ( i = 0; i < 200; ++i ) {
            order._state = tw::channel_or::eOrderState::kWorking;
            order._clOrderId = boost::lexical_cast<std::string>(i+1);
            
            orders.push_back(order);
        }
        
        for ( i = 0; i < orders.size(); ++i ) {
            ordersPtrs.push_back(&orders[i]);
        }
        
        t1 = tw::common::THighResTime::now();
        
        for ( i = 0; i < orders.size(); ++i ) {
            orders_SaveOrderTransaction.clear();
            orders_SaveOrderTransaction.add(orders[i], rej2);
            if ( !orders_SaveOrderTransaction.execute(statement) ) {
                LOGGER_ERRO << "Failed to insert order: "  << order.toString() << "\n" << "\n";
            }
        }
        
        t2 = tw::common::THighResTime::now();
        
        LOGGER_ERRO << "\nOne-by-one t2-t1: "  << orders.size() << " :: " << static_cast<double>(t2-t1)/1000000.0 << "\n";
        
        
        orders_SaveOrderTransaction.clear();
        t1 = tw::common::THighResTime::now();
        
        for ( i = 0; i < orders.size(); ++i ) {
            orders_SaveOrderTransaction.add(orders[i], rej2);
        }
        
        if ( !orders_SaveOrderTransaction.execute(statement) ) {
            LOGGER_ERRO << "Failed to bulk insert orders"  << "\n" << "\n";
        }
        
        t2 = tw::common::THighResTime::now();
        
        LOGGER_ERRO << "\nBulk1 t2-t1: " << orders.size() << " :: " << static_cast<double>(t2-t1)/1000000.0 << "\n";
        
        
        t1 = tw::common::THighResTime::now();
        
        test.execute(statement, ordersPtrs);
        
        t2 = tw::common::THighResTime::now();
        
        LOGGER_ERRO << "\nBulk2 t2-t1: " << orders.size() << " :: " << static_cast<double>(t2-t1)/1000000.0 << "\n";
        
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";        
    }
    
    return 0;
}

