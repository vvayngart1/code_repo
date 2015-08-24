#pragma once

#include <tw/common/defs.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <stdint.h>
#include <sstream>

namespace tw {
namespace common {

typedef boost::uuids::uuid TUuid;
    
inline TUuid generateUuid() {
    return boost::uuids::random_generator()();
}

struct TUuidGenerator {
    
    inline TUuid generateUuid() {
        return _uuidGenerator();
    }
    
private:
    boost::uuids::random_generator _uuidGenerator;
};

struct TUuidBuffer {
    typedef std::pair<uint64_t, uint64_t> TBuffer;
    
    TUuidBuffer() {
        clear();        
    }
    
    TUuidBuffer(const TUuid& rhs) {
        *this = rhs;
    }
    
    TUuidBuffer(const TUuidBuffer& rhs) {
        *this = rhs;
    }
    
    TUuidBuffer& operator=(const TUuid& rhs) {
        clear();
        
        std::copy(rhs.begin(), rhs.end(), begin());
        return *this;
    }
    
    TUuidBuffer& operator=(const TUuidBuffer& rhs) {
        clear();
        
        if ( this != &rhs )                    
            std::copy(rhs.begin(), rhs.end(), begin());
        
        return *this;
    }
    
    operator TUuid() const {
        TUuid o;
        
        std::copy(begin(), end(), o.begin());
        return o;
    }
    
    void clear() {
        ::memset(begin(), 0, size());
    }
    
    bool isValid() const {
        return !(_buffer.first == 0 && _buffer.second == 0);
    }
    
    static size_t size() {
        return sizeof(TUuidBuffer);
    }
    
    uint8_t* begin() {
        return reinterpret_cast<uint8_t*>(&_buffer.first);
    }
    
    uint8_t* end() {
        return begin()+size();
    }
    
    const uint8_t* begin() const {
        return reinterpret_cast<const uint8_t*>(&_buffer.first);
    }
    
    const uint8_t* end() const {
        return begin()+size();
    }
    
    std::string toString() const {
        TUuid o;
        
        std::copy(begin(), end(), o.begin());
        std::stringstream os;
        os << o;
        return os.str();
    }
    
    void fromString(const std::string& value) {
        clear();
        
        TUuid o;        
        std::stringstream os;
        
        os << value;
        os >> o;
        
        *this = o;        
    }
    
    bool operator<(const TUuidBuffer& rhs) const {
        return _buffer < rhs._buffer;
    }
    
    bool operator>(const TUuidBuffer& rhs) const {
        return _buffer > rhs._buffer;
    }
    
    bool operator==(const TUuidBuffer& rhs) const {
        return _buffer == rhs._buffer;
    }
    
    bool operator!=(const TUuidBuffer& rhs) const {
        return _buffer != rhs._buffer;
    }
    
    bool operator<=(const TUuidBuffer& rhs) const {
        return _buffer <= rhs._buffer;
    }
    
    bool operator>=(const TUuidBuffer& rhs) const {
        return _buffer >= rhs._buffer;
    }    
    
    friend std::ostream& operator<<(std::ostream& os, const TUuidBuffer& x) {
        return os << x.toString();
    }
    
    friend bool operator>>(std::istream& input, TUuidBuffer& x) {
        try {
            std::string value;
            input >> value;
            x.fromString(value);
        } catch(...) {
            return false;
        }
        
        return true;
    }
    
    TBuffer _buffer;
};

} // namespace channel_or
} // namespace tw
