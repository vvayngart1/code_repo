#pragma once

#include <tw/common_str_util/fast_stream.h>

#include <exception>
#include <sstream>

namespace tw {
namespace common {

class Exception : public std::exception {
public:
    typedef tw::common_str_util::FastStream<2*1024> TBuffer;
    
public:
    Exception() {
        clear();
    }
    
    Exception(const Exception& rhs) {
        *this = rhs;
    }
    
    Exception& operator=(const Exception& rhs) {
        if ( this != &rhs )
            _out = rhs._out;
        
        return *this;
    }
    
    virtual ~Exception() throw() {
        
    }
    
    void clear() {
        _out.clear();
    }
    
    virtual const char* what() const throw() {
        return _out.c_str();
    }
    
    template <typename TItem>
    TBuffer& operator<<(const TItem& item) {
        _out << item;
        return _out;
    }
    
private:
    TBuffer _out;
};

} // namespace common
} // namespace tw




