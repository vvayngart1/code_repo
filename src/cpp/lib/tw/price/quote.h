#pragma once

#include <tw/common/defs.h>
#include <tw/common/high_res_time.h>
#include <tw/common/command.h>
#include <tw/generated/instrument.h>
#include <tw/price/defs.h>
#include <tw/price/ticks_converter.h>
#include <tw/common_str_util/fast_stream.h>

#include <boost/functional.hpp>

#include <algorithm>
#include <sstream>
#include <iomanip>
#include <bitset>

namespace tw {
namespace price {

typedef tw::price::TicksConverter TTickConverter;
typedef TTickConverter::TConverterTo TConverterTo;
    
struct PriceSize 
{
    PriceSize() {
        clear();
    }
    
    void clear() {
        _price.clear();
        _size.clear();
    }
    
    bool isValid() const {
        return (_price.isValid() && _size > 0);
    }
    
    bool operator==(const PriceSize& rhs) const {
        return (isValid() && rhs.isValid() && _price == rhs._price && _size == rhs._size);
    }
    
    void set(const Ticks& price, const Size& size) {
        _price = price;
        _size = size;
    }
    
    std::string toString(bool invert = false) const {
        std::stringstream out;
        if ( !invert ) {
            out << std::setw(10) <<  _size << std::setw(20) << _price;
        } else {
            out << std::setw(20) <<  std::left << _price << std::setw(20) << std::left << _size;
        }
        
        return out.str();
    }
    
    std::string toShortString(bool invert = false, const TConverterTo& c = TTickConverter::createTo()) const {
        tw::common_str_util::FastStream<128> out;
        
        if ( !invert ) {
            out << _size << "," << c(_price);
        } else {
            out << c(_price) << "," << _size;
        }
        
        return out.str();
    }
    
    Ticks _price;
    Size _size;
};

struct PriceSizeNumOfOrders : public PriceSize {
    PriceSizeNumOfOrders() {
        clear();
    }
    
    void clear() {
        PriceSize::clear();
        _numOrders = 0;
    }
    
    bool operator==(const PriceSizeNumOfOrders& rhs) const {
        return (_numOrders == rhs._numOrders && PriceSize::operator==(rhs));
    }
    
    void set(const Ticks& price, const Size& size, uint32_t numOrders = 0) {
        PriceSize::set(price, size);
        _numOrders = numOrders;
    }
    
    std::string toString(bool invert = false) const {
        std::stringstream out;
        if ( !invert ) {
            out << std::setw(10) <<  _numOrders << PriceSize::toString(invert);
        } else {
            out << PriceSize::toString(invert) << std::setw(10) <<  std::left << _numOrders;
        }
        
        return out.str();
    }
    
    std::string toShortString(bool invert = false, const TConverterTo& c = TTickConverter::createTo()) const {
        std::stringstream out;
        if ( !invert ) {
            out << _numOrders << "," << PriceSize::toShortString(invert, c);
        } else {
             out << PriceSize::toShortString(invert, c) << "," << _numOrders;
        }
        return out.str();
    }
    
    uint32_t _numOrders;
};

struct PriceLevel
{    
    PriceLevel() {
        clear();
    }
    
    void clear() {
        _bid.clear();
        _ask.clear();
    }
    
    bool isValid() const {
        return (_bid.isValid() && _ask.isValid());
    }
    
    bool operator==(const PriceLevel& rhs) const {                
        return (_bid == rhs._bid && _ask == rhs._ask);
    }
    
    PriceSizeNumOfOrders _bid;
    PriceSizeNumOfOrders _ask;
    
    std::string toString() const {
        std::stringstream out;
        
        out << _bid.toString() << " | "  << _ask.toString(true);
                
        return out.str();
    }
    
    std::string toShortString() const {
        std::stringstream out;
        
        out << _bid.toShortString() << "|"  << _ask.toShortString(true);
                
        return out.str();
    }
};


struct Trade : public PriceSize {
    Trade() {
        clear();
    }
    
    enum eCondition {
        kConditionUnknown,
        kConditionOpening,
        kConditionPriceCalculatedByExchange        
    };
    
