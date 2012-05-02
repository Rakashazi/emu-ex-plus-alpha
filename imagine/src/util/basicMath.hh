#pragma once

namespace IG
{

#ifdef max
	#warning max() previously defined
	#undef max
#endif
template <class T>
static const T& max ( const T& a, const T& b )
{
	return (b < a) ? a : b;
}

#ifdef min
	#warning min() previously defined
	#undef min
#endif
template <class T>
static const T& min ( const T& a, const T& b )
{
	return (a < b) ? a : b;
}

}
