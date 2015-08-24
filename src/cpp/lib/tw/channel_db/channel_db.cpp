#include <tw/channel_db/channel_db.h>
#include <tw/log/defs.h>

#include <boost/algorithm/string.hpp>

namespace tw {
namespace channel_db {
    
ChannelDb::ChannelDb() {
    clear();
}

ChannelDb::~ChannelDb() {
    clear();
}

void ChannelDb::clear() {
    _driver = NULL;
}

bool ChannelDb::init(const std::string& connection) {
    bool status = true;
    try {
        _driver = sql::mysql::get_driver_instance();
        _connection = connection;
        
        // Test getting connection
        //
        TConnectionPtr connection = getConnection();
        if ( !connection ) {
            status = false;        
        } else {
            connection->close();
            connection.reset();
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }

    if ( !status )
        clear();

    return status;
}

bool ChannelDb::isValid() {
    return ( _driver != NULL );
}

ChannelDb::TConnectionPtr ChannelDb::getConnection() {
    const char* DELIM = ";";
    TConnectionPtr connection;
    try {
        if ( isValid() ) {
            std::vector<std::string> dbParams;
            boost::split(dbParams, _connection, boost::is_any_of(DELIM));
            switch ( dbParams.size() ) {
                case 3:
                    connection.reset(_driver->connect(dbParams[0], dbParams[1], ""));
                    connection->setSchema(dbParams[2]);
                    break;
                case 4:
                    connection.reset(_driver->connect(dbParams[0], dbParams[1], dbParams[2]));
                    connection->setSchema(dbParams[3]);
                    break;
                default:
                    LOGGER_ERRO << "Incorrect configuration of connection: " << _connection << "\n";
                    break;
            }
        }
    } catch(const std::exception& e) {
        connection.reset();
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        connection.reset();
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";        
    }
    
    return connection;
}

ChannelDb::TStatementPtr ChannelDb::getStatement(TConnectionPtr& connection) {
    TStatementPtr statement;
    try {
        if ( connection.get() != NULL )
            statement.reset(connection->createStatement());
    } catch(const std::exception& e) {
        statement.reset();
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        statement.reset();
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";        
    }
    
    return statement;
}

ChannelDb::TResultSetPtr ChannelDb::executeQuery(TStatementPtr& statement, const std::string& sql) {
    TResultSetPtr resultSet;
    try {
        if ( statement.get() != NULL )
            resultSet.reset(statement->executeQuery(sql));
    } catch(const std::exception& e) {
        resultSet.reset();
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        resultSet.reset();
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";        
    }
    
    return resultSet;
}
    
} // namespace channel_db
} // namespace tw
