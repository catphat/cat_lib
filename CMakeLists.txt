cmake_minimum_required(VERSION 3.22)





# Determine if catlib is built as a subproject (using add_subdirectory)
# or if it is the master project.
if (NOT DEFINED CATLIB_MASTER_PROJECT)
    set(CATLIB_MASTER_PROJECT OFF)
    if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        set(CATLIB_MASTER_PROJECT ON)
        message(STATUS "CMake version: ${CMAKE_VERSION}")
    endif ()
endif ()

# Joins arguments and places the results in ${result_var}.
function(join result_var)
    set(result "")
    foreach (arg ${ARGN})
        set(result "${result}${arg}")
    endforeach ()
    set(${result_var} "${result}" PARENT_SCOPE)
endfunction()

function(enable_module target)
    if (MSVC)
        set(BMI ${CMAKE_CURRENT_BINARY_DIR}/${target}.ifc)
        target_compile_options(${target}
                PRIVATE /interface /ifcOutput ${BMI}
                INTERFACE /reference catlib=${BMI})
    endif ()
    set_target_properties(${target} PROPERTIES ADDITIONAL_CLEAN_FILES ${BMI})
    set_source_files_properties(${BMI} PROPERTIES GENERATED ON)
endfunction()
# https://cmake.org/cmake/help/v3.0/module/CMakeParseArguments.html
include(CMakeParseArguments)

# Sets a cache variable with a docstring joined from multiple arguments:
#   set(<variable> <value>... CACHE <type> <docstring>...)
# This allows splitting a long docstring for readability.
function(set_verbose)
    # cmake_parse_arguments is broken in CMake 3.4 (cannot parse CACHE) so use
    # list instead.
    list(GET ARGN 0 var)
    list(REMOVE_AT ARGN 0)
    list(GET ARGN 0 val)
    list(REMOVE_AT ARGN 0)
    list(REMOVE_AT ARGN 0)
    list(GET ARGN 0 type)
    list(REMOVE_AT ARGN 0)
    join(doc ${ARGN})
    set(${var} ${val} CACHE ${type} ${doc})
endfunction()

# Set the default CMAKE_BUILD_TYPE to Release.
# This should be done before the project command since the latter can set
# CMAKE_BUILD_TYPE itself (it does so for nmake).
if (CATLIB_MASTER_PROJECT AND NOT CMAKE_BUILD_TYPE)
    set_verbose(CMAKE_BUILD_TYPE Release CACHE STRING
            "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or "
            "CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
endif ()

project(CATLIB CXX)
# https://cmake.org/cmake/help/latest/module/GNUInstallDirs.html
include(GNUInstallDirs)
set_verbose(CATLIB_INC_DIR ${CMAKE_INSTALL_INCLUDEDIR} CACHE STRING
        "Installation directory for include files, a relative path that "
        "will be joined with ${CMAKE_INSTALL_PREFIX} or an absolute path.")

option(CATLIB_PEDANTIC "Enable extra warnings and expensive tests." OFF)
option(CATLIB_WERROR "Halt the compilation with an error on compiler warnings."
        OFF)

# Options that control generation of various targets.
option(CATLIB_DOC "Generate the doc target." ${CATLIB_MASTER_PROJECT})
option(CATLIB_INSTALL "Generate the install target." ${CATLIB_MASTER_PROJECT})
option(CATLIB_TEST "Generate the test target." ${CATLIB_MASTER_PROJECT})
option(CATLIB_OS "Include core requiring OS (Windows/Posix) " ON)
option(CATLIB_MODULE "Build a module instead of a traditional library." OFF)
option(CATLIB_SYSTEM_HEADERS "Expose headers with marking them as system." OFF)

set(CATLIB_CAN_MODULE OFF)
if (CMAKE_CXX_STANDARD GREATER 17 AND
        # msvc 16.10-pre4
        MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 19.29.30035)
    set(CATLIB_CAN_MODULE OFF)
endif ()
if (NOT CATLIB_CAN_MODULE)
    set(CATLIB_MODULE OFF)
    message(STATUS "Module support is disabled.")
endif ()
if (CATLIB_TEST AND CATLIB_MODULE)
    # The tests require {catlib} to be compiled as traditional library
    message(STATUS "Testing is incompatible with build mode 'module'.")
endif ()
set(CATLIB_SYSTEM_HEADERS_ATTRIBUTE "")
if (CATLIB_SYSTEM_HEADERS)
    set(CATLIB_SYSTEM_HEADERS_ATTRIBUTE SYSTEM)
endif ()

# Get version from core.h
file(READ include/catlib/core.h core_h)
if (NOT core_h MATCHES "CATLIB_VERSION ([0-9]+)([0-9][0-9])([0-9][0-9])")
    message(FATAL_ERROR "Cannot get CATLIB_VERSION from core.h.")
endif ()
# Use math to skip leading zeros if any.
math(EXPR CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_MATCH_1})
math(EXPR CPACK_PACKAGE_VERSION_MINOR ${CMAKE_MATCH_2})
math(EXPR CPACK_PACKAGE_VERSION_PATCH ${CMAKE_MATCH_3})
join(CATLIB_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.
        ${CPACK_PACKAGE_VERSION_PATCH})
