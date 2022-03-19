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

    // TODO: Don't accept passing arguments to this function.
    /** TODO */
    template<
        option_or_group Option1,
        option_or_group Option2,
        option_or_group... Options>
    detail::option_group<
        detail::exclusive_t::yes,
        detail::subcommand_t::no,
        Option1,
        Option2,
        Options...>
    exclusive(Option1 opt1, Option2 opt2, Options... opts)
    {
        return {{}, {std::move(opt1), std::move(opt2), std::move(opts)...}};
    }

    /** TODO */
    template<command_ Command1, command_ Command2, command_... Commands>
    detail::option_group<
        detail::exclusive_t::yes,
        detail::subcommand_t::no,
        Command1,
        Command2,
        Commands...>
    subcommands(Command1 command1, Command2 command2, Commands... commands)
    {
        return {
            {},
            {std::move(command1), std::move(command2), std::move(commands)...}};
    }

    /** TODO */
    template<option_or_group... Options>
    detail::option_group<
        detail::exclusive_t::no,
        detail::subcommand_t::yes,
        Options...>
    command(std::string_view name, Options... opts)
    {
        return {name, {std::move(opts)...}};
    }

    /** TODO */
    template<
        option_or_group Option1,
        option_or_group Option2,
        option_or_group... Options>
    detail::option_group<
        detail::exclusive_t::no,
        detail::subcommand_t::no,
        Option1,
        Option2,
        Options...>
    group(Option1 opt1, Option2 opt2, Options... opts)
    {
        return {{}, {std::move(opt1), std::move(opt2), std::move(opts)...}};
    }

}}

#endif
