#include <tw/common_comm/tcp_ip_server_connection.h>
#include <tw/common_comm/tcp_ip_server.h>
#include <tw/common_thread/utils.h>

#include <errno.h>

namespace tw {
namespace common_comm {
    
const int MAX_CONNECTIONS = 64;
const int TIMEOUT_RESOLUTION = 10;  // 10 ms

static char DELIM = '\n';
static char PRE_DELIM = '\r';

static uint32_t MAX_MSG_QUEUE_SIZE = 1000;
static uint32_t MSG_QUEUE_HIGH_WATER_MARK = 800;
static uint32_t MAX_NUMBER_OF_TIMEOUTS = 5000000;

static std::string HEARTBEAT = "H";
static std::string HEARTBEAT_W_DELIM = HEARTBEAT + DELIM;

static uint32_t ACCEPT_SOCKET_TIMEOUT_MS = 2000; // 2 secs

// TcpIpServerConnection implementation
//
TcpIpServerConnection::TcpIpServerConnection() : _connected(false),
                                                 _sendQueue(),
                                                 _threadSend(),
                                                 _threadRecv(),
                                                 _parent(NULL),
                                                 _buffer(),
                                                 _socket(-1),
                                                 _port(-1),
                                                 _message(),
                                                 _msgRecvPerTimeSlice(0),
                                                 _timeSlicesWithoutMsgs(0) {
    ::memset(&_addr, 0, sizeof(_addr));
    _timeoutTimer.setToNow();
}  

TcpIpServerConnection::~TcpIpServerConnection() {
    stop();
    LOGGER_INFO << " Connection destroyed"  << "\n";
}

bool TcpIpServerConnection::create() {
    try {
        _socket = ::socket(AF_INET, SOCK_STREAM, 0);

        if ( !isValid() ) {
            LOGGER_ERRO << "Failed to create socket" << "\n" << "\n";
            return false;
        }
        
        int on = 1;
        if ( -1 == ::setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, (const char*) &on, sizeof(on)) ) {
            LOGGER_ERRO << "Failed to setsockopt SO_REUSEADDR" << "\n" << "\n";
            return false;
        }
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }    
    
