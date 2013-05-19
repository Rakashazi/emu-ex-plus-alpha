#pragma once
#include <assert.h>
#include <util/ansiTypes.h>
#include <new>
#include <type_traits>

template <typename T> class DelegateFunc {};

template <typename R, typename ...ARGS> class DelegateFunc<R(ARGS...)>
{
public:
	constexpr DelegateFunc() {}

	// construct from lambda
	template<class T>
	DelegateFunc(T const &lambda, typename std::enable_if<!std::is_function<T>::value>::type* = 0) :
		exec
		{
			[](const Storage &lambda, ARGS... arguments) -> R
			{
				return ((T*)lambda.data())->operator()(arguments...);
			}
		}
	{
			static_assert(sizeof(T) <= STORAGE_SIZE, "Delegate too big for storage");
			new (this->lambdaMem.mem) T(lambda);
	}

	// construct from free function
	/*template <class T>
	constexpr DelegateFunc(T const &func, typename std::enable_if<std::is_function<T>::value>::type* = 0) :
		exec
		{
			[](const Storage &lambda, ARGS... arguments) -> R
			{
				return lambda.func(arguments...);
			}
		},
		lambdaMem{func}
	{}*/

	operator bool() const
	{
		return exec;
	}

	R operator()(ARGS ... in) const
	{
		assert(exec);
		return exec(lambdaMem, in...);
	}

	static constexpr int STORAGE_SIZE = sizeof(ptrsize)*2;

private:

	struct Storage
	{
		constexpr Storage() {}
		//constexpr Storage(R (*func)(ARGS...)): func{func} {}
		const void *data() const { return mem; }
		//union
		//{
			//R (*func)(ARGS...);
			uint8 mem[STORAGE_SIZE] {0};
		//};
	};

	R (*exec)(const Storage &, ARGS...) = nullptr;
	Storage lambdaMem;
};
