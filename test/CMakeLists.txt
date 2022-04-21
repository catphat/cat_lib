include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

#todo: review from fmt
#set(TEST_MAIN_SRC test-main.cc gtest-extra.cc gtest-extra.h util.cc)

set(TEST_MAIN_SRC test-main.cc ringbuffer-test.cc)

add_library(test-main STATIC ${TEST_MAIN_SRC})
target_include_directories(test-main PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
target_link_libraries(test-main gtest catlib)

function(add_catlib_executable name)
  add_executable(${name} ${ARGN})
  if (MINGW)
    target_link_libraries(${name} -static-libgcc -static-libstdc++)
  endif ()
  # (Wstringop-overflow) - [meta-bug] bogus/missing -Wstringop-overflow warnings
  #   https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88443
  # Bogus -Wstringop-overflow warning
  #   https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100395
  # [10 Regression] spurious -Wstringop-overflow writing to a trailing array plus offset
  #   https://gcc.gnu.org/bugzilla/show_bug.cgi?id=95353
  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND
      NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0)
    target_link_libraries(${name} -Wno-stringop-overflow)
  endif ()
endfunction()

# Adds a test.
# Usage: add_catlib_test(name srcs...)
function(add_catlib_test name)
  cmake_parse_arguments(ADD_catlib_TEST "HEADER_ONLY;MODULE" "" "" ${ARGN})

  set(sources ${name}.cc ${ADD_CATLIB_TEST_UNPARSED_ARGUMENTS})
  if (ADD_CATLIB_TEST_HEADER_ONLY)
    set(sources ${sources} ${TEST_MAIN_SRC} ../src/os.cc)
    set(libs gtest catlib-header-only)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      set(PEDANTIC_COMPILE_FLAGS ${PEDANTIC_COMPILE_FLAGS} -Wno-weak-vtables)
    endif ()
  elseif (ADD_CATLIB_TEST_MODULE)
    set(libs gtest test-module)
    set_source_files_properties(${name}.cc PROPERTIES OBJECT_DEPENDS test-module)
  else ()
    set(libs test-main catlib)
  endif ()
  add_catlib_executable(${name} ${sources})
  target_link_libraries(${name} ${libs})

  # Define if certain C++ features can be used.
  if (CATLIB_PEDANTIC)
    target_compile_options(${name} PRIVATE ${PEDANTIC_COMPILE_FLAGS})
  endif ()
  if (CATLIB_WERROR)
    target_compile_options(${name} PRIVATE ${WERROR_FLAG})
  endif ()
  add_test(NAME ${name} COMMAND ${name})
endfunction()

#add_catlib_test(args-test)
#add_catlib_test(assert-test)
#add_catlib_test(chrono-test)
#add_catlib_test(color-test)
#add_catlib_test(core-test)
add_catlib_test(ringbuffer-test)
#add_catlib_test(gtest-extra-test)
#add_catlib_test(format-test mock-allocator.h)
if (MSVC)
  target_compile_options(catlib-test PRIVATE /bigobj)
endif ()
if (NOT (MSVC AND BUILD_SHARED_LIBS))
  #todo
#  add_catlib_test(catlib-impl-test HEADER_ONLY header-only-test.cc)
endif ()
#add_catlib_test(ostream-test)
#add_catlib_test(compile-test)
#add_catlib_test(compile-fp-test HEADER_ONLY)
if (MSVC)
  # Without this option, MSVC returns 199711L for the __cplusplus macro.
  target_compile_options(compile-fp-test PRIVATE /Zc:__cplusplus)
endif()
#add_catlib_test(printf-test)
#add_catlib_test(ranges-test ranges-odr-test.cc)
#add_catlib_test(scan-test)
#add_catlib_test(unicode-test HEADER_ONLY)
if (MSVC)
  target_compile_options(unicode-test PRIVATE /utf-8)
endif ()
#add_catlib_test(xchar-test)
#add_catlib_test(enforce-checks-test)
#target_compile_definitions(enforce-checks-test PRIVATE
#                           -DCATLIB_ENFORCE_COMPILE_STRING)