    enum eUpdateAction {
        kUpdateActionUnknown,
        kUpdateActionNew,
        kUpdateActionDelete        
    };
    
    enum eTickDirection {
        kTickDirectionUnknown,
        kTickDirectionPlus,
        kTickDirectionMinus
    };
    
    enum eAggressorSide {
        kAggressorSideUnknown,
        kAggressorSideBuy,
        kAggressorSideSell
    };
    
    static const char* conditionToString(eCondition value) {
        switch (value) {
            case kConditionOpening:                     return "O";
            case kConditionPriceCalculatedByExchange:   return "X";
            default:                                    return "u";
        }
    }
    
    static const char* updateActionToString(eUpdateAction value) {
        switch (value) {
            case kUpdateActionNew:                      return "N";
            case kUpdateActionDelete:                   return "D";
            default:                                    return "u";
        }
    }
    
    static const char* tickDirectionToString(eTickDirection value) {
        switch (value) {
            case kTickDirectionPlus:                    return "+";
            case kTickDirectionMinus:                   return "-";
            default:                                    return "u";
        }
    }
    
    static const char* aggressorSideToString(eAggressorSide value) {
        switch (value) {
            case kAggressorSideBuy:                     return "B";
            case kAggressorSideSell:                    return "S";
            default:                                    return "u";
        }
    }
    
    void clear() {
        PriceSize::clear();
        _condition = kConditionUnknown;
        _updateAction = kUpdateActionUnknown;
        _tickDirection = kTickDirectionUnknown;
        _aggressorSide = kAggressorSideUnknown;        
    }
    
    void set(const Ticks& price, const Size& size) {
        PriceSize::set(price, size);
    }
    
    std::string toString() const {
        std::stringstream out;
        
        out << std::setw(10) << _size << "@" << std::setw(10) << std::left << _price << " :: ";
        
        out << conditionToString(_condition) << ",";
        out << updateActionToString(_updateAction) << ",";
        out << tickDirectionToString(_tickDirection) << ",";
        out << aggressorSideToString(_aggressorSide);
        
        return out.str();
    }
    
    std::string toShortString(const TConverterTo& c = TTickConverter::createTo()) const {
        std::stringstream out;
        
        out << PriceSize::toShortString(false, c) << ","        
                << conditionToString(_condition) << ","
                << updateActionToString(_updateAction) << ","
                << tickDirectionToString(_tickDirection) << ","
                << aggressorSideToString(_aggressorSide);
        
        return out.str();
    }
    
    eCondition _condition;
    eUpdateAction _updateAction;
    eTickDirection _tickDirection;
    eAggressorSide _aggressorSide;
};

struct QuoteWire
{
    enum eStatus {
        kSuccess = 0,
        kExchangeDown,
        kExchangeUp,
        kExchangeSlow,
        kStaleQuote,
        kDropRate,        
        kTradingHaltedOrStopped,
        kTradingPause,
        kTradingResumeOrOpen,
        kTradingSessionEnd,
        kHLOReset,
        kDataRecoveryStart,
        kDataRecoveryFinish,
        kReplayError,
        kReplayFinished,
        kConnectionHandlerError,
        kPriceGap
    };
    
    static const char* statusToString(eStatus value) {
        switch (value) {
            case kSuccess:                      return "kSuccess";
            case kExchangeDown:                 return "kExchangeDown";
            case kExchangeUp:                   return "kExchangeUp";
            case kExchangeSlow:                 return "kExchangeSlow";
            case kStaleQuote:                   return "kStaleQuote";
            case kDropRate:                     return "kDropRate";            
            case kTradingHaltedOrStopped:       return "kTradingHaltedOrStopped";
            case kTradingPause:                 return "kTradingPause";
            case kTradingResumeOrOpen:          return "kTradingResumeOrOpen";
            case kTradingSessionEnd:            return "kTradingSessionEnd"; 
            case kHLOReset:                     return "kHLOReset";
            case kDataRecoveryStart:            return "kDataRecoveryStart";
            case kDataRecoveryFinish:           return "kDataRecoveryFinish";
            case kReplayError:                  return "kReplayError";
            case kReplayFinished:               return "kReplayFinished";
            case kConnectionHandlerError:       return "kConnectionHandlerError";
            case kPriceGap:                     return "kPriceGap";
            default:                            return "kUnknown";
        }
    }

