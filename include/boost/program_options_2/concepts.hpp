// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PROGRAM_OPTIONS_2_CONCEPTS_HPP
#define BOOST_PROGRAM_OPTIONS_2_CONCEPTS_HPP

#include <boost/program_options_2/fwd.hpp>

#if defined(BOOST_PROGRAM_OPTIONS_2_DOXYGEN) || \
    BOOST_PROGRAM_OPTIONS_2_USE_CONCEPTS

#include <ranges>


namespace boost { namespace program_options_2 {

    namespace detail {
        struct arbitrary_type
        {};
    }

    // clang-format off

    template<typename T>
    concept option_ = detail::is_option<T>::value;
    template<typename T>
    concept group_ = detail::is_group<T>::value;
    template<typename T>
    concept option_or_group = option_<T> || group_<T>;
    template<typename T>
    concept command_ = detail::is_command<T>::value;

    template<typename V, typename T>
    concept validator = std::invocable<V, T const &> &&
        std::same_as<std::invoke_result_t<V, T const &>, validation_result>;

    template<typename T, typename Char>
    concept range_of_string_view = std::ranges::range<T> && std::same_as<
        std::decay_t<std::ranges::range_value_t<T>>,
        std::basic_string_view<Char>>;

    template<typename T>
    concept insertable = requires(T t) {
        t.insert(t.end(), *t.begin());
    };

    template<typename T>
    concept erased_type = requires(T t) {
        // If these are all well-formed, this is probably an erased type.
        t = (void *)0;
        t = no_value{};
        t = std::pair<int, std::vector<double> *>{};
        t = 0;
        t = 0.0;
        t = std::string_view{};
    };

    template<typename T>
    using map_key_t =
        std::remove_const_t<typename std::ranges::range_value_t<T>::first_type>;
    template<typename T>
    using map_value_t = typename std::ranges::range_value_t<T>::second_type;

    template<typename T>
    concept options_map = std::ranges::range<T> &&
        (std::same_as<map_key_t<T>, std::string> &&
         requires(T t, std::string const & s) {
             { t[s] } -> std::same_as<map_value_t<T> &>;
         } ||
         std::same_as<map_key_t<T>, std::string_view> &&
         requires(T t, std::string_view const & s) {
             { t[s] } -> std::same_as<map_value_t<T> &>;
         }) &&
         erased_type<map_value_t<T>> &&
         requires(T t) { t.erase(t.begin()); };

    // clang-format on

}}

#endif

#endif
