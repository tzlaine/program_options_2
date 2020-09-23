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
        template<typename R>
        bool contains_ws(R const & r)
        {
            auto const last = r.end();
            for (auto first = r.begin(); first != last; ++first) {
                auto const cp = *first;
                // space, tab through lf
                if (cp == 0x0020 || (0x0009 <= cp && cp <= 0x000d))
                    return true;
                if (cp == 0x0085 || cp == 0x00a0 || cp == 0x1680 ||
                    (0x2000 <= cp && cp <= 0x200a) || cp == 0x2028 ||
                    cp == 0x2029 || cp == 0x202F || cp == 0x205F ||
                    cp == 0x3000) {
                    return true;
                }
            }
            return false;
        }

        inline bool valid_nonpositional_names(std::string_view names)
        {
            if (detail::contains_ws(names))
                return false;
            for (auto name : detail::names_view(names)) {
                auto const trimmed_name = detail::trim_leading_dashes(name);
                auto const trimmed_dashes = name.size() - trimmed_name.size();
                if (trimmed_dashes != 1u && trimmed_dashes != 2u)
                    return false;
            }
            return true;
        }

        template<typename... Options>
        void check_options(Options const &... opts)
        {
            using opt_tuple_type = hana::tuple<Options const &...>;
            opt_tuple_type opt_tuple{opts...};

            bool already_saw_remainder = false;
            hana::for_each(opt_tuple, [&](auto const & opt) {
                // Any option that uses args == remainder must come last.
                BOOST_ASSERT(!already_saw_remainder);
                if (opt.positional && !opt.required)
                    already_saw_remainder = true;

                // Whitespace characters are not allowed within the names
                // or display-names of options, even if they are
                // comma-delimited multiple names.
                BOOST_ASSERT(
                    !detail::contains_ws(opt.names) &&
                    !detail::contains_ws(opt.arg_display_name));

                // This assert indicates that you're using an option with no
                // name.  Fix this.
                BOOST_ASSERT(!opt.names.empty());
            });
        }
    }

    /** TODO */
    template<typename T = std::string_view>
    detail::option<detail::option_kind::argument, T>
    argument(std::string_view names, std::string_view help_text)
    {
        // There's something wrong with the argument names in "names".  Either
        // it contains whitespace, or it contains at least one name that is
        // not of the form "-<name>" or "--<name>".
        BOOST_ASSERT(detail::valid_nonpositional_names(names));
        return {names, help_text, detail::action_kind::assign, 1};
    }

    /** TODO */
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
#if !BOOST_PROGRAM_OPTIONS_2_USE_CONCEPTS
        // Each type in the parameter pack Choices... must be assignable to T,
        // or insertable into T.
        static_assert(
            ((std::is_assignable_v<T &, Choices> &&
              std::is_constructible_v<T, Choices>)&&...) ||
            (detail::is_insertable_from<T, Choices>::value && ...));
#endif
        // There's something wrong with the argument names in "names".  Either
        // it contains whitespace, or it contains at least one name that is
        // not of the form "-<name>" or "--<name>".
        BOOST_ASSERT(detail::valid_nonpositional_names(names));
        // An argument with args=0 and no default is a flag.  Use flag()
        // instead.
        BOOST_ASSERT(args != 0);
        // args must be one of the named values, like zero_or_one, or must be
        // non-negative.
        BOOST_ASSERT(one_or_more <= args);
        // If you specify more than one argument with args, T must be a type
        // that can be inserted into.
        BOOST_ASSERT(args == 1 || args == zero_or_one || insertable<T>);
        // For a argument that takes zero or more args, T must be a
        // std::optional.
        BOOST_ASSERT(args != zero_or_one || detail::is_optional<T>::value);
        return {
            names,
            help_text,
            args == 1 || args == zero_or_one ? detail::action_kind::assign
                                             : detail::action_kind::insert,
            args,
            {},
            {{std::move(choices)...}}};
    }

    /** TODO */
    template<typename T = std::string_view>
    detail::option<
        detail::option_kind::positional,
        T,
        no_value,
        detail::required_t::yes>
    positional(std::string_view name, std::string_view help_text)
    {
        // Looks like you tried to create a positional argument that starts
        // with a '-'.  Don't do that.
        BOOST_ASSERT(detail::positional(name));
        return {name, help_text, detail::action_kind::assign, 1};
    }

    /** TODO */
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
#if !BOOST_PROGRAM_OPTIONS_2_USE_CONCEPTS
        // Each type in the parameter pack Choices... must be assignable to T,
        // or insertable into T.
        static_assert(
            ((std::is_assignable_v<T &, Choices> &&
              std::is_constructible_v<T, Choices>)&&...) ||
            (detail::is_insertable_from<T, Choices>::value && ...));
