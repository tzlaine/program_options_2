// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PROGRAM_OPTIONS_2_OPTION_GROUPS_HPP
#define BOOST_PROGRAM_OPTIONS_2_OPTION_GROUPS_HPP

#include <boost/program_options_2/options.hpp>
#include <boost/program_options_2/concepts.hpp>


namespace boost { namespace program_options_2 {

    // TODO: Allow automated abbreviation matching?

    // TODO: Support a required mutually-exclusive group, in which exactly one
    // must be provided on the command line?

    namespace detail {
        template<typename Option>
        struct contains_exclusive_option_impl
        {
            constexpr bool operator()() { return false; }
        };

        template<
            exclusive_t MutuallyExclusive,
            subcommand_t Subcommand,
            required_t Required,
            typename... Options>
        struct contains_exclusive_option_impl<detail::option_group<
            MutuallyExclusive,
            Subcommand,
            Required,
            Options...>>
        {
            constexpr bool operator()()
            {
                if constexpr (MutuallyExclusive == exclusive_t::yes) {
                    return true;
                } else {
                    return (
                        detail::contains_exclusive_option_impl<Options>{}() ||
                        ...);
                }
            }
        };

        template<typename... Options>
        constexpr bool contains_exclusive_option()
        {
            return (detail::contains_exclusive_option_impl<Options>{}() || ...);
        }

        template<typename Option>
        struct contains_positional_option_impl
        {
            constexpr bool operator()() { return Option::positional; }
        };

        template<
            exclusive_t MutuallyExclusive,
            subcommand_t Subcommand,
            required_t Required,
            typename... Options>
        struct contains_positional_option_impl<detail::option_group<
            MutuallyExclusive,
            Subcommand,
            Required,
            Options...>>
        {
            constexpr bool operator()()
            {
                return (
                    detail::contains_positional_option_impl<Options>{}() ||
                    ...);
            }
        };

        template<typename... Options>
        constexpr bool contains_positional_option()
        {
            return (detail::contains_positional_option_impl<Options>{}() || ...);
        }
    }

    /** TODO */
    template<
        option_or_group Option1,
        option_or_group Option2,
        option_or_group... Options>
    detail::option_group<
        detail::exclusive_t::yes,
        detail::subcommand_t::no,
        detail::required_t::no,
        Option1,
        Option2,
        Options...>
    exclusive(Option1 opt1, Option2 opt2, Options... opts)
    {
        static_assert(
            !detail::contains_positional_option<Option1, Option2, Options...>(),
            "Positional options are not allowed in exclusive groups.");
        static_assert(
            !detail::contains_exclusive_option<Option1, Option2, Options...>(),
            "Nesting of exclusive options is not allowed.");
        return {{}, {}, {std::move(opt1), std::move(opt2), std::move(opts)...}};
    }

    // TODO: Add a required_t::yes overload of exclusive.

    // TODO: Allow commands to contain a callable that can be dispatched to
    // after the parse.

    /** TODO */
    template<option_or_group... Options>
    detail::option_group<
        detail::exclusive_t::no,
        detail::subcommand_t::yes,
        detail::required_t::no,
        Options...>
    command(std::string_view names, Options... opts)
    {
        return {names, {}, {std::move(opts)...}};
    }

    /** TODO */
    template<option_or_group... Options>
    detail::option_group<
        detail::exclusive_t::no,
        detail::subcommand_t::yes,
        detail::required_t::no,
        Options...>
    command(std::string_view names, std::string_view help_text, Options... opts)
    {
        return {names, help_text, {std::move(opts)...}};
    }

    // TODO: Support group names and descriptions?

    /** TODO */
    template<
        option_or_group Option1,
        option_or_group Option2,
        option_or_group... Options>
    detail::option_group<
        detail::exclusive_t::no,
        detail::subcommand_t::no,
        detail::required_t::no,
        Option1,
        Option2,
        Options...>
    group(Option1 opt1, Option2 opt2, Options... opts)
    {
        return {{}, {}, {std::move(opt1), std::move(opt2), std::move(opts)...}};
    }

}}

#endif
