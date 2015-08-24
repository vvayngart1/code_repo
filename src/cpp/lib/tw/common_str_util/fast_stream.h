#pragma once

#include <tw/common/defs.h>
#include <tw/common/high_res_time.h>
#include <tw/common/uuid.h>
#include <tw/common_str_util/fast_numtoa.h>

#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits>
#include <sstream>
#include <string>

namespace tw {
namespace common_str_util {
//
// Crude implementation of bounds checking fast string stream, with
// buffer allocated on stack and not reallocated.
// NOTE:  For numbers (longs/doubles), upper limit of input length is taking
// to avoid extra operations/copying, but that might force users of that class
// to either allocate extra storage or on certain conditions not have data being
// output to buffer with buffer having space, but thinking data is too ts_long.
// Future enhancements might address this issue.
//
const size_t MAX_LENGTH_CHAR = 1+1;
const size_t MAX_LENGTH_INT16_T = std::numeric_limits<int16_t>::digits10+1;
const size_t MAX_LENGTH_UINT16_T = std::numeric_limits<uint16_t>::digits10+1;
const size_t MAX_LENGTH_INT32_T = std::numeric_limits<int32_t>::digits10+1;
const size_t MAX_LENGTH_UINT32_T = std::numeric_limits<uint32_t>::digits10+1;
const size_t MAX_LENGTH_INT64_T = std::numeric_limits<int64_t>::digits10+1;
const size_t MAX_LENGTH_UINT64_T = std::numeric_limits<uint64_t>::digits10+1;
const size_t MAX_FLOAT_LENGTH = std::numeric_limits<float>::digits10+2;
const size_t MAX_DOUBLE_LENGTH = std::numeric_limits<double>::digits10+2;
const size_t MAX_VOID_P_LENGTH = MAX_LENGTH_UINT64_T;

template <size_t SIZE = 1024, bool PrintDoubleZeroes = false>
class FastStream
{
public:
    FastStream() {
        clear();
        
        _fast_dtoa_ptr = PrintDoubleZeroes ? (&fast_dtoa) : (&fast_dtoa2);
    }

    FastStream(const FastStream& rhs) {
        *this = rhs;
    }

    FastStream& operator=(const FastStream& rhs)
    {
        if ( this != &rhs ) {
            ::memcpy(&_buffer[0], &rhs._buffer[0], SIZE);
            _ptr = _buffer + rhs.size();
        }

        return *this;			
    }

    void clear() {
        _ptr = _buffer;
        ::memset(_buffer, 0, SIZE);
        _precision = 8;
    }

    size_t capacity() const {
        return SIZE - 1;
    }

    size_t size() const {
        return static_cast<size_t>(_ptr - _buffer);
    }

    size_t remained_capacity() const {
        return (capacity() - size());
    }

    bool empty() const {
        return (_ptr == _buffer);
    }

    const char* c_str() const {
        return _buffer;
    }
    
    std::string str() const {
        std::string s(c_str());
        return s;
    }

    bool operator==(const FastStream& rhs) {
        if ( empty() || rhs.empty() )
            return false;

        return ( 0 == ::strcmp(_buffer, rhs._buffer) );
    }

public:
    FastStream& operator<<(char v) {
        if ( canInput(MAX_LENGTH_CHAR) )
            *_ptr++ = v;

        return (*this);
    }

    FastStream& operator<<(int16_t v) {
        if ( canInput(MAX_LENGTH_INT16_T) )
            _ptr += fast_itoa10(v, _ptr);

        return (*this);
    }

    FastStream& operator<<(uint16_t v) {
        if ( canInput(MAX_LENGTH_UINT16_T) )
            _ptr += fast_uitoa10(v, _ptr);

        return (*this);
    }

    FastStream& operator<<(int32_t v) {
        if ( canInput(MAX_LENGTH_INT32_T) )
            _ptr += fast_itoa10(v, _ptr);

        return (*this);
    }

    FastStream& operator<<(uint32_t v) {
        if ( canInput(MAX_LENGTH_UINT32_T) )
            _ptr += fast_uitoa10(v, _ptr);

        return (*this);
    }

    FastStream& operator<<(int64_t v) {
        if ( canInput(MAX_LENGTH_INT64_T) )
            _ptr += fast_litoa10(v, _ptr);

        return (*this);
    }

    FastStream& operator<<(uint64_t v) {
        if ( canInput(MAX_LENGTH_UINT64_T) )
            _ptr += fast_ulitoa10(v, _ptr);

        return (*this);
    }

    FastStream& operator<<(float v) {
        if ( canInput(MAX_FLOAT_LENGTH) )
            _ptr += _fast_dtoa_ptr(v, _ptr, _precision);

        return (*this);
    }

    FastStream& operator<<(double v) {
        if ( canInput(MAX_DOUBLE_LENGTH) )
            _ptr += _fast_dtoa_ptr(v, _ptr, _precision);

        return (*this);
    }

    FastStream& operator<<(void* v) {
        if ( v && canInput(MAX_VOID_P_LENGTH) )
            _ptr += ::sprintf(_ptr, "%p", v);

        return (*this);
    }		

    FastStream& operator<<(const char* v) {
        if ( v ) {
            size_t l = ::strlen(v);
            if ( l > 0 && canInput(l) ) {
                ::memcpy(_ptr, v, l);
                _ptr += l;
            }
        }

        return (*this);
    }            

    FastStream& operator<<(const std::string& v) {
        size_t l = v.size();
        if ( l > 0 && canInput(l) ) {
            ::memcpy(_ptr, v.c_str(), l);
            _ptr += l;
        }

        return (*this);
    }
    
    FastStream& operator<<(const tw::common::THighResTime& v) {
        return ((*this) << v.toString());
    }
    
    FastStream& operator<<(const tw::common::TUuid& v) {
        std::stringstream s;
        s << v;
        
        return ((*this) << s.str());
    }    
    
    FastStream& operator<<(const std::_Setprecision& v) {
        setPrecision(v._M_n);
        
        return (*this);
    }

public:
    int32_t write(const char* buffer, int32_t size) {
        if ( size <=0 || !canInput(static_cast<size_t>(size)) )
            return -1;
        
        ::memcpy(_ptr, buffer, size);
        _ptr += size;
        return size;
    }
    
    
    size_t sprintf(const char* format, ...) {
        va_list list;
        va_start(list, format);
        size_t rc = remained_capacity();
        int32_t size = vsnprintf(_ptr, rc, format, list);
        if ( 0 >= size ) {
            *_ptr = '\0';
            return -1;
        }
        
        if ( static_cast<size_t>(size) > rc ) {
            ::memset(_ptr, 0, rc);
            return -1;
        }
        
        _ptr += size;
        return size;
    }
    
    void setPrecision(uint32_t precision) {
        _precision = precision;
    }
    

private:
    bool canInput(size_t s) {
        return ( capacity() >= (size() + s) );
    }

private:
    // Pointer to a formatting function for doubles
    //
    int32_t (*_fast_dtoa_ptr)(double, char*, int);
    
    char* _ptr;
    char _buffer[SIZE];
    uint32_t _precision;
};

typedef FastStream<> TFastStream;

template <typename TItem>
std::string to_string(const TItem& v) {
    TFastStream s;
    s << v;
    return s.str();
}
        
} // namespace common_str_util
} // namespace tw
