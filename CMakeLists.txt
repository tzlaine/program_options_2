cmake_minimum_required(VERSION 3.5)
project(parse)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

##################################################
# C++ standard version selection
##################################################
set(CXX_STD 17 CACHE STRING "Set to 14, 17, etc., to enable C++14, C++17, etc.")
message("-- Using -std=c++${CXX_STD}")

set(clang_on_linux false)
if (CMAKE_CXX_COMPILER_ID STREQUAL Clang)
    add_definitions(-g -Wall)
    if (CMAKE_SYSTEM_NAME STREQUAL Linux)
        set(clang_on_linux true)
    endif ()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    add_definitions(-g -Wall)
endif ()

set(boost_components filesystem system)
include(dependencies)

find_package(PythonInterp)
if (PYTHONINTERP_FOUND)
    message("-- Found Python ${PYTHON_VERSION_STRING} (${PYTHON_EXECUTABLE})")
endif ()


##################################################
# Sanitizers
##################################################
set(USE_ASAN false CACHE BOOL "Set to true to enable -fsanitize=address when building tests.") 
set(USE_UBSAN false CACHE BOOL "Set to true to enable -fsanitize=undefined when building tests.")
if (USE_ASAN AND USE_UBSAN)
    message(FATAL_ERROR "USE_ASAN and USE_UBSAN must not be enabled at the same time")
elseif (USE_ASAN)
    set(link_flags -fsanitize=address)
elseif (USE_UBSAN)
    set(link_flags -fsanitize=undefined)
endif()


add_library(program_options_2 INTERFACE)
target_include_directories(program_options_2 INTERFACE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(program_options_2 INTERFACE boost)
if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    target_compile_options(program_options_2 INTERFACE -ftemplate-backtrace-limit=0)
endif()

add_subdirectory(test)
add_subdirectory(example)
