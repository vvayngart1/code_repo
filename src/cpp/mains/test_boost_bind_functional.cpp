#include <tw/common/defs.h>
#include <tw/log/defs.h>

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <vector>
#include <map>

class TestClient1 {
public:
    void onEvent(uint32_t v, const std::string& s) {
        LOGGER_INFO << this << " :: " << v << " :: " << s << "\n";
    }
};

class TestClient2 {
public:
    void onEvent(uint32_t v, const std::string& s) {
        LOGGER_INFO << this << " :: " << v << " :: " << s << "\n";
    }
};

class TestManager {
public:
    typedef boost::function<void (uint32_t, const std::string&)> TClient;
    typedef std::vector<TClient> TClients;
    
    template <typename TType>
    void add(TType* client) {
        _cl.push_back(boost::bind(boost::mem_fn(&TType::onEvent), client, _1, _2));
    }
    
    void onEvent(uint32_t v, const std::string& s) {
        //std::for_each(_cl.begin(), _cl.end(), std::bind2obj(std::mem_fun_ref(&TestManager::doOnEvent(v)), *this));
        TClients::iterator iter = _cl.begin();
        TClients::iterator end = _cl.end();
        
        for ( ; iter != end; ++iter ) {
            (*iter)(v, s);
        }
    }
    
    TClients _cl;
};

template <typename TType>
class Test
{
public:
    typedef std::map<int, TType> TContainer;

    void foo() {
        typename TContainer::iterator iter = _container.find(10);
    }

    TContainer _container;
};

int main(int argc, char * argv[]) {
    TestManager tm;
    
    TestClient1 t1;
    TestClient2
    t2;
    
    tm.add(&t1);
    tm.add(&t2);
    
    LOGGER_INFO << "t1: " << &t1 << "\n";
    LOGGER_INFO << "t2: " << &t2 << "\n";
    
    
    tm.onEvent(100, "hello");
    tm.onEvent(200, "hello again");
    
    return 0;
}
