// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PROGRAM_OPTIONS_2_PARSE_COMMAND_LINE_HPP
#define BOOST_PROGRAM_OPTIONS_2_PARSE_COMMAND_LINE_HPP

#include <boost/program_options_2/fwd.hpp>
#include <boost/program_options_2/arg_view.hpp>
#include <boost/program_options_2/concepts.hpp>
#include <boost/program_options_2/options.hpp>
#include <boost/program_options_2/storage.hpp>
#include <boost/program_options_2/detail/parsing.hpp>
#include <boost/program_options_2/decorators.hpp>


namespace boost { namespace program_options_2 {

    // tuple overloads

    /** Parse `args` for the options `opt, opts...`, and return a tuple that
        contains the results of the parse.  Each element of tuple corresponds
        one-to-one to `opt, opts...` (except that grouping is ignored).  If an
        error occurs, or if the user requests help or version, output will be
        printed to `os` and the program will exit.  The return code on exit
        will be `1` if an error occurred, or `0` otherwise.  The given options
        must not contain any commands. */
    template<
        range_of_string_view<char> Args,
        option_or_group Option,
        option_or_group... Options>
    requires(!detail::contains_commands<Option, Options...>())
        // clang-format off
    auto parse_command_line(
        Args const & args,
        std::string_view program_desc,
        std::ostream & os,
        customizable_strings const & strings,
        Option opt,
        Options... opts)
    // clang-format on
    {
        BOOST_ASSERT(args.begin() != args.end());
        detail::check_options(strings, opt, opts...);

        bool const no_help = detail::no_help_option(opt, opts...);

        if (no_help && detail::argv_contains_default_help_flag(strings, args)) {
            detail::parse_contexts_vec const parse_contexts;
            detail::print_help_and_exit(
                0,
                strings,
                *args.begin(),
                program_desc,
                os,
                true,
                parse_contexts,
                opt,
                opts...);
        }

        return detail::parse_options_as_tuple(
            strings, args, program_desc, os, no_help, opt, opts...);
    }

    /** Parse `args` for the options `opt, opts...`, and return a tuple that
        contains the results of the parse.  Each element of tuple corresponds
        one-to-one to `opt, opts...` (except that grouping is ignored).  If an
        error occurs, or if the user requests help or version, output will be
        printed to `os` and the program will exit.  The return code on exit
        will be `1` if an error occurred, or `0` otherwise.  The given options
        must not contain any commands. */
    template<
        range_of_string_view<char> Args,
        option_or_group Option,
        option_or_group... Options>
    requires(!detail::contains_commands<Option, Options...>())
        // clang-format off
    auto parse_command_line(
        Args const & args,
        std::string_view program_desc,
        std::ostream & os,
        Option opt,
        Options... opts)
    // clang-format on
    {
        return program_options_2::parse_command_line(
            args, program_desc, os, customizable_strings{}, opt, opts...);
    }

    /** Parse `[argv, argv + argc)` for the options `opt, opts...`, and return
        a tuple that contains the results of the parse.  Each element of tuple
        corresponds one-to-one to `opt, opts...` (except that grouping is
        ignored).  If an error occurs, or if the user requests help or
        version, output will be printed to `os` and the program will exit.
        The return code on exit will be `1` if an error occurred, or `0`
        otherwise.  The given options must not contain any commands. */
    template<option_or_group Option, option_or_group... Options>
    requires(!detail::contains_commands<Option, Options...>())
        // clang-format off
     auto parse_command_line(
        int argc,
        char const ** argv,
        std::string_view program_desc,
        std::ostream & os,
        customizable_strings const & strings,
        Option opt,
        Options... opts)
    // clang-format on
    {
        return program_options_2::parse_command_line(
            arg_view(argc, argv), program_desc, os, strings, opt, opts...);
    }

    /** Parse `[argv, argv + argc)` for the options `opt, opts...`, and return
        a tuple that contains the results of the parse.  Each element of tuple
        corresponds one-to-one to `opt, opts...` (except that grouping is
        ignored).  If an error occurs, or if the user requests help or
        version, output will be printed to `os` and the program will exit.
        The return code on exit will be `1` if an error occurred, or `0`
        otherwise.  The given options must not contain any commands. */
    template<option_or_group Option, option_or_group... Options>
    requires(!detail::contains_commands<Option, Options...>())
        // clang-format off
     auto parse_command_line(
        int argc,
        char const ** argv,
        std::string_view program_desc,
        std::ostream & os,
        Option opt,
        Options... opts)
    // clang-format on
    {
        return program_options_2::parse_command_line(
            arg_view(argc, argv),
            program_desc,
            os,
            customizable_strings{},
            opt,
            opts...);
    }



