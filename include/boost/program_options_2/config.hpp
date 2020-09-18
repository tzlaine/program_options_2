// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PROGRAM_OPTIONS_2_CONFIG_HPP
#define BOOST_PROGRAM_OPTIONS_2_CONFIG_HPP

// Included for definition of __cpp_lib_concepts.
#include <iterator>


#ifdef BOOST_PROGRAM_OPTIONS_2_DOXYGEN

/** Boost.ProgramOptions2 will automatically use concepts to constrain templates when
    building in C++20 mode, if the compiler defines `__cpp_lib_concepts`.  To
    disable the use of concepts, define this macro. */
#    define BOOST_PROGRAM_OPTIONS_2_DISABLE_CONCEPTS

/** Boost.ProgramOptions2 will use `std::filesystem` by default.  If you want
    to disable this, define this macro, and `boost::filesystem` will be used
    instead. */
#    define BOOST_PROGRAM_OPTIONS_2_DISABLE_STD_FILESYSTEM

#endif

#if defined(__cpp_lib_concepts) &&                                             \
    !defined(BOOST_PROGRAM_OPTIONS_2_DISABLE_CONCEPTS)
#define BOOST_PROGRAM_OPTIONS_2_USE_CONCEPTS 1
#else
#define BOOST_PROGRAM_OPTIONS_2_USE_CONCEPTS 0
#endif

#if defined(__cpp_lib_filesystem) &&                                           \
    !defined(BOOST_PROGRAM_OPTIONS_2_DISABLE_STD_FILESYSTEM)
#define BOOST_PROGRAM_OPTIONS_2_USE_STD_FILESYSTEM 1
#else
#define BOOST_PROGRAM_OPTIONS_2_USE_STD_FILESYSTEM 0
#endif

#endif
