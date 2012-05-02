template <TEMPLATE_LIST>
class Delegate<TEMPLATE_IMPL_LIST>
{
  typedef void* InstancePtr;
  typedef R (*InternalFunction)(InstancePtr TEMPLATE_ARG_LIST);
  //typedef std::pair<InstancePtr, InternalFunction> Stub;
  struct Callback
  {
  	constexpr Callback(): inst(nullptr), func(nullptr) { }
  	constexpr Callback(InstancePtr inst, InternalFunction func): inst(inst), func(func) { }
  	InstancePtr inst;
  	InternalFunction func;
  };

  // turns a free function into our internal function stub
  template <R (*Function)(TEMPLATE_ARG_LIST2)>
  static inline R FunctionStub(InstancePtr TEMPLATE_ARG_LIST3)
  {
    // we don't need the instance pointer because we're dealing with free functions
    return (Function)(TEMPLATE_ARG_LIST6);
  }

  // turns a member function into our internal function stub
  template <class C, R (C::*Function)(TEMPLATE_ARG_LIST2)>
  static inline R ClassMethodStub(InstancePtr instance TEMPLATE_ARG_LIST3)
  {
    // cast the instance pointer back into the original class instance
    return (static_cast<C*>(instance)->*Function)(TEMPLATE_ARG_LIST6);
  }

public:
  constexpr Delegate() { }

  /*template <R (*Function)(TEMPLATE_ARG_LIST2)>
  constexpr Delegate(): callback { nullptr, &FunctionStub<Function> } { }

  template <class C, R (C::*Function)(TEMPLATE_ARG_LIST2)>
  constexpr Delegate(C* instance): callback { instance, &ClassMethodStub<C, Function> } { }*/

  /// Binds a free function
  template <R (*Function)(TEMPLATE_ARG_LIST2)>
  void bind(void)
  {
  	callback.inst = nullptr;
  	callback.func = &FunctionStub<Function>;
  }

  /// Binds a class method
  template <class C, R (C::*Function)(TEMPLATE_ARG_LIST2)>
  void bind(C* instance)
  {
  	callback.inst = instance;
    callback.func = &ClassMethodStub<C, Function>;
  }
  
  /*template <class C>
  void Bind(C* instance, R (C::*Function)(ARG0))
  {
    m_stub.first = instance;
    m_stub.second = &ClassMethodStub<C, Function>;
  }*/

	/// Invokes the delegate
	R invoke(TEMPLATE_ARG_LIST5) const
	{
		assert(callback.func != nullptr);
		return callback.func(callback.inst TEMPLATE_ARG_LIST4);
	}

	R invokeSafe(TEMPLATE_ARG_LIST5) const
	{
		if(callback.func != nullptr)
		{
			return callback.func(callback.inst TEMPLATE_ARG_LIST4);
		}
		else
		{
			//logMsg("null callback");
		}
	}

private:
  //Stub m_stub;
	Callback callback;
};

#undef TEMPLATE_LIST
#undef TEMPLATE_IMPL_LIST
#undef TEMPLATE_ARG_LIST
#undef TEMPLATE_ARG_LIST2
#undef TEMPLATE_ARG_LIST3
#undef TEMPLATE_ARG_LIST4
#undef TEMPLATE_ARG_LIST5
#undef TEMPLATE_ARG_LIST6

