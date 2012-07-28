#pragma once

namespace IG
{

#ifdef max
	#warning max() previously defined
	#undef max
#endif
template <class T>
static constexpr T const& max ( const T& a, const T& b )
{
	return (b < a) ? a : b;
}

#ifdef min
	#warning min() previously defined
	#undef min
#endif
template <class T>
static constexpr T const& min ( const T& a, const T& b )
{
	return (a < b) ? a : b;
}

}
