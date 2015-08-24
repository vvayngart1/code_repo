#pragma once

#include <string.h>

#include <tw/common/type_wrap.h>

#define NULLIFY(a) ::memset(&(a), 0, sizeof((a)));

namespace tw {
namespace price {

typedef tw::common::TypeWrap<int32_t> Ticks;
typedef tw::common::TypeWrap<float> FractionalTicks;

// TODO: might need to switch to 'double' type if 
// start dealing with fracional sizes, like in FX
//
typedef tw::common::TypeWrap<int32_t, true> Size;

typedef uint32_t TChangeFlag;

    
} // namespace price
} // namespace tw

