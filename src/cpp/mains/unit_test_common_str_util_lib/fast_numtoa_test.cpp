#include <tw/common_str_util/fast_numtoa.h>

#include <string>
#include <limits.h>
#include <math.h>

#include <gtest/gtest.h>

// Helper function - removes trailing zeros -  just for testing
//
static void stripTrailingZeros(char* buf)
{
    size_t i;
    int hasdot = 0;
    for (i = 0; i < strlen(buf); ++i) {
        if (buf[i] == '.') {
            hasdot = 1;
            break;
        }
    }

    // it's just an integer
    //
    if (!hasdot) {
        return;
    }

    i = strlen(buf);
    if (i == 0) {
        return;
    }
    --i;

    while (i > 0 && (buf[i] == '0' || buf[i] == '.')) {
        if (buf[i] == '.') {
            buf[i] = '\0';
            break;
        } else {
            buf[i] = '\0';
            --i;
        }
    }
}

TEST(CommonStrUtilLibTestSuit, fast_itoa10)
{
    char buf1[100];
    char buf2[100];
    
    int32_t l1 = 0;
    int32_t l2 = 0;
    
    for (int32_t i = 0; i < 100000; ++i) {
        l1 = sprintf(buf1, "%d", i);
        l2 = fast_itoa10(i, buf2);
        ASSERT_EQ(std::string(buf1), std::string(buf2));
        ASSERT_EQ(l1, l2);

        l1 = sprintf(buf1, "%d", -i);
        l2 = fast_itoa10(-i, buf2);
        ASSERT_EQ(std::string(buf1), std::string(buf2));
        ASSERT_EQ(l1, l2);

        l1 = sprintf(buf1, "%d", INT_MAX - i);
        l2 = fast_itoa10(INT_MAX - i, buf2);
        ASSERT_EQ(std::string(buf1), std::string(buf2));
        ASSERT_EQ(l1, l2);

        l1 = sprintf(buf1, "%d", -(INT_MAX - i));
        l2 = fast_itoa10(-(INT_MAX - i), buf2);
        ASSERT_EQ(std::string(buf1), std::string(buf2));
        ASSERT_EQ(l1, l2);
    }
}

TEST(CommonStrUtilLibTestSuit, fast_uitoa10)
{
    char buf1[100];
    char buf2[100];
    
    int32_t l1 = 0;
    int32_t l2 = 0;
    
    uint32_t i = 0;
    for (i = 0; i < 1000000; ++i) {
        l1 = sprintf(buf1, "%u", i);
        l2 = fast_uitoa10(i, buf2);
        ASSERT_EQ(std::string(buf1), std::string(buf2));
        ASSERT_EQ(l1, l2);
    }

    for (i = 0; i < 1000000; ++i) {
        l1 = sprintf(buf1, "%u", 0xFFFFFFFFu - i);
        l2 = fast_uitoa10(0xFFFFFFFFu - i, buf2);
        ASSERT_EQ(std::string(buf1), std::string(buf2));
        ASSERT_EQ(l1, l2);
    }
}

TEST(CommonStrUtilLibTestSuit, fast_litoa10)
{
    char buf1[100];
    char buf2[100];
    
    int32_t l1 = 0;
    int32_t l2 = 0;
    
    for (int64_t i = 0; i < 100000; ++i) {
        l1 = sprintf(buf1, "%ld", i);
        l2 = fast_litoa10(i, buf2);
        ASSERT_EQ(std::string(buf1), std::string(buf2));
        ASSERT_EQ(l1, l2);

        l1 = sprintf(buf1, "%ld", -i);
        l2 = fast_litoa10(-i, buf2);
        ASSERT_EQ(std::string(buf1), std::string(buf2));
        ASSERT_EQ(l1, l2);

        l1 = sprintf(buf1, "%ld", LONG_MAX - i);
        l2 = fast_litoa10(LONG_MAX - i, buf2);
        ASSERT_EQ(std::string(buf1), std::string(buf2));
        ASSERT_EQ(l1, l2);

        l1 = sprintf(buf1, "%ld", -(LONG_MAX - i));
        l2 = fast_litoa10(-(LONG_MAX - i), buf2);
        ASSERT_EQ(std::string(buf1), std::string(buf2));
        ASSERT_EQ(l1, l2);
    }
}

