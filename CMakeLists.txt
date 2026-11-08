cmake_minimum_required(VERSION 3.14...3.20)
project(program_options_2)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

##################################################
# C++ standard version selection
##################################################
set(CXX_STD 20 CACHE STRING "Set to 14, 17, etc., to enable C++14, C++17, etc.")
message("-- Using -std=c++${CXX_STD}")

# to just check the different frontend flavours we could use the following code
if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    get_filename_component(CNAME "${CMAKE_CXX_COMPILER}" NAME)
    message(STATUS "Detected MSVC style compiler frontend '" ${CNAME} "'")
    add_definitions(/W3)
    add_compile_options(/Zc:__cplusplus) # not sure, if it necessary for clang-cl
elseif (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
    get_filename_component(CNAME "${CMAKE_CXX_COMPILER}" NAME)
    message(STATUS "Detected GNU style compiler frontend '" ${CNAME} "'")
    add_definitions(-Wall)
endif ()
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_definitions(-Wall -Wno-overloaded-shift-op-parentheses)
endif()

##################################################
# Dependencies
##################################################
include(dependencies)


##################################################
# Sanitizers
##################################################
set(USE_ASAN false CACHE BOOL "Set to true to enable -fsanitize=address when building tests.")
set(USE_UBSAN false CACHE BOOL "Set to true to enable -fsanitize=undefined when building tests.")
if (USE_ASAN AND USE_UBSAN)
    message(FATAL_ERROR "USE_ASAN and USE_UBSAN must not be enabled at the same time")
elseif (USE_ASAN)
    set(compile_flags -fsanitize=address)
    set(link_flags -fsanitize=address)
elseif (USE_UBSAN)
    set(compile_flags -fsanitize=undefined)
    set(link_flags -fsanitize=undefined)
endif()


add_library(program_options_2 INTERFACE)
target_include_directories(program_options_2 INTERFACE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(program_options_2 INTERFACE boost)
if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    target_compile_options(program_options_2 INTERFACE -ftemplate-backtrace-limit=0)
endif()


##################################################
# Subdirectories
##################################################
add_subdirectory(test)
add_subdirectory(example)