if (CATLIB_CAN_MODULE)
  # The tests need {catlib} to be compiled as traditional library
  # because of visibility of implementation details.
  # If module support is present the module tests require a
  # test-only module to be built from {catlib}
  add_library(test-module OBJECT ${CMAKE_SOURCE_DIR}/src/catlib.cc)
  target_compile_features(test-module PUBLIC ${CATLIB_REQUIRED_FEATURES})
  target_include_directories(test-module PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
  enable_module(test-module)

  add_catlib_test(module-test MODULE test-main.cc)
  if (MSVC)
    target_compile_options(test-module PRIVATE /utf-8 /Zc:__cplusplus
                           /Zc:externConstexpr /Zc:inline)
    target_compile_options(module-test PRIVATE /utf-8 /Zc:__cplusplus
                           /Zc:externConstexpr /Zc:inline)
  endif ()
endif ()

if (NOT DEFINED MSVC_STATIC_RUNTIME AND MSVC)
  foreach (flag_var
			 CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
			 CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
	if (${flag_var} MATCHES "^(/|-)(MT|MTd)")
	  set(MSVC_STATIC_RUNTIME ON)
	  break()
	endif()
  endforeach()
endif()

#if (NOT MSVC_STATIC_RUNTIME)
#  add_catlib_executable(posix-mock-test
#    posix-mock-test.cc ../src/catlib.cc ${TEST_MAIN_SRC})
#  target_include_directories(
#    posix-mock-test PRIVATE ${PROJECT_SOURCE_DIR}/include)
#  target_link_libraries(posix-mock-test gtest)
#  if (CATLIB_PEDANTIC)
#    target_compile_options(posix-mock-test PRIVATE ${PEDANTIC_COMPILE_FLAGS})
#  endif ()
#  add_test(NAME posix-mock-test COMMAND posix-mock-test)
#  add_catlib_test(os-test)
#endif ()

message(STATUS "CATLIB_PEDANTIC: ${CATLIB_PEDANTIC}")

if (CATLIB_PEDANTIC)
  # Test that the library can be compiled with exceptions disabled.
  # -fno-exception is broken in icc: https://github.com/fmtlib/fmt/issues/822.
  if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    check_cxx_compiler_flag(-fno-exceptions HAVE_FNO_EXCEPTIONS_FLAG)
  endif ()
  if (HAVE_FNO_EXCEPTIONS_FLAG)
    add_library(noexception-test ../src/catlib.cc noexception-test.cc)
    target_include_directories(
      noexception-test PRIVATE ${PROJECT_SOURCE_DIR}/include)
    target_compile_options(noexception-test PRIVATE -fno-exceptions)
    target_compile_options(noexception-test PRIVATE ${PEDANTIC_COMPILE_FLAGS})
  endif ()

  # Test that the library compiles without locale.
  add_library(nolocale-test ../src/catlib.cc)
  target_include_directories(
    nolocale-test PRIVATE ${PROJECT_SOURCE_DIR}/include)
  target_compile_definitions(
    nolocale-test PRIVATE CATLIB_STATIC_THOUSANDS_SEPARATOR=1)
endif ()

# These tests are disabled on Windows because they take too long.
if (CATLIB_PEDANTIC AND NOT WIN32)
  # Test if incorrect API usages produce compilation error.
  add_test(compile-error-test ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMAKE_CURRENT_SOURCE_DIR}/compile-error-test"
    "${CMAKE_CURRENT_BINARY_DIR}/compile-error-test"
    --build-generator ${CMAKE_GENERATOR}
    --build-makeprogram ${CMAKE_MAKE_PROGRAM}
    --build-options
    "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}"
    "-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}"
    "-DCXX_STANDARD_FLAG=${CXX_STANDARD_FLAG}"
    "-DCATLIB_DIR=${CMAKE_SOURCE_DIR}")

  # Test if the targets are found from the build directory.
  add_test(find-package-test ${CMAKE_CTEST_COMMAND}
    -C ${CMAKE_BUILD_TYPE}
    --build-and-test
    "${CMAKE_CURRENT_SOURCE_DIR}/find-package-test"
    "${CMAKE_CURRENT_BINARY_DIR}/find-package-test"
    --build-generator ${CMAKE_GENERATOR}
    --build-makeprogram ${CMAKE_MAKE_PROGRAM}
    --build-options
    "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}"
    "-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}"
    "-DCATLIB_DIR=${PROJECT_BINARY_DIR}"
    "-DPEDANTIC_COMPILE_FLAGS=${PEDANTIC_COMPILE_FLAGS}"
    "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

  # Test if the targets are found when add_subdirectory is used.
  add_test(add-subdirectory-test ${CMAKE_CTEST_COMMAND}
    -C ${CMAKE_BUILD_TYPE}
    --build-and-test
    "${CMAKE_CURRENT_SOURCE_DIR}/add-subdirectory-test"
    "${CMAKE_CURRENT_BINARY_DIR}/add-subdirectory-test"
    --build-generator ${CMAKE_GENERATOR}
    --build-makeprogram ${CMAKE_MAKE_PROGRAM}
    --build-options
    "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    "-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}"
    "-DPEDANTIC_COMPILE_FLAGS=${PEDANTIC_COMPILE_FLAGS}"
    "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
endif ()

# This test are disabled on Windows because it is only *NIX issue.
if (CATLIB_PEDANTIC AND NOT WIN32)
  add_test(static-export-test ${CMAKE_CTEST_COMMAND}
    -C ${CMAKE_BUILD_TYPE}
    --build-and-test
    "${CMAKE_CURRENT_SOURCE_DIR}/static-export-test"
    "${CMAKE_CURRENT_BINARY_DIR}/static-export-test"
    --build-generator ${CMAKE_GENERATOR}
    --build-makeprogram ${CMAKE_MAKE_PROGRAM}
    --build-options
    "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    "-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}"
    "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
endif ()

# Activate optional CUDA tests if CUDA is found. For version selection see
# https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#cpp14-language-features
if (CATLIB_CUDA_TEST)
  if (${CMAKE_VERSION} VERSION_LESS 3.15)
    find_package(CUDA 9.0)
  else ()
    include(CheckLanguage)
    check_language(CUDA)
    if (CMAKE_CUDA_COMPILER)
      enable_language(CUDA OPTIONAL)
      set(CUDA_FOUND TRUE)
    endif ()
  endif ()

  if (CUDA_FOUND)
    add_subdirectory(cuda-test)
    add_test(NAME cuda-test COMMAND catlib-in-cuda-test)
  endif ()
endif ()