TEST(CommonStrUtilLibTestSuit, fast_ulitoa10)
{
    char buf1[100];
    char buf2[100];
    
    int32_t l1 = 0;
    int32_t l2 = 0;
    
    unsigned long long int i = 0;
    for (i = 0; i < 1000000; ++i) {
        l1 = sprintf(buf1, "%llu", i);
        l2 = fast_ulitoa10(i, buf2);
        ASSERT_EQ(std::string(buf1), std::string(buf2));
        ASSERT_EQ(l1, l2);
    }

    for (i = 0; i < 1000000; ++i) {
        l1 = sprintf(buf1, "%llu", 0xFFFFFFFFFFFFFFFFllu - i);
        l2 = fast_ulitoa10(0xFFFFFFFFFFFFFFFFull -i, buf2);
        ASSERT_EQ(std::string(buf1), std::string(buf2));
        ASSERT_EQ(l1, l2);
    }
}

TEST(CommonStrUtilLibTestSuit, fast_dtoa)
{
    char buf1[100];
    char buf2[100];
    
    double d = 0.0;
    
    int32_t l1 = 0;
    int32_t l2 = 0;

    // Test each combination of whole number + fraction,
    // at every precision and test negative version
    //
    double wholes[] = {0,1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0,10.0,11.0,
                       67.0,101.0, 10000, 99999};
    double frac[] = {0.0, 0.1, 0.2, 0.3, 0.4, 0.49, 0.5, 0.51, 0.6, 0.7,
                     0.9, 0.01, 0.25, 0.125, 0.05, 0.005, 0.0005, 0.00005,
                     0.001, 0.00001, 0.99, 0.999, 0.9999};
    const char* formats[] = {"%.0f", "%.1f", "%.2f", "%.3f", "%.4f", "%.5f",
                             "%.6f", "%.7f", "%.8f", "%.9f"};

    int imax = sizeof(wholes)/sizeof(double);
    int jmax = sizeof(frac)/sizeof(double);
    int kmax = sizeof(formats)/sizeof(const char*);

    int i,j,k;
    for (i = 0; i < imax; ++i) {
        for (j = 0; j < jmax; ++j) {
            for (k = 0; k < kmax; ++k) {
                d = wholes[i] + frac[j];
                
                l1 = sprintf(buf1, formats[k], d);
                l2 = fast_dtoa(d, buf2, k);
                ASSERT_EQ(std::string(buf1), std::string(buf2));
                ASSERT_EQ(l1, l2);

                if (d != 0) {
                    // not dealing with "-0" issues
                    //
                    d = -d;
                    l1 = sprintf(buf1, formats[k], d);
                    l2 = fast_dtoa(d, buf2, k);
                    ASSERT_EQ(std::string(buf1), std::string(buf2));
                    ASSERT_EQ(l1, l2);
                }
            }
        }
    }

    // Test very large positive number
    //
    d = 1.0e200;
    l1 = fast_dtoa(d, buf2, 6);    
    ASSERT_EQ(std::string("1.000000e+200"), std::string(buf2));
    ASSERT_EQ(l1, std::string("1.000000e+200").length());

    // Test very large negative number
    //
    d = -1.0e200;
    l1 = fast_dtoa(d, buf2, 6);
    ASSERT_EQ(std::string("-1.000000e+200"), std::string(buf2));
    ASSERT_EQ(l1, std::string("-1.000000e+200").length());

    // Test very small positive number
    //
    d = 1e-10;
    l1 = sprintf(buf1, "%.6f", d);
    l2 = fast_dtoa(d, buf2, 6);
    ASSERT_EQ(std::string(buf1), std::string(buf2));
    ASSERT_EQ(l1, l2);

    // Test very small negative number
    //
    d = -1e-10;
    l1 = sprintf(buf1, "%.6f", d);
    l2 = fast_dtoa(d, buf2, 6);
    ASSERT_EQ(std::string(buf1), std::string(buf2));
    ASSERT_EQ(l1, l2);
    
    // test bad precision values
    //
    d = 1.1;
    l1 = fast_dtoa(d, buf2, -1);
    ASSERT_EQ(std::string("1"), std::string(buf2));
    ASSERT_EQ(l1, std::string("1").length());
    
    l1 = fast_dtoa(d, buf2, 10);
    ASSERT_EQ(std::string("1.100000000"), std::string(buf2));
    ASSERT_EQ(l1, std::string("1.100000000").length());
}