    // map overloads

    /** Parse `args` for the options `opt, opts...`, and place the results of
        the parse in `map`.  For any option `o`, the key for its associated
        entry in `map` is `storage_name(o)`.  If an error occurs, or if the
        user requests help or version, output will be printed to `os` and the
        program will exit. The return code on exit will be `1` if an error
        occurred, or `0` otherwise. */
    template<
        range_of_string_view<char> Args,
        options_map OptionsMap,
        option_or_group Option,
        option_or_group... Options>
    void parse_command_line(
        Args const & args,
        OptionsMap & map,
        std::string_view program_desc,
        std::ostream & os,
        customizable_strings const & strings,
        Option opt,
        Options... opts)
    {
        BOOST_ASSERT(args.begin() != args.end());
        detail::check_options(strings, opt, opts...);

        if constexpr (detail::contains_commands<Option, Options...>()) {
            detail::parse_commands(
                map, strings, args, program_desc, os, true, opt, opts...);
        } else {
            bool const no_help = detail::no_help_option(opt, opts...);

            detail::parse_contexts_vec const parse_contexts;
            if (no_help &&
                detail::argv_contains_default_help_flag(strings, args)) {
                detail::print_help_and_exit(
                    0,
                    strings,
                    *args.begin(),
                    program_desc,
                    os,
                    true,
                    parse_contexts,
                    opt,
                    opts...);
            }

            detail::parse_options_into_map(
                map,
                strings,
                false,
                args,
                program_desc,
                os,
                no_help,
                true,
                opt,
                opts...);
        }
    }

    /** Parse `args` for the options `opt, opts...`, and place the results of
        the parse in `map`.  For any option `o`, the key for its associated
        entry in `map` is `storage_name(o)`.  If an error occurs, or if the
        user requests help or version, output will be printed to `os` and the
        program will exit. The return code on exit will be `1` if an error
        occurred, or `0` otherwise. */
    template<
        range_of_string_view<char> Args,
        options_map OptionsMap,
        option_or_group Option,
        option_or_group... Options>
    void parse_command_line(
        Args const & args,
        OptionsMap & map,
        std::string_view program_desc,
        std::ostream & os,
        Option opt,
        Options... opts)
    {
        return program_options_2::parse_command_line(
            args, map, program_desc, os, customizable_strings{}, opt, opts...);
    }

    /** Parse `[argv, argv + argc)` for the options `opt, opts...`, and place
        the results of the parse in `map`.  For any option `o`, the key for
        its associated entry in `map` is `storage_name(o)`.  If an error
        occurs, or if the user requests help or version, output will be
        printed to `os` and the program will exit. The return code on exit
        will be `1` if an error occurred, or `0` otherwise. */
    template<
        options_map OptionsMap,
        option_or_group Option,
        option_or_group... Options>
    void parse_command_line(
        int argc,
        char const ** argv,
        OptionsMap & map,
        std::string_view program_desc,
        std::ostream & os,
        customizable_strings const & strings,
        Option opt,
        Options... opts)
    {
        return program_options_2::parse_command_line(
            arg_view(argc, argv), map, program_desc, os, strings, opt, opts...);
    }

    /** Parse `[argv, argv + argc)` for the options `opt, opts...`, and place
        the results of the parse in `map`.  For any option `o`, the key for
        its associated entry in `map` is `storage_name(o)`.  If an error
        occurs, or if the user requests help or version, output will be
        printed to `os` and the program will exit. The return code on exit
        will be `1` if an error occurred, or `0` otherwise. */
    template<
        options_map OptionsMap,
        option_or_group Option,
        option_or_group... Options>
    void parse_command_line(
        int argc,
        char const ** argv,
        OptionsMap & map,
        std::string_view program_desc,
        std::ostream & os,
        Option opt,
        Options... opts)
    {
        return program_options_2::parse_command_line(
            arg_view(argc, argv),
            map,
            program_desc,
            os,
            customizable_strings{},
            opt,
            opts...);
    }


#if defined(BOOST_PROGRAM_OPTIONS_2_DOXYGEN) || defined(_MSC_VER)

