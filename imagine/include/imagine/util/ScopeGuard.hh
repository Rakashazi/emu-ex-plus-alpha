#pragma once

namespace IG
{

template <class F>
class ScopeGuard
{
	F func;
	bool active;

public:
	ScopeGuard() = delete;

	constexpr ScopeGuard(F func):
		func(std::move(func)),
		active(true)
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
ScopeGuard<F> scopeGuard(F func)
{
	return ScopeGuard<F>(std::move(func));
}

}