#endif
        // Looks like you tried to create a positional argument that starts
        // with a '-'.  Don't do that.
        BOOST_ASSERT(detail::positional(name));
        // 0 occurrences makes no sense for a non-optional positional
        // argument.
        BOOST_ASSERT(
            args != 0 && args != zero_or_one && args != zero_or_more);
        // args must be one of the named values, like zero_or_one, or must be
        // non-negative.
        BOOST_ASSERT(one_or_more <= args);
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

    /** TODO */
    template<insertable T = std::vector<std::string_view>>
    detail::option<detail::option_kind::positional, T>
    remainder(std::string_view name, std::string_view help_text)
    {
        // Looks like you tried to create a positional argument that starts
        // with a '-'.  Don't do that.
        BOOST_ASSERT(detail::positional(name));
        return {name, help_text, detail::action_kind::insert, zero_or_more};
    }

    // TODO: Add built-in handling of "--" on the command line, when
    // positional args are in play?  If not, provide an example that
    // demonstrates how simple it is to configure to take "--" followed by an
    // arbitrary number of args.

    /** TODO */
    inline detail::option<
        detail::option_kind::argument,
        bool,
        bool,
        detail::required_t::yes>
    flag(std::string_view names, std::string_view help_text)
    {
        // Looks like you tried to create a non-positional argument that does
        // not start with a '-'.  Don't do that.
        BOOST_ASSERT(!detail::positional(names));
        return {names, help_text, detail::action_kind::assign, 0, false};
    }

    /** TODO */
    inline detail::option<
        detail::option_kind::argument,
        bool,
        bool,
        detail::required_t::yes>
    inverted_flag(std::string_view names, std::string_view help_text)
    {
        // Looks like you tried to create a non-positional argument that does
        // not start with a '-'.  Don't do that.
        BOOST_ASSERT(!detail::positional(names));
        return {names, help_text, detail::action_kind::assign, 0, true};
    }

    /** TODO */
    inline detail::option<detail::option_kind::argument, int>
    counted_flag(std::string_view names, std::string_view help_text)
    {
        // Looks like you tried to create a non-positional argument that does
        // not start with a '-'.  Don't do that.
        BOOST_ASSERT(!detail::positional(names));
        // For a counted flag, the first short name in names must be of the
        // form "-<name>", where "<name>" is a single character.
        BOOST_ASSERT(
            detail::short_(detail::first_shortest_name(names)) &&
            detail::first_shortest_name(names).size() == 2u);
        return {names, help_text, detail::action_kind::count, 0};
    }

    /** TODO */
    inline detail::option<detail::option_kind::argument, void, std::string_view>
    version(
        std::string_view version,
        std::string_view names = "--version,-v",
        std::string_view help_text = "Print the version and exit")
    {
        // Looks like you tried to create a non-positional argument that does
        // not start with a '-'.  Don't do that.
        BOOST_ASSERT(!detail::positional(names));
        return {names, help_text, detail::action_kind::version, 0, version};
    }

    /** TODO */
    template<std::invocable HelpStringFunc>
    detail::option<detail::option_kind::argument, void, HelpStringFunc> help(
        HelpStringFunc f,
        std::string_view names = "--help,-h",
        std::string_view help_text = "Print this help message and exit")
    {
        // Looks like you tried to create a non-positional argument that does
        // not start with a '-'.  Don't do that.
        BOOST_ASSERT(!detail::positional(names));
        return {names, help_text, detail::action_kind::help, 0, std::move(f)};
    }

    /** TODO */
    inline detail::option<detail::option_kind::argument, void> help(
        std::string_view names,
        std::string_view help_text = "Print this help message and exit")
    {
        // Looks like you tried to create a non-positional argument that does
        // not start with a '-'.  Don't do that.
        BOOST_ASSERT(!detail::positional(names));
        return {names, help_text, detail::action_kind::help, 0};
    }

    /** TODO */
    inline auto response_file(
        std::string_view names,
        std::string_view help_text =
            "Load the given response file, and parse the options it contains; "
            "'@file' works as well",
        customizable_strings const & strings = customizable_strings{})
    {
        // Looks like you tried to create a non-positional argument that does
        // not start with a '-'.  Don't do that.
        BOOST_ASSERT(!detail::positional(names));
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
