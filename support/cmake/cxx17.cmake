# C++17 feature support detection

include(CheckCXXCompilerFlag)
function (fmt_check_cxx_compiler_flag flag result)
  if (NOT MSVC)
    check_cxx_compiler_flag("${flag}" ${result})
  endif ()
endfunction ()

if (NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
message(STATUS "CXX_STANDARD: ${CMAKE_CXX_STANDARD}")

if (CMAKE_CXX_STANDARD EQUAL 20)
  fmt_check_cxx_compiler_flag(-std=c++20 has_std_20_flag)
  fmt_check_cxx_compiler_flag(-std=c++2a has_std_2a_flag)

  if (has_std_20_flag)
    set(CXX_STANDARD_FLAG -std=c++20)
  elseif (has_std_2a_flag)
    set(CXX_STANDARD_FLAG -std=c++2a)
  endif ()

elseif (CMAKE_CXX_STANDARD EQUAL 17)
  fmt_check_cxx_compiler_flag(-std=c++17 has_std_17_flag)
  fmt_check_cxx_compiler_flag(-std=c++1z has_std_1z_flag)

  if (has_std_17_flag)
    set(CXX_STANDARD_FLAG -std=c++17)
  elseif (has_std_1z_flag)
    set(CXX_STANDARD_FLAG -std=c++1z)
  endif ()

endif ()
