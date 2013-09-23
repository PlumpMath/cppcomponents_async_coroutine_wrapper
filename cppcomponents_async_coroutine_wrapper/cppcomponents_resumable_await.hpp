
// Copyright (c) 2013 John R. Bandela
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once
#ifndef INCLUDE_GUARD_CPPCOMPONENTS_RESUMABLE_AWAIT_HPP_
#define INCLUDE_GUARD_CPPCOMPONENTS_RESUMABLE_AWAIT_HPP_

#include <cppcomponents/future.hpp>
#include "cppcomponents_async_coroutine_wrapper.hpp"

#include <memory>
#include <exception>
#include <utility>
#include <assert.h>
#include <type_traits>
#include <functional>

#ifdef PPL_HELPER_OUTPUT_ENTER_EXIT
#include <cstdio>
#include <atomic>
#define PPL_HELPER_ENTER_EXIT ::cppcomponents::detail::EnterExit ppl_helper_enter_exit_var;
#else
#define PPL_HELPER_ENTER_EXIT
#endif

namespace cppcomponents{
	namespace detail{
#ifdef PPL_HELPER_OUTPUT_ENTER_EXIT
		// Used for debugging to make sure all functions are exiting correctly
		struct EnterExit{
			EnterExit() : n_(++number()){ increment(); std::printf("==%d Entering\n", n_); }
			int n_;
			std::string s_;
			static int& number(){ static int number = 0; return number; }
			static std::atomic<int>& counter(){ static std::atomic<int> counter_ = 0; return counter_; }
			static void increment(){ ++counter(); }
			static void decrement(){ --counter(); }
			static void check_all_destroyed(){ assert(counter() == 0); if (counter())throw std::exception("Not all EnterExit destroyed"); };
			~EnterExit(){ decrement(); std::printf("==%d Exiting\n", n_); }

		};
#endif
#undef PPL_HELPER_OUTPUT_ENTER_EXIT


		struct coroutine_holder : std::enable_shared_from_this<coroutine_holder>{
			PPL_HELPER_ENTER_EXIT;
			typedef cppcomponents_async_coroutine_wrapper::CoroutineVoidPtr co_type;
			std::unique_ptr<co_type> coroutine_;
			co_type::CallerType* coroutine_caller_;
			coroutine_holder() : coroutine_(), coroutine_caller_(nullptr){}

		};

		struct ret_type{
			std::exception_ptr eptr_;
			void* pv_;

			template<class T>
			T& get(){
				if (eptr_){
					std::rethrow_exception(eptr_);
				}
				return *static_cast<T*>(pv_);
			}
		};

		struct convertible_to_async_helper{};


	}

	template<class T>
	class awaiter{
		typedef detail::coroutine_holder* co_ptr;
		typedef std::function<use<IFuture<T>>()> func_type;
		co_ptr co_;

		template<class R>
		func_type get_function(use < IFuture < R >> t){
			auto co = co_;
			func_type retfunc([co, t]()mutable{
				auto sptr = co->shared_from_this();
				return t.Then([sptr](use < IFuture < R >> et)mutable{
					detail::ret_type ret;
					ret.eptr_ = nullptr;
					ret.pv_ = nullptr;
					ret.pv_ = &et;
					(*sptr->coroutine_)(&ret);
					try{
						func_type f(std::move(*static_cast<func_type*>(sptr->coroutine_->Get())));
						return f();
					}
					catch (std::exception&){
						ret.eptr_ = std::current_exception();
						ret.pv_ = nullptr;
						(*sptr->coroutine_)(&ret);
						throw;
					}
				}).Unwrap();
			});

			return retfunc;

		}
		template<class R>
		func_type get_function(use<IExecutor> executor,use < IFuture < R >> t){
			auto co = co_;
			func_type retfunc([co, t,executor]()mutable{
				auto sptr = co->shared_from_this();
				return t.Then(executor,[sptr](use < IFuture < R >> et)mutable{
					detail::ret_type ret;
					ret.eptr_ = nullptr;
					ret.pv_ = nullptr;
					ret.pv_ = &et;
					(*sptr->coroutine_)(&ret);
					try{
						func_type f(std::move(*static_cast<func_type*>(sptr->coroutine_->Get())));
						return f();
					}
					catch (std::exception& e){
						(void)e;
						ret.eptr_ = std::current_exception();
						ret.pv_ = nullptr;
						(*sptr->coroutine_)(&ret);
						throw;
					}
				}).Unwrap();
			});

			return retfunc;

		}

