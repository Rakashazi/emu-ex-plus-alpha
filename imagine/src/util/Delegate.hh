#pragma once
#include <util/builtins.h>
#include <assert.h>

// Based on:
// http://molecularmusings.wordpress.com/2011/09/19/generic-type-safe-delegates-and-events-in-c/
// Improved with C++11 features (constexpr construction/creation, variadic templates)

template <typename T>
class Delegate {};

template <typename R, typename... ARGS>
class Delegate<R(ARGS...)>
{
  typedef void* InstancePtr;
  typedef R (*InternalFunction)(InstancePtr, ARGS...);
  struct Callback
  {
  	constexpr Callback() { }
  	constexpr Callback(InstancePtr inst, InternalFunction func): inst(inst), func(func) { }
  	InstancePtr inst = nullptr;
  	InternalFunction func = nullptr;
  };

  // turns a free function into our internal function stub
  template <R (*Function)(ARGS...)>
  static inline R FunctionStub(InstancePtr, ARGS... args)
  {
    // we don't need the instance pointer because we're dealing with free functions
    return (Function)(args...);
  }

  // turns a member function into our internal function stub
  template <class C, R (C::*Function)(ARGS...)>
  static inline R ClassMethodStub(InstancePtr instance, ARGS... args)
  {
    // cast the instance pointer back into the original class instance
    return (static_cast<C*>(instance)->*Function)(args...);
  }

public:
  constexpr Delegate() { }
  constexpr Delegate(InstancePtr inst, InternalFunction func): callback(inst, func) { }

  template <R (*Function)(ARGS...)>
  static constexpr Delegate create()
  {
  	return Delegate(nullptr, &FunctionStub<Function>);
  }

  template <class C,R (C::*Function)(ARGS...)>
  static constexpr Delegate create(C* instance)
  {
  	return Delegate(instance, &ClassMethodStub<C, Function>);
  }

  /// Binds a free function
  template <R (*Function)(ARGS...)>
  void bind(void)
  {
  	callback.inst = nullptr;
  	callback.func = &FunctionStub<Function>;
  }

  /// Binds a class method
  template <class C, R (C::*Function)(ARGS...)>
  void bind(C* instance)
  {
  	callback.inst = instance;
    callback.func = &ClassMethodStub<C, Function>;
  }

	/// Invokes the delegate
	R invoke(ARGS... args) const
	{
		assert(callback.func != nullptr);
		return callback.func(callback.inst, args...);
	}

	R invokeSafe(ARGS... args) const
	{
		if(callback.func != nullptr)
		{
			return callback.func(callback.inst, args...);
		}
		else
		{
			//logMsg("null callback");
		}
	}

	void clear()
	{
		callback.inst = nullptr;
		callback.func = nullptr;
	}

	bool hasCallback() const
	{
		return callback.func != nullptr;
	}

private:
	Callback callback;
};
