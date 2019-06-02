cmake_minimum_required(VERSION 3.0)
project(cache)

#  to add this -std=c++11
set (CMAKE_CXX_STANDARD 11)

add_executable(unittest_CacheLine unittest_CacheLine.cpp)
add_executable(unittest_CacheStore unittest_CacheStore.cpp)
