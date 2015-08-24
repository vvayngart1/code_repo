#include <tw/common_trade/Defs.h>

#include <deque>

#include <gtest/gtest.h>

namespace tw {
namespace common_trade {
    
const double EPSILON = 0.0001;

typedef std::deque<double> TArray;

TEST(CommonTradeLibTestSuit, corr_test)
{
    TArray x;
    TArray y;
    
    x.push_back(9255.843096);
    x.push_back(9255.85808);
    x.push_back(9255.861163);
    x.push_back(9255.861513);
    x.push_back(9255.862161);
    
    y.push_back(10152.84615);
    y.push_back(10152.84615);
    y.push_back(10152.89842);
    y.push_back(10152.9559);
    y.push_back(10152.95689);
    
    ASSERT_NEAR(FinCalc::calcCorrel(x, y), 0.693114, EPSILON);
    
    x.push_back(9255.862486);
    
    y.push_back(10152.97017);
    
    ASSERT_NEAR(FinCalc::calcCorrel(x, y), 0.718721, EPSILON);
    
}



} // common_trade
} // tw
