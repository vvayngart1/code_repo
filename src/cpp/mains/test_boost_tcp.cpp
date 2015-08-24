#include <tw/common_thread/utils.h>

#include <boost/asio.hpp>

#include <iostream>

using boost::asio::ip::tcp;

int main(int argc, char * argv[]) {
    try
    {
        if (argc < 3) {
            std::cout << "Usage: " << argv[0] << " <host> <port> [sleep between reads in ms]\n";
            return 1;
        }

        boost::asio::io_service io_service;

        tcp::resolver resolver(io_service);
        tcp::resolver::query query(tcp::v4(), argv[1], argv[2]);
        tcp::resolver::iterator iterator = resolver.resolve(query);

        tcp::socket s(io_service);
        s.connect(*iterator);

        while ( true ) {
            char msg[1024];
            size_t length = boost::asio::read(s, boost::asio::buffer(msg, sizeof(msg)));
            
            std::cout << "\n ---------- Recv: " << length << " ----------- \n";
            std::cout.write(msg, length);
            
            if ( argc > 3 )
                tw::common_thread::sleep(::atol(argv[3]));
        }
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    } catch (...) {
        std::cerr << "Exception: " << "\n";
    }
    
    return 0;
}

