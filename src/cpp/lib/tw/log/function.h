#pragma once

#if __STDC_VERSION__ < 199901L
# if __GNUC__ >= 2
#  define TW_FUNCTION __FUNCTION__
# else
#  define TW_FUNCTION "<unknown>"
# endif
#else
# define TW_FUNCTION __func__
#endif

#ifdef __GNUC__
#define TW_PRETTY_FUNCTION __PRETTY_FUNCTION__
#else
#define TW_PRETTY_FUNCTION TW_FUNCTION "()"
#endif

