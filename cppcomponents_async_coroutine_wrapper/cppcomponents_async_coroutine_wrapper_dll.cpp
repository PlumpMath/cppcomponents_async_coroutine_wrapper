//          Copyright John R. Bandela 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include "cppcomponents_async_coroutine_wrapper.hpp"

using namespace cppcomponents;
using namespace cppcomponents_async_coroutine_wrapper;

#include <boost/coroutine/all.hpp>

typedef boost::coroutines::coroutine<void*(void*) > co_type;


// Visual C++ does not yet implement
// thread-safe static initializations
// boost::coroutines::detail::system_info looks like this
//SYSTEM_INFO system_info()
//{
//	static SYSTEM_INFO si = system_info_();
//	return si;
//}
// However, because Visual C++ does not have thread-safe
// static initialization, there is a race. However, if we
// call this function ourselves, before any other call to the function
// then si get initialized properly without a race
// We only need to do this with MSVC
#ifdef _MSC_VER
#include <windows.h>

namespace boost {
	namespace coroutines {
		namespace detail {



			SYSTEM_INFO system_info();

		}
	}
}

SYSTEM_INFO info = boost::coroutines::detail::system_info();

#endif

inline std::string CoroutineCallerId(){ return "cppcomponents_async_coroutine_wrapper!CoroutineCaller"; }

typedef cppcomponents::runtime_class<CoroutineCallerId, object_interfaces<ICoroutineVoidPtr>, factory_interface<NoConstructorFactoryInterface>> Caller_t;

struct ImplementCaller : public implement_runtime_class < ImplementCaller, Caller_t>
{
	co_type* ca_;
	ImplementCaller(co_type* ca) : ca_(ca){};

	void* Get(){
		return ca_->get();
	}

	void Call(void* v){
		(*ca_)(v);
	}



};
CPPCOMPONENTS_REGISTER(ImplementCaller)

struct ImplementCoroutineVoidPtr : public implement_runtime_class < ImplementCoroutineVoidPtr, CoroutineVoidPtr_t>
{
	co_type co_;
	ImplementCoroutineVoidPtr(use<CoroutineHandler> h, void* v) : co_([h](co_type& ca)mutable{h(ImplementCaller::create(&ca).QueryInterface<ICoroutineVoidPtr>()); }, v){};

	void* Get(){
		return co_.get();
	}

	void Call(void* v){
		co_(v);
	}



};
CPPCOMPONENTS_REGISTER(ImplementCoroutineVoidPtr)

CPPCOMPONENTS_DEFINE_FACTORY();