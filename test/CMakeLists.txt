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
