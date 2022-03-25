include_directories(${CMAKE_HOME_DIRECTORY})

include(CTest)

enable_testing()

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} -j4 -C ${CMAKE_CFG_INTDIR})

if (UNIX)
  add_definitions(-DUNIX_BUILD)
endif()

add_executable(
    compile_tests
    compile_tests_main.cpp
)
set_property(TARGET compile_tests PROPERTY CXX_STANDARD ${CXX_STD})
target_link_libraries(compile_tests program_options_2)

macro(add_test_executable name)
    add_executable(${name} ${name}.cpp)
    set_property(TARGET ${name} PROPERTY CXX_STANDARD ${CXX_STD})
    target_link_libraries(${name} program_options_2 ${Boost_LIBRARIES} gtest gtest_main ${link_flags})
    target_compile_options(${name} PRIVATE ${link_flags})
    if (MSVC)
        target_compile_options(${name} PRIVATE /source-charset:utf-8)
    endif ()
    add_test(NAME ${name} COMMAND ${name} --gtest_catch_exceptions=1)
endmacro()

add_test_executable(detail)
add_test_executable(options)
add_test_executable(printing)
add_test_executable(parse_command_line)
add_test_executable(storage)
add_test_executable(groups)

function(add_compile_fail_test name)
    try_compile(
        compiles
        ${CMAKE_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/${name}.cpp
        COMPILE_DEFINITIONS
            -std=c++20 -I${Boost_INCLUDE_DIR} -I${TEXT_ROOT}/include
            -I${PARSER_ROOT}/include -I${CMAKE_SOURCE_DIR}/include
        OUTPUT_VARIABLE foo
    )
    if (compiles)
        message(FATAL_ERROR "Compile-fail test ${name} does not fail to compile.")
    endif ()
    #message("${foo}")
endfunction()

add_compile_fail_test(fail_nested_exclusives_1)
add_compile_fail_test(fail_nested_exclusives_2)
add_compile_fail_test(fail_exclusives_with_positionals_1)
add_compile_fail_test(fail_exclusives_with_positionals_2)
add_compile_fail_test(fail_tuple_parse_with_commands)
add_compile_fail_test(fail_leaf_command_no_invocable)
add_compile_fail_test(fail_interior_command_with_invocable)
