#pragma once

#include <tw/common/defs.h>
#include <tw/common/exception.h>

namespace tw {
namespace common_comm {
//
// Expected Parser interface:
//
//		Parse buffer and return the number of parsed bytes.
//		size_t process(t_pcchar theBuffer, size_t theNumBytes);
//
// User is supposed to set parser before buffer usage.
//
// MesgSize: estimated max message size. Underestimating it will result in buffer
//           reallocations - watch log for reallocation messages.
//
// MesgMultiplier: multiplier to calculate initial buffer size. Choose bigger one if
//           it is highly likely that the messages will come in irregular chunks.
//
// Usage:
//
// - obtain current empty size:                         getFreeSize()
// - obtain pointer to the available space:		getFreeBuffer()
// - populate up to getFreeSize() bytes starting from getFreeBuffer()
// - call process(theBytes) to process the data currently in the buffer
//   (actual number of processed bytes will include data already in the buffer)
//   Negative value returned from process(...) indicates an error to be able to act
//   accordingly (probably disconnect).
//
// Data in the buffer may be moved and the buffer may be reallocated to allow
// at least MesgSize bytes available at any moment. Watch log for the reallocation
// messages and increase initial buffer size accordingly.
//
//
template<class TParser, size_t TMesgSize = 256, size_t TMesgMultiplier = 4>
class Buffer {
public:
    Buffer() : _multiplier(TMesgMultiplier),
               _parser(NULL),
               _start(_stackBuffer),
               _end(_start + sizeof(_stackBuffer)),
               _currentFilled(_start),
               _currentEmpty(_start) {
				::memset(_stackBuffer, 0, sizeof(_stackBuffer));
    }

    ~Buffer() {
        if (_start != _stackBuffer)
            delete[] _start;
    }

public:
    void setParser(TParser* theParser) {
        _parser = theParser;
    }
    
    char* getFreeBuffer(void) {
        return _currentEmpty;
    }
    
    size_t getFreeSize(void) {
        return _end - _currentEmpty;
    }

    void clear(void) {
        ::memset(_stackBuffer, 0, sizeof(_stackBuffer));
        if (_start != _stackBuffer)
            delete[] _start;
        _start = _stackBuffer;
        _end = _start + sizeof(_stackBuffer);
        _currentFilled = _start;
        _currentEmpty = _start;
    }

    int32_t process(size_t theBytes) {
        if (theBytes > getFreeSize()) {
            tw::common::Exception ex;
            ex << "caller claims to have written more than was available ( " << theBytes << " written, " << getFreeSize() << " available )";
            throw(ex);
        }
        
        _currentEmpty += theBytes;

        uint32_t aTotal = 0;
        int32_t aCount = 0;

        do {
            aCount = _parser->process(getFilledBuffer(), getFilledSize());
            if (aCount < 0)
                return aCount;
            advance(aCount);
            aTotal += aCount;
        } while (aCount && getFilledSize());

        return aTotal;
    }
    
    std::string toString() const {
        std::string message;
        if ( getFilledSize() > 0 )
            message.assign(getFilledBuffer(), getFilledBuffer() + getFilledSize());
        
        return message;
    }

private:

    char* getFilledBuffer(void) const {
        return _currentFilled;
    }
    size_t getFilledSize(void) const {
        return _currentEmpty - _currentFilled;
    }

    void advance(size_t theCount) {
        size_t aFilledSize = getFilledSize();

        if (theCount > aFilledSize)
            theCount = aFilledSize;

        _currentFilled += theCount;
        aFilledSize -= theCount;

        if (0 == aFilledSize) {
            _currentFilled = _start;
            _currentEmpty = _start;
        }

        if (getFreeSize() < TMesgSize) {
            if (_currentFilled > _start) {
                memmove(_start, _currentFilled, aFilledSize);
                _currentFilled = _start;
                _currentEmpty = _currentFilled + aFilledSize;
            }

            if (static_cast<size_t> (_end - _currentEmpty) < TMesgSize) {
                // Reallocating buffer - log error here !!!!!

                size_t aNewSize = TMesgSize * (_multiplier *= 2);

                char* aBuffer = new char[aNewSize];

                memcpy(aBuffer, _currentFilled, aFilledSize);

                if (_start != _stackBuffer)
                    delete[] _start;

                _start = aBuffer;
                _end = _start + aNewSize;

                _currentFilled = _start;
                _currentEmpty = _currentFilled + aFilledSize;
            }
        }
    }

private:
    size_t _multiplier;
    char _stackBuffer[TMesgSize * TMesgMultiplier];

    TParser* _parser;

    char* _start;
    char* _end;
    char* _currentFilled;
    char* _currentEmpty;
};

} // namespace common_comm
} // namespace tw

