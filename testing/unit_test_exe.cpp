//          Copyright John R. Bandela 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <string>
#include "../cppcomponents_async_coroutine_wrapper/cppcomponents_async_coroutine_wrapper.hpp"
#define CATCH_CONFIG_MAIN  
#include "external/catch.hpp"






void Handler(cppcomponents::use<cppcomponents_async_coroutine_wrapper::ICoroutineVoidPtr> ca){
	auto s = *static_cast<std::string*>(ca.Get());
	s += " World";
	ca(&s);
	auto i = *static_cast<int*>(ca.Get());
	i += 5;
	ca(&i);

}
TEST_CASE("Test wrapper", "[coroutine]") {
	std::string s = "Hello";

	cppcomponents_async_coroutine_wrapper::CoroutineVoidPtr co(cppcomponents::make_delegate<cppcomponents_async_coroutine_wrapper::CoroutineHandler>(Handler), &s);

	auto s2 = *static_cast<std::string*>(co.Get());

	REQUIRE(s2 == "Hello World");

	int i = 0;
	co(&i);
	auto i2 = *static_cast<int*>(co.Get());

	REQUIRE(i2 == 5);
}

