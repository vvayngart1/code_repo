#include <tw/log/defs.h>
#include <tw/channel_db/channel_db.h>

#include <mysql_driver.h>
#include <mysql_connection.h>

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

void print(sql::ResultSet* res) {
    int counter = 0;
    while (res->next())
    {
        try
        {
            sql::SQLString read_ticker = res->getString("name");
            sql::SQLString read_timestamp = res->getString("timestamp");
            std::cout << read_ticker.asStdString() << ",";
            std::cout << read_timestamp.asStdString() << "." << "\n";
            std::cout << "\n";
        }
        catch (sql::SQLException &e)
        {
            std::cout << "#ERR: " << e.what() << "\n";
            std::cout << " MySQL error code: " << e.getErrorCode() << "\n";
            std::cout << " SQLState: " << e.getSQLState() << " )" << "\n";
        }

        counter++;
    }

    std::cout << "number of lines parsed = " << counter << "\n";
}

int main(int argc, char * argv[]) {
    try {
        LOGGER_INFO << "\n" << "Starting..." << "\n";

        std::string sql = "select * from products";
        std::string database = "tw_test";

        sql::mysql::MySQL_Driver driver;
        sql::Connection* conn = driver.connect("tcp://172.20.10.158:3306", "root", "");
        conn->setSchema(database);

        if ( conn )
            LOGGER_INFO << "\n" << "Connected!" << "\n";
        else
            LOGGER_ERRO << "\n" << "Failed to connect" << "\n";

        sql::Statement* stmt = conn->createStatement();
        sql::ResultSet* res = stmt->executeQuery(sql.c_str());
        print(res);

        {
            // Test wrapper around mysql connector C++
            //
            std::string connectionString;
            connectionString = "tcp://172.20.10.158:3306;root;tw_test";

            tw::channel_db::ChannelDb channelDb;
            if ( !channelDb.init(connectionString) )
                return -1;

            tw::channel_db::ChannelDb::TConnectionPtr connection = channelDb.getConnection();
            tw::channel_db::ChannelDb::TStatementPtr statement = channelDb.getStatement(connection);
            
            tw::channel_db::ChannelDb::TResultSetPtr res = channelDb.executeQuery(statement, sql);
            print(res.get());
        }
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";        
    }
    
    return 0;
}

