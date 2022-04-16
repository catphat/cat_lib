cmake_minimum_required(VERSION 3.14)
project(cat_lib_test)

# GoogleTest requires at least C++11
set(cat_lib_test PROPERTIES
        CXX_STANDARD 17
        )
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
add_library(cat_lib INTERFACE )
target_include_directories(cat_lib INTERFACE ../../include/)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
        ringbuffer_test
        ringbuffer_test.cc
)
target_link_libraries(
        ringbuffer_test
        gtest_main
        cat_lib
)

include(GoogleTest)
gtest_discover_tests(ringbuffer_test)