message(STATUS "Version: ${CATLIB_VERSION}")


message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif ()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
        "${CMAKE_CURRENT_SOURCE_DIR}/support/cmake")

include(cxx17)
include(JoinPaths)


# todo: necessary?
list(FIND CMAKE_CXX_COMPILE_FEATURES "cxx_variadic_templates" index)
if (${index} GREATER -1)
    # Use cxx_variadic_templates instead of more appropriate cxx_std_11 for
    # compatibility with older CMake versions.
    set(CATLIB_REQUIRED_FEATURES cxx_variadic_templates)
endif ()
message(STATUS "Required features: ${CATLIB_REQUIRED_FEATURES}")

if (CATLIB_MASTER_PROJECT AND NOT DEFINED CMAKE_CXX_VISIBILITY_PRESET)
    set_verbose(CMAKE_CXX_VISIBILITY_PRESET hidden CACHE STRING
            "Preset for the export of private symbols")
    set_property(CACHE CMAKE_CXX_VISIBILITY_PRESET PROPERTY STRINGS
            hidden default)
endif ()

if (CATLIB_MASTER_PROJECT AND NOT DEFINED CMAKE_VISIBILITY_INLINES_HIDDEN)
    set_verbose(CMAKE_VISIBILITY_INLINES_HIDDEN ON CACHE BOOL
            "Whether to add a compile flag to hide symbols of inline functions")
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(PEDANTIC_COMPILE_FLAGS -pedantic-errors -Wall -Wextra -pedantic
            -Wold-style-cast -Wundef
            -Wredundant-decls -Wwrite-strings -Wpointer-arith
            -Wcast-qual -Wformat=2 -Wmissing-include-dirs
            -Wcast-align
            -Wctor-dtor-privacy -Wdisabled-optimization
            -Winvalid-pch -Woverloaded-virtual
            -Wconversion -Wundef
            -Wno-ctor-dtor-privacy -Wno-format-nonliteral)
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.6)
        set(PEDANTIC_COMPILE_FLAGS ${PEDANTIC_COMPILE_FLAGS}
                -Wno-dangling-else -Wno-unused-local-typedefs)
    endif ()
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
        set(PEDANTIC_COMPILE_FLAGS ${PEDANTIC_COMPILE_FLAGS} -Wdouble-promotion
                -Wtrampolines -Wzero-as-null-pointer-constant -Wuseless-cast
                -Wvector-operation-performance -Wsized-deallocation -Wshadow)
    endif ()
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.0)
        set(PEDANTIC_COMPILE_FLAGS ${PEDANTIC_COMPILE_FLAGS} -Wshift-overflow=2
                -Wnull-dereference -Wduplicated-cond)
    endif ()
    set(WERROR_FLAG -Werror)
endif ()


if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(PEDANTIC_COMPILE_FLAGS -Wall -Wextra -pedantic -Wconversion -Wundef
            -Wdeprecated -Wweak-vtables -Wshadow
            -Wno-gnu-zero-variadic-macro-arguments)
    check_cxx_compiler_flag(-Wzero-as-null-pointer-constant HAS_NULLPTR_WARNING)
    if (HAS_NULLPTR_WARNING)
        set(PEDANTIC_COMPILE_FLAGS ${PEDANTIC_COMPILE_FLAGS}
                -Wzero-as-null-pointer-constant)
    endif ()
    set(WERROR_FLAG -Werror)
