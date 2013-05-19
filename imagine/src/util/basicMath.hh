#pragma once

namespace IG
{

template <class T>
static constexpr T const& maxConst ( const T& a, const T& b )
{
	return (b < a) ? a : b;
}

template <class T>
static constexpr T const& minConst ( const T& a, const T& b )
{
	return (a < b) ? a : b;
}

}
