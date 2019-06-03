cmake_minimum_required(VERSION 3.0)
set (CMAKE_C_COMPILER "/usr/bin/clang")
set (CMAKE_CXX_COMPILER "/usr/bin/clang++")

#  to use C++ 11 , add this -std=c++11 via this cmake option
set (CMAKE_CXX_STANDARD 11)

include(cm/systemc_cmake.txt)
project (tlm2freesampler)
include(cm/systemc_exe_cmake.txt)
