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
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>

#include <string>

namespace tw {
namespace common_comm {
// Forward declaration
//
class TcpIpServer;

// TcpIpServerConnection class
//
class TcpIpServerConnection {
public:
    typedef boost::shared_ptr<TcpIpServerConnection> pointer;
    typedef int native_type;
    typedef int TSocket;
    typedef TcpIpServer TParent;
    
public:
    TcpIpServerConnection();
    ~TcpIpServerConnection();
    
public:
    native_type id() {
        return _socket;
    }
    
    bool isValid() const { 
        return (_socket != -1);
    }
    
public:
    // Server socket related methods
    //
    bool create();
    bool bind(const int port);
    bool listen() const;
    pointer accept(TParent* parent) const;
    
public:
    bool start(TParent* parent, TSocket& socket);
    void stop();
    void send(const std::string& message);
    
public:
    int32_t process(char* data, size_t size);

private:
    bool setTimeout(const uint32_t ms, TSocket& socket);
    bool setTcpNoDelay(TSocket& socket);
    bool onTimeout();
    void onConnectionError(const std::string& reason);
    
    bool doSend(const std::string& message);
    
    void threadMainSend();
    void threadMainRecv();
    
private:
    // NOTE: size of the messages are limited to 4K for now
    //
    typedef tw::common_comm::Buffer<TcpIpServerConnection, 4*1024> TBuffer;
    typedef tw::common_thread::ThreadPipe<std::string> TThreadPipe;
    
private:
    bool _connected;
    tw::common::THighResTime _timeoutTimer;
    
    TThreadPipe _sendQueue;
    tw::common_thread::ThreadPtr _threadSend;
    tw::common_thread::ThreadPtr _threadRecv;
    
    TParent* _parent;
    TBuffer _buffer;
    TSocket _socket;
    int _port;
    sockaddr_in _addr;
    
    std::string _message;
    
    uint16_t _msgRecvPerTimeSlice;
    uint16_t _timeSlicesWithoutMsgs;
};

typedef TcpIpServerConnection::native_type TConnectionId;

} // namespace common_comm
} // namespace tw

