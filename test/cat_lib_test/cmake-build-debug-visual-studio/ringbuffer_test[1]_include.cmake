if(EXISTS "D:/ISHQ-V/DEV/CPP/cat_lib/test/cat_lib_test/cmake-build-debug-visual-studio/ringbuffer_test[1]_tests.cmake")
  include("D:/ISHQ-V/DEV/CPP/cat_lib/test/cat_lib_test/cmake-build-debug-visual-studio/ringbuffer_test[1]_tests.cmake")
else()
  add_test(ringbuffer_test_NOT_BUILT ringbuffer_test_NOT_BUILT)
endif()
