#pragma once

#include <tw/common/settings.h>
#include <tw/common_thread/locks.h>
#include <tw/common_comm/buffer.h>
#include <tw/common_thread/thread.h>
#include <tw/log/defs.h>

#include <tw/common_comm/tcp_ip_server_connection.h>

#include <string>
#include <map>

namespace tw {
namespace common_comm {
    
// TcpIpServer callback interface
//
class TcpIpServerCallback {
public:
    typedef TcpIpServerConnection TConnection;
    
public:
    TcpIpServerCallback() {
    }
    
    virtual ~TcpIpServerCallback() {        
    }
    
public:
    virtual void onConnectionUp(TConnection::native_type id, TConnection::pointer connection) = 0;
    virtual void onConnectionDown(TConnection::native_type id) = 0;
    
    // NOTE: all messages are assumed to be '\n' (new line) terminated
    //
    virtual void onConnectionData(TConnection::native_type id, const std::string& message) = 0;
};


// TcpIpServer class, which accepts connections and send subscribed data to them
//
class TcpIpServer {
    typedef TcpIpServerCallback TClient;
    
    typedef TcpIpServerConnection TConnection;
    typedef TConnection::pointer TConnectionPtr;
    typedef std::map<TConnection::native_type, TConnectionPtr> TConnections;
    
    typedef std::pair<TConnection::native_type, std::string> TData;
    typedef tw::common_thread::ThreadPipe<TData> TThreadPipe;
    
public:
    TcpIpServer();
    ~TcpIpServer();
    
    void clear();
    
public:
    bool init(const tw::common::Settings& settings);    
    bool start(TClient* client, bool async);    
    bool stop();
    
    uint32_t getHeartbeatInterval() const {
        return _settings._common_comm_heartbeat_interval;
    }
    
    bool verbose() const {
        return _settings._common_comm_verbose;
    }
    
    bool tcpNoDelay() const {
        return _settings._common_comm_tcp_no_delay;
    }
    
public:
    void sendToConnection(TConnection::native_type id, const std::string& message);
    void sendToAll(const std::string& message);
    
public:    
    void onConnectionError(TConnection::native_type id, const std::string& reason);
    void onData(TConnection::native_type id, const std::string& message);
    
private:
    void doOnConnectionError(TConnection::native_type id, const std::string& reason);
    void removeConnection(TConnection::native_type id);
    
    void threadMainDisconnectedConnections();
    void run();
    
private:
    typedef tw::common_thread::Lock TLock;
    
    TLock _lock;
    bool _isDone;
    tw::common_thread::ThreadPtr _thread;
    TThreadPipe _queueDisconnectedConnections;
    tw::common_thread::ThreadPtr _threadDisconnectedConnections;
    
    TClient* _client;
    TConnections _connections;
    tw::common::Settings _settings;
    TcpIpServerConnection _server;
};

} // namespace common_comm
} // namespace tw
