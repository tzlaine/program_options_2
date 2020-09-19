// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PROGRAM_OPTIONS_2_CONCEPTS_HPP
#define BOOST_PROGRAM_OPTIONS_2_CONCEPTS_HPP

#include <boost/program_options_2/fwd.hpp>

#if defined(BOOST_PROGRAM_OPTIONS_2_DOXYGEN) ||                                \
    BOOST_PROGRAM_OPTIONS_2_USE_CONCEPTS

#include <ranges>


namespace boost { namespace program_options_2 {

    template<typename T>
    concept option_ = detail::is_option<T>::value;
    template<typename T>
    concept group_ = detail::is_group<T>::value;
    template<typename T>
    concept option_or_group = option_<T> || group_<T>;

    template<typename V, typename T>
    concept validator = std::invocable<V, T const &> &&
        std::same_as<std::invoke_result_t<V, T const &>, validation_result>;

    template<typename T, typename Char>
    concept range_of_string_view = std::ranges::range<T> && std::same_as<
        std::decay_t<std::ranges::range_value_t<T>>,
        std::basic_string_view<Char>>;

}}

#endif

#endif
