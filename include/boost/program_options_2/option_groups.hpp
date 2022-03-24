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

    // TODO: Allow automated accumulation of short args, a la "-ub" in "git
    // diff -ub"?

    // TODO: Support a required mutually-exclusive group, in which exactly one
    // must be provided on the command line?

    namespace detail {
        template<typename... Options>
        constexpr bool contains_wrong_group_option(Options const &... opts);

        template<typename Option>
        constexpr bool contains_wrong_group_option(Option const & opt)
        {
            if constexpr (group_<Option>) {
                if constexpr (opt.mutually_exclusive) {
                    return true;
                } else if constexpr (opt.subcommand) {
                    return true;
                } else if constexpr (opt.named_group) {
                    return true;
                } else {
                    return hana::unpack(
                        opt.options, detail::contains_wrong_group_option);
                }
            } else {
                return false;
            }
        }

        template<typename... Options>
        constexpr bool contains_wrong_group_option(Options const &... opts)
        {
            return (detail::contains_wrong_group_option(opts) || ...);
        }

        template<typename... Options>
        constexpr bool contains_positional_option(Options const &... opts);

        template<typename Option>
        constexpr bool contains_positional_option_impl(Option const & opt)
        {
            if constexpr (group_<Option>) {
                return hana::unpack(
                    opt.options, detail::contains_positional_option);
            } else {
                return Option::positional;
            }
        }

        template<typename... Options>
        constexpr bool contains_positional_option(Options const &... opts)
        {
            return (detail::contains_positional_option_impl(opts) || ...);
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
        detail::named_group_t::no,
        Option1,
        Option2,
        Options...>
    exclusive(Option1 opt1, Option2 opt2, Options... opts)
    {
        static_assert(
            !detail::contains_positional_option(opts...),
            "Positional options are not allowed in exclusive groups.");
        static_assert(
            !detail::contains_wrong_group_option(opts...),
            "Mutually-exclusive groups may not contain other exclusive "
            "groups, commands, or named groups.  Unnamed groups are fine.");
        return {{}, {}, {std::move(opt1), std::move(opt2), std::move(opts)...}};
    }

    // TODO: Allow commands to contain a callable that can be dispatched to
    // after the parse.

    /** TODO */
    template<option_or_group... Options>
    detail::option_group<
        detail::exclusive_t::no,
        detail::subcommand_t::yes,
        detail::required_t::no,
        detail::named_group_t::yes,
        Options...>
    command(std::string_view names, Options... opts)
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
    detail::option_group<
        detail::exclusive_t::no,
        detail::subcommand_t::yes,
        detail::required_t::no,
        detail::named_group_t::yes,
        Options...>
    command(std::string_view names, std::string_view help_text, Options... opts)
    {
        BOOST_ASSERT(
            detail::positional(names) &&
            "Command names must not start with dashes.");
        BOOST_ASSERT(
            !names.empty() && "A command with an empty name is not supported.");
        return {names, help_text, {std::move(opts)...}};
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
