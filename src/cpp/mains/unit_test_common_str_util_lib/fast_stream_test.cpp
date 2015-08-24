#include <tw/common_str_util/fast_stream.h>

#include <gtest/gtest.h>

#define CAPACITY 40L

#define FORMAT_STRING1 "%.10d::%.10d::"
#define FORMAT_STRING_OFFSET1 24

#define FORMAT_STRING2 "%.4d::%.4d::"
#define FORMAT_STRING_OFFSET2 12

typedef tw::common_str_util::FastStream<CAPACITY, true> TStreamZeroes;
typedef tw::common_str_util::FastStream<CAPACITY> TStream;

TEST(CommonStrUtilLibTestSuit, fast_stream_double_precision_zeroes_print)
{   
    TStreamZeroes stream;
    std::string str;
    double d = 0.0;
    int32_t size = 0L;

    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);

    stream << 4L;
    size += 1L;
    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("4"), std::string(stream.c_str()));

    stream << 8L;
    size += 1L;
    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("48"), std::string(stream.c_str()));

    str = "0123456789";
    stream << str;    

    size += (int32_t)str.size();
    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("480123456789"), std::string(stream.c_str()));

    d = 10885.00390625;
    stream << d;

    size += 14;
    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("48012345678910885.00390625"), std::string(stream.c_str()));

    stream << d;		

    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("48012345678910885.00390625"), std::string(stream.c_str()));

    str = "0123456789";
    stream << str;

    size += (int32_t)str.size();
    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("48012345678910885.003906250123456789"), std::string(stream.c_str()));


    str = "0123456789";
    stream << str;

    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("48012345678910885.003906250123456789"), std::string(stream.c_str()));


    str = "012";
    stream << str;
    size += (int32_t)str.size();

    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("48012345678910885.003906250123456789012"), std::string(stream.c_str()));

    stream << 'A';

    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("48012345678910885.003906250123456789012"), std::string(stream.c_str()));

    stream.clear();
    size = 0L;

    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);

    stream << 4L << 8L;
    size += 2L;
    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("48"), std::string(stream.c_str()));

    stream << 8L;
    size += 1L;
    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("488"), std::string(stream.c_str()));

    int32_t i = 7;
    int16_t j = 5;

    uint32_t ui = 7;
    uint16_t uj = 5;

    float f = -4.0;

    stream << i << j << ui << uj << f;
    size += 15L;

    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("4887575-4.00000000"), std::string(stream.c_str()));

    stream.clear();
    size = 0L;

    int64_t l64 = 12345;
    stream << l64;
    size += 5L;
    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("12345"), std::string(stream.c_str()));

    uint64_t ul64 = 54321;
    stream << ul64;
    size += 5L;
    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(std::string("1234554321"), std::string(stream.c_str()));
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    
    // Test single character insertion
    //
    stream << "\n";
    size += 1L;
    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(std::string("1234554321\n"), std::string(stream.c_str()));
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    
    str = "\n";
    stream << str;
    size += 1L;
    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(std::string("1234554321\n\n"), std::string(stream.c_str()));
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    

    // Test formatted input
    //
    stream.clear();
    size = 0L;

    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);

    stream.sprintf(FORMAT_STRING1, 1, 40);
    size += FORMAT_STRING_OFFSET1;

    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("0000000001::0000000040::"), std::string(stream.c_str()));

    stream.sprintf(FORMAT_STRING1, 1, 40);

    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("0000000001::0000000040::"), std::string(stream.c_str()));

    stream.sprintf(FORMAT_STRING2, 1, 40);
    size += FORMAT_STRING_OFFSET2;

    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("0000000001::0000000040::0001::0040::"), std::string(stream.c_str()));

    stream.write("0123456789", 10);

    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("0000000001::0000000040::0001::0040::"), std::string(stream.c_str()));

    stream.write("0123456789", 2);
    size += 2;

    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("0000000001::0000000040::0001::0040::01"), std::string(stream.c_str()));

    TStreamZeroes stream2(stream);

    ASSERT_EQ(stream2.capacity(), CAPACITY-1);
    ASSERT_EQ(stream2.size(), size);
    ASSERT_EQ(stream2.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("0000000001::0000000040::0001::0040::01"), std::string(stream2.c_str()));

    TStreamZeroes stream3;
    stream3 = stream;

    ASSERT_EQ(stream3.capacity(), CAPACITY-1);
    ASSERT_EQ(stream3.size(), size);
    ASSERT_EQ(stream3.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("0000000001::0000000040::0001::0040::01"), std::string(stream3.c_str()));
}

