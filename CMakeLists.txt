cmake_minimum_required(VERSION 3.22)
project(catlib VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)

add_library(catlib INTERFACE )
target_include_directories(catlib INTERFACE include/)
