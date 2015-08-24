#include <tw/log/defs.h>
#include <tw/functional/utils.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include <vector>
#include <map>
#include <algorithm>
#include <string>
#include <sstream>

template <typename TType>
class TestFunctor {
public:
    TestFunctor(std::stringstream& out) :_out(out) {        
    }
    
    void operator()(const TType& item) {
        _out << item << "\n";
    }
    
    std::stringstream& _out;
};

int main(int argc, char * argv[]) {
    std::string itemsStr = "a,b,c,d";
    std::vector<std::string> items;
    
    boost::split(items, itemsStr, boost::is_any_of(","));
    std::for_each(items.begin(), items.end(), tw::functional::printHelper<std::string>("List of items:"));
    
    typedef std::map<uint32_t, std::string> TMap;
    TMap m;
    
    m[0] = "zero";
    m[1] = "one";
    m[2] = "two";
    m[3] = "three";
    m[4] = "four";
    
    std::stringstream out;    
    
    tw::functional::for_each1st(m, TestFunctor<TMap::value_type::first_type>(out));
    LOGGER_INFO << "map's keys: " << "\n" << out.str();    
    
    out.str("");
    tw::functional::for_each2nd(m, TestFunctor<TMap::value_type::second_type>(out));
    LOGGER_INFO << "map's values: " << "\n" << out.str();
    
    return 0;
}

