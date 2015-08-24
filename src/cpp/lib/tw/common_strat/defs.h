#pragma once

#include <tw/common/defs.h>
#include <tw/common/timer_server.h>
#include <tw/common_comm/tcp_ip_server.h>
#include <tw/price/quote_store.h>
#include <tw/generated/channel_or_defs.h>

namespace tw {
namespace common_strat {    
    typedef tw::price::QuoteStore TQuoteStore;
    typedef TQuoteStore::TQuote TQuote;
    typedef tw::channel_or::Client TChannelOrClient;
    
    typedef tw::common_comm::TcpIpServer TMsgBusServer;
    typedef tw::common_comm::TcpIpServerConnection TMsgBusConnection;
    typedef tw::common_comm::TcpIpServerCallback TMsgBusServerCallback;
    
} // common_strat
} // tw
