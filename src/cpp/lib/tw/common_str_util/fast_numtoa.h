#pragma once

#ifdef __cplusplus
#define BEGIN_C extern "C" {
#define END_C }
#else
#define BEGIN_C
#define END_C
#endif

BEGIN_C

#include <stdint.h>

int32_t fast_itoa10(int32_t value, char* buf);
int32_t fast_uitoa10(uint32_t value, char* buf);
int32_t fast_litoa10(int64_t value, char* buf);
int32_t fast_ulitoa10(uint64_t value, char* buf);
int32_t fast_dtoa(double value, char* buf, int precision);
int32_t fast_dtoa2(double value, char* buf, int precision);

END_C
