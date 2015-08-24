#include <tw/common/defs.h>
#include <tw/common/high_res_time.h>
#include <tw/common_str_util/fast_numtoa.h>

#include <iostream>
#include <stdint.h>
#include <stdio.h>

typedef tw::common::THighResTime __prop_clock_t;

__prop_clock_t __prop_clock() {
    return __prop_clock_t::now();
}

int main()
{
    char buf[100];
    uint32_t max = 1000000;
    __prop_clock_t t0,t1;
    double d, last;
    uint32_t i;
    uint8_t ju8;
    uint16_t ju16;
    int8_t j8;
    int16_t j16;
    std::cout << "    type   \tsprintf\tsnprf\tnumtoa\timprovement\n";

    printf("unsigned 8\t");
    t0 = __prop_clock();
    for (i = 0; i < max; ++i) {
        ju8 = (uint8_t) i;
        sprintf(buf, "%hhu", ju8);
    }
    t1 = __prop_clock();
    std::cout << (t1-t0) << "\t";
    fflush(stdout);

    t0 = __prop_clock();
    for ( i = 0; i < max; ++i) {
        ju8 = (uint8_t) i;
        snprintf(buf, sizeof(buf), "%hhu", ju8);
    }
    t1 = __prop_clock();
    last = (t1-t0);
    std::cout << (t1-t0) << "\t";
    fflush(stdout);

    t0 = __prop_clock();
    for (i = 0; i < max; ++i) {
        ju8 = (uint8_t) i;
        fast_uitoa10(ju8, buf);
    }
    t1 = __prop_clock();
    std::cout << (t1-t0) << "\t";
    std::cout << last/(t1-t0) << "x\n";
    fflush(stdout);

    printf("unsigned 16\t");
    t0 = __prop_clock();
    for (i = 0; i < max; ++i) {
        ju16 = (uint16_t) i;
        sprintf(buf, "%hu", ju16);
    }
    t1 = __prop_clock();
    std::cout << (t1-t0) << "\t";    
    fflush(stdout);

    t0 = __prop_clock();
    for ( i = 0; i < max; ++i) {
        ju16 = (uint16_t) i;
        snprintf(buf, sizeof(buf), "%hu", ju16);
    }
    t1 = __prop_clock();
    last = (t1-t0);
    std::cout << (t1-t0) << "\t";    
    fflush(stdout);

    t0 = __prop_clock();
    for (i = 0; i < max; ++i) {
        ju16 = (uint16_t) i;
        fast_uitoa10(ju16, buf);
    }
    t1 = __prop_clock();
    std::cout << (t1-t0) << "\t";
    std::cout << last/(t1-t0) << "x\n";
    fflush(stdout);


    printf("unsigned 32\t");
    t0 = __prop_clock();
    for (i = 0; i < max; ++i) {
        sprintf(buf, "%u", i);
    }
    t1 = __prop_clock();
    std::cout << (t1-t0) << "\t";    
    fflush(stdout);

    t0 = __prop_clock();
    for ( i = 0; i < max; ++i) {
        snprintf(buf, sizeof(buf), "%u", i);
    }
    t1 = __prop_clock();
    last = (t1-t0);
    std::cout << (t1-t0) << "\t";
    fflush(stdout);

    t0 = __prop_clock();
    for (i = 0; i < max; ++i) {
        fast_uitoa10(i, buf);
    }
    t1 = __prop_clock();
    std::cout << (t1-t0) << "\t"; 
    std::cout << last/(t1-t0) << "x\n";
    fflush(stdout);

    printf("signed 8\t");
    t0 = __prop_clock();
    for (i = 0; i < max; ++i) {
        j8 = (int8_t) i;
        sprintf(buf, "%hhd", ju8);
    }
    t1 = __prop_clock();
    std::cout << (t1-t0) << "\t";    
    fflush(stdout);

    t0 = __prop_clock();
    for ( i = 0; i < max; ++i) {
        j8 = (int8_t) i;
        snprintf(buf, sizeof(buf), "%hhd", j8);
    }
    t1 = __prop_clock();
    last = (t1-t0);
    std::cout << (t1-t0) << "\t";    
    fflush(stdout);

    t0 = __prop_clock();
    for (i = 0; i < max; ++i) {
        j8 = (int8_t) i;
        fast_itoa10(j8, buf);
    }
    t1 = __prop_clock();
    std::cout << (t1-t0) << "\t";
    std::cout << last/(t1-t0) << "x\n";
    fflush(stdout);

    printf("signed 16\t");
    t0 = __prop_clock();
    for (i = 0; i < max; ++i) {
        j16 = (int16_t) i;
        sprintf(buf, "%hd", j16);
    }
    t1 = __prop_clock();
    std::cout << (t1-t0) << "\t";    
    fflush(stdout);

    t0 = __prop_clock();
    for ( i = 0; i < max; ++i) {
        j16 = (int16_t) i;
        snprintf(buf, sizeof(buf), "%hd", j16);
    }
    t1 = __prop_clock();
    last = (t1-t0);
    std::cout << (t1-t0) << "\t";    
    fflush(stdout);

    t0 = __prop_clock();
    for (i = 0; i < max; ++i) {
        j16 = (int16_t) i;
        fast_itoa10(j16, buf);
    }
    t1 = __prop_clock();
    std::cout << (t1-t0) << "\t";
    std::cout << last/(t1-t0) << "x\n";
    fflush(stdout);

    printf("%s", "signed 32\t");
    fflush(stdout);

    t0 = __prop_clock();
    for (i = 0; i < max; ++i) {
        sprintf(buf, "%d", i);
    }
    t1 = __prop_clock();
    std::cout << (t1-t0) << "\t";    
    fflush(stdout);

    t0 = __prop_clock();
    for ( i = 0; i < max; ++i) {
        snprintf(buf, sizeof(buf), "%d", i);
    }
    t1 = __prop_clock();
    last = (t1-t0);
    std::cout << (t1-t0) << "\t";
    fflush(stdout);

    t0 = __prop_clock();
    for (i = 0; i < max; ++i) {
        fast_itoa10(i, buf);
    }
    t1 = __prop_clock();
    std::cout << (t1-t0) << "\t";
    std::cout << last/(t1-t0) << "x\n";
    fflush(stdout);

    printf("%s", "\n    type   \t%e\t%f\t%g\tdtoa\timprovement\n");
    printf("%s", "double\t\t");
    t0 = __prop_clock();
    for (i = 0; i < max; ++i) {
        sprintf(buf, "%e", (double)(i));
    }
    t1 = __prop_clock();
    std::cout << (t1-t0) << "\t";    
    fflush(stdout);

    t0 = __prop_clock();
    for (i = 0; i < max; ++i) {
        d = (double)i + 0.1;
        snprintf(buf, sizeof(buf), "%f", d);
    }
    t1 = __prop_clock();
    last = (t1-t0);
    std::cout << (t1-t0) << "\t";    
    fflush(stdout);

    t0 = __prop_clock();
    for (i = 0; i < max; ++i) {
        d = (double)i + 0.123456;
        snprintf(buf, sizeof(buf), "%g", d);
    }
    t1 = __prop_clock();
    last = (t1-t0);
    std::cout << (t1-t0) << "\t";
    fflush(stdout);

    t0 = __prop_clock();
    for (i = 0; i < max; ++i) {
        d = (double)i + 0.123456;
        fast_dtoa(d, buf, 6);
    }
    t1 = __prop_clock();
    std::cout << (t1-t0) << "\t";
    std::cout << last/(t1-t0) << "x\n";
    fflush(stdout);

    return 0;
}
