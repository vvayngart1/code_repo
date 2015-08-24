#pragma once

#include <tw/log/defs.h>

namespace tw {
namespace functional
{
template <typename THeader>
class printHelper {
public:
    printHelper(const THeader& header) {
        LOGGER_INFO << header << "\n"; 
    }

    template <typename TItem>
    void operator()(const TItem& item) {
        LOGGER_INFO << item << "\n";
    }
};

template <typename TContainer, typename TFunctor>
void for_each1st(TContainer& container, TFunctor functor) {
    typename TContainer::iterator iter = container.begin();
    typename TContainer::iterator end = container.end();
    
    for ( ; iter != end; ++iter ) {
        functor(iter->first);
    }
}

template <typename TContainer, typename TFunctor>
void for_each2nd(TContainer& container, TFunctor functor) {
    typename TContainer::iterator iter = container.begin();
    typename TContainer::iterator end = container.end();
    
    for ( ; iter != end; ++iter ) {
        functor(iter->second);
    }
}

} // functional
} // tw
