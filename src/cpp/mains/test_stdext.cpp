#include <tw/common/defs.h>

#include <iostream>
#include <map>
#include <algorithm>
#include <stdlib.h>
#include <time.h>

#include <tw/common/command.h>

typedef std::map<int, int> TMap;

struct TestPrinter1 {
    typedef void result_type;
    void operator()(const TMap::value_type::first_type& v) const {
        std::cout << "print0: == " << "first :: " << v << "\n";
    }
};

struct TestPrinter2 {
    void operator()(const TMap::value_type::second_type& v) {
        std::cout << "print0: == " << "second :: " << v << "\n";
    }
};

struct Test {
  
  TMap _map;
  
  Test() {
      _map[1] = 1;
      _map[2] = 2;
      _map[3] = 3;
      _map[4] = 4;
      _map[5] = 5;        
  }
  
  void operator()(const TMap::value_type& v) {
      print1(v);
  }
  
  void print1(const TMap::value_type& v) {
      std::cout << "print1: == " << v.first << " :: " << v.second << "\n";
  }
  
  void print2(const TMap::value_type& v) {
      std::cout << "print2: == " << v.first << " :: " << v.second << "\n";
  }
  
  void print3(const TMap::value_type& v) {
      std::cout << "print3: == " << v.first << " :: " << v.second << "\n";
  }
  
  void print4(const TMap::value_type& v) const {
      std::cout << "print4: == " << v.first << " :: " << v.second << "\n";
  }  
  
  void printAll() {
      TestPrinter1 tp1;
      
      std::cout << "Printing 'first' using TestPrinter::operator()" << "\n";
      std::for_each(_map.begin(), _map.end(), stdext::compose1(tp1, stdext::select1st<TMap::value_type>()));      
      
      std::cout << "Printing using &Test::operator()" << "\n";
      std::for_each(_map.begin(), _map.end(), *this);       
      
      std::cout << "Printing using &Test::print1()" << "\n";
      std::for_each(_map.begin(), _map.end(), std::bind2obj(std::mem_fun_ref(&Test::print1), *this));
      
      std::cout << "Printing using &Test::print2()" << "\n";
      std::for_each(_map.begin(), _map.end(), std::bind2obj(std::mem_fun_ref(&Test::print2), *this));
  }
  
};

void print(const Test& t) {
    std::cout << "Printing using &Test::print4() const" << "\n";    
    std::for_each(t._map.begin(), t._map.end(), std::bind2obj(std::mem_fun_ref(&Test::print4), t));
}



struct ActivePriceParamsWire {
    static const size_t FIELDS_NUM=2-0;

    ActivePriceParamsWire() {
        clear();
    }

    void clear() {               
       _leanRatio=float();        
       _leanQtyDelta=float();
    }

    static ActivePriceParamsWire fromCommand(const tw::common::Command& cmnd) {
        ActivePriceParamsWire x;
        
        if ( cmnd.has("leanRatio") )
            cmnd.get("leanRatio", x._leanRatio);
        
        if ( cmnd.has("leanQtyDelta") )
            cmnd.get("leanQtyDelta", x._leanQtyDelta);            
    
        return x;
    }
    
    tw::common::Command toCommand() const {
        tw::common::Command cmnd;
        
        cmnd.addParams("leanRatio", _leanRatio);            
        cmnd.addParams("leanQtyDelta", _leanQtyDelta);
        
        return cmnd;
    }        
    
    std::string toString() const {
        std::stringstream out;        
        out << _leanRatio << FIELDS_DELIM ;
        out << _leanQtyDelta << FIELDS_DELIM ;        
        return out.str();
    }
    
    std::string toStringVerbose() const {
        std::stringstream out;
        
	 out << "leanRatio=" << _leanRatio << "\n";         
         out << "leanQtyDelta=" << _leanQtyDelta << "\n";         
        
        return out.str();
    }
    
    friend tw::log::StreamDecorator& operator<<(tw::log::StreamDecorator& os, const ActivePriceParamsWire& x) {
        return os << x.toString();
    }

    friend std::ostream& operator<<(std::ostream& os, const ActivePriceParamsWire& x) {
        return os << x.toString();
    }    
    
    float _leanRatio;    
    float _leanQtyDelta;
};