TEST(CommonStrUtilLibTestSuit, fast_dtoa2)
{
    char buf1[100];
    char buf2[100];
    
    double d = 0.0;
    
    int32_t l1 = 0;

    // Test each combination of whole number + fraction,
    // at every precision and test negative version
    //
    double wholes[] = {0,1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0,10.0,11.0,
                       67.0,101.0, 10000, 99999};
    double frac[] = {0.0, 0.1, 0.2, 0.3, 0.4, 0.49, 0.5, 0.51, 0.6, 0.7,
                     0.9, 0.01, 0.25, 0.125, 0.05, 0.005, 0.0005, 0.00005,
                     0.001, 0.00001, 0.99, 0.999, 0.9999};
    const char* formats[] = {"%.0f", "%.1f", "%.2f", "%.3f", "%.4f", "%.5f",
                             "%.6f", "%.7f", "%.8f", "%.9f"};

    int imax = sizeof(wholes)/sizeof(double);
    int jmax = sizeof(frac)/sizeof(double);
    int kmax = sizeof(formats)/sizeof(const char*);

    int i,j,k;
    for (i = 0; i < imax; ++i) {
        for (j = 0; j < jmax; ++j) {
            for (k = 0; k < kmax; ++k) {
                d = wholes[i] + frac[j];
                
                sprintf(buf1, formats[k], d);
                stripTrailingZeros(buf1);
                l1 = fast_dtoa2(d, buf2, k);
                ASSERT_EQ(std::string(buf1), std::string(buf2));
                ASSERT_EQ(l1, std::string(buf1).length());

                if (d != 0) {
                    // not dealing with "-0" issues
                    //
                    d = -d;
                    
                    sprintf(buf1, formats[k], d);
                    stripTrailingZeros(buf1);
                    l1 = fast_dtoa2(d, buf2, k);
                    ASSERT_EQ(std::string(buf1), std::string(buf2));
                    ASSERT_EQ(l1, std::string(buf1).length());
                }
            }
        }
    }

    // test very large positive number
    //
    d = 1.0e200;
    l1 = fast_dtoa2(d, buf2, 6);
    ASSERT_EQ(std::string("1.000000e+200"), std::string(buf2));
    ASSERT_EQ(l1, std::string("1.000000e+200").length());

    // test very large negative number
    //
    d = -1.0e200;
    l1 = fast_dtoa2(d, buf2, 6);
    ASSERT_EQ(std::string("-1.000000e+200"), std::string(buf2));
    ASSERT_EQ(l1, std::string("-1.000000e+200").length());

    // test very small positive number
    //
    d = 1e-10;
    sprintf(buf1, "%.6f", d);
    stripTrailingZeros(buf1);
    
    l1 = fast_dtoa2(d, buf2, 6);
    ASSERT_EQ(std::string(buf1), std::string(buf2));
    ASSERT_EQ(l1, std::string(buf1).length());

    // test very small negative number
    //
    d = -1e-10;
    sprintf(buf1, "%.6f", d);
    stripTrailingZeros(buf1);

    l1 = fast_dtoa2(d, buf2, 6);
    ASSERT_EQ(std::string(buf1), std::string(buf2));
    ASSERT_EQ(l1, std::string(buf1).length());

    // test bad precision values
    //
    d = 1.1;
    l1 = fast_dtoa2(d, buf2, -1);
    ASSERT_EQ(std::string("1"), std::string(buf2));
    ASSERT_EQ(l1, std::string("1").length());
    
    l1 = fast_dtoa2(d, buf2, 10);
    ASSERT_EQ(std::string("1.1"), std::string(buf2));
    ASSERT_EQ(l1, std::string("1.1").length());
}