    // tuple overloads

    /** Parse `args` for the options `opt, opts...`, and return a tuple that
        contains the results of the parse.  Each element of tuple corresponds
        one-to-one to `opt, opts...` (except that grouping is ignored).  If an
        error occurs, or if the user requests help or version, output will be
        printed to `os` and the program will exit.  The return code on exit
        will be `1` if an error occurred, or `0` otherwise.  The given options
        must not contain any commands.  This function is defined on Windows
        only. */
    template<
        range_of_string_view<wchar_t> Args,
        option_or_group Option,
        option_or_group... Options>
    requires(!detail::contains_commands<Option, Options...>())
        // clang-format off
    auto parse_command_line(
        Args const & args,
        std::wstring_view program_desc,
        std::wostream & os,
        customizable_strings const & strings,
        Option opt,
        Options... opts)
    // clang-format on
    {
        BOOST_ASSERT(args.begin() != args.end());
        detail::check_options(strings, opt, opts...);

        bool const no_help = detail::no_help_option(opt, opts...);

        if (no_help && detail::argv_contains_default_help_flag(strings, args)) {
            detail::print_help_and_exit(
                0, strings, args.front(), program_desc, os, true, opt, opts...);
        }

        return detail::parse_options_as_tuple(
            strings, args, program_desc, os, no_help, opt, opts...);
    }

    /** Parse `args` for the options `opt, opts...`, and return a tuple that
        contains the results of the parse.  Each element of tuple corresponds
        one-to-one to `opt, opts...` (except that grouping is ignored).  If an
        error occurs, or if the user requests help or version, output will be
        printed to `os` and the program will exit.  The return code on exit
        will be `1` if an error occurred, or `0` otherwise.  The given options
        must not contain any commands.  This function is defined on Windows
        only. */
    template<
        range_of_string_view<wchar_t> Args,
        option_or_group Option,
        option_or_group... Options>
    requires(!detail::contains_commands<Option, Options...>())
        // clang-format off
    auto parse_command_line(
        Args const & args,
        std::wstring_view program_desc,
        std::wostream & os,
        Option opt,
        Options... opts)
    // clang-format on
    {
        return program_options_2::parse_command_line(
            args, program_desc, os, customizable_strings{}, opt, opts...);
    }

    /** Parse `[argv, argv + argc)` for the options `opt, opts...`, and return
        a tuple that contains the results of the parse.  Each element of tuple
        corresponds one-to-one to `opt, opts...` (except that grouping is
        ignored).  If an error occurs, or if the user requests help or
        version, output will be printed to `os` and the program will exit.
        The return code on exit will be `1` if an error occurred, or `0`
        otherwise.  The given options must not contain any commands.  This
        function is defined on Windows only. */
    template<option_or_group Option, option_or_group... Options>
    requires(!detail::contains_commands<Option, Options...>())
        // clang-format off
    auto parse_command_line(
        int argc,
        wchar_t const ** argv,
        std::wstring_view program_desc,
        customizable_strings const & strings,
        std::wostream & os,
        Option opt,
        Options... opts)
    // clang-format on
    {
        return program_options_2::parse_command_line(
            arg_view(argc, argv), program_desc, os, strings, opt, opts...);
    }

    /** Parse `[argv, argv + argc)` for the options `opt, opts...`, and return
        a tuple that contains the results of the parse.  Each element of tuple
        corresponds one-to-one to `opt, opts...` (except that grouping is
        ignored).  If an error occurs, or if the user requests help or
        version, output will be printed to `os` and the program will exit.
        The return code on exit will be `1` if an error occurred, or `0`
        otherwise.  The given options must not contain any commands.  This
        function is defined on Windows only. */
    template<option_or_group Option, option_or_group... Options>
    requires(!detail::contains_commands<Option, Options...>())
        // clang-format off
    auto parse_command_line(
        int argc,
        wchar_t const ** argv,
        std::wstring_view program_desc,
        std::wostream & os,
        Option opt,
        Options... opts)
    // clang-format on
    {
        return program_options_2::parse_command_line(
            arg_view(argc, argv),
            program_desc,
            os,
            customizable_strings{},
            opt,
            opts...);
    }



    // TODO: map overloads.

#endif

}}

#endif
