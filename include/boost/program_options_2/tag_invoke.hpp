// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PROGRAM_OPTIONS_2_TAG_INVOKE_HPP
#define BOOST_PROGRAM_OPTIONS_2_TAG_INVOKE_HPP

#include <type_traits>


namespace boost::program_options_2::tag_invoke_fn_ns {

    void tag_invoke();

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

    /** Used to wrap a type that must be specified on some `tag_invoke`able
        entity.  Most `tag_invoke`ables are nontemplates.  For templates, the
        template parameter must be specified, but the `tag_invoke` function
        provides no way to do that.  `type_` is part of the solution, and
        `type_c` is the other.

        \see `any_cast_fn` for an example of use. */
    template<typename T>
    struct type_
    {
        using type = T;
    };

    /** Used to pass a type that must be specified on some `tag_invoke`able
        entity, when invoking that entity.  Most `tag_invoke`ables are
        nontemplates.  For templates, the template parameter must be
        specified, but the `tag_invoke` function provides no way to do that.
        `type_c` is part of the solution, and `type_` is the other.

        \see `any_cast_fn` for an example of use. */
    template<typename T>
    inline constexpr type_<T> type_c{};

}

#endif
