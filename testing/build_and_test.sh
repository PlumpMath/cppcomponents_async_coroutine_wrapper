#!/bin/bas
g++ -std=c++11 -I ../../cppcomponents ../cppcomponents_async_coroutine_wrapper/cppcomponents_async_coroutine_wrapper_dll.cpp -lboost_context -lboost_coroutine -o cppcomponents_async_coroutine_wrapper_dll.so -O3 -shared -fPIC -pthread

g++ -std=c++11 unit_test_exe.cpp -I ../../cppcomponents -ldl -pthread

./a.out

