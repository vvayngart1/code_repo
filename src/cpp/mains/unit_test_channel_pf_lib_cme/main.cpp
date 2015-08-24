
#include <tw/channel_pf_cme/channel_pf_onix.h>
#include <gtest/gtest.h>

TEST(ChannelPfCmeLibTestSuite, channelIdToInt) {
    //test channelIdToInt
    OnixS::CME::MarketData::ChannelId id_1_digit("1");
    OnixS::CME::MarketData::ChannelId id_2_digit("11");
    OnixS::CME::MarketData::ChannelId id_3_digit("111");
    
    int id1 = tw::channel_pf_cme::channelIdToInt(id_1_digit);
    EXPECT_EQ(1, id1);

    int id2 = tw::channel_pf_cme::channelIdToInt(id_2_digit);
    EXPECT_EQ(11, id2);

    int id3 = tw::channel_pf_cme::channelIdToInt(id_3_digit);
    EXPECT_EQ(111, id3);
}
