#include <tw/common_comm/tcp_ip_server.h>

namespace tw {
namespace common_comm {
    
// TcpIpServer implementation
//
TcpIpServer::TcpIpServer() : _lock(),
                             _isDone(false),
                             _thread(),
                             _queueDisconnectedConnections(),
                             _threadDisconnectedConnections(),
                             _client(NULL),
                             _connections(),
                             _server() {
}

TcpIpServer::~TcpIpServer() {
    stop();
}

void TcpIpServer::clear() {
    // Lock for thread synchronization
    //
    tw::common_thread::LockGuard<TLock> lock(_lock);    
    
    _client = NULL;
    _connections.clear();
}
    
bool TcpIpServer::init(const tw::common::Settings& settings) {
    bool status = true;
    try {
        if ( !_server.create() )
            return false;
        
        if ( !_server.bind(settings._common_comm_tcp_ip_server_port) )
            return false;
        
        if ( !_server.listen() )
            return false;
        
        _settings = settings;
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }

    return status;
}

bool TcpIpServer::start(TClient* client, bool async) {
    bool status = true;
    try {
        if ( !client ) {
            LOGGER_ERRO << "Client is NULL" << "\n" << "\n";        
            return false;
        }
        
        _client = client;
        if ( !async )
            run();
        else
           _thread = tw::common_thread::ThreadPtr(new tw::common_thread::Thread(boost::bind(&TcpIpServer::run, this))); 
            
        _threadDisconnectedConnections = tw::common_thread::ThreadPtr(new tw::common_thread::Thread(boost::bind(&TcpIpServer::threadMainDisconnectedConnections, this)));
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }

    return status;
}

bool TcpIpServer::stop() {
    try {
        if ( _isDone )
            return true;
        
        LOGGER_INFO << "Stopping..." << "\n";
        
        _isDone = true;        
        _server.stop();
        
        if ( _thread != NULL ) {
            LOGGER_INFO << "Stopping 'accept' thread..." << "\n";
            
            _thread->join();
            _thread.reset();
            
            LOGGER_INFO << "Stopped 'accept' thread" << "\n";
        }
        
        if ( _threadDisconnectedConnections != NULL ) {
            LOGGER_INFO << "Stopping 'disconnected connections' thread..." << "\n";
            
            _queueDisconnectedConnections.stop();
            
            _threadDisconnectedConnections->join();
            _threadDisconnectedConnections.reset();
            
            _queueDisconnectedConnections.clear();
            
            LOGGER_INFO << "Stopped 'disconnected connections' thread" << "\n";
        }
        
        clear();
        
        LOGGER_INFO << "Stopped" << "\n";
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    
    return true;
}

void TcpIpServer::onData(TConnection::native_type id, const std::string& message) {
    try {
        if ( _client )
            _client->onConnectionData(id, message);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void TcpIpServer::sendToConnection(TConnection::native_type id, const std::string& message) {
    try {
        TConnectionPtr connection;
        {
            // Lock for thread synchronization
            //
            tw::common_thread::LockGuard<TLock> lock(_lock);

            TConnections::iterator iter = _connections.find(id);
            if ( iter == _connections.end() ) {
                LOGGER_ERRO << "Can't find connection: " << id << "\n";
                return;
            }
            
            connection = iter->second;
        }
        
        if ( connection )
            connection->send(message);
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void TcpIpServer::sendToAll(const std::string& message) {
    try {
        // NOTE: copy of connections is made in case sendMessage() triggers
        // onConnectionError() call in the same call stack, which would
        // invalidate iterators
        //
        TConnections connections;
        {
            // Lock for thread synchronization
            //
            tw::common_thread::LockGuard<TLock> lock(_lock);
            connections = _connections;
        }
        
        TConnections::iterator iter = connections.begin();
        TConnections::iterator end = connections.end();
        
        for ( ; iter != end; ++iter ) {
            iter->second->send(message);
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void TcpIpServer::onConnectionError(TConnection::native_type id, const std::string& reason) {
    try {
        LOGGER_WARN << "Notifying client of down connection: " << id << " :: " << reason << "\n";
        
        TData item;
        item.first = id;
        item.second = reason;
        _queueDisconnectedConnections.push(item);        
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void TcpIpServer::doOnConnectionError(TConnection::native_type id, const std::string& reason) {
    try {
        if ( id < 0 )
            return;
        
        if ( _client )
            _client->onConnectionDown(id);
        
        removeConnection(id);
        
        LOGGER_WARN << "Notified client of down connection: " << id << " :: " << reason << "\n";
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void TcpIpServer::removeConnection(TConnection::native_type id) {
    try {
        if ( id < 0 )
            return;
        
        TConnectionPtr connection;
        {
            // Lock for thread synchronization
            //
            tw::common_thread::LockGuard<TLock> lock(_lock);

            TConnections::iterator iter = _connections.find(id);
            if ( iter == _connections.end() ) {
                LOGGER_ERRO << "Can't find connection: " << id << "\n";
                return;
            }

            connection = iter->second;
            _connections.erase(iter);
        }
        
        if ( connection )
            connection->stop();
        
        LOGGER_WARN << "Removed connection: " << id << "\n";
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void TcpIpServer::run() {
    try {
        LOGGER_INFO << "Started"  << "\n";
        
        while ( !_isDone ) {
            TConnection::pointer connection = _server.accept(this);
            if ( !_isDone ) {
                if ( connection ) {
                    {
                        // Lock for thread synchronization
                        //
                        tw::common_thread::LockGuard<TLock> lock(_lock);

                        _connections[connection->id()] = connection;
                        LOGGER_INFO << "Connected socket: " << connection->id() << "\n";
                    }

                    if ( _client )
                        _client->onConnectionUp(connection->id(), connection);
                }
            }
        }
    
        LOGGER_INFO << "Exited"  << "\n";
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void TcpIpServer::threadMainDisconnectedConnections() {
    LOGGER_INFO << "Started" << "\n";
    
    try {
        TData item;
        while( !_queueDisconnectedConnections.isStopped() ) {
            item.first = -1;
            item.second.clear();
            
            _queueDisconnectedConnections.read(item);
            doOnConnectionError(item.first, item.second);
            
            while ( _queueDisconnectedConnections.try_read(item) ) {
                doOnConnectionError(item.first, item.second);
            }
        }        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    LOGGER_INFO << "Exited" << "\n";
}

} // namespace common_comm
} // namespace tw
