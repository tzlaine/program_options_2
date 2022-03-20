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
            !opt1.positional && !opt1.positional && (!opts.positional && ...),
            "Positional options are not allowed in exclusive groups.");
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
