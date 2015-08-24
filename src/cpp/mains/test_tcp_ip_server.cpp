#include <tw/common_comm/tcp_ip_server.h>
#include <tw/common/signal_catcher.h>
#include <tw/config/settings_cmnd_line.h>
#include <tw/config/settings_config_file.h>

class Client : public tw::common_comm::TcpIpServerCallback {
public:
    Client(tw::common_comm::TcpIpServer& server) : _server(server) {
        
    }
    
    virtual ~Client() {
        
    }
    
public:
    virtual void onConnectionUp(TConnection::native_type id, TConnection::pointer connection) {
        LOGGER_INFO << id << "\n";
    }
    
    virtual void onConnectionDown(TConnection::native_type id) {
        LOGGER_INFO << id << "\n";
    }    
    
    virtual void onConnectionData(TConnection::native_type id, const std::string& message) {
        LOGGER_INFO << id << " :: " << message << "\n";
        _server.sendToConnection(id, std::string("Echo back: ") + message + "\n");
        
    }
    
private:
    tw::common_comm::TcpIpServer& _server;
};

int main(int argc, char * argv[]) {
    tw::config::SettingsCmndLineCommonUse cmndLine;
    if ( !cmndLine.parse(argc, argv) ) {
        LOGGER_ERRO << "failed to parse command line arguments" << cmndLine.toStringDescription() << "\n";
        return -1;
    }

    if ( cmndLine.help() ) {
        LOGGER_INFO << "<USAGE>: " << cmndLine.toStringDescription() << "\n";
        return 0;
    }
    
    tw::common::Settings settings;
    if ( !settings.parse(cmndLine.configFile()) ) {
        LOGGER_ERRO << "failed to parse config: " << cmndLine.configFile() << " :: " << settings.toStringDescription() << "\n";
        return -1;
    }
    
    if ( !tw::log::Logger::instance().start(settings) ) {
        LOGGER_ERRO << "failed to start logger: " << settings.toStringDescription() << "\n";
        return -1;
    }
    
    tw::common_comm::TcpIpServer server;
    Client client(server);
    if ( !server.init(settings) )
        return -1;
    
    if ( !server.start(&client, true) )
        return -1;
    
    tw::common::SignalCatcher signalCatcher;
    signalCatcher.run();
    
    server.stop();
    tw::log::Logger::instance().stop();    
    
    return 0;
}
