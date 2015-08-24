#pragma once

#include <tw/common/defs.h>
#include <tw/common_comm/tcp_ip_server.h>
#include <tw/log/defs.h>
#include <tw/generated/enums_common.h>

#include <map>

namespace tw {
namespace common {
    
static const char* PARAMS_DELIM = ";";
static const char* PARAMS_SEPAR = "=";
static const size_t FIELDS_NUM=2;

struct is_any_of_find_once_counter {
    is_any_of_find_once_counter() : _count(0) {
    }
    
    uint32_t _count;
};

class is_any_of_find_once { 
public:
    is_any_of_find_once(const std::string& range, is_any_of_find_once_counter* counter) : _range(range),
                                                                                          _counter(counter) {
    }            

    bool operator()(char c) const {
        if ( _counter && 0 < _counter->_count )
            return false;

        if ( std::string::npos == _range.find(c) )
            return false;

        if ( _counter )
            ++((const_cast<is_any_of_find_once&>(*this))._counter->_count);
        return true;
    }   

private:
    std::string _range;
    is_any_of_find_once_counter* _counter;
};
    
struct Command {
    typedef std::map<std::string, std::string, std::iless> TParams;
    
    
    
    Command() { 
        clear();
    }

    void clear() {
        _type = eCommandType();
        _subType = eCommandSubType();
        _params.clear();
        _connectionId = tw::common_comm::TConnectionId();
        _s.clear();
        _s.setPrecision(4);
    }
    
    Command& operator+=(const Command& rhs) {
        TParams::const_iterator iter = rhs._params.begin();
        TParams::const_iterator end = rhs._params.end();
        
        for ( ; iter != end; ++iter ) {
            addParams(iter->first, iter->second);
        }
        
        return *this;
    }
    
    std::string toString() const {
        std::stringstream message;
        
        message << _type << FIELDS_DELIM << _subType << FIELDS_DELIM;
        TParams::const_iterator iter = _params.begin();
        TParams::const_iterator end = _params.end();
        
        for ( ; iter != end; ++iter ) {
            if ( iter != _params.begin() )
                message << PARAMS_DELIM;
            
            message << iter->first << PARAMS_SEPAR << iter->second;
        }
        
        return message.str();
    }
    
    bool fromString(const std::string& line) {
        std::vector<std::string> values;
        boost::split(values, line, boost::is_any_of(FIELDS_DELIM));
	
        size_t s1 = values.size();
        size_t s2 = FIELDS_NUM;
        if ( s1 < s2 ) 
            return false;
            
        std::for_each(values.begin(), values.end(),
              boost::bind(boost::algorithm::trim<std::string>, _1, std::locale()));              

        for ( size_t counter = 0; counter < s1; ++counter ) {
            if ( !values[counter].empty() ) {
                switch (counter) {
                    case 0:
                        _type = boost::lexical_cast<eCommandType>(values[counter]);
                        break;    
                    case 1:
                        _subType = boost::lexical_cast<eCommandSubType>(values[counter]);
                        break;
                    case 2:
                        if ( !parseParams(values[counter]) )
                            return false;
                } 
            }
        }

        return true;
    }
    
    bool parseParams(const std::string& line) {    
        std::vector<std::string> values;
        boost::split(values, line, boost::is_any_of(PARAMS_DELIM));
            
        std::for_each(values.begin(), values.end(),
              boost::bind(boost::algorithm::trim<std::string>, _1, std::locale()));

        for ( size_t counter = 0; counter < values.size(); ++counter ) {
            std::vector<std::string> param;
            
            is_any_of_find_once_counter c;
            boost::split(param, values[counter], is_any_of_find_once(PARAMS_SEPAR, &c));
            
            if ( param.size() != 2 ) {
                LOGGER_ERRO << "corrupted param: " << values[counter] << "\n";
                return false;
            }
            
            if ( has(param[0]) ) {
                LOGGER_ERRO << "params with the same name are NOT supported: " << param[0] << "\n";
                return false;
            }
            
            _params[param[0]] = param[1];
        }
        
        return true;
    }
    
    template <typename TType>
    void addParams(const std::string& name, const TType& value) {
        _params[name] = boost::lexical_cast<std::string>(value);
    }
    
    void addParams(const std::string& name, const float& value) {
        addParams(name, static_cast<const double&>(value));
    }
    
    void addParams(const std::string& name, const double& value) {
        _s.clear();
        _s << value;
        _params[name] = _s.str();
    }

    bool has(const std::string& name) const {
        if ( _params.find(name) == _params.end() )
            return false;

        return true;
    }
    
    template <typename TValue>
    bool get(const std::string& name, TValue& value) const {
        if ( !has(name) )
            return false;
        
        try {
            value = boost::lexical_cast<TValue>(_params.find(name)->second);
            return true;
        } catch(...) {
            LOGGER_ERRO << "Exception converting param: " << name << "\n";
        }
        
        return false;
    }

    eCommandType _type;
    eCommandSubType _subType;
    TParams _params;
    tw::common_comm::TConnectionId _connectionId;
    tw::common_str_util::FastStream<256> _s;
};

} // namespace common
} // namespace tw





