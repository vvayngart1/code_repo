#pragma once

#include <tw/common/settings.h>

#include <boost/shared_ptr.hpp>

#include <mysql_driver.h>
#include <mysql_connection.h>

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

namespace tw {
namespace channel_db {
    
class ChannelDb {
public:
    typedef sql::Driver TDriver;
    typedef boost::shared_ptr<sql::Connection> TConnectionPtr;
    typedef boost::shared_ptr<sql::Statement> TStatementPtr;
    typedef boost::shared_ptr<sql::ResultSet> TResultSetPtr;
    
public:
    ChannelDb();
    ~ChannelDb();
    
    void clear();
    
public:
    bool init(const std::string& connection);
    bool isValid();
    
    TConnectionPtr getConnection();
    TStatementPtr getStatement(TConnectionPtr& connection);
    TResultSetPtr executeQuery(TStatementPtr& statement, const std::string& sql);
    
private:
    TDriver* _driver;
    std::string _connection;
};

    
} // namespace channel_db
} // namespace tw
