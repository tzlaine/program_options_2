// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PROGRAM_OPTIONS_2_OPTIONS_HPP
#define BOOST_PROGRAM_OPTIONS_2_OPTIONS_HPP

#include <boost/program_options_2/fwd.hpp>
#include <boost/program_options_2/detail/utility.hpp>


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
                auto const trimmed_name = trim_leading_dashes(name);
                auto const trimmed_dashes = name.size() - trimmed_name.size();
                if (trimmed_dashes != 1u && trimmed_dashes != 2u)
                    return false;
            }
            return true;
        }

        template<typename... Options>
        void check_options(bool positionals_need_names, Options const &... opts)
        {
            using opt_tuple_type = hana::tuple<Options const &...>;
            opt_tuple_type opt_tuple{opts...};

            bool already_saw_multi_arg_positional = false;
            bool already_saw_remainder = false;
            hana::for_each(opt_tuple, [&](auto const & opt) {
                // Any option that uses args == remainder must come last.
                BOOST_ASSERT(!already_saw_remainder);
                if (opt.args == remainder)
                    already_saw_remainder = true;

                // Whitespace characters are not allowed within the names
                // or display-names of options, even if they are
                // comma-delimited multiple names.
                BOOST_ASSERT(
                    !detail::contains_ws(opt.names) &&
                    !detail::contains_ws(opt.arg_display_name));

                // This assert indicates that you're using an option with
                // no name, with a parse operation that requires a name
                // for every option.  It's ok to use unnamed positional
                // arguments when you're parsing the command line into a
                // hana::tuple.  Maybe that's what you meant. // TODO:
                // names of API functions
                BOOST_ASSERT(!positionals_need_names || !opt.names.empty());

                // Regardless of the parsing operation you're using (see
                // the assert directly above), you must give *some* name
                // that can be displayed to the user.  That can be a
                // proper name, or the argument's display-name.
                BOOST_ASSERT(
                    !opt.names.empty() || !opt.arg_display_name.empty());

                // This assert means that you've specified one or more
                // positional arguments that follow a previous positional
                // argument that takes more than one argument on the command
                // line.  This creates an ambiguity.
                BOOST_ASSERT(
                    !detail::positional(opt) ||
                    !already_saw_multi_arg_positional);

                if (detail::multi_arg(opt)) {
                    // This assert indicates that you've specified more
                    // than one positional argument that may consist of
                    // more than one argument on the command line.
                    // Anything more than one creates an ambiguity.
                    BOOST_ASSERT(!already_saw_multi_arg_positional);
                    already_saw_multi_arg_positional = true;
                }
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
        BOOST_ASSERT(remainder <= args);
        // If you specify more than one argument with args, T must be a type
        // that can be inserted into.
        BOOST_ASSERT(args == 1 || args == zero_or_one || detail::insertable<T>);
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
        // A value of 0 for args makes no sense for a positional argument.
        BOOST_ASSERT(args != 0);
        // args must be one of the named values, like zero_or_one, or must be
        // non-negative.
        BOOST_ASSERT(remainder <= args);
        // If you specify more than one argument with args, T must be a type
        // that can be inserted into.
        BOOST_ASSERT(args == 1 || args == zero_or_one || detail::insertable<T>);
        BOOST_ASSERT(args == 1 || args == zero_or_one || detail::insertable<T>);
        // For a positional that takes zero or more args, T must be a
        // std::optional.
        BOOST_ASSERT(args != zero_or_one || detail::is_optional<T>::value);
        return {
            name,
            help_text,
            args == 1 || args == zero_or_one ? detail::action_kind::assign
                                             : detail::action_kind::insert,
            args,
            {},
            {{std::move(choices)...}}};
    }

    // TODO: Add built-in handling of "--" on the command line, when
    // positional args are in play?  If not, provide an example that
    // demonstrates how simple it is to configure to take "--" followed by an
    // arbitrary number of args.

    /** TODO */
    inline detail::option<detail::option_kind::argument, bool, bool>
    flag(std::string_view names, std::string_view help_text)
    {
        // Looks like you tried to create a non-positional argument that does
        // not start with a '-'.  Don't do that.
        BOOST_ASSERT(!detail::positional(names));
        return {names, help_text, detail::action_kind::assign, 0, false};
    }

    /** TODO */
    inline detail::option<detail::option_kind::argument, bool, bool>
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

}}

#endif