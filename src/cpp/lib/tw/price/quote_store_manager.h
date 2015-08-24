#pragma once

#include <tw/common_thread/locks.h>
#include <tw/common_thread/thread.h>
#include <tw/common_thread/thread_pipe.h>
#include <tw/common_strat/consumer_proxy.h>
#include <tw/price/quote_store.h>

namespace tw {
namespace price {

class QuoteStoreManager
{
public:
    QuoteStoreManager() : _mainQuoteStore(QuoteStore::instance())
    {
        clear();
    }
    
    void clear() {
        _isMultithreaded = false;
        _verbose = false;
        _isDone = false;
        _thread.reset();
    }

    bool start(bool isMultithreaded, bool verbose = false) {
        bool status = true;    
        try {
            _isMultithreaded = isMultithreaded;
            _verbose = verbose;
        
            if ( _isMultithreaded ) {
                _threadLocalQuoteStore.copy(_mainQuoteStore);
                
                // Create and start thread
                //
                _thread = TThreadPtr(new TThread(boost::bind(&QuoteStoreManager::threadMain, this)));
            }
            
            LOGGER_INFO << "Started QuoteStoreManager in _isMultithreaded=" << (_isMultithreaded ? "true" : "false") << " mode" << "\n";
        } catch(const std::exception& e) {        
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
            status = false;
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
            status = false;
        }
        
        if ( !status )
            stop();
        
        return true;
    }
    
    void stop() {
        try {
            if ( _isDone )
                return;
            
            _isDone = true;        
            
            if ( _thread ) {
                _threadPipe.stop();
                _thread->join();
            }
            
            LOGGER_INFO << "Stopped QuoteStoreManager" << "\n";
            clear();
        } catch(const std::exception& e) {        
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        } 
    }
    
    QuoteStore& getStore() {
        if ( _isMultithreaded )
            return _threadLocalQuoteStore;
        
        return _mainQuoteStore;
    }
    
    void onQuote(const QuoteStore::TQuote& quote) {
        if ( !_isMultithreaded ) {
            tw::common_strat::ConsumerProxy::instance().onQuote(quote);
            return;
        }
        
        {
            TLockGuard lockGuard(_lock);
            ++(const_cast<QuoteStore::TQuote&>(quote))._intSeqNum;
            _changedQuotes[quote._instrumentId] = quote;
            if ( _verbose )
                LOGGER_INFO << "==> QUOTE_FROM_ONIX --> " << quote.toInfoString() << "\n";
        }
        
        _threadPipe.push(true);
    }
    
private:
    void threadMain() {
        LOGGER_INFO << "Started" << "\n";
    
        try {
            TQuotes changedQuotes;
            while( !_isDone ) {
                bool b = false;
                _threadPipe.read(b, true);
                _threadPipe.clear();
                {
                    TLockGuard lockGuard(_lock);
                    if ( _changedQuotes.empty() )
                        continue;
                    
                    changedQuotes = _changedQuotes;
                    _changedQuotes.clear();
                }
                
                TQuotes::iterator iter = changedQuotes.begin();
                TQuotes::iterator end = changedQuotes.end();
                for ( ; iter != end; ++iter ) {
                    QuoteStore::TQuote& quote = _mainQuoteStore.getQuoteByKeyId(iter->second._instrumentId);
                    copyQuote(iter->second, quote);
                    if ( quote.isChanged() )
                        tw::common_strat::ConsumerProxy::instance().onQuote(quote);
                }
            }
        } catch(const std::exception& e) {        
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        }
        
        LOGGER_INFO << "Finished" << "\n";
    }
    
    void copyQuote(const Quote& x, Quote& y) {
        if ( _verbose )
            LOGGER_INFO << "<==> QUOTE_TO_COPY x --> " << x.toInfoString() << " :: y --> " << y.toInfoString() << "\n";
        
        uint32_t numTrades = y._numTrades;
        uint32_t numBooks = y._numBooks;
        static_cast<QuoteWire&>(y) = static_cast<const QuoteWire&>(x);
        y._numTrades = numTrades;
        y._numBooks = numBooks;
        y._intGaps += x._intSeqNum - y._intSeqNum - 1;
        y._intSeqNum = x._intSeqNum;
        y._exTimestamp = x._exTimestamp;
        y._timestamp1 = x._timestamp1;
        y._timestamp2 = x._timestamp2;
    }
    
private:
    typedef tw::common_thread::Lock TLock;
    typedef tw::common_thread::LockGuard<TLock> TLockGuard;
    typedef tw::common_thread::Thread TThread;
    typedef tw::common_thread::ThreadPtr TThreadPtr;
    typedef tw::common_thread::ThreadPipe<bool> TThreadPipe;
    
    typedef std::tr1::unordered_map<tw::instr::Instrument::TKeyId, Quote> TQuotes;
    
private:
    bool _isMultithreaded;
    bool _verbose;
    bool _isDone;
    
    TLock _lock;    
    TThreadPtr _thread;
    TThreadPipe _threadPipe;
    
    QuoteStore& _mainQuoteStore;
    QuoteStore _threadLocalQuoteStore;        
    TQuotes _changedQuotes;
};

} // namespace price
} // namespace tw

