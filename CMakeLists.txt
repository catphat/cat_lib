cmake_minimum_required(VERSION 3.22)
project(cat_lib VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)

add_library(cat_lib INTERFACE )
target_include_directories(cat_lib INTERFACE include/)