    // TODO: potentially need to change to 10/flexible number of levels
    //
    enum _ENUM {
        SIZE = 5,
        NUMBITS = 2 * SIZE + 1,
        NOTUSED = 8 * sizeof(TChangeFlag) - NUMBITS
    };
    
    uint32_t _seqNum;
    uint32_t _numTrades;
    uint32_t _numBooks;
    
    tw::instr::Instrument::TKeyId _instrumentId;
    QuoteWire::eStatus _status;
    
    Ticks _open;
    Ticks _high;
    Ticks _low;
    
    // Current flag supports up to 7 levels of prices:
    // uint32_t - 32 bits:
    // 1 bit for trade
    // 28(4*7) bits for book
    // 1 bit for _open
    // 1 bit for high
    // 1 bit for _low
    // TOTAL: 32 bits
    //
    TChangeFlag _flag;
    Trade _trade;
    PriceLevel _book[SIZE];
};

struct Quote : public QuoteWire
{
    uint32_t _intSeqNum;
    uint32_t _intGaps;
    
    tw::common::THighResTime _exTimestamp;
    tw::common::THighResTime _timestamp1;
    tw::common::THighResTime _timestamp2;
    tw::common::THighResTime _timestamp3;
    tw::common::THighResTime _timestamp4;
    tw::common::THighResTime _lastUpdateTimestamp;

    const tw::instr::Instrument* _instrument;

public:    
    Quote() {
        clear();
    }        

    void clear() {
        _seqNum = 0;
        _numTrades = 0;
        _numBooks = 0;
        
        _intSeqNum = 0;
        _intGaps = 0;
        _exTimestamp.clear();
        
        _timestamp1.setToNow();
        _timestamp2.setToNow();
        _timestamp3.setToNow();
        _timestamp4.setToNow();
        
        _instrumentId = tw::instr::Instrument::TKeyId();
        
        clearStatus();
        
        _open.clear();
        _high.clear();
        _low.clear();
        
        clearFlag();
        _trade.clear();
        std::for_each(_book, _book+SIZE, boost::mem_fun_ref(&PriceLevel::clear));
        
        _instrument = NULL;
    }

    void clearFlag() {
        _flag = 0;
    }
    
    void clearStatus() {
        _status = Quote::kSuccess;
    }
    
    void clearRuntime(bool resetTime=true) {
        if ( resetTime )
            _timestamp2.setToNow();
        
        clearFlag();
        clearStatus();
    }
    
    void updateStats() {
        if ( !isChanged() )
            return;
        
        if ( isNormalTrade() )
            ++_numTrades;
        
        if ( isBookUpdate() )
            ++_numBooks;
        
        _lastUpdateTimestamp = _timestamp1;
    }
    
    
public:
    tw::common::Command statsToCommand() const {
        tw::common::Command c;
        
        if ( _instrument ) {
            c.addParams("exchange", _instrument->_exchange.toString());
            c.addParams("displayName", _instrument->_displayName);
        }
        
        c.addParams("seqNum", _seqNum);
        c.addParams("numTrades", _numTrades);
        c.addParams("numBooks", _numBooks);
        c.addParams("lastUpdateTimestamp", _lastUpdateTimestamp.toString());
        
        return c;
    }
    
    std::string toShortString() const {
        std::stringstream out;
        
        out << "book[0]=" << _book[0].toShortString() << ",trade=" << _trade.toShortString();
        return out.str();
    }
    
