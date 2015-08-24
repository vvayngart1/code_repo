#include <tw/common_comm/tcp_ip_client_connection.h>
#include <tw/common_thread/utils.h>

#include <errno.h>

namespace tw {
namespace common_comm {

const int TIMEOUT_RESOLUTION = 10;  // 10 ms

static char DELIM = '\n';
static char PRE_DELIM = '\r';

static uint32_t MAX_MSG_QUEUE_SIZE = 1000;
static uint32_t MSG_QUEUE_HIGH_WATER_MARK = 800;

// TcpIpServerConnection implementation
//
TcpIpClientConnection::TcpIpClientConnection() : _connected(false),
                                                 _sendQueue(),
                                                 _threadSend(),
                                                 _threadRecv(),
                                                 _parent(NULL),
                                                 _buffer(),
                                                 _socket(-1),
                                                 _host(),
                                                 _port(-1),
                                                 _message(),
                                                 _rawProcessing(false) {
    ::memset(&_addr, 0, sizeof(_addr));
}  

TcpIpClientConnection::~TcpIpClientConnection() {
    stop();
    LOGGER_INFO << " Connection destroyed"  << "\n";
}


TcpIpClientConnection::pointer TcpIpClientConnection::create(const std::string& host, int port, TParent* parent, bool rawProcessing ) {
    pointer connection;
    
    try {        
        connection.reset(new TcpIpClientConnection());
        connection->init(host, port, parent, rawProcessing);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    return connection;
}

void TcpIpClientConnection::init(const std::string& host, int port, TParent* parent, bool rawProcessing) {
    _host = host;
    _port = port;
    _parent = parent;
    _rawProcessing = rawProcessing;
}

bool TcpIpClientConnection::start() {
    try {
        if ( !_parent )
            return false;                
        
        if ( !connect() )
            return false;

        // Start send/recv threads
        //
        _threadSend = tw::common_thread::ThreadPtr(new tw::common_thread::Thread(boost::bind(&TcpIpClientConnection::threadMainSend, this)));
        _threadRecv = tw::common_thread::ThreadPtr(new tw::common_thread::Thread(boost::bind(&TcpIpClientConnection::threadMainRecv, this)));
        
        return true;
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    return false;
}

void TcpIpClientConnection::stop() {
    try {
        TSocket socket = _socket;        
        bool validSocket = isValid();
        
        if ( validSocket ) {
            LOGGER_INFO << socket << " :: Closing connection..."  << "\n";
        
            ::shutdown(_socket, SHUT_RDWR);
            ::close(_socket);
            _socket = -1;
        }
        
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
        
        if ( validSocket )
            LOGGER_INFO << socket << " :: Connection closed"  << "\n";
    
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

bool TcpIpClientConnection::connect() {
    try {
        _buffer.clear();
        _buffer.setParser(this);
        _connected = false;
        
        if ( isValid() )
            ::close(_socket);
        
        _socket = ::socket(AF_INET, SOCK_STREAM, 0);

        if ( !isValid() ) {
            LOGGER_ERRO << "Failed to create socket" << "\n" << "\n";
            return false;
        }
        
        _addr.sin_family = AF_INET;
        _addr.sin_addr.s_addr = ::inet_addr(_host.c_str());
        _addr.sin_port = htons(_port);
        
        LOGGER_INFO << "Connecting to: " << _host << " :: " << _port << "\n" << "\n";
        
        if ( ::connect(_socket, reinterpret_cast<sockaddr*>(&_addr), sizeof(_addr)) != 0 ) {
            LOGGER_ERRO << "Failed to connect to: " << _host << " :: " << _port << "\n" << "\n";
            return false;
        }        
        
        if ( !setTimeout(TIMEOUT_RESOLUTION, _socket) ) {
            LOGGER_ERRO << "Failed to setTimeout() for socket: " << _host << " :: " << _port << "\n" << "\n";
            return false;
        }
        
        _connected = true;
        LOGGER_INFO << "Connected to: " << _host << " :: " << _port << "\n" << "\n";
        
        return true;
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    return false;
}

bool TcpIpClientConnection::setTimeout(const uint32_t ms, TSocket& socket) {
    try {
        struct timeval tv;
        tv.tv_sec = (ms - (ms%1000))/1000;
        tv.tv_usec = (ms - tv.tv_sec*1000)*1000;
        
        if ( -1 == setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,  sizeof(tv)) ) {
            LOGGER_ERRO << "Failed to setsockopt SO_RCVTIMEO" << "\n" << "\n";
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
    
void TcpIpClientConnection::send(const std::string& message) {
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
    
int32_t TcpIpClientConnection::process(char* data, size_t size) {
    // diagnostic logging for 2015-06-01; delete later
    if ( data && size > 0 ) {
        std::string msg;
        msg.assign(data, size);
        
        if ( _parent && _parent->verbose() )
            LOGGER_INFO << " raw TCP/IP message=" << msg << "\n";
    }
    
    if ( _rawProcessing ) {
        if ( _parent )
            return _parent->process(id(), data, size);
        
        return size;
    }
    
    char* end = data+size;
    char* iter = std::find(data, end, DELIM);
    if ( iter == end )
        return 0;

    if ( iter - 1 > data && *(iter-1) == PRE_DELIM )
        _message.assign(data, iter-1);
    else
        _message.assign(data, iter);    
    
    if ( _parent )
        _parent->onConnectionData(id(), _message);

    return (iter-data+1);
}

void TcpIpClientConnection::onConnectionError(const std::string& reason) {
    // Prevent duplicate notifications
    //
    if ( !_connected ) {
        LOGGER_INFO << "Not connected" << "\n";
        return;
    }
    
    _parent->onConnectionError(id(), reason);
    _connected = false;
}

bool TcpIpClientConnection::doSend(const std::string& message) {
    try {
       if ( message.empty() ) 
           return true;
       
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

void TcpIpClientConnection::threadMainSend() {
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

void TcpIpClientConnection::threadMainRecv() {
    TSocket socket = _socket;
    LOGGER_INFO << socket << " :: Started"  << "\n";
    try {
        bool isDone = false;
        while ( !isDone && isValid() ) {
            ssize_t bytes = ::recv(_socket, _buffer.getFreeBuffer(), _buffer.getFreeSize(), 0);
            
            if ( _parent && _parent->verbose() && (bytes > -1) )
                LOGGER_INFO << "TCP bytes=" << bytes << ", buffer.getFreeSize()=" << _buffer.getFreeSize() << "\n";
            
            if ( bytes > 0 ) {
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

