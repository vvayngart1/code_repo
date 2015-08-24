#pragma once

#define TW_STRINGOFARGUMENT(x)       #x
#define TW_STRINGOFEXPANSION(x)      TW_STRINGOFARGUMENT(x)
#define TW_AT                        __FILE__ "(" TW_STRINGOFEXPANSION(__LINE__) ")"
