#pragma once

#include <tw/common/defs.h>

#include <algorithm>

#include <boost/array.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/count.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/extended_p_square.hpp>

namespace tw {
namespace common {
    
using namespace boost::accumulators;
static const boost::array<double,8> PROBS = {{ 0.01, 0.1, 0.25, 0.5, 0.75, 0.9, 0.99, 0.999 }};

template <typename TType=int64_t>
class StatsManager {
    
    typedef accumulator_set<TType, stats<tag::count, tag::max, tag::extended_p_square> > TStat;
    typedef std::pair<uint32_t, TStat> TStatInfo;
    typedef std::pair<std::string, TStatInfo> TNameStatInfo;
    typedef std::vector<TNameStatInfo> TStats;
    
public:
    StatsManager() {
    }
    
    std::string toString() const {
        std::stringstream s;
        for ( size_t i = 0; i < _stats.size(); ++i ) {
            const TNameStatInfo& statInfo = _stats[i];
            const TStat& stat = statInfo.second.second;
            
            if ( i > 0 )
                s << " <==> ";
                    
            s << "name=" << statInfo.first;
            s << ",count=" << statInfo.second.first;
            s << ",p010=" << extended_p_square(stat)[0];
            s << ",p100=" << extended_p_square(stat)[1];
            s << ",p250=" << extended_p_square(stat)[2];
            s << ",p500=" << extended_p_square(stat)[3];
            s << ",p750=" << extended_p_square(stat)[4];
            s << ",p900=" << extended_p_square(stat)[5];
            s << ",p990=" << extended_p_square(stat)[6];
            s << ",p999=" << extended_p_square(stat)[7];
            s << ",max=" << max(stat);
        }
        
        return s.str();
    }
    
    void clearStats() {
        for ( size_t i = 0; i < _stats.size(); ++i ) {
            TStatInfo& stat = _stats[i].second;
            stat.first = 0;
            stat.second = TStat(tag::extended_p_square::probabilities = PROBS);
        }
    }

public:
    size_t registerNewStat(const std::string& name) {
        _stats.push_back(TNameStatInfo(name, TStatInfo(0, TStat(tag::extended_p_square::probabilities = PROBS))));
        return (_stats.size()-1);
    }
    
    uint32_t addValue(size_t index, const TType& value) {
        if ( index > (_stats.size()-1) )
            return 0;
        
        TStatInfo& stat = _stats[index].second;
        stat.second(value);
        return ++(stat.first);
    }
    
    uint32_t getCount(size_t index) {
        if ( index > (_stats.size()-1) )
            return 0;
        
        TStatInfo& stat = _stats[index].second;
        return (stat.first);
    }
    
private:
    TStats _stats;
};

} // namespace common
} // namespace tw