endif ()

if (MSVC)
    set(PEDANTIC_COMPILE_FLAGS /W3)
    set(WERROR_FLAG /WX)
endif ()

if (CATLIB_MASTER_PROJECT AND CMAKE_GENERATOR MATCHES "Visual Studio")
    # If Microsoft SDK is installed create script run-msbuild.bat that
    # calls SetEnv.cmd to set up build environment and runs msbuild.
    # It is useful when building Visual Studio projects with the SDK
    # toolchain rather than Visual Studio.
    include(FindSetEnv)
    if (WINSDK_SETENV)
        set(MSBUILD_SETUP "call \"${WINSDK_SETENV}\"")
    endif ()
    # Set FrameworkPathOverride to get rid of MSB3644 warnings.
    join(netfxpath
            "C:\\Program Files\\Reference Assemblies\\Microsoft\\Framework\\"
            ".NETFramework\\v4.0")
    file(WRITE run-msbuild.bat "
    ${MSBUILD_SETUP}
    ${CMAKE_MAKE_PROGRAM} -p:FrameworkPathOverride=\"${netfxpath}\" %*")
endif ()


function(add_headers VAR)
    set(headers ${${VAR}})
    foreach (header ${ARGN})
        set(headers ${headers} include/catlib/${header})
    endforeach()
    set(${VAR} ${headers} PARENT_SCOPE)
endfunction()

# Define the catlib library, its includes and the needed defines.
# todo
add_headers(CATLIB_HEADERS core.h ringbuffer.hpp)
if (CATLIB_MODULE)
    set(CATLIB_SOURCES src/catlib.module.cc)
elseif (CATLIB_OS)
    set(CATLIB_SOURCES src/catlib.cc src/os.cc)
else()
    set(CATLIB_SOURCES src/catlib.cc)
endif ()

add_library(catlib ${CATLIB_SOURCES} ${CATLIB_HEADERS} README.rst ChangeLog.rst)
add_library(catlib::catlib ALIAS catlib)


if (CATLIB_WERROR)
    target_compile_options(catlib PRIVATE ${WERROR_FLAG})
endif ()
if (CATLIB_PEDANTIC)
    target_compile_options(catlib PRIVATE ${PEDANTIC_COMPILE_FLAGS})
endif ()
if (CATLIB_MODULE)
    enable_module(catlib)
endif ()

target_compile_features(catlib INTERFACE ${catlib_REQUIRED_FEATURES})

target_include_directories(catlib ${CATLIB_SYSTEM_HEADERS_ATTRIBUTE} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CATLIB_INC_DIR}>)

set(CATLIB_DEBUG_POSTFIX d CACHE STRING "Debug library postfix.")

set_target_properties(catlib PROPERTIES
        VERSION ${CATLIB_VERSION} SOVERSION ${CPACK_PACKAGE_VERSION_MAJOR}
        PUBLIC_HEADER "${CATLIB_HEADERS}"
        DEBUG_POSTFIX "${CATLIB_DEBUG_POSTFIX}")


# Set CATLIB_LIB_NAME for pkg-config catlib.pc. We cannot use the OUTPUT_NAME target
# property because it's not set by default.
set(CATLIB_LIB_NAME catlib)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CATLIB_LIB_NAME ${CATLIB_LIB_NAME}${CATLIB_DEBUG_POSTFIX})
endif ()

if (BUILD_SHARED_LIBS)
    if (UNIX AND NOT APPLE AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "SunOS" AND
            NOT EMSCRIPTEN)
        # Fix rpmlint warning:
        # unused-direct-shlib-dependency /usr/lib/libformat.so.1.1.0 /lib/libm.so.6.
        target_link_libraries(catlib -Wl,--as-needed)
    endif ()
    target_compile_definitions(catlib PRIVATE CATLIB_EXPORT INTERFACE CATLIB_SHARED)
