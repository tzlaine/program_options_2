// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PROGRAM_OPTIONS_2_TAG_INVOKE_HPP
#define BOOST_PROGRAM_OPTIONS_2_TAG_INVOKE_HPP

#include <type_traits>


namespace boost::program_options_2::tag_invoke_fn_ns {

#if 0 // TODO: Leaving this here, as it was in the code that accompanies
      // P1895R0, breaks everything.  I should probably figure out why.
    void tag_invoke() = delete;
#endif

    struct tag_invoke_fn
    {
        template<typename Tag, typename... Args>
        // clang-format off
            requires requires(Tag tag, Args &&... args) {
                tag_invoke((Tag &&) tag, (Args &&) args...);
            }
        // clang-format on
        constexpr auto operator()(Tag tag, Args &&... args) const
            noexcept(noexcept(tag_invoke((Tag &&) tag, (Args &&) args...)))
                -> decltype(tag_invoke((Tag &&) tag, (Args &&) args...))
        {
            return tag_invoke((Tag &&) tag, (Args &&) args...);
        }
    };

}

namespace boost::program_options_2 {

    inline namespace tag_invoke_ns {
        inline constexpr tag_invoke_fn_ns::tag_invoke_fn tag_invoke = {};
    }

    // clang-format off
    template<typename Tag, typename... Args>
    concept tag_invocable = requires(Tag tag, Args... args) {
        program_options_2::tag_invoke((Tag &&) tag, (Args &&) args...);
    };

    template<typename Tag, typename... Args>
    concept nothrow_tag_invocable =
        tag_invocable<Tag, Args...> &&
        requires(Tag tag, Args... args) {
            {
                program_options_2::tag_invoke(
                    (Tag &&) tag, (Args &&) args...)
            } noexcept;
        };
    // clang-format on

    template<typename Tag, typename... Args>
    using tag_invoke_result = std::invoke_result<
        decltype(program_options_2::tag_invoke),
        Tag,
        Args...>;

    template<typename Tag, typename... Args>
    using tag_invoke_result_t = std::invoke_result_t<
        decltype(program_options_2::tag_invoke),
        Tag,
        Args...>;

    template<auto & Tag>
    using tag_t = std::decay_t<decltype(Tag)>;

    /** TODO */
    template<typename T>
    struct type_
    {
        using type = T;
    };

    /** TODO */
    template<typename T>
    inline constexpr type_<T> type_c{};

}

#endif
