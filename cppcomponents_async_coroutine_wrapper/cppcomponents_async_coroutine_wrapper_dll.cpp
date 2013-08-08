#ifndef _DEBUG

#pragma comment(lib,"libboost_coroutine-vc120-mt-s-1_54")

#endif
#include "cppcomponents_async_coroutine_wrapper.hpp"

using namespace cppcomponents;
using namespace cppcomponents_async_coroutine_wrapper;

#include <boost/coroutine/all.hpp>

typedef boost::coroutines::coroutine<void*(void*) > co_type;





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

CPPCOMPONENTS_DEFINE_FACTORY();