TEST(CommonStrUtilLibTestSuit, fast_litoa10_overflow_bug)
{
    char buf1[100];
    char buf2[100];
    
    int32_t l1 = 0;
    int32_t l2 = 0;

    long long int longmin = LONG_MIN;
    l1 = sprintf(buf1, "%lld", longmin);
    l2 = fast_litoa10(longmin, buf2);
    ASSERT_EQ(std::string(buf1), std::string(buf2));
    ASSERT_EQ(l1, l2);

    long long int longmax = LONG_MAX;
    l1 = sprintf(buf1, "%lld", longmax);
    l2 = fast_litoa10(longmax, buf2);
    ASSERT_EQ(std::string(buf1), std::string(buf2));
    ASSERT_EQ(l1, l2);
}


TEST(CommonStrUtilLibTestSuit, fast_itoa10_overflow_bug)
{
    char buf1[100];
    char buf2[100];
    
    int32_t l1 = 0;
    int32_t l2 = 0;

    int32_t intmin = INT_MIN;
    l1 = sprintf(buf1, "%d", intmin);
    l2 = fast_itoa10(intmin, buf2);
    ASSERT_EQ(std::string(buf1), std::string(buf2));
    ASSERT_EQ(l1, l2);

    int32_t intmax = INT_MAX;
    l1 = sprintf(buf1, "%d", intmax);
    l2 = fast_itoa10(intmax, buf2);
    ASSERT_EQ(std::string(buf1), std::string(buf2));
    ASSERT_EQ(l1, l2);
}


TEST(CommonStrUtilLibTestSuit, fast_dtoa_NaN_infinity)
{
    char buf1[100];
    char buf2[100];
    
    double d = 0.0;
    
    int32_t l1 = 0;

    // down below are some IFDEFs that may or may not exist.
    // depending on compiler settings "buf1" might not be used
    // and halt compilation.  The next line touches buf1 so this
    // doesn't happen
    //
    (void)buf1;

    // Test for infinity
    //
    d = 1e200 * 1e200;
    
    // NOTE!!! next line will core dump!
    //sprintf(buf1, "%.6f", d);
    //
    buf2[0] = '\0';
    l1 = fast_dtoa2(d, buf2, 6);
    ASSERT_EQ(std::string("inf"), std::string(buf2));
    ASSERT_EQ(l1, std::string("inf").length());
    
#ifdef INFINITY
    d = INFINITY;

    // test libc support
    //
    sprintf(buf1, "%f", d);
    ASSERT_EQ(std::string("inf"), std::string(buf1));

    // now test prop implementation
    //
    buf2[0] = '\0';
    l1 = fast_dtoa(d, buf2, 6);
    ASSERT_EQ(std::string("inf"), std::string(buf2));
    ASSERT_EQ(l1, std::string("inf").length());

    buf2[0] = '\0';
    l1 = fast_dtoa2(d, buf2, 6);
    ASSERT_EQ(std::string("inf"), std::string(buf2));
    ASSERT_EQ(l1, std::string("inf").length());
#endif

    
#ifdef NAN
    d = NAN;

    // test libc support
    //
    sprintf(buf1, "%f", d);
    ASSERT_EQ(std::string("nan"), std::string(buf1));

    // now test prop implementation
    //
    buf2[0] = '\0';
    l1 = fast_dtoa(d, buf2, 6);
    ASSERT_EQ(std::string("nan"), std::string(buf2));
    ASSERT_EQ(l1, std::string("nan").length());
    
    buf2[0] = '\0';    
    l1 = fast_dtoa2(d, buf2, 6);
    ASSERT_EQ(std::string("nan"), std::string(buf2));
    ASSERT_EQ(l1, std::string("nan").length());
#endif
}


