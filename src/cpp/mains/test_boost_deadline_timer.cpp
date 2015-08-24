#include <boost/asio.hpp>
#include <boost/thread.hpp> 
#include <iostream> 

boost::asio::io_service io_service; 
boost::asio::deadline_timer* timer1 = NULL;
boost::asio::deadline_timer* timer2 = NULL;
boost::asio::deadline_timer* timer3 = NULL;

void createAndRegTimer1();
void createAndRegTimer2();
void createAndRegTimer3();

// This will be called continuosly
//
void handler1(const boost::system::error_code &ec) 
{
    static size_t counter = 0;
    
    std::cout << "2 s. :: " << ++counter << "\n"; 
    std::cout << "<========================>" << "\n" << "\n"; 
    createAndRegTimer1();
} 

// This will be called once
//
void handler2(const boost::system::error_code &ec) 
{ 
    std::cout << "5 s." << "\n"; 
    std::cout << "<========================>" << "\n";  
    
    createAndRegTimer3();
}

// This will be called once
//
void handler3(const boost::system::error_code &ec) 
{ 
    std::cout << "1 s." << "\n"; 
    std::cout << "<========================>" << "\n";
    
    createAndRegTimer3();
}

void run() 
{
    while ( true ) {
        io_service.run(); 
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    }
} 

void createAndRegTimer1() {
    if ( NULL == timer1 ) {
        timer1 = NULL;
        timer1 = new boost::asio::deadline_timer(io_service, boost::posix_time::seconds(2));
    } else {
        timer1->expires_at(timer1->expires_at() + boost::posix_time::seconds(2));
    }

    timer1->async_wait(handler1);
}

void createAndRegTimer2() {
    if ( NULL == timer2 ) {
        timer2 = NULL;
        timer2 = new boost::asio::deadline_timer(io_service, boost::posix_time::seconds(5));
    } else {
        timer2->expires_at(timer2->expires_at() + boost::posix_time::seconds(5));
    }
    timer2->async_wait(handler2);
}

void createAndRegTimer3() {
    if ( NULL == timer3 ) {
        timer3 = NULL;
        timer3 = new boost::asio::deadline_timer(io_service, boost::posix_time::seconds(1));
    } else {
        timer3->expires_at(timer3->expires_at() + boost::posix_time::seconds(1));
    }
    timer3->async_wait(handler3);
}


int main() 
{
    boost::thread thread1(run);        
    
    createAndRegTimer1();
    createAndRegTimer2();      
    
    thread1.join();
}
