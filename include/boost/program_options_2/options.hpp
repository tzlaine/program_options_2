// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PROGRAM_OPTIONS_2_OPTIONS_HPP
#define BOOST_PROGRAM_OPTIONS_2_OPTIONS_HPP

#include <boost/program_options_2/concepts.hpp>
#include <boost/program_options_2/detail/utility.hpp>
#include <boost/program_options_2/detail/validation.hpp>


namespace boost { namespace program_options_2 {

    namespace detail {
        inline bool valid_nonpositional_names(
            std::string_view names, customizable_strings const & strings)
        {
            if (detail::contains_ws(names))
                return false;
            for (auto name : detail::names_view(names)) {
                if (name == strings.short_option_prefix)
                    return false;
                if (name == strings.long_option_prefix)
                    return false;
                if (!detail::leading_dash(name, strings))
                    return false;
            }
            return true;
        }

        struct option_checker
        {
            template<typename... Options>
            void operator()(Options const &... opts)
            {
                auto result = ((*this)(opts), ..., 0);
                (void)result;
            }

            template<typename Option>
            void operator()(Option const & opt)
            {
                BOOST_ASSERT(
                    opt.positional !=
                        detail::valid_nonpositional_names(opt.names, strings) &&
                    "Whether an option is considered positional must match its "
                    "name.");

                BOOST_ASSERT(
                    !already_saw_remainder_ &&
                    "Any option that uses args == remainder must come last.");
                if (opt.positional && !opt.required)
                    already_saw_remainder_ = true;

                BOOST_ASSERT(
                    !detail::contains_ws(opt.names) &&
                    !detail::contains_ws(opt.arg_display_name) &&
                    "Whitespace characters are not allowed within the names or "
                    "display-names of options, even if they are "
                    "comma-delimited multiple names.");

                BOOST_ASSERT(
                    !opt.names.empty() &&
                    "This assert indicates that you're using an option with no "
                    "name.  Please fix.");

                if constexpr (!std::is_same_v<
                                  typename Option::value_type,
                                  no_value>) {
                    BOOST_ASSERT(
                        !detail::positional(opt, strings) &&
                        "It looks like you're trying to give a positional a "
                        "default value, but that makes no sense");
                }

                if (opt.action == action_kind::count) {
                    BOOST_ASSERT(
                        detail::short_(
                            detail::first_short_name(opt.names, strings),
                            strings) &&
                        detail::first_short_name(opt.names, strings).size() ==
                            strings.short_option_prefix.size() + 1 &&
                        "For a counted flag, the first short name in names "
                        "must be of the form '-<name>', where '<name>' is a "
                        "single character.");
                }
            }

            template<
                exclusive_t MutuallyExclusive,
                subcommand_t Subcommand,
                required_t Required,
                named_group_t NamedGroup,
                typename Func,
                typename... Options>
            void operator()(option_group<
                            MutuallyExclusive,
                            Subcommand,
                            Required,
                            NamedGroup,
                            Func,
                            Options...> const & group)
            {
                if constexpr (is_command<std::remove_cvref_t<decltype(group)>>::
                                  value) {
                    BOOST_ASSERT(
                        detail::positional(group.names, strings) &&
                        "Command names must not start with an option prefix.");
                }
                BOOST_ASSERT(
                    (NamedGroup == named_group_t::yes ||
                     !detail::contains_ws(group.names)) &&
                    "Whitespace characters are not allowed within the names or "
                    "display-names of commands.");
                return hana::unpack(group.options, *this);
            }

            customizable_strings const & strings;
            bool already_saw_remainder_ = false;
        };

        template<typename... Options>
        void check_options(
            customizable_strings const & strings, Options const &... opts)
        {
            option_checker check{strings};
            check(opts...);
        }
    }

    /** Returns an optional option that may appear anywhere in the input.  The
        option names must each begin with `"-"` or `"--"`.  The names must be
        comma-delimited, and may not contain whitespace. */
    template<typename T = std::string_view>
    detail::option<detail::option_kind::argument, T>
    argument(std::string_view names, std::string_view help_text)
    {
        return {names, help_text, detail::action_kind::assign, 1};
    }

