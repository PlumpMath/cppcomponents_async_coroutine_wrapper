//          Copyright John R. Bandela 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <string>
#include "../cppcomponents_async_coroutine_wrapper/cppcomponents_resumable_await.hpp"
#include <thread>
#include <vector>

#define CATCH_CONFIG_MAIN  
#include "external/catch.hpp"


TEST_CASE("Test multithread", "[coroutine]") {
	std::atomic<bool> test{ false };

	auto co = [](cppcomponents::awaiter await){};
	auto threaded = [&](){
		auto resfunc = cppcomponents::resumable(co);
		while (!test.load());
		resfunc();
	};

	auto co2 = [](cppcomponents::awaiter await){};
	std::vector<std::thread> threads;
	for (int i = 0; i < 10; ++i){
		threads.push_back(std::thread{ threaded });
	}
	auto resfunc = cppcomponents::resumable(co2);
	test.store(true);
	resfunc();

	for (auto& t : threads){
		t.join();
	}
}

struct ICounter : public cppcomponents::define_interface < cppcomponents::uuid<0x35779857, 0x9090, 0x4668, 0x8eee, 0xa67e359b66ae>>
{
	std::uint64_t Increment();
	std::uint64_t Decrement();
	std::uint64_t Count();

	CPPCOMPONENTS_CONSTRUCT(ICounter, Increment, Decrement, Count);
};



inline std::string launch_on_new_thread_executor_id(){ return "launch_on_new_thread"; }
typedef cppcomponents::runtime_class < launch_on_new_thread_executor_id, cppcomponents::object_interfaces<cppcomponents::IExecutor, ICounter>,
	cppcomponents::factory_interface < cppcomponents::NoConstructorFactoryInterface >> launch_on_new_thread_executor_t;

struct launch_on_new_thread_executor : public cppcomponents::implement_runtime_class<launch_on_new_thread_executor, launch_on_new_thread_executor_t>{
	typedef cppcomponents::delegate < void() > ClosureType;
	std::atomic<std::uint64_t> count_;
	launch_on_new_thread_executor() : count_{ 0 }{}

	struct counter_helper{

		cppcomponents::use<ICounter> c_;
		bool success_;

		counter_helper(cppcomponents::use<ICounter> c)
			: c_{ c }, success_{ false }
		{
			c_.Increment();
		}

		void success(){
			success_ = true;
		}

		~counter_helper(){
			if (!success_){
				c_.Decrement();
			}
		}

	};

	void AddDelegate(cppcomponents::use<ClosureType> d){
		auto counter = this->QueryInterface<ICounter>();

		counter_helper helper(counter);

		std::thread t(
			[d, counter](){
				d();
				counter.Decrement();
		});

		helper.success();

		if (t.joinable()){
			t.detach();
		}

	}
	std::size_t NumPendingClosures(){
		return static_cast<std::size_t>(ICounter_Count());
	}

	std::uint64_t ICounter_Increment(){
		return count_.fetch_add(1);
	}
	std::uint64_t ICounter_Decrement(){
		return count_.fetch_sub(1);
	}
	std::uint64_t ICounter_Count(){
		return count_.load();
	}

};





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


TEST_CASE("Async", "[async]"){

	std::atomic<bool> done{false};

	int i = 0;

	auto f = cppcomponents::resumable([](std::atomic<bool>* pdone,int* pi,cppcomponents::awaiter await){

		auto f = cppcomponents::async(launch_on_new_thread_executor::create().QueryInterface<cppcomponents::IExecutor>(),
			[]{return 10; });

		*pi = await(f);
		pdone->store(true);

	});

	f(&done, &i);
	// busy wait
	while (done.load() == false);

	REQUIRE(i == 10);
}
TEST_CASE("Async2", "[async2]"){

	std::atomic<bool> done{ false };

	int i = 0;

	auto f = cppcomponents::resumable([](std::atomic<bool>* pdone, int* pi, cppcomponents::awaiter await){
		auto f = cppcomponents::resumable([](cppcomponents::awaiter await){

			auto f = cppcomponents::async(launch_on_new_thread_executor::create().QueryInterface<cppcomponents::IExecutor>(),
				[]{return 10; });
			return await(f);
		});
		*pi = await(f());
		pdone->store(true);

	});

	f(&done, &i);
	// busy wait
	while (done.load() == false);

	REQUIRE(i == 10);
}
TEST_CASE("Async3", "[async2]"){

	std::atomic<bool> done{ false };

	int i = 0;

	auto f = cppcomponents::resumable([](std::atomic<bool>* pdone, int* pi, cppcomponents::awaiter await){
		auto f = cppcomponents::resumable([](cppcomponents::awaiter await){
			auto e = launch_on_new_thread_executor::create().QueryInterface<cppcomponents::IExecutor>();
			auto f = cppcomponents::async(e,
				[]{return 10; });
			return await(e,f);
		});
		*pi = await(f());
		pdone->store(true);

	});

	f(&done, &i);
	// busy wait
	while (done.load() == false);

	REQUIRE(i == 10);
}


#include <cppcomponents/loop_executor.hpp>
TEST_CASE("Async Execption", "Async Exception"){

	cppcomponents::LoopExecutor executor;

	auto f = cppcomponents::resumable([&](cppcomponents::awaiter await){
		auto f2 = []()->int{
			cppcomponents::throw_if_error(-101);
			return 5;
		};

		auto fut = cppcomponents::async(executor, f2);
		auto resfut = await(fut);
		cppcomponents::throw_if_error(-777);
		return 7;

	});

	auto fut = f();
	while(executor.NumPendingClosures())
		executor.RunQueuedClosures();
	auto ec = fut.ErrorCode();
	REQUIRE(ec == -101);

}