TEST(CommonStrUtilLibTestSuit, fast_stream_double_precision_zeroes_no_print)
{   
    TStream stream;
    std::string str;
    double d = 0.0;
    int32_t size = 0L;

    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);

    stream << 4L;
    size += 1L;
    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("4"), std::string(stream.c_str()));

    stream << 8L;
    size += 1L;
    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("48"), std::string(stream.c_str()));

    str = "0123456789";
    stream << str;    

    size += (int32_t)str.size();
    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("480123456789"), std::string(stream.c_str()));

    d = 10885.00390625;
    stream << d;

    size += 14;
    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("48012345678910885.00390625"), std::string(stream.c_str()));

    stream << d;		

    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("48012345678910885.00390625"), std::string(stream.c_str()));

    str = "0123456789";
    stream << str;

    size += (int32_t)str.size();
    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("48012345678910885.003906250123456789"), std::string(stream.c_str()));


    str = "0123456789";
    stream << str;

    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("48012345678910885.003906250123456789"), std::string(stream.c_str()));


    str = "012";
    stream << str;
    size += (int32_t)str.size();

    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("48012345678910885.003906250123456789012"), std::string(stream.c_str()));

    stream << 'A';

    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("48012345678910885.003906250123456789012"), std::string(stream.c_str()));

    stream.clear();
    size = 0L;

    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);

    stream << 4L << 8L;
    size += 2L;
    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("48"), std::string(stream.c_str()));

    stream << 8L;
    size += 1L;
    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("488"), std::string(stream.c_str()));

    int32_t i = 7;
    int16_t j = 5;

    uint32_t ui = 7;
    uint16_t uj = 5;

    float f = -4.0;

    stream << i << j << ui << uj << f;
    size += 6L;

    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("4887575-4"), std::string(stream.c_str()));

    stream.clear();
    size = 0L;

    int64_t l64 = 12345;
    stream << l64;
    size += 5L;
    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("12345"), std::string(stream.c_str()));

    uint64_t ul64 = 54321;
    stream << ul64;
    size += 5L;
    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(std::string("1234554321"), std::string(stream.c_str()));
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    
    // Test single character insertion
    //
    stream << "\n";
    size += 1L;
    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(std::string("1234554321\n"), std::string(stream.c_str()));
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    
    str = "\n";
    stream << str;
    size += 1L;
    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(std::string("1234554321\n\n"), std::string(stream.c_str()));
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    

    // Test formatted input
    //
    stream.clear();
    size = 0L;

    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);

    stream.sprintf(FORMAT_STRING1, 1, 40);
    size += FORMAT_STRING_OFFSET1;

    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("0000000001::0000000040::"), std::string(stream.c_str()));

    stream.sprintf(FORMAT_STRING1, 1, 40);

    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("0000000001::0000000040::"), std::string(stream.c_str()));

    stream.sprintf(FORMAT_STRING2, 1, 40);
    size += FORMAT_STRING_OFFSET2;

    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("0000000001::0000000040::0001::0040::"), std::string(stream.c_str()));

    stream.write("0123456789", 10);

    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("0000000001::0000000040::0001::0040::"), std::string(stream.c_str()));

    stream.write("0123456789", 2);
    size += 2;

    ASSERT_EQ(stream.capacity(), CAPACITY-1);
    ASSERT_EQ(stream.size(), size);
    ASSERT_EQ(stream.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("0000000001::0000000040::0001::0040::01"), std::string(stream.c_str()));

    TStream stream2(stream);

    ASSERT_EQ(stream2.capacity(), CAPACITY-1);
    ASSERT_EQ(stream2.size(), size);
    ASSERT_EQ(stream2.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("0000000001::0000000040::0001::0040::01"), std::string(stream2.c_str()));

    TStream stream3;
    stream3 = stream;

    ASSERT_EQ(stream3.capacity(), CAPACITY-1);
    ASSERT_EQ(stream3.size(), size);
    ASSERT_EQ(stream3.remained_capacity(), CAPACITY-1-size);
    ASSERT_EQ(std::string("0000000001::0000000040::0001::0040::01"), std::string(stream3.c_str()));
}

TEST(CommonStrUtilLibTestSuit, fast_stream_double_precision_setting)
{   
    TStream stream;    
    double d = 0.0;
    
    d = 1.234567;
    
    stream << std::setprecision(4) << d << " :: " << std::setprecision(6) << d;
    ASSERT_EQ(std::string("1.2346 :: 1.234567"), std::string(stream.c_str()));
}