    /** Returns an optional option, whose argument(s) must be one of the
        values `choices...`.  The option names must each begin with `"-"` or
        `"--"`.  The names must be comma-delimited, and may not contain
        whitespace. */
    template<typename T = std::string_view, typename... Choices>
        // clang-format off
        requires
           ((std::assignable_from<T &, Choices> &&
             std::constructible_from<T, Choices>) && ...) ||
           (detail::insertable_from<T, Choices> && ...)
    detail::option<
        detail::option_kind::argument,
        T,
        no_value,
        detail::required_t::no,
        sizeof...(Choices),
        detail::choice_type_t<T, Choices...>>
    argument(
        std::string_view names,
        std::string_view help_text,
        int args,
        Choices... choices)
    // clang-format on
    {
        BOOST_ASSERT(
            args != 0 &&
            "An argument with args=0 and no default is a flag.  Use flag() "
            "instead.");
        BOOST_ASSERT(
            one_or_more <= args &&
            "args must be one of the named values, like zero_or_one, or must "
            "be non-negative.");
        // If you specify more than one argument with args, T must be a type
        // that can be inserted into.
        BOOST_ASSERT(args == 1 || args == zero_or_one || insertable<T>);
        BOOST_ASSERT(
            (args != zero_or_one || detail::is_optional<T>::value) &&
            "For a argument that takes zero or more args, T must be a "
            "std::optional.");
        return {
            names,
            help_text,
            args == 1 || args == zero_or_one ? detail::action_kind::assign
                                             : detail::action_kind::insert,
            args,
            {},
            {{std::move(choices)...}}};
    }

    /** Returns an required option, which must appear in order in the input,
        relative to the other positional options.  The option name may not
        begin with `"-"` or `"--"`, and may not contain whitespace. */
    template<typename T = std::string_view>
    detail::option<
        detail::option_kind::positional,
        T,
        no_value,
        detail::required_t::yes>
    positional(std::string_view name, std::string_view help_text)
    {
        BOOST_ASSERT(
            !detail::contains_ws(name) &&
            "Looks like you tried to create a positional argument that contains"
            "whitespace.  Don't do that.");
        return {name, help_text, detail::action_kind::assign, 1};
    }

    /** Returns an required option, which must appear in order in the input,
        relative to the other positional options.  Its parsed value must be
        one of the values `choices...`.  The option name may not begin with
        `"-"` or `"--"`, and may not contain whitespace. */
    template<typename T = std::string_view, typename... Choices>
        // clang-format off
        requires
            ((std::assignable_from<T &, Choices> &&
              std::constructible_from<T, Choices>) && ...) ||
            (detail::insertable_from<T, Choices> && ...)
    detail::option<
        detail::option_kind::positional,
        T,
        no_value,
        detail::required_t::yes,
        sizeof...(Choices),
        detail::choice_type_t<T, Choices...>>
    positional(
        std::string_view name,
        std::string_view help_text,
        int args,
        Choices... choices)
    // clang-format on
    {
        BOOST_ASSERT(
            !detail::contains_ws(name) &&
            "Looks like you tried to create a positional argument that contains"
            "whitespace.  Don't do that.");
        BOOST_ASSERT(
            args != 0 && args != zero_or_one && args != zero_or_more &&
            "0 occurrences makes no sense for a non-optional positional "
            "argument.");
        BOOST_ASSERT(
            one_or_more <= args &&
            "args must be one of the named values, like zero_or_one, or must "
            "be non-negative.");
        // If you specify more than one argument with args, T must be a type
        // that can be inserted into.
        BOOST_ASSERT(args == 1 || insertable<T>);
        return {
            name,
            help_text,
            args == 1 || args == zero_or_one ? detail::action_kind::assign
                                             : detail::action_kind::insert,
            args,
            {},
            {{std::move(choices)...}}};
    }

    /** Returns a positional that will capture all tokens of input that remain
        after all other options are parsed. */
    template<insertable T = std::vector<std::string_view>>
    detail::option<detail::option_kind::positional, T>
    remainder(std::string_view name, std::string_view help_text)
    {
        BOOST_ASSERT(
            !detail::contains_ws(name) &&
            "Looks like you tried to create a positional argument that contains"
            "whitespace.  Don't do that.");
        return {name, help_text, detail::action_kind::insert, zero_or_more};
    }