struct ActivePriceParamsWireEnter : public ActivePriceParamsWire {
    typedef ActivePriceParamsWire TParent;
    ActivePriceParamsWireEnter() : _leanRatio_enter(TParent::_leanRatio),
                                   _leanQtyDelta_enter(TParent::_leanQtyDelta) {        
    }        

    static ActivePriceParamsWireEnter fromCommand(const tw::common::Command& cmnd) {
        ActivePriceParamsWireEnter x;
        
        if ( cmnd.has("leanRatio_enter") )
            cmnd.get("leanRatio_enter", x._leanRatio_enter);
        
        if ( cmnd.has("leanQtyDelta_enter") )
            cmnd.get("leanQtyDelta_enter", x._leanQtyDelta_enter);            
    
        return x;
    }
    
    tw::common::Command toCommand() const {
        tw::common::Command cmnd;
        
        cmnd.addParams("leanRatio_enter", _leanRatio_enter);            
        cmnd.addParams("leanQtyDelta_enter", _leanQtyDelta_enter);
        
        return cmnd;
    }
    
    std::string toStringVerbose() const {
        std::stringstream out;
        
	 out << "leanRatio_enter=" << _leanRatio_enter << "\n";         
         out << "leanQtyDelta_enter=" << _leanQtyDelta_enter << "\n";         
        
        return out.str();
    }
    
    float& _leanRatio_enter;    
    float& _leanQtyDelta_enter;
};


struct ActivePriceParamsWireExit : public ActivePriceParamsWire {
    typedef ActivePriceParamsWire TParent;
    ActivePriceParamsWireExit() : _leanRatio_exit(TParent::_leanRatio),
                                  _leanQtyDelta_exit(TParent::_leanQtyDelta) {
    }

    static ActivePriceParamsWireExit fromCommand(const tw::common::Command& cmnd) {
        ActivePriceParamsWireExit x;
        
        if ( cmnd.has("leanRatio_exit") )
            cmnd.get("leanRatio_exit", x._leanRatio_exit);
        
        if ( cmnd.has("leanQtyDelta_exit") )
            cmnd.get("leanQtyDelta_exit", x._leanQtyDelta_exit);            
    
        return x;
    }
    
    tw::common::Command toCommand() const {
        tw::common::Command cmnd;
        
        cmnd.addParams("leanRatio_exit", _leanRatio_exit);            
        cmnd.addParams("leanQtyDelta_exit", _leanQtyDelta_exit);
        
        return cmnd;
    }
    
    std::string toStringVerbose() const {
        std::stringstream out;
        
	 out << "leanRatio_exit=" << _leanRatio_exit << "\n";         
         out << "leanQtyDelta_exit=" << _leanQtyDelta_exit << "\n";         
        
        return out.str();
    }
    
    float& _leanRatio_exit;    
    float& _leanQtyDelta_exit;
};


struct BWaveParams : public ActivePriceParamsWireEnter, public ActivePriceParamsWireExit
{
    
};

void printAsParent(ActivePriceParamsWire& p) {
    std::cout << p.toStringVerbose() << "\n";
}

void printAsEnter(ActivePriceParamsWireEnter& p) {
    std::cout << p.toStringVerbose() << "\n";
}

void printAsExit(ActivePriceParamsWireExit& p) {
    std::cout << p.toStringVerbose() << "\n";
}

struct Test2 {
    Test2() : _v1(0) {        
    }
    
    int32_t _v1;
};

typedef boost::shared_ptr<Test2> TTest2Ptr;

struct Test3 {
    Test3() : _v1(0),
              _v2()
    {
    }
    
    int32_t _v1;
    TTest2Ptr _v2;
};

typedef std::list<Test3> TTests; 


