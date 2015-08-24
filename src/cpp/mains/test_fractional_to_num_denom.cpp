#include <tw/common/fractional.h>

#include <iostream>

void print(double d) {
    tw::common::Fractional::TFraction f = tw::common::Fractional::getFraction(d);
    
    std::cout << d << "=" << f.first << "/" << f.second << "\n";
    return;
}


int main(){
    print(1.0/8.0);
    print (1.0/32.0);
    print (1.0/64.0);
    print (1.0/128.0);
    
    print (1.0/100.0);
    print (1.0/10000.0);
    print (1.0/100000000.0);
    print (1.0/4.0);
    print(1.0/3.0);
    
    print(25);
    std::cin.get();
    return 0;
}