    return true;
}

bool TcpIpServerConnection::bind(const int port) {
    try {
        if ( !isValid() ) 
            return false;
        
        _addr.sin_family = AF_INET;
        _addr.sin_addr.s_addr = INADDR_ANY;
        _addr.sin_port = htons(port);

        if ( -1 == ::bind(_socket, (struct sockaddr*) &_addr, sizeof(_addr)) ) {
            LOGGER_ERRO << "Failed to bind to port (" << port << "): " << ::strerror(errno) << "\n" << "\n";
            return false;
        }
        
        if ( !setTimeout(ACCEPT_SOCKET_TIMEOUT_MS, _socket) )
            return false;
        
        _port = port;
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }    
    
    return true;
}

bool TcpIpServerConnection::listen() const {
    try {
        if ( !isValid() ) 
            return false;
        
        if ( -1 == ::listen(_socket, MAX_CONNECTIONS) ) {
            LOGGER_ERRO << "Failed to listen on port: " << _port << "\n" << "\n";
            return false;
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }    
    
    return true;
}

TcpIpServerConnection::pointer TcpIpServerConnection::accept(TParent* parent) const {
    pointer connection;
    
    try {
        int addr_length = sizeof(_addr);
        TSocket socket = ::accept(_socket, (sockaddr*)&_addr, (socklen_t*) &addr_length);

        if ( socket <= 0 ) {
            if ( EAGAIN != errno && EINTR != errno ) {
                LOGGER_ERRO << "Failed to accept on port: " << _port << " - error: " << errno << " :: " << ::strerror(errno) << "\n" << "\n";
                // Serious error occurred - sleep for 1 sec so we don't potentially 'flood' the disk with error messages
                //
                tw::common_thread::sleep(1000);
            }
            return connection;
        }
        
        connection.reset(new TcpIpServerConnection());
        if ( !connection->start(parent, socket) )
            connection.reset();
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    return connection;
}

bool TcpIpServerConnection::start(TParent* parent, TSocket& socket) {
    try {
        if ( !parent )
            return false;
        
        if ( !setTimeout(TIMEOUT_RESOLUTION, socket) )
            return false;
        
        if ( parent->tcpNoDelay() && !setTcpNoDelay(socket) )
            return false;
        
        _parent = parent;
        _socket = socket;
        _buffer.setParser(this);
        _connected = true;

        // Start send/recv threads
        //
        _threadSend = tw::common_thread::ThreadPtr(new tw::common_thread::Thread(boost::bind(&TcpIpServerConnection::threadMainSend, this)));
        _threadRecv = tw::common_thread::ThreadPtr(new tw::common_thread::Thread(boost::bind(&TcpIpServerConnection::threadMainRecv, this)));
        
        LOGGER_INFO << _socket << " :: Connection open"  << "\n";
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    
    return true;
}

void TcpIpServerConnection::stop() {
    try {
        if ( !isValid() )
            return;
        
        TSocket socket = _socket;
        LOGGER_INFO << socket << " :: Closing connection..."  << "\n";
        
        ::shutdown(_socket, SHUT_RDWR);
        ::close(_socket);
        _socket = -1;
        
        if ( _threadRecv != NULL ) {
            LOGGER_INFO << socket << " :: Stopping recv thread..."  << "\n";
            
            _threadRecv->join();
            _threadRecv.reset();
            
            LOGGER_INFO << socket << " :: Stopped recv thread"  << "\n";
        }
        
        if ( _threadSend != NULL ) {
            LOGGER_INFO << socket << " :: Stopping send queue..."  << "\n";
            
            _sendQueue.stop();
            
            LOGGER_INFO << socket << " :: Stopped send queue"  << "\n";
            
            LOGGER_INFO << socket << " :: Stopping send thread..."  << "\n";
            
            _threadSend->join();
            _threadSend.reset();
            
            LOGGER_INFO << socket << " :: Stopped send thread"  << "\n";
            
            _sendQueue.clear();
        }        
        
        LOGGER_INFO << socket << " :: Connection closed"  << "\n";
    
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

bool TcpIpServerConnection::setTimeout(const uint32_t ms, TSocket& socket) {
    try {
        struct timeval tv;
        tv.tv_sec = (ms - (ms%1000))/1000;
        tv.tv_usec = (ms - tv.tv_sec*1000)*1000;
        
        if ( -1 == setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,  sizeof(tv)) ) {
            LOGGER_ERRO << "Failed to setsockopt SO_RCVTIMEO" << "\n" << "\n";
            return false;
        }
        
        LOGGER_INFO << "Succeeded to setsockopt SO_RCVTIMEO - timeout_ms=" << ms << "\n";
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    
    return true;
}

bool TcpIpServerConnection::setTcpNoDelay(TSocket& socket) {
    try {
        int on = 1;
        if ( 0 > setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(int)) ) {
            LOGGER_ERRO << "Failed to setsockopt TCP_NODELAY" << "\n" << "\n";
            return false;
        }
        
        LOGGER_INFO << "Succeeded to setsockopt TCP_NODELAY" << "\n";
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    
    return true;
}
    
void TcpIpServerConnection::send(const std::string& message) {
    try {
        if ( !isValid() )
            return;
        
        if ( _sendQueue.size() > MAX_MSG_QUEUE_SIZE ) {
            LOGGER_ERRO << "Messages queue overflow: "  << _sendQueue.size() << " :: " << MAX_MSG_QUEUE_SIZE << "\n" << "\n";            
            onConnectionError("MAX_MSG_QUEUE_SIZE");
            return;
        }
        
        _sendQueue.push(message);
        if ( MSG_QUEUE_HIGH_WATER_MARK == _sendQueue.size() )
            LOGGER_WARN << "Reached MSG_QUEUE_HIGH_WATER_MARK: "  << MSG_QUEUE_HIGH_WATER_MARK << "\n" << "\n";
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}
    
int32_t TcpIpServerConnection::process(char* data, size_t size) {
    char* end = data+size;
    char* iter = std::find(data, end, DELIM);
    if ( iter == end )
        return 0;

    if ( iter - 1 > data && *(iter-1) == PRE_DELIM )
        _message.assign(data, iter-1);
    else
        _message.assign(data, iter);    
    
    if ( _parent && !(_message == HEARTBEAT) ) {  
        _parent->onData(id(), _message);
    }

    return (iter-data+1);
}

bool TcpIpServerConnection::onTimeout() {
    tw::common::THighResTime now = tw::common::THighResTime::now();
    
    // NOTE: assumption is made that if 2*delta are smaller then
    // heartbeat interval, socket is closed
    //
    uint64_t delta = now-_timeoutTimer;
    if ( delta*2 < _parent->getHeartbeatInterval()*1000) {
        return false;
    }
    
    if ( _msgRecvPerTimeSlice > 0 ) {
        _timeSlicesWithoutMsgs = 0;
        _msgRecvPerTimeSlice = 0;
    } else {
        ++_timeSlicesWithoutMsgs;
    }
    
    send(HEARTBEAT_W_DELIM);
    _timeoutTimer = now;
    
    return true;
}

void TcpIpServerConnection::onConnectionError(const std::string& reason) {
    // Prevent duplicate notifications
    //
    if ( !_connected ) {
        LOGGER_INFO << "Not connected" << "\n";
        return;
    }
    
    _parent->onConnectionError(id(), reason);
    _connected = false;
}

bool TcpIpServerConnection::doSend(const std::string& message) {
    try {
       if ( message.empty() ) 
           return true;
       
       if ( !isValid() )
           return false;
       
       if ( -1 == ::send(_socket, message.c_str(), message.size(), MSG_NOSIGNAL) )
           return false;
       
       if ( _parent && _parent->verbose() )
           LOGGER_INFO << "\n\t" << "@ tcp/ip ==> " << message << "\n";
       
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    
    return true;
}

void TcpIpServerConnection::threadMainSend() {
    TSocket socket = _socket;
    LOGGER_INFO << socket << " :: Started"  << "\n";
    try {
        bool status = true;
        while( status && !_sendQueue.isStopped() ) {
            std::string item;
            _sendQueue.read(item);
            status = doSend(item);
            
            while ( status && !_sendQueue.isStopped() && _sendQueue.try_read(item) ) {
                status = doSend(item);
            }
        }
        
        if ( !status )
            onConnectionError("Send error");
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    LOGGER_INFO << socket << " :: Exited"  << "\n";
}

void TcpIpServerConnection::threadMainRecv() {
    TSocket socket = _socket;
    LOGGER_INFO << socket << " :: Started"  << "\n";
    try {
        bool isDone = false;
        tw::common::THighResTime timer = tw::common::THighResTime::now();
        int64_t delta = 0;
        while ( !isDone && isValid() ) {
            ssize_t bytes = ::recv(_socket, _buffer.getFreeBuffer(), _buffer.getFreeSize(), 0);
            if ( bytes > 0 ) {
                ++_msgRecvPerTimeSlice;
                _buffer.process(bytes);
            } else {
                switch ( errno ) {
                    case EAGAIN:
                    case EINTR:
                        // Ignore - not a critical error
                        //
                        break;
                    default:
                        LOGGER_ERRO << socket << " :: failed to receive - error: " << errno << " :: " << ::strerror(errno) << "\n";
                        onConnectionError("Failed to receive");
                        isDone = true;
                        break;
                }
            }
            
            // NOTE: resolution is made to be 10 ms
            //
            delta = (tw::common::THighResTime::now() - timer)/1000 + TIMEOUT_RESOLUTION;
            if ( delta >= _parent->getHeartbeatInterval() ) {
                if ( onTimeout() ) {
                    if ( _timeSlicesWithoutMsgs > MAX_NUMBER_OF_TIMEOUTS ) {
                        LOGGER_ERRO << _socket << " :: Didn't receive messages for: " << _timeSlicesWithoutMsgs << " > " << MAX_NUMBER_OF_TIMEOUTS << " timeouts" << "\n";                        
                        onConnectionError("onTimeout(): idle connection");
                        isDone = true;
                    }
                } else {
                    LOGGER_ERRO << _socket << " :: onTimeout() failed - socket disconnected" << "\n";                        
                    onConnectionError("onTimeout() failed - socket disconnected");
                    isDone = true;
                }
                
                timer.setToNow();
            }
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    LOGGER_INFO << socket << " :: Exited"  << "\n";
}

} // namespace common_comm
} // namespace tw