int main(int argc, char * argv[]) {    
    // Test std::list erase
    //
    TTests tests;
    
    Test3 t1;
    t1._v1 = 1;
    t1._v2.reset(new Test2());
    t1._v2->_v1 = 5;
    
    Test3 t2;
    t2._v1 = 2;
    t2._v2.reset(new Test2());
    t2._v2->_v1 = 2;
    
    Test3 t3;
    t3._v1 = 3;
    t3._v2.reset(new Test2());
    t3._v2->_v1 = 5;
    
    
    Test3 t4;
    t4._v1 = 3;
    t4._v2.reset(new Test2());
    t4._v2->_v1 = 5;
    
    Test3 t5;
    t5._v1 = 3;
    t5._v2.reset(new Test2());
    t5._v2->_v1 = 7;
    
    tests.push_back(t1);
    tests.push_back(t2);
    tests.push_back(t3);
    tests.push_back(t4);
    tests.push_back(t5);
    
    for ( TTests::iterator iter = tests.begin(); iter != tests.end();  ) {
        TTest2Ptr test2 = (*iter)._v2;
        if ( test2 && test2->_v1 == 5 ) {
            std::string s = boost::lexical_cast<std::string>(test2->_v1);
            iter = tests.erase(iter);
            std::cout << "erased: " << s << "\n";
        } else {
            ++iter;
        }
    }
    
    for ( TTests::iterator iter = tests.begin(); iter != tests.end(); ) {
        TTest2Ptr test2 = (*iter)._v2;
        if ( test2 && test2->_v1 == 2 ) {
            std::string s = boost::lexical_cast<std::string>(test2->_v1);
            iter = tests.erase(iter);
            std::cout << "erased: " << s << "\n";
        } else {
            ++iter;
        }
    }
    
    for ( TTests::iterator iter = tests.begin(); iter != tests.end(); ) {
        TTest2Ptr test2 = (*iter)._v2;
        if ( test2 && test2->_v1 == 7 ) {
            std::string s = boost::lexical_cast<std::string>(test2->_v1);
            iter = tests.erase(iter);
            std::cout << "erased: " << s << "\n";
        } else {
            ++iter;
        }
    }
    
    
    
    BWaveParams p;
    p._leanQtyDelta_enter = 1.2;
    p._leanRatio_enter = 1.3;
    p._leanQtyDelta_exit = 2.2;
    p._leanRatio_exit = 2.3;
    printAsParent(static_cast<ActivePriceParamsWireEnter&>(p));
    printAsParent(static_cast<ActivePriceParamsWireExit&>(p));
    printAsEnter(p);
    printAsExit(p);
    
    std::cout << "ActivePriceParamsWireEnter: " << static_cast<ActivePriceParamsWireEnter&>(p) << "\n";
    std::cout << "ActivePriceParamsWireExit: " << static_cast<ActivePriceParamsWireExit&>(p) << "\n";
    
    Test t;
    t.printAll();
    
    std::cout << "Printing using &Test::print3()" << "\n";
    std::for_each(t._map.begin(), t._map.end(), std::bind2obj(std::mem_fun_ref(&Test::print3), t));
    
    // Initialize random seed
    //
    srand (time(NULL));
    
    for ( uint32_t i = 0; i < 10; ++i ) {
        std::cout << "dice_roll: " << i << "--" << (rand()%6+1) << "\n";
    }
    
    std::map<int32_t, uint32_t> ps;
    
    ps[32] = 320;
    ps[31] = 310;
    ps[34] = 340;
    ps[33] = 330;
    
    std::cout << "Forward iteration" << std::endl;
    for ( std::map<int32_t, uint32_t>::iterator iter = ps.begin(); iter != ps.end(); ++iter ) {
        std::cout << "ps[" << iter->first << "]=" << iter->second << std::endl;
    }
    
    std::cout << "Reverse iteration" << std::endl;
    for ( std::map<int32_t, uint32_t>::reverse_iterator iter = ps.rbegin(); iter != ps.rend(); ++iter ) {
        std::cout << "ps[" << iter->first << "]=" << iter->second << std::endl;
    }
    
    std::string temp = "ChannelOr,Alert,strategyId=1158;text=1158::ZCN5_ESM5_a1f57f9a-7991-11e4-9112-90d8871f04b6 -- ExitClose on stop(): ==> stopped strategy on disconnect of all connections;type=StratRegimeChange";
    std::cout << "temp" << std::endl;
    
    tw::common::Command cmnd;
    if ( !cmnd.fromString(temp) )
        std::cout << "Failed to parse command to string" << std::endl;
    else
        std::cout << "Parsed command to string: " << cmnd.toString() << std::endl;
    
    return 0;
}
