// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PROGRAM_OPTIONS_2_OPTION_GROUPS_HPP
#define BOOST_PROGRAM_OPTIONS_2_OPTION_GROUPS_HPP

#include <boost/program_options_2/option.hpp>
#include <boost/program_options_2/concepts.hpp>


namespace boost { namespace program_options_2 {

    /** TODO */
    template<option_or_group Option, option_or_group... Options>
    detail::option_group<true, false, Option, Options...>
    exclusive(Option opt, Options... opts)
    {
        return {{}, {std::move(opt), std::move(opts)...}};
    }

    /** TODO */
    template<command Command1, command Command2, command... Commands>
    detail::option_group<true, false, Option, Options...>
    subcommands(Command1 command1, Command2 command2, Commands... commands)
    {
        return {name, {std::move(opt), std::move(opts)...}};
    }

    /** TODO */
    template<option_or_group Option, option_or_group... Options>
    detail::option_group<false, true, Option, Options...>
    command(std::string_view name, Option opt, Options... opts)
    {
        return {name, {std::move(opt), std::move(opts)...}};
    }

    /** TODO */
    template<option_or_group Option, option_or_group... Options>
    detail::option_group<false, false, Option, Options...>
    group(Option opt, Options... opts)
    {
        return {{}, {std::move(opt), std::move(opts)...}};
    }

}}

#endif