    std::string toInfoString() const {
        std::stringstream out;
        
        out << "_intSeqNum: " << std::setw(10) << std::left << _intSeqNum;
        out << "_intGaps: " << std::setw(10) << std::left << _intGaps;
        out << "_seqNum: " << std::setw(10) << std::left << _seqNum;
        out << "_numTrades: " << std::setw(15) << std::left << _numTrades;
        out << "_numBooks: " << std::setw(15) << std::left << _numBooks;
        out << " _exTimestamp: " << std::setw(30) << std::left <<  _exTimestamp;
        out << " _timestamp1: " << std::setw(30) << std::left <<  _timestamp1;
        out << " _timestamp2: " << std::setw(30) << std::left <<  _timestamp2;
        out << " _timestamp3: " << std::setw(30) << std::left <<  _timestamp3;
        out << " _timestamp4: " << std::setw(30) << std::left <<  _timestamp4;
        out << " _lastUpdateTimestamp: " << std::setw(30) << std::left <<  _lastUpdateTimestamp;
        
        if ( _instrument ) {
            std::string instrumentId;
            instrumentId = boost::lexical_cast<std::string>(_instrumentId) + "-" + _instrument->_exchange.toString() + ":" + _instrument->_displayName;
            out << " _instrumentId: " << std::setw(30) << std::left <<  instrumentId;
        }
        out << " _status: " << std::setw(10) << std::left << statusToString(_status);
        out << " top_of_book" << toShortString();
        
        return out.str();
    }
    
    std::string toStringWhatChanged() const {
        std::stringstream out;
        
//        out << "_flag: T|  Bids   |  Asks  |OHL" << "\n";
        out << "_flag: T=";
        
        char buffer[SIZE*4+5];
        ::memset(&buffer, '_', sizeof(buffer));
        buffer[sizeof(buffer)-1] = '\0';
        
        if ( isTrade() )
            out << '1';
        else
            out << '0';
        out << ",B";
        
        buffer[0] = '=';
        for ( size_t count = 0; count < SIZE; ++count ) {
            size_t bidPriceIndex = 2*count+1;
            size_t bidSizeIndex = 2*count+2;
            size_t askPriceIndex = 2*SIZE+bidPriceIndex;
            size_t askSizeIndex = 2*SIZE+bidSizeIndex;
            
            if ( isBidUpdatePrice(count) )
                buffer[bidPriceIndex] = '1';
            
            if ( isBidUpdateSize(count) )
                buffer[bidSizeIndex] = '1';
            
            if ( isAskUpdatePrice(count) )
                buffer[askPriceIndex] = '1';
            
            if ( isAskUpdateSize(count) )
                buffer[askSizeIndex] = '1';
        }
        
        out << buffer;
        return out.str();
    }
    
    std::string toString() const {
        std::stringstream out;
        
        out << "\n";
        out << "_seqNum: " << std::setw(15) << std::left << _seqNum;
        out << "_numTrades: " << std::setw(15) << std::left << _numTrades;
        out << "_numBooks: " << std::setw(15) << std::left << _numBooks;
        out << " _exTimestamp: " << std::setw(30) << std::left <<  _exTimestamp;
        out << " _timestamp1: " << std::setw(30) << std::left <<  _timestamp1;
        out << " _timestamp2: " << std::setw(30) << std::left <<  _timestamp2;
        out << " _timestamp3: " << std::setw(30) << std::left <<  _timestamp3;
        out << " _timestamp4: " << std::setw(30) << std::left <<  _timestamp4;
        out << " _lastUpdateTimestamp: " << std::setw(30) << std::left <<  _lastUpdateTimestamp;
        
        if ( _instrument ) {
            std::string instrumentId;
            instrumentId = boost::lexical_cast<std::string>(_instrumentId) + "-" + _instrument->_exchange.toString() + ":" + _instrument->_displayName;
            out << " _instrumentId: " << std::setw(30) << std::left <<  instrumentId;
        }
        out << " _status: " << std::setw(10) << std::left <<  statusToString(_status);
        out << "\n";
        
        out << "_open: " << std::setw(10) << std::left <<  _open;
        out << " _high: " << std::setw(10) << std::left << _high;
        out << " _low: " << std::setw(10) << std::left <<  _low;
        out << "\n";
        
        out << "_flag: T|  Bids   |  Asks  |OHL" << "\n";
        out << "       ";        
        
        char buffer[SIZE*4+5];
        ::memset(&buffer, '_', sizeof(buffer));
        buffer[sizeof(buffer)-1] = '\0';
        
        if ( isTrade() )
            buffer[0] = '1';
        
        for ( size_t count = 0; count < SIZE; ++count ) {
            size_t bidPriceIndex = 2*count+1;
            size_t bidSizeIndex = 2*count+2;
            size_t askPriceIndex = 2*SIZE+bidPriceIndex;
            size_t askSizeIndex = 2*SIZE+bidSizeIndex;
            
            if ( isBidUpdatePrice(count) )
                buffer[bidPriceIndex] = '1';
            
            if ( isBidUpdateSize(count) )
                buffer[bidSizeIndex] = '1';
            
            if ( isAskUpdatePrice(count) )
                buffer[askPriceIndex] = '1';
            
            if ( isAskUpdateSize(count) )
                buffer[askSizeIndex] = '1';
        }
        
        out << buffer << "\n";        
        out << "_trade: " << _trade.toString();
        out << "\n";
        
        out << "_book: " << "\n";
        for ( uint32_t count = 0; count < SIZE; ++count ) {
            out << "[" << count+1 << "] " << _book[count].toString() << "\n";
        }        
                
        return out.str();
    }    
    
