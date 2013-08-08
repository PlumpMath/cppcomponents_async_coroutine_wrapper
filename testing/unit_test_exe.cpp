#include <assert.h>
#include <string>
#include "../cppcomponents_async_coroutine_wrapper/cppcomponents_async_coroutine_wrapper.hpp"

void Handler(cppcomponents::use<cppcomponents_async_coroutine_wrapper::ICoroutineVoidPtr> ca){
	auto s = *static_cast<std::string*>(ca.Get());
	s += " World";
	ca(&s);
	auto i = *static_cast<int*>(ca.Get());
	i += 5;
	ca(&i);

}


int main(){
	std::string s = "Hello";

	cppcomponents_async_coroutine_wrapper::CoroutineVoidPtr co(cppcomponents::make_delegate<cppcomponents_async_coroutine_wrapper::CoroutineHandler>(Handler),&s);

	auto s2 = *static_cast<std::string*>(co.Get());

	assert(s2 == "Hello World");

	int i = 0;
	co(&i);
	auto i2 = *static_cast<int*>(co.Get());

	assert(i2 == 5);
}