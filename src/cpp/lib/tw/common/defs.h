#pragma once

#include <stdint.h>
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include <boost/noncopyable.hpp>
#include <boost/algorithm/string.hpp>
#include <functional>
#include <algorithm>
#include <ext/functional>

#define FIELDS_DELIM ","

typedef boost::noncopyable noncopyable;

namespace stdext
{
    using namespace __gnu_cxx;  // oooooh, scandalous!
    
    template<typename _Key>
    struct hash {
        size_t operator()(const _Key* x) const {
            return (size_t)(x);
        }
    };        
}

namespace std
{
static std::string replace_all(const std::string& source, const std::string& search, const std::string& replace) {
    std::string s = source;
    boost::replace_all(s, search, replace);
    return s;
}
    
static std::string to_lower(const std::string& x) {
    std::string i_x = x;
    std::transform(i_x.begin(), i_x.end(), i_x.begin(), ::tolower);
    return i_x;
}

static std::string to_upper(const std::string& x) {
    std::string i_x = x;
    std::transform(i_x.begin(), i_x.end(), i_x.begin(), ::toupper);
    return i_x;
}

static std::string trim(const std::string& x) {
    return boost::algorithm::trim_copy(x);
}

struct iless : binary_function <std::string,std::string,bool> {
    bool operator() (const std::string& x, const std::string& y) const {
        return ( to_lower(x) < to_lower(y) );
    }
};
    
template <class TOperation>
class binder2obj {
protected:
    TOperation _op;
    typename TOperation::first_argument_type& _obj;
        
public:
    binder2obj(const TOperation& op,
               typename TOperation::first_argument_type& obj) : _op(op), _obj(obj) {
    }

    typename TOperation::result_type
    operator()(const typename TOperation::second_argument_type args) const {
        return _op(_obj, args);
    }
};

template <class TOperation>
class binder2obj_const {
protected:
    TOperation _op;
    const typename TOperation::first_argument_type& _obj;
        
public:
    binder2obj_const(const TOperation& op,
                     const typename TOperation::first_argument_type& obj) : _op(op), _obj(obj) {
    }

    typename TOperation::result_type
    operator()(typename TOperation::second_argument_type args) const {
        return _op(_obj, args);
    }
};

template <class TOperation, class TObj>
inline binder2obj<TOperation> bind2obj(const TOperation& op, TObj& obj) {
    return binder2obj<TOperation>(op, obj);
}

template <class TOperation, class TObj>
inline binder2obj_const<TOperation> bind2obj(const TOperation& op, const TObj& obj) {
    return binder2obj_const<TOperation>(op, obj);
}

struct alloc {
    template <typename T>
    void operator()(T*& p) {
        p = new T();
    }
};

struct dealloc {
    template <typename T>
    void operator()(T*& p) {
        delete p;
        p = NULL;
    }
};

}