endif ()
if (CATLIB_SAFE_DURATION_CAST)
    target_compile_definitions(catlib PUBLIC CATLIB_SAFE_DURATION_CAST)
endif()


add_library(catlib-header-only INTERFACE)
add_library(catlib::catlib-header-only ALIAS catlib-header-only)

target_compile_definitions(catlib-header-only INTERFACE CATLIB_HEADER_ONLY=1)
target_compile_features(catlib-header-only INTERFACE ${CATLIB_REQUIRED_FEATURES})

target_include_directories(catlib-header-only ${CATLIB_SYSTEM_HEADERS_ATTRIBUTE} INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CATLIB_INC_DIR}>)


# Install targets.
if (CATLIB_INSTALL)
    include(CMakePackageConfigHelpers)
    set_verbose(CATLIB_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/catlib CACHE STRING
            "Installation directory for cmake files, a relative path that "
            "will be joined with ${CMAKE_INSTALL_PREFIX} or an absolute "
            "path.")
    set(version_config ${PROJECT_BINARY_DIR}/catlib-config-version.cmake)
    set(project_config ${PROJECT_BINARY_DIR}/catlib-config.cmake)
    set(pkgconfig ${PROJECT_BINARY_DIR}/catlib.pc)
    set(targets_export_name catlib-targets)

    set_verbose(CATLIB_LIB_DIR ${CMAKE_INSTALL_LIBDIR} CACHE STRING
            "Installation directory for libraries, a relative path that "
            "will be joined to ${CMAKE_INSTALL_PREFIX} or an absolute path.")

    set_verbose(CATLIB_PKGCONFIG_DIR ${CMAKE_INSTALL_LIBDIR}/pkgconfig CACHE PATH
            "Installation directory for pkgconfig (.pc) files, a relative "
            "path that will be joined with ${CMAKE_INSTALL_PREFIX} or an "
            "absolute path.")

    # Generate the version, config and target files into the build directory.
    write_basic_package_version_file(
            ${version_config}
            VERSION ${CATLIB_VERSION}
            COMPATIBILITY AnyNewerVersion)

    join_paths(libdir_for_pc_file "\${exec_prefix}" "${CATLIB_LIB_DIR}")
    join_paths(includedir_for_pc_file "\${prefix}" "${CATLIB_INC_DIR}")

    configure_file(
            "${PROJECT_SOURCE_DIR}/support/cmake/catlib.pc.in"
            "${pkgconfig}"
            @ONLY)
    configure_package_config_file(
            ${PROJECT_SOURCE_DIR}/support/cmake/catlib-config.cmake.in
            ${project_config}
            INSTALL_DESTINATION ${CATLIB_CMAKE_DIR})

    set(INSTALL_TARGETS catlib catlib-header-only)

    # Install the library and headers.
    install(TARGETS ${INSTALL_TARGETS} EXPORT ${targets_export_name}
            LIBRARY DESTINATION ${CATLIB_LIB_DIR}
            ARCHIVE DESTINATION ${CATLIB_LIB_DIR}
            PUBLIC_HEADER DESTINATION "${CATLIB_INC_DIR}/catlib"
            FRAMEWORK DESTINATION "."
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    # Use a namespace because CMake provides better diagnostics for namespaced
    # imported targets.
    export(TARGETS ${INSTALL_TARGETS} NAMESPACE catlib::
            FILE ${PROJECT_BINARY_DIR}/${targets_export_name}.cmake)

    # Install version, config and target files.
    install(
            FILES ${project_config} ${version_config}
            DESTINATION ${CATLIB_CMAKE_DIR})
    install(EXPORT ${targets_export_name} DESTINATION ${CATLIB_CMAKE_DIR}
            NAMESPACE catlib::)

    install(FILES $<TARGET_PDB_FILE:${INSTALL_TARGETS}>
            DESTINATION ${CATLIB_LIB_DIR} OPTIONAL)
    install(FILES "${pkgconfig}" DESTINATION "${CATLIB_PKGCONFIG_DIR}")
endif ()

if (CATLIB_DOC)
    add_subdirectory(doc)
endif ()

if (CATLIB_TEST)
    enable_testing()
    add_subdirectory(test)
endif ()
