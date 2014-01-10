#!/bin/bash
#This script assumes that libboost_context.a and libboost_coroutine.a are in the current directory and are built with -fPIC. For boost you can use ./b2 cxxflags='-fPIC'

g++ -std=c++11 -I ../../cppcomponents ../cppcomponents_async_coroutine_wrapper/cppcomponents_async_coroutine_wrapper_dll.cpp libboost_context.a libboost_coroutine.a -o cppcomponents_async_coroutine_wrapper_dll.so -O3 -shared -fPIC -pthread

strip cppcomponents_async_coroutine_wrapper_dll.so

g++ -std=c++11 unit_test_exe.cpp -I ../../cppcomponents -ldl -pthread

./a.out

