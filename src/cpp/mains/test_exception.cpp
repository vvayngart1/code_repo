#include <tw/common/defs.h>
#include <tw/common/exception.h>
#include <tw/log/defs.h>


int main(int argc, char * argv[]) {
    tw::common::Exception ex;
    long l = 0;
    
    try {
        l = 1;
        ex << "This is sample exception1 with sample value: " << l << "\n";
        throw(ex);
    } catch (std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what();
    } catch (...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
    try {
        l = 2;
        ex.clear();
        ex << "This is sample exception2 with sample value: " << l << "\n";
        throw(ex);
    } catch (std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what();
    } catch (...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
    
   return 0;
}
