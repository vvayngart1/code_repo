#include <tw/common_comm/tcp_ip_client_connection.h>
#include <tw/common/signal_catcher.h>
#include <tw/config/settings_cmnd_line.h>
#include <tw/config/settings_config_file.h>

class Client : public tw::common_comm::TcpIpClientCallback {
public:
    Client() {        
    }
    
    virtual ~Client() {
    }
    
public:
    virtual void onConnectionUp(TConnection::native_type id, TConnection::pointer connection) {
        LOGGER_INFO << id << "\n";
    }
    
    virtual void onConnectionError(TConnection::native_type id, const std::string& message) {
        LOGGER_ERRO << id << " :: " << message << "\n";
    }    
    
    virtual void onConnectionData(TConnection::native_type id, const std::string& message) {
        LOGGER_INFO << id << " :: " << message << "\n";
    }
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
    
    Client client;
    tw::common_comm::TcpIpClientConnection::pointer connection = tw::common_comm::TcpIpClientConnection::create(settings._channel_pf_historical_host, settings._channel_pf_historical_port, &client, false);
    if ( !connection )
        return -1;
    
    if ( !connection->start() )
        return -1;    
    
    tw::common::SignalCatcher signalCatcher;
    signalCatcher.run();
    
    connection->stop();
    tw::log::Logger::instance().stop();    
    
    return 0;
}
