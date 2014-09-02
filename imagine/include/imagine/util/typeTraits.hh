#pragma once

#include <type_traits>

namespace IG
{

	namespace detail
	{
	enum class Enabler{};
	}

#ifdef __clang__
// TODO: remove work-around for Clang, needed as of version 3.4.2
constexpr detail::Enabler dummy = {};
#define ENABLE_IF_COND(...) IG::EnableIfCond< __VA_ARGS__ > = IG::dummy
#define DISABLE_IF_COND(...) IG::DisableIfCond< __VA_ARGS__ > = IG::dummy
#else
#define ENABLE_IF_COND(...) IG::EnableIfCond< __VA_ARGS__ >...
#define DISABLE_IF_COND(...) IG::DisableIfCond< __VA_ARGS__ >...
#endif

template<typename CONDITION>
using EnableIfCond = typename std::enable_if<CONDITION::value, detail::Enabler>::type;

template<typename CONDITION>
using DisableIfCond = typename std::enable_if<!CONDITION::value, detail::Enabler>::type;

}
