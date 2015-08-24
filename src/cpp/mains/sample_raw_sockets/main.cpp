#include <tw/common_thread/utils.h>

#include "Socket.h"
#include "ServerSocket.h"
#include "SocketException.h"
#include <string>

#include <iostream>

int main (int argc, char* argv[])
{
    try {
        if ( argc > 2 ) {
            Socket s;
            std::string host = argv[1];
            int port = ::atoi(argv[2]);
            
            if ( !s.create() ) {
                std::cout << "Failed to create socket" << std::endl;
                return -1;
            }
            
            if ( !s.connect(host, port) ) {
                std::cout << "Failed to connect to: " << host << " :: "  << port << std::endl;
                return -1;
            }
            
            std::cout << "Connected to: " << host << " :: "  << port << std::endl;
            
            uint32_t sleep_ms = argc > 3 ? ::atoi(argv[3]) : 0;
            std::string temp;
            std::cout << "Enter command to send: " << std::endl;
            std::cin >> temp;
            
            if ( !s.send(temp+"\n") ) {
                std::cout << "Failed to send to: " << host << " :: "  << port << std::endl;
                return -1;
            }
            
            std::cout << "Send command: " << temp << std::endl;
            
            while ( true ) {
                std::string m;
                if ( 0 < s.recv(m) ) {
                    std::cout << "recv: " << m << std::endl;
                } else {
                    std::cout << "Failed to recv from" << host << " :: "  << port << std::endl;
                    return -1;
                }
                
                tw::common_thread::sleep(sleep_ms);
            }            
            
            return 0;
        }      
      
        // Create the socket
        //
        int port = argc > 1 ? ::atoi(argv[1]) : 30000;      
        std::cout << "running on port: " << port << "....\n";

        ServerSocket server (port);

        while ( true ) {
            ServerSocket new_sock;
            server.accept ( new_sock );
            
            try {
                while ( true ) {
                    std::string data;
                    new_sock >> data;
                    new_sock << "Echo of: " << data;
                }
            } catch ( SocketException& ) {
            }
        }
    } catch ( SocketException& e ) {
        std::cout << "Exception was caught:" << e.description() << "\nExiting.\n";
    }  
    
    return 0;
}

