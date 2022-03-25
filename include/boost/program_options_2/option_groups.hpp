// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PROGRAM_OPTIONS_2_OPTION_GROUPS_HPP
#define BOOST_PROGRAM_OPTIONS_2_OPTION_GROUPS_HPP

#include <boost/program_options_2/options.hpp>
#include <boost/program_options_2/concepts.hpp>
#include <boost/program_options_2/detail/utility.hpp>


namespace boost { namespace program_options_2 {

    // TODO: Allow automated abbreviation matching?

    // TODO: Allow automated accumulation of short args, a la "-ub" in "git
    // diff -ub"?

    // TODO: Support a required mutually-exclusive group, in which exactly one
    // must be provided on the command line?

    namespace detail {
        template<typename Option>
        struct contains_wrong_group_option_impl
        {
            constexpr static bool call() { return false; }
        };

        template<
            exclusive_t MutuallyExclusive,
            subcommand_t Subcommand,
            required_t Required,
            named_group_t NamedGroup,
            typename Func,
            typename... Options>
        struct contains_wrong_group_option_impl<option_group<
            MutuallyExclusive,
            Subcommand,
            Required,
            NamedGroup,
            Func,
            Options...>>
        {
            constexpr static bool call()
            {
                if constexpr (
                    MutuallyExclusive == exclusive_t::yes ||
                    Subcommand == subcommand_t::yes ||
                    NamedGroup == named_group_t::yes) {
                    return true;
                } else {
                    return (
                        detail::contains_wrong_group_option_impl<
                            Options>::call() ||
                        ...);
                }
            }
        };

        template<typename... Options>
        constexpr bool contains_wrong_group_option()
        {
            return (
                detail::contains_wrong_group_option_impl<Options>::call() ||
                ...);
        }

        template<typename Option>
        struct contains_positional_option_impl
        {
            constexpr static bool call() { return Option::positional; }
        };

        template<
            exclusive_t MutuallyExclusive,
            subcommand_t Subcommand,
            required_t Required,
            named_group_t NamedGroup,
            typename Func,
            typename... Options>
        struct contains_positional_option_impl<option_group<
            MutuallyExclusive,
            Subcommand,
            Required,
            NamedGroup,
            Func,
            Options...>>
        {
            constexpr static bool call()
            {
                return (
                    detail::contains_positional_option_impl<Options>::call() ||
                    ...);
            }
        };

        template<typename... Options>
        constexpr bool contains_positional_option()
        {
            return (
                detail::contains_positional_option_impl<Options>::call() ||
                ...);
        }
    }

    /** TODO */
    template<
        option_or_group Option1,
        option_or_group Option2,
        option_or_group... Options>
    requires(
        !detail::contains_positional_option<Option1, Option2, Options...>() &&
        !detail::contains_wrong_group_option<Option1, Option2, Options...>())
        // clang-format off
    detail::option_group<
        detail::exclusive_t::yes,
        detail::subcommand_t::no,
        detail::required_t::no,
        detail::named_group_t::no,
        detail::no_func,
        Option1,
        Option2,
        Options...>
    exclusive(Option1 opt1, Option2 opt2, Options... opts)
    // clang-format on
    {
        return {{}, {}, {std::move(opt1), std::move(opt2), std::move(opts)...}};
    }

    /** TODO */
    template<option_or_group... Options>
    requires(detail::contains_commands<Options...>())
        // clang-format off
    detail::option_group<
        detail::exclusive_t::no,
        detail::subcommand_t::yes,
        detail::required_t::no,
        detail::named_group_t::yes,
        detail::no_func,
        Options...>
    command(std::string_view names, Options... opts)
    // clang-format on
    {
        BOOST_ASSERT(
            detail::positional(names) &&
            "Command names must not start with dashes.");
        BOOST_ASSERT(
            !names.empty() && "A command with an empty name is not supported.");
        return {names, {}, {std::move(opts)...}};
    }

    /** TODO */
    template<option_or_group... Options>
    requires(detail::contains_commands<Options...>())
        // clang-format off
    detail::option_group<
        detail::exclusive_t::no,
        detail::subcommand_t::yes,
        detail::required_t::no,
        detail::named_group_t::yes,
        detail::no_func,
        Options...>
    command(std::string_view names, std::string_view help_text, Options... opts)
    // clang-format on
    {
        BOOST_ASSERT(
            detail::positional(names) &&
            "Command names must not start with dashes.");
        BOOST_ASSERT(
            !names.empty() && "A command with an empty name is not supported.");
        return {names, help_text, {std::move(opts)...}};
    }

    /** TODO */
    template<typename Func, option_or_group... Options>
    requires(!detail::contains_commands<Options...>())
        // clang-format off
    detail::option_group<
        detail::exclusive_t::no,
        detail::subcommand_t::yes,
        detail::required_t::no,
        detail::named_group_t::yes,
        std::remove_cvref_t<Func>,
        Options...>
    command(Func && func, std::string_view names, Options... opts)
    // clang-format on
    {
        BOOST_ASSERT(
            detail::positional(names) &&
            "Command names must not start with dashes.");
        BOOST_ASSERT(
            !names.empty() && "A command with an empty name is not supported.");
        return {names, {}, {std::move(opts)...}, (Func &&) func};
    }

    /** TODO */
    template<typename Func, option_or_group... Options>
    requires(!detail::contains_commands<Options...>())
        // clang-format off
    detail::option_group<
        detail::exclusive_t::no,
        detail::subcommand_t::yes,
        detail::required_t::no,
        detail::named_group_t::yes,
        std::remove_cvref_t<Func>,
        Options...>
    command(
        Func && func,
        std::string_view names,
        std::string_view help_text,
        Options... opts)
    // clang-format on
    {
        BOOST_ASSERT(
            detail::positional(names) &&
            "Command names must not start with dashes.");
        BOOST_ASSERT(
            !names.empty() && "A command with an empty name is not supported.");
        return {names, help_text, {std::move(opts)...}, (Func &&) func};
    }

    /** Creates a group of options.  The group is always flattened into the
        other options it is with; it exists only for organizational
        purposes. */
    template<
        option_or_group Option1,
        option_or_group Option2,
        option_or_group... Options>
    detail::option_group<
        detail::exclusive_t::no,
        detail::subcommand_t::no,
        detail::required_t::no,
        detail::named_group_t::no,
        detail::no_func,
        Option1,
        Option2,
        Options...>
    group(Option1 opt1, Option2 opt2, Options... opts)
    {
        return {{}, {}, {std::move(opt1), std::move(opt2), std::move(opts)...}};
    }

    /** Creates a group of options that appears with the given name and
        description in the printed help text.  The group is only significant
        when printing help; a group is always flattened into the other options
        it is with when not printing.  The description may be empty. */
    template<
        option_or_group Option1,
        option_or_group Option2,
        option_or_group... Options>
    detail::option_group<
        detail::exclusive_t::no,
        detail::subcommand_t::no,
        detail::required_t::no,
        detail::named_group_t::yes,
        detail::no_func,
        Option1,
        Option2,
        Options...>
    group(
        std::string_view name,
        std::string_view description,
        Option1 opt1,
        Option2 opt2,
        Options... opts)
    {
        BOOST_ASSERT(
            !name.empty() &&
            "A named group with an empty name is not supported.");
        return {
            name,
            description,
            {std::move(opt1), std::move(opt2), std::move(opts)...}};
    }

}}

#endif
