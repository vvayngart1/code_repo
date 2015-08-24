#include <tw/log/defs.h>
#include <tw/functional/delegate.hpp>

#include <vector>
#include <algorithm>

struct Quote {
    long _l;
    
    template <typename TClient>
    void operator()(TClient& client) {
        client(*this);
    }
};

class TestClient1 {
public:
    void onQuote(const Quote& q) {
        LOGGER_INFO << this << " :: " << q._l << "\n";
    }
};

class TestClient2 {
public:
    void onQuote(const Quote& q) {
        LOGGER_INFO << this << " :: " << q._l << "\n";
    }
};

class TestFunctor {
public:
    TestFunctor(const Quote& q) : _q(q) {
    }

    template <typename TDelegate>
    void operator()(TDelegate& delegate) {
        delegate(_q);
    }

    const Quote& _q;
};


class TestBase {
public:
    TestBase() {        
    }
    
    virtual ~TestBase() {        
    }
    
public:
    virtual void foo() = 0;
    
};

class TestD1 : public TestBase {
public:
    virtual void foo() {
        LOGGER_INFO << this << "\n";
    }
    
    void bar(uint32_t i) {
        LOGGER_INFO << this << " :: " << i+1 << "\n";
    }
};

class TestD2 : public TestBase {
public:
    virtual void foo() {
        LOGGER_INFO << this << "\n";
    }
    
    void bar(uint32_t i) {
        LOGGER_INFO << this << " :: " << i+2 << "\n";
    }
};

class TestContainer {
public:
    typedef std::vector<TestBase*> TClients;
    
    typedef tw::functional::delegate1<void, uint32_t> TDelegate;
    typedef std::vector<TDelegate> TDelegates;
    
public:
    template <typename TClient>
    void add(TClient* client) {
        TestBase* c = dynamic_cast<TestBase*>(client);
        if ( c == NULL )
            return;
        
        _clients.push_back(c);        
        _delegates.push_back(TDelegate::from_method<TClient, &TClient::bar>(client));
    }
    
public:
    void doFoo() {
        TClients::iterator iter = _clients.begin();
        TClients::iterator end = _clients.end();
        
        for ( ; iter!= end; ++iter ) {
            (*iter)->foo();
        }
        
    }
    
    void doBar(uint32_t i) {
        TDelegates::iterator iter = _delegates.begin();
        TDelegates::iterator end = _delegates.end();
        
        for ( ; iter!= end; ++iter ) {
            (*iter)(i);
        }
    }
    
private:
    TDelegates _delegates;
    TClients _clients;    
};

class TestManager {    
public:
    typedef tw::functional::delegate1<void, const Quote&> TDelegate;
    typedef std::vector<TDelegate> TDelegates;
    
    TDelegates _delegates;

    template <typename TClient>
    void add(TClient& client) {
        _delegates.push_back(TDelegate::from_method<TClient, &TClient::onQuote>(&client));
    }
    
    template <typename TClient>
    void rem(TClient& client) {
        TDelegate delegate = TDelegate::from_method<TClient, &TClient::onQuote>(&client);
        TDelegates::iterator iter = _delegates.begin();
        while ( iter != _delegates.end() ) {
            if ( *iter == delegate ) {
                _delegates.erase(iter);
                iter = _delegates.begin();
            } else {
                ++iter;
            }
        }
    }
    
    void onQuote(const Quote& q) {
        LOGGER_INFO << "TestFunctor() :: q._l: " << q._l << "\n";
        std::for_each(_delegates.begin(), _delegates.end(), TestFunctor(q));
        
        LOGGER_INFO << "Quote() :: q._l: " << q._l << "\n";
        std::for_each(_delegates.begin(), _delegates.end(), q);
    }
};

int main(int argc, char * argv[]) {
    Quote q;
    TestClient1 tc1;
    TestClient2 tc2;
    
    TestManager tm;
    
    tm.add(tc1);
    tm.add(tc2);
    tm.add(tc1);

    q._l = 5;
    tm.onQuote(q);

    q._l = 10;
    tm.onQuote(q);
    
    tm.rem(tc1);
    
    q._l = 15;
    tm.onQuote(q);
    
    tm.rem(tc2);
    
    q._l = 20;
    tm.onQuote(q);
    
    
    TestD1 td1;
    TestD2 td2;
    
    TestContainer testContainer;
    
    testContainer.add(&td1);
    testContainer.add(&td2);
    
    testContainer.doFoo();
    testContainer.doBar(1);

    return 0;
}

