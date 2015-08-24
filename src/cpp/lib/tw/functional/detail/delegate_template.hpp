#if DELEGATE_PARAM_COUNT > 0
#define DELEGATE_SEPARATOR ,
#else
#define DELEGATE_SEPARATOR
#endif

// see BOOST_JOIN for explanation
#define DELEGATE_JOIN_MACRO( X, Y) DELEGATE_DO_JOIN( X, Y )
#define DELEGATE_DO_JOIN( X, Y ) DELEGATE_DO_JOIN2(X,Y)
#define DELEGATE_DO_JOIN2( X, Y ) X##Y

namespace tw {
namespace functional {
#ifdef DELEGATE_PREFERRED_SYNTAX
#define DELEGATE_CLASS_NAME delegate
#define DELEGATE_INVOKER_CLASS_NAME delegate_invoker
#else
#define DELEGATE_CLASS_NAME DELEGATE_JOIN_MACRO(delegate,DELEGATE_PARAM_COUNT)
#define DELEGATE_INVOKER_CLASS_NAME DELEGATE_JOIN_MACRO(delegate_invoker,DELEGATE_PARAM_COUNT)
	template <typename R DELEGATE_SEPARATOR DELEGATE_TEMPLATE_PARAMS>
	class DELEGATE_INVOKER_CLASS_NAME;
#endif

	template <typename R DELEGATE_SEPARATOR DELEGATE_TEMPLATE_PARAMS>
#ifdef DELEGATE_PREFERRED_SYNTAX
	class DELEGATE_CLASS_NAME<R (DELEGATE_TEMPLATE_ARGS)>
#else
	class DELEGATE_CLASS_NAME
#endif
	{
	public:
		typedef R return_type;
#ifdef DELEGATE_PREFERRED_SYNTAX
		typedef return_type (DELEGATE_CALLTYPE *signature_type)(DELEGATE_TEMPLATE_ARGS);
		typedef DELEGATE_INVOKER_CLASS_NAME<signature_type> invoker_type;
#else
		typedef DELEGATE_INVOKER_CLASS_NAME<R DELEGATE_SEPARATOR DELEGATE_TEMPLATE_ARGS> invoker_type;
#endif

		DELEGATE_CLASS_NAME() {
                    clear();
                }
                
                void clear() {
                    object_ptr = NULL;
                    stub_ptr = NULL;
                }
                
                bool empty() const {
                    return !(*this);
                }

		template <return_type (*TMethod)(DELEGATE_TEMPLATE_ARGS)>
		static DELEGATE_CLASS_NAME from_function()
		{
			return from_stub(0, &function_stub<TMethod>);
		}

		template <class T, return_type (T::*TMethod)(DELEGATE_TEMPLATE_ARGS)>
		static DELEGATE_CLASS_NAME from_method(T* object_ptr)
		{
			return from_stub(object_ptr, &method_stub<T, TMethod>);
		}

		template <class T, return_type (T::*TMethod)(DELEGATE_TEMPLATE_ARGS) const>
		static DELEGATE_CLASS_NAME from_const_method(T const* object_ptr)
		{
			return from_stub(const_cast<T*>(object_ptr), &const_method_stub<T, TMethod>);
		}

		return_type operator()(DELEGATE_PARAMS) const
		{
			return (*stub_ptr)(object_ptr DELEGATE_SEPARATOR DELEGATE_ARGS);
		}

		operator bool () const
		{
			return stub_ptr != 0;
		}

		bool operator!() const
		{
			return !(operator bool());
		}
                
                bool operator==(const DELEGATE_CLASS_NAME& rhs) const {
                    if ( stub_ptr != rhs.stub_ptr )
                        return false;
                    
                    if ( object_ptr != rhs.object_ptr )
                        return false;
                    
                    return true;
                }
                
                bool operator!=(const DELEGATE_CLASS_NAME& rhs) const {
                    return !(this->operator==(rhs));
                }

	private:
		
		typedef return_type (DELEGATE_CALLTYPE *stub_type)(void* object_ptr DELEGATE_SEPARATOR DELEGATE_PARAMS);

		void* object_ptr;
		stub_type stub_ptr;

		static DELEGATE_CLASS_NAME from_stub(void* object_ptr, stub_type stub_ptr)
		{
			DELEGATE_CLASS_NAME d;
			d.object_ptr = object_ptr;
			d.stub_ptr = stub_ptr;
			return d;
		}

		template <return_type (*TMethod)(DELEGATE_TEMPLATE_ARGS)>
		static return_type DELEGATE_CALLTYPE function_stub(void* DELEGATE_SEPARATOR DELEGATE_PARAMS)
		{
			return (TMethod)(DELEGATE_ARGS);
		}

		template <class T, return_type (T::*TMethod)(DELEGATE_TEMPLATE_ARGS)>
		static return_type DELEGATE_CALLTYPE method_stub(void* object_ptr DELEGATE_SEPARATOR DELEGATE_PARAMS)
		{
			T* p = static_cast<T*>(object_ptr);
			return (p->*TMethod)(DELEGATE_ARGS);
		}

		template <class T, return_type (T::*TMethod)(DELEGATE_TEMPLATE_ARGS) const>
		static return_type DELEGATE_CALLTYPE const_method_stub(void* object_ptr DELEGATE_SEPARATOR DELEGATE_PARAMS)
		{
			T const* p = static_cast<T*>(object_ptr);
			return (p->*TMethod)(DELEGATE_ARGS);
		}
	};

	template <typename R DELEGATE_SEPARATOR DELEGATE_TEMPLATE_PARAMS>
#ifdef DELEGATE_PREFERRED_SYNTAX
	class DELEGATE_INVOKER_CLASS_NAME<R (DELEGATE_TEMPLATE_ARGS)>
#else
	class DELEGATE_INVOKER_CLASS_NAME
#endif
	{
		DELEGATE_INVOKER_DATA

	public:
		DELEGATE_INVOKER_CLASS_NAME(DELEGATE_PARAMS)
#if DELEGATE_PARAM_COUNT > 0
			:
#endif
			DELEGATE_INVOKER_INITIALIZATION_LIST
		{
		}

		template <class TDelegate>
		R operator()(TDelegate d) const
		{
			return d(DELEGATE_ARGS);
		}
	};
} // functional
} // tw

#undef DELEGATE_CLASS_NAME
#undef DELEGATE_SEPARATOR
#undef DELEGATE_JOIN_MACRO
#undef DELEGATE_DO_JOIN
#undef DELEGATE_DO_JOIN2
