#pragma once

#include <tw/common_strat/defs.h>

namespace tw {
namespace common_strat {

class ConsumerProxyImplSt {
public:
    void onQuote(const TQuote& quote) {
        quote.notifySubscribers();
    }
    
    bool onTimeout(tw::common::TimerClient* client, tw::common::TTimerId id) {
        if ( client )
            return client->onTimeout(id);
        
        return false;
    }
};
	
} // common_strat
} // tw

