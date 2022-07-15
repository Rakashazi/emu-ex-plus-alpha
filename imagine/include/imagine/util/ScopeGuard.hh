#pragma once

#include <concepts>
#include <utility>

namespace IG
{

template <std::invocable F>
class ScopeGuard
{
	F func;
	bool active;

public:
	ScopeGuard() = delete;

	constexpr ScopeGuard(F func, bool active):
		func(std::move(func)),
		active(active)
	{}

	constexpr ScopeGuard(F func):
		ScopeGuard(std::move(func), true)
	{}

	ScopeGuard(const ScopeGuard &) = delete;

	ScopeGuard(ScopeGuard &&rhs):
		func(std::move(rhs.func)),
		active(rhs.active)
	{
		rhs.cancel();
	}

	~ScopeGuard()
	{
		if(active)
			func();
	}

	void operator()()
	{
		if(active)
		{
			func();
			active = false;
		}
	}

	void cancel()
	{
		active = false;
	}
};

template<class F>
ScopeGuard<F> scopeGuard(F func, bool active = true)
{
	return ScopeGuard<F>(std::move(func), active);
}

}