    friend tw::log::StreamDecorator& operator<<(tw::log::StreamDecorator& os, const Quote& x) {
        return os << x.toString();
    }

    friend std::ostream& operator<<(std::ostream& os, const Quote& x) {
        return os << x.toString();
    }
    
public:
    bool isValid() const  {
        return (NULL != _instrument && _instrument->isValid());
    }
    
    bool isError() const {
        return (Quote::kSuccess != _status);
    }
    
    const tw::instr::Instrument* getInstrument() const {
        return _instrument;
    }
    
    // This helper method allows clients with overloaded
    // operator()(const Quote&) to be called in std::for_each
    //    
    template <typename TClient>
    void operator()(TClient& client) {
        client(*this);
    }
    
public:
    // Helper methods to set data
    //
    void setTrade(const Ticks& price, const Size& size) {
        _trade.set(price, size);        
        setTrade();        
    }
    
    void setBid(const Ticks& price, const Size& size, size_t level, uint32_t numOrders = 0) {
        // TODO: Potentially need to throw an exception
        //
        if ( level > SIZE - 1)
            return;
        
        _book[level]._bid.set(price, size, numOrders);
        setBidUpdate(level);        
    }
    
    void setBidPrice(const Ticks& price, size_t level) {
        // TODO: Potentially need to throw an exception
        //
        if ( level > SIZE - 1)
            return;
        
        _book[level]._bid._price = price;
        setBidUpdatePrice(level);        
    }
    
    void setBidSize(const Size& size, size_t level) {
        // TODO: Potentially need to throw an exception
        //
        if ( level > SIZE - 1)
            return;
        
        _book[level]._bid._size = size;
        setBidUpdateSize(level);        
    }
    
    void setAsk(const Ticks& price, const Size& size, size_t level, uint32_t numOrders = 0) {
        // TODO: Potentially need to throw an exception
        //
        if ( level > SIZE - 1)
            return;
        
        _book[level]._ask.set(price, size, numOrders);
        setAskUpdate(level);        
    }
    
    void setAskPrice(const Ticks& price, size_t level) {
        // TODO: Potentially need to throw an exception
        //
        if ( level > SIZE - 1)
            return;
        
        _book[level]._ask._price = price;
        setAskUpdatePrice(level);        
    }
    
    void setAskSize(const Size& size, size_t level) {
        // TODO: Potentially need to throw an exception
        //
        if ( level > SIZE - 1)
            return;
        
        _book[level]._ask._size = size;
        setAskUpdateSize(level);        
    }
    
    void setOpen(const Ticks& price) {
        _open = price;
        setOpen();        
    }
    
    void setHigh(const Ticks& price) {
        _high = price;
        setHigh();        
    }
    
    void setLow(const Ticks& price) {
        _low = price;
        setLow();        
    }

    // Methods to check what changed
    //
    bool isChanged() const {
        return 0 != _flag;
    }

    bool isTrade() const {
        return 0 != (_flag & tradeBits() );
    }
    
    bool isNormalTrade() const {
        return (isTrade() && tw::price::Trade::kConditionUnknown == _trade._condition);
    }
    