    // TODO: Add built-in handling of "--" on the command line, when
    // positional args are in play?  If not, provide an example that
    // demonstrates how simple it is to configure to take "--" followed by an
    // arbitrary number of args.

    /** Returns an optional option that acts as a boolean flag.  The value of
        the flag is considered to be `true` if the flag appears in the input,
        and `false` otherwise. */
    inline detail::option<
        detail::option_kind::argument,
        bool,
        bool,
        detail::required_t::yes>
    flag(std::string_view names, std::string_view help_text)
    {
        return {names, help_text, detail::action_kind::assign, 0, false};
    }

    /** Returns an optional option that acts as a boolean flag.  The value of
        the flag is considered to be `false` if the flag appears in the input,
        and `true` otherwise. */
    inline detail::option<
        detail::option_kind::argument,
        bool,
        bool,
        detail::required_t::yes>
    inverted_flag(std::string_view names, std::string_view help_text)
    {
        return {names, help_text, detail::action_kind::assign, 0, true};
    }

    /** Returns an optional option that acts as an integer count.  The value
        of the count is `0` if the flag does not appear in the input, and `n`
        otherwise, where `n` is the nubmer of times that the flag appears in a
        row.  For example, if there is a counted flag fore verbosity, `-v`
        will produce a count of `1`, and `=vvv` will produce a count of `3`.*/
    inline detail::option<detail::option_kind::argument, int>
    counted_flag(std::string_view names, std::string_view help_text)
    {
        return {names, help_text, detail::action_kind::count, 0};
    }

    /** Returns an optional version option that prints `version` and exits
        if the version is requested. */
    inline detail::option<detail::option_kind::argument, void, std::string_view>
    version(
        std::string_view version,
        std::string_view names = "--version,-v",
        std::string_view help_text = "Print the version and exit")
    {
        return {names, help_text, detail::action_kind::version, 0, version};
    }

    /** Returns an optional help option that prints `f()` and exits if help is
        requested. */
    template<std::invocable HelpStringFunc>
    detail::option<detail::option_kind::argument, void, HelpStringFunc> help(
        HelpStringFunc f,
        std::string_view names = "--help,-h",
        std::string_view help_text = "Print this help message and exit")
    {
        return {names, help_text, detail::action_kind::help, 0, std::move(f)};
    }

    /** Returns an optional help option that prints the default help message
        and exits if help is requested. */
    inline detail::option<detail::option_kind::argument, void> help(
        std::string_view names,
        std::string_view help_text = "Print this help message and exit")
    {
        return {names, help_text, detail::action_kind::help, 0};
    }

    /** Returns an optional option that can be used to load a response file.
        Note that there is built-in support for response file command line
        arguments of the form `@filename`.  Use `response_file()` when you
        want to create one with a name. */
    inline auto response_file(
        std::string_view names,
        std::string_view help_text =
            "Load the given response file, and parse the options it contains; "
            "'@file' works as well",
        customizable_strings const & strings = customizable_strings{})
    {
        auto opt = detail::option<detail::option_kind::argument, void>{
            names, help_text, detail::action_kind::response_file, 1};

        auto not_found_str = strings.file_not_found;
        auto not_a_file_str = strings.found_directory_not_file;
        auto cannot_read_str = strings.cannot_read;
        std::string scratch;
        auto f = [not_found_str, not_a_file_str, cannot_read_str, scratch](
                     auto sv) mutable {
#if BOOST_PROGRAM_OPTIONS_2_USE_STD_FILESYSTEM
            namespace fs = std::filesystem;
#else
            namespace fs = filesystem;
#endif
            fs::path p(sv.begin(), sv.end());
            detail::error_code ec;
            auto const status = fs::status(p, ec);
            if (ec || !fs::exists(status) || status.type() == fs::status_error)
                return detail::validation_error(not_found_str, sv, scratch);
            if (fs::is_directory(status))
                return detail::validation_error(not_a_file_str, sv, scratch);
            std::ifstream ifs(p.c_str());
            if (!ifs)
                return detail::validation_error(cannot_read_str, sv, scratch);
            return validation_result{};
        };

        return detail::with_validator(opt, f);
    }

}}

#endif
