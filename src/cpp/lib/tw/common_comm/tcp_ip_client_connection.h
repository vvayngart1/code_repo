#pragma once

#include <tw/common_comm/buffer.h>
#include <tw/common_thread/thread.h>
#include <tw/log/defs.h>

#include <boost/shared_ptr.hpp>

// Raw scokets related header files
//
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>

#include <string>

namespace tw {
namespace common_comm {
    
// Forward declaration
//
class TcpIpClientCallback;
    
// TcpIpClientConnection class
//
class TcpIpClientConnection {
public:
    typedef boost::shared_ptr<TcpIpClientConnection> pointer;
    typedef int native_type;
    typedef int TSocket;
    typedef TcpIpClientCallback TParent;
    
public:
    TcpIpClientConnection();
    ~TcpIpClientConnection();
    
public:
    native_type id() {
        return _socket;
    }
    
    bool isValid() const { 
        return (_socket != -1);
    }
    
    bool isConnected() const { 
        return _connected;
    }
    
    const std::string& getHost() const {
        return _host;
    } 
    
    int getPort() const {
        return _port;
    } 
    
public:
    // Client socket related methods
    //
    static pointer create(const std::string& host, int port, TParent* parent, bool rawProcessing);
    
public:
    bool start();
    void stop();
    void send(const std::string& message);
    
    bool connect();
    
public:
    int32_t process(char* data, size_t size);

private:
    void init(const std::string& host, int port, TParent* parent, bool rawProcessing);
    bool setTimeout(const uint32_t ms, TSocket& socket);
    void onConnectionError(const std::string& reason);
    
    bool doSend(const std::string& message);
    
    void threadMainSend();
    void threadMainRecv();
    
private:
    // NOTE: size of the messages are limited to 4K for now
    //
    typedef tw::common_comm::Buffer<TcpIpClientConnection, 4*1024> TBuffer;
    typedef tw::common_thread::ThreadPipe<std::string> TThreadPipe;
    
private:
    bool _connected;
    
    TThreadPipe _sendQueue;
    tw::common_thread::ThreadPtr _threadSend;
    tw::common_thread::ThreadPtr _threadRecv;
    
    TParent* _parent;
    TBuffer _buffer;
    TSocket _socket;
    std::string _host;
    int _port;
    sockaddr_in _addr;
    
    std::string _message;
    bool _rawProcessing;
};

// TcpIpClient callback interface
//
class TcpIpClientCallback {
public:
    typedef TcpIpClientConnection TConnection;
    
public:
    TcpIpClientCallback() {
    }
    
    virtual ~TcpIpClientCallback() {        
    }
    
public:
    virtual void onConnectionUp(TConnection::native_type id, TConnection::pointer connection) = 0;
    virtual void onConnectionError(TConnection::native_type id, const std::string& message) = 0;
    
    // NOTE: all messages are assumed to be '\n' (new line) terminated
    //
    virtual void onConnectionData(TConnection::native_type id, const std::string& message) = 0;
    
    // Raw view of the message if client want to parse message itself and/or
    // message is not '\n' terminated
    //
    virtual int32_t process(TConnection::native_type id, char* data, size_t size) {
        return size;
    }
    
    virtual bool verbose() const {
        return false;
    }
};

} // namespace common_comm
} // namespace tw

