#pragma once

namespace tw {
namespace functional
{
#ifdef DELEGATE_PREFERRED_SYNTAX
	template <typename TSignature> class delegate;
	template <typename TSignature> class delegate_invoker;
#endif
} // functional
} // tw

#ifdef _MSC_VER
#define DELEGATE_CALLTYPE __fastcall
#else
#define DELEGATE_CALLTYPE
#endif

#include "detail/delegate_list.hpp"

#undef DELEGATE_CALLTYPE