	public:
		awaiter(co_ptr c)
			: co_(c)
		{

		}



		awaiter(detail::convertible_to_async_helper);

		template<class R>
		use<IFuture<R>> as_future(use < IFuture < R >> t){
			if (t.Ready()){
				return t;
			}
			auto retfunc = get_function(t);
			PPL_HELPER_ENTER_EXIT;
			assert(co_->coroutine_caller_);
			(*co_->coroutine_caller_)(&retfunc);
			return static_cast<detail::ret_type*>(co_->coroutine_caller_->Get())->get < use < IFuture<R >> >();
		}
		template<class R>
			use<IFuture<R>> as_future(use<IExecutor> executor, use < IFuture < R >> t){
				if (t.Ready()){
					return t;
				}
				auto retfunc = get_function(executor,t);
				PPL_HELPER_ENTER_EXIT;
				assert(co_->coroutine_caller_);
				(*co_->coroutine_caller_)(&retfunc);
				return static_cast<detail::ret_type*>(co_->coroutine_caller_->Get())->get < use < IFuture<R >> >();
		}

			template<class R>
			R operator()(use < IFuture < R >> t){
				return as_future(t).Get();
			}

			template<class R>
			R operator()(use<IExecutor> executor,use < IFuture < R >> t){
				return as_future(executor,t).Get();
			}

	};
	namespace detail{

		template<class T>
		struct ret_holder{
			T value_;
			template<class FT>
			ret_holder(FT& f, awaiter<T> h) : value_(f(h)){}
			const T& get()const{ return value_; }

			use<IFuture<T>> get_ready_future()const{ return cppcomponents::make_ready_future(value_); }
		};
		template<>
		struct ret_holder<void>{
			template<class FT>
			ret_holder(FT& f, awaiter<void> h){
				f(h);
			}
			void get()const{}
			use<IFuture<void>> get_ready_future()const{ return cppcomponents::make_ready_future(); }

		};

		template<class F>
		class simple_async_function_holder : public coroutine_holder{


			F f_;
			typedef typename std::result_of<F(convertible_to_async_helper)>::type return_type;
			typedef use<IFuture<return_type>> task_t;
			typedef std::function<task_t()> func_type;


			static void coroutine_function(cppcomponents::use<cppcomponents_async_coroutine_wrapper::ICoroutineVoidPtr> ca){
				PPL_HELPER_ENTER_EXIT;;

				auto p = ca.Get();
				auto pthis = reinterpret_cast<simple_async_function_holder*>(p);
				pthis->coroutine_caller_ = &ca;
				try{
					PPL_HELPER_ENTER_EXIT;
					awaiter<return_type> helper(pthis);
					ret_holder<return_type> ret(pthis->f_, helper);
					func_type retfunc([ret](){
						return ret.get_ready_future();
					});
					ca(&retfunc);
				}
				catch (std::exception& e){
					auto ec = cppcomponents::error_mapper::error_code_from_exception(e);
					func_type retfunc([ec](){
						return cppcomponents::make_error_future<return_type>(ec);
					});
					ca(&retfunc);
				}
			}
		public:
			simple_async_function_holder(F f) : f_(f){}

			task_t run(){
				coroutine_.reset(new coroutine_holder::co_type(cppcomponents::make_delegate<cppcomponents_async_coroutine_wrapper::CoroutineHandler>(coroutine_function), this));
				func_type f(std::move(*static_cast<func_type*>(coroutine_->Get())));
				return f();

			}
		};

		template<class F>
		struct return_helper{};

		template<class R>
		struct return_helper<R(awaiter<R>)>{
			typedef R type;
		};


	template<class F>
	use<IFuture<typename std::result_of<F(detail::convertible_to_async_helper)>::type>> do_async(F f){
		auto ret = std::make_shared<detail::simple_async_function_holder<F>>(f);
		return ret->run();
	}

	
	template<class R,class F>
	struct do_async_functor{
		F f_;
		template<class... T>
		use<IFuture<R>> operator()(T && ... t){
			using namespace std::placeholders;
			return do_async(std::bind(f_,std::forward<T>(t)..., _1));
		}

		do_async_functor(F f) : f_{ f }{}
	};

	}

	template<class R, class F>
	detail::do_async_functor<R, F> resumable(F f){
		return detail::do_async_functor<R, F>{f};
	}

}


#endif