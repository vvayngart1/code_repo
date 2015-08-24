#pragma once

#include <tw/common/high_res_time.h>
#include <tw/common_comm/tcp_ip_client_connection.h>
#include <tw/channel_pf_cme/settings.h>
#include <tw/channel_pf/ichannel_impl.h>

// NOTE: no thread synchornization provisions are implemented in this class
// The following assumptions need to be valid to avoid race conditions:
//      1. ALL calls to subscribe()/unsubscribe() methods are done AFTER init() 
//      but BEFORE start() or AFTER stop()
//      2. All tw::price::TQuotes objects are created BEFORE init() call and destroyed
//      AFTER stop() call 
//      3. Any thread syncronizations needed for managing tw::price::TQuotes are done
//      OUTSIDE this class
//

namespace tw {
namespace channel_pf_historical {
class ChannelPfHistorical : public tw::channel_pf::IChannelImpl,
                            public tw::common_comm::TcpIpClientCallback {
public:
    ChannelPfHistorical();
    virtual ~ChannelPfHistorical();

public:
    // tw::channel_pf::IChannelImpl interface
    //
    virtual bool init(const tw::common::Settings& settings);
    virtual bool start();    
    virtual void stop();
    
    virtual bool subscribe(tw::instr::InstrumentConstPtr instrument);
    virtual bool unsubscribe(tw::instr::InstrumentConstPtr instrument);
    
public:
    // tw::common_comm::TcpIpClientCallback interface
    //
    virtual void onConnectionUp(TConnection::native_type id, TConnection::pointer connection);    
    virtual void onConnectionError(TConnection::native_type id, const std::string& message);    
    virtual void onConnectionData(TConnection::native_type id, const std::string& message);
    virtual int32_t process(TConnection::native_type id, char* data, size_t size);
    
private:
    tw::common::Settings _settings;
    tw::common_comm::TcpIpClientConnection::pointer _connection;
    std::vector<std::string> _tokens;
};
	
} // channel_pf_cme
} // tw