    bool isSpreadLegTrade() const {
        return (isTrade() && tw::price::Trade::kConditionPriceCalculatedByExchange == _trade._condition);
    }
    
    bool isOpeningTrade() const {
        return (isTrade() && tw::price::Trade::kConditionOpening == _trade._condition);
    }
    
    bool isBookUpdate() const {
        return 0 != (_flag & bookUpdateBits());
    }   
    
    bool isLevelUpdate(size_t level) const {
        return 0 != (_flag & levelUpdateBits(level));
    }
    
    bool isBidUpdate(size_t level) const {
        return 0 != (_flag & bidUpdateBits(level));
    }
    
    bool isBidUpdatePrice(size_t level) const {
        return 0 != (_flag & bidUpdatePriceBits(level));
    }
    
    bool isBidUpdateSize(size_t level) const {
        return 0 != (_flag & bidUpdateSizeBits(level));
    }
    
    bool isAskUpdate(size_t level) const {
        return 0 != (_flag & askUpdateBits(level));
    }
    
    bool isAskUpdatePrice(size_t level) const {
        return 0 != (_flag & askUpdatePriceBits(level));
    }
    
    bool isAskUpdateSize(size_t level) const {
        return 0 != (_flag & askUpdateSizeBits(level));
    }
    
    bool isOpen() const {
        return 0 != (_flag & openBits());
    }
    
    bool isHigh() const {
        return 0 != (_flag & highBits());
    }
    
    bool isLow() const {
        return 0 != (_flag & lowBits());
    }

    // Methods to set what changed
    //
    void setAll(void) {
        _flag = TChangeFlag(-1) >> NOTUSED;
    }
    
    void setTrade(void) {
        _flag |= tradeBits();
    }
    
    void setLevelUpdate(size_t level) {
        _flag |= levelUpdateBits(level);
    }
    
    void setBidUpdate(size_t level) {
        _flag |= bidUpdateBits(level);
    }
    
    void setBidUpdatePrice(size_t level) {
        _flag |= bidUpdatePriceBits(level);
    }
    
    void setBidUpdateSize(size_t level) {
        _flag |= bidUpdateSizeBits(level);
    }
    
    void setAskUpdate(size_t level) {
        _flag |= askUpdateBits(level);
    }
    
    void setAskUpdatePrice(size_t level) {
        _flag |= askUpdatePriceBits(level);
    }
    
    void setAskUpdateSize(size_t level) {
        _flag |= askUpdateSizeBits(level);
    }
    
    void setOpen() {
        _flag |= openBits();
    }
    
    void setHigh() {
        _flag |= highBits();
    }
    
    void setLow() {
        _flag |= lowBits();
    }
    
private:
    TChangeFlag tradeBits() const { return 1; }
    TChangeFlag bookUpdateBits() const { return 0x1ffffffe; }                          // 0x1ffffffe = 0001 1111 1111 1111 1111 1111 1111 1110
    
    TChangeFlag levelUpdateBits(size_t level) const { return (0xf << (level + 1)); }   // 0xf = 1111
    
    TChangeFlag bidUpdateBits(size_t level) const { return (0x3 << (4 * level + 1)); }
    TChangeFlag bidUpdatePriceBits(size_t level) const { return (0x1 << (4 * level + 1)); }
    TChangeFlag bidUpdateSizeBits(size_t level) const { return (0x2 << (4 * level + 1)); }
    
    TChangeFlag askUpdateBits(size_t level) const { return (0xc << (4 * level + 1)); }
    TChangeFlag askUpdatePriceBits(size_t level) const { return (0x4 << (4 * level + 1)); }
    TChangeFlag askUpdateSizeBits(size_t level) const { return (0x8 << (4 * level + 1)); }
    
    TChangeFlag openBits() const { return 0x20000000; }        // 0x20000000 = 0010 0000 0000 0000 0000 0000 0000 0000
    TChangeFlag highBits() const { return 0x40000000; }        // 0x40000000 = 0100 0000 0000 0000 0000 0000 0000 0000
    TChangeFlag lowBits() const { return 0x80000000; }         // 0x80000000 = 1000 0000 0000 0000 0000 0000 0000 0000
};

} // namespace price
} // namespace tw
