// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PROGRAM_OPTIONS_2_PARSE_COMMAND_LINE_HPP
#define BOOST_PROGRAM_OPTIONS_2_PARSE_COMMAND_LINE_HPP

#include <boost/program_options_2/fwd.hpp>
#include <boost/program_options_2/arg_view.hpp>
#include <boost/program_options_2/options.hpp>
#include <boost/program_options_2/concepts.hpp>
#include <boost/program_options_2/detail/parsing.hpp>
#include <boost/program_options_2/decorators.hpp>


namespace boost { namespace program_options_2 {

    /** TODO */
    template<
        range_of_string_view<char> Args,
        option_or_group Option,
        option_or_group... Options>
    auto parse_command_line(
        Args const & args,
        std::string_view program_desc,
        std::ostream & os,
        customizable_strings const & strings,
        Option opt,
        Options... opts)
    {
        BOOST_ASSERT(args.begin() != args.end());
        detail::check_options(false, opt, opts...);

        bool const no_help = detail::no_help_option(opt, opts...);

        if (no_help && detail::argv_contains_default_help_flag(strings, args)) {
            detail::print_help_and_exit(
                0,
                strings,
                *args.begin(),
                program_desc,
                os,
                true,
                opt,
                opts...);
        }

        return detail::parse_options_into_tuple(
            strings, args, program_desc, os, no_help, opt, opts...);
    }

    /** TODO */
    template<
        range_of_string_view<char> Args,
        option_or_group Option,
        option_or_group... Options>
    auto parse_command_line(
        Args const & args,
        std::string_view program_desc,
        std::ostream & os,
        Option opt,
        Options... opts)
    {
        return program_options_2::parse_command_line(
            args, program_desc, os, customizable_strings{}, opt, opts...);
    }

    /** TODO */
    template<option_or_group Option, option_or_group... Options>
    auto parse_command_line(
        int argc,
        char const ** argv,
        std::string_view program_desc,
        std::ostream & os,
        customizable_strings const & strings,
        Option opt,
        Options... opts)
    {
        return program_options_2::parse_command_line(
            arg_view(argc, argv), program_desc, os, strings, opt, opts...);
    }

    /** TODO */
    template<option_or_group Option, option_or_group... Options>
    auto parse_command_line(
        int argc,
        char const ** argv,
        std::string_view program_desc,
        std::ostream & os,
        Option opt,
        Options... opts)
    {
        return program_options_2::parse_command_line(
            arg_view(argc, argv),
            program_desc,
            os,
            customizable_strings{},
            opt,
            opts...);
    }

#if defined(BOOST_PROGRAM_OPTIONS_2_DOXYGEN) || defined(_MSC_VER)

    /** TODO */
    template<
        range_of_string_view<wchar_t> Args,
        option_or_group Option,
        option_or_group... Options>
    auto parse_command_line(
        Args const & args,
        std::wstring_view program_desc,
        std::wostream & os,
        customizable_strings const & strings,
        Option opt,
        Options... opts)
    {
        BOOST_ASSERT(args.begin() != args.end());
        detail::check_options(false, opt, opts...);

        bool const no_help = detail::no_help_option(opt, opts...);

        if (no_help && detail::argv_contains_default_help_flag(strings, args)) {
            detail::print_help_and_exit(
                0, strings, args.front(), program_desc, os, true, opt, opts...);
        }

        return detail::parse_options_into_tuple(
            strings, args, program_desc, os, no_help, opt, opts...);
    }

    /** TODO */
    template<
        range_of_string_view<wchar_t> Args,
        option_or_group Option,
        option_or_group... Options>
    auto parse_command_line(
        Args const & args,
        std::wstring_view program_desc,
        std::wostream & os,
        Option opt,
        Options... opts)
    {
        return program_options_2::parse_command_line(
            args, program_desc, os, customizable_strings{}, opt, opts...);
    }

    /** TODO */
    template<option_or_group Option, option_or_group... Options>
    auto parse_command_line(
        int argc,
        wchar_t const ** argv,
        std::wstring_view program_desc,
        customizable_strings const & strings,
        std::wostream & os,
        Option opt,
        Options... opts)
    {
        return program_options_2::parse_command_line(
            arg_view(argc, argv), program_desc, os, strings, opt, opts...);
    }

    /** TODO */
    template<option_or_group Option, option_or_group... Options>
    auto parse_command_line(
        int argc,
        wchar_t const ** argv,
        std::wstring_view program_desc,
        std::wostream & os,
        Option opt,
        Options... opts)
    {
        return program_options_2::parse_command_line(
            arg_view(argc, argv),
            program_desc,
            os,
            customizable_strings{},
            opt,
            opts...);
    }

#endif

}}

#endif
