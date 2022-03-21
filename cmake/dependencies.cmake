# Copyright Louis Dionne 2016
# Copyright Zach Laine 2016
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

###############################################################################
# Boost
###############################################################################
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.71.0 COMPONENTS ${boost_components})
if (Boost_INCLUDE_DIR)
  add_library(boost INTERFACE)
  target_include_directories(boost INTERFACE ${Boost_INCLUDE_DIR})
else ()
  if (NOT BOOST_BRANCH)
    set(BOOST_BRANCH master)
  endif()
  if (NOT EXISTS ${CMAKE_BINARY_DIR}/boost_root)
    message("-- Boost was not found; it will be cloned locally from ${BOOST_BRANCH}.")
    add_custom_target(
      boost_root_clone
      git clone --depth 100 -b ${BOOST_BRANCH}
        https://github.com/boostorg/boost.git boost_root
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    if (MSVC)
      set(bootstrap_cmd ./bootstrap.bat)
    else()
      set(bootstrap_cmd ./bootstrap.sh)
    endif()
    add_custom_target(
      boost_clone
      COMMAND git submodule init libs/assert
      COMMAND git submodule init libs/config
      COMMAND git submodule init libs/core
      COMMAND git submodule init tools/build
      COMMAND git submodule init libs/headers
      COMMAND git submodule init tools/boost_install
      COMMAND git submodule update --jobs 3
      COMMAND ${bootstrap_cmd}
      COMMAND ./b2 headers
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/boost_root
      DEPENDS boost_root_clone)
  endif()
  add_library(boost INTERFACE)
  add_dependencies(boost boost_clone)
  target_include_directories(boost INTERFACE ${CMAKE_BINARY_DIR}/boost_root)
  set(Boost_INCLUDE_DIR ${CMAKE_BINARY_DIR}/boost_root) # Needed for try_compile()s.
endif ()


###############################################################################
# Boost.Text (Proposed)
###############################################################################
if (NOT TEXT_ROOT)
  message(FATAL_ERROR "TEXT_ROOT not defined")
endif()
add_library(text STATIC IMPORTED GLOBAL)
set_target_properties(text PROPERTIES IMPORTED_LOCATION ${TEXT_ROOT}/build/libtext.a)
target_link_libraries(boost INTERFACE text)
include_directories(${TEXT_ROOT}/include)


###############################################################################
# Boost.Parser (Proposed)
###############################################################################
if (NOT PARSER_ROOT)
  message(FATAL_ERROR "PARSER_ROOT not defined")
endif()
include_directories(${PARSER_ROOT}/include)


###############################################################################
# GoogleTest
###############################################################################
add_subdirectory(${CMAKE_SOURCE_DIR}/googletest-release-1.10.0)
target_include_directories(gtest      INTERFACE ${CMAKE_HOME_DIRECTORY}/googletest-release-1.10.0/googletest/include)
target_include_directories(gtest_main INTERFACE ${CMAKE_HOME_DIRECTORY}/googletest-release-1.10.0/googletest/include)


###############################################################################
# Google Benchmark
###############################################################################
add_subdirectory(${CMAKE_SOURCE_DIR}/benchmark-v1.1.0)
target_include_directories(benchmark INTERFACE ${CMAKE_HOME_DIRECTORY}/benchmark-v1.1.0/include)
