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

#if 1 // TODO
    // TODO: Form exclusive groups using operator||?

    /** TODO */
    template<option_or_group Option, option_or_group... Options>
    detail::option_group<true, Option, Options...>
    exclusive_options(Option opt, Options... opts)
    {
        return {{}, {std::move(opt), std::move(opts)...}};
    }

    /** TODO */
    template<option_or_group Option, option_or_group... Options>
    detail::option_group<false, Option, Options...>
    subcommand(std::string_view name, Option opt, Options... opts)
    {
        return {name, {std::move(opt), std::move(opts)...}};
    }

    /** TODO */
    template<option_or_group Option, option_or_group... Options>
    detail::option_group<false, Option, Options...>
    option_group(Option opt, Options... opts)
    {
        return {{}, {std::move(opt), std::move(opts)...}};
    }
#endif

}}

#endif
