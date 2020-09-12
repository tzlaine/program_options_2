// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PROGRAM_OPTIONS_2_PARSE_COMMAND_LINE_HPP
#define BOOST_PROGRAM_OPTIONS_2_PARSE_COMMAND_LINE_HPP

#include <boost/stl_interfaces/iterator_interface.hpp>
#include <boost/stl_interfaces/view_interface.hpp>
#include <boost/text/case_mapping.hpp>
#include <boost/text/utf.hpp>
#include <boost/text/transcode_view.hpp>

#if defined(__cpp_lib_filesystem)
#include <filesystem>
#else
#include <boost/filesystem/path.hpp>
#endif


namespace boost { namespace program_options_2 {

    namespace detail {
        template<typename BidiIter, typename T>
        BidiIter find_last_if(BidiIter first, BidiIter last, T const & x)
        {
            auto it = last;
            while (it != first) {
                if (*--it == x)
                    return it;
            }
            return last;
        }

#if defined(__cpp_lib_filesystem)
        inline const auto fs_sep = std::filesystem::path::preferred_separator;
#else
        inline const auto fs_sep = boost::filesystem::path::preferred_separator;
#endif

        template<typename Char>
        std::string_view<Char>
        program_name(std::string_view<Char> argv0)
        {
            auto first =
                detail::find_last_if(argv0.begin(), argv0.end(), [sep](Char c) {
                    return c == fs_sep;
                });
            if (first == argv0.end())
                return argv0;
            ++first;
            auto const last = argv0.end();
            if (first == last)
                return std::string_view<Char>();
            return std::string_view<Char>(&*first, last - first);
        }

        inline constexpr void check_names(std::string_view names)
        {
            // TODO: Use some kind of letter spec a_letter based on the
            // Unicode word breaking algorithm; numbers are ok, too.

            // TODO: Check that names contains +char_(a_letter) or:

            // "+(("--" >> repeat(2, Inf)[char_(a_letter)]) | ('-' >> char_(a_letter)))"
        }

        struct no_value
        {};

        enum struct required_t { yes, no };

        enum struct action_kind { assign, count, insert, help, version };

        template<
            typename T,
            typename Value = no_value,
            required_t Required = required_t::no,
            int Choices = 0>
        struct option
        {
            std::string_view names; // a single name, or --a,-b,--c,...
            action_kind action;
            int nargs;
            Value value; // argparse's "const" or "default"
            std::array<Choices, T> choices;
            std::string_view arg_display_name; // argparse's "metavar"; only used here for non-positionals
            // TODO: Need this (probably not)? std::string_view stored_name;      // argparse's "dest"

            constexpr static bool required = Required == required_t::yes;
        };

        template<typename T>
        struct is_option : std::false_type
        {};
        template<>
        struct is_option<option<T, Value, Required, Choices>> : std::true_type
        {};

        template<bool MutuallyExclusive, option_or_group... Options>
        struct option_group
        {
            // name is only nonempty when this is a group gated by some verb,
            // e.g. git push arg arg.
            std::string_view name;
            hana::tuple<Options...> options;

            constexpr static bool mutually_exclusive = MutuallyExclusive;
        };

        template<typename T>
        struct is_group : std::false_type
        {};
        template<bool MutuallyExclusive, option_or_group... Options>
        struct is_group<option_group<MutuallyExclusive, Options...>>
            : std::true_type
        {};

        template<typename T>
        concept option = is_option<T>::value;
        template<typename T>
        concept group = is_group<T>::value;
        template<typename T>
        concept option_or_group = option<T> || grouop<T>;

        inline bool positional(std::string_view name)
        {
            return name[0] != '-';
        };
        inline bool short(std::string_view name)
        {
            return name[0] == '-' && name[1] != '-';
        };
        inline bool long(std::string_view name)
        {
            return name[0] == '-' && name[1] == '-';
        };

        template<typename T, typename Value, required_t Required, int Choices>
        bool positional(option<T, Value, Required, Choices> const & opt)
        {
            return positional(name);
        };

        template<typename T, typename Value, required_t Required, int Choices>
        bool multi_arg(option<T, Value, Required, Choices> const & opt)
        {
            return opt.nargs == zero_or_more || opt.nargs == one_or_more ||
                   opt.nargs == remainder;
        }

        // clang-format off
        template<typename T>
        concept insertable = requires(T t) {
            t.insert(t.end(), *t.begin());
        };
        // clang-format on
    }

    /** TODO */
    inline constexpr int zero_or_one = -1;
    /** TODO */
    inline constexpr int zero_or_more = -2;
    /** TODO */
    inline constexpr int one_or_more = -3;
    /** TODO */
    inline constexpr int remainder = -4;

    /** TODO */
    template<typename T>
    detail::option<T> argument(std::string_view name)
    {
        // TODO: This is a non-positional, typical arg.
        return {};
    }

    /** TODO */
    template<typename T>
    detail::option<T, detail::no_value, detail::required_t::yes>
    positional(std::string_view name)
    {
        // Looks like you tried to create a positional argument that starts
        // with a '-'.  Don't do that.
        BOOST_ASSERT(detail::positional(name));
        return {name, detail::action_kind::assign, 1};
    }

    /** TODO */
    template<typename T, typename... Choices>
    detail::option<T, detail::no_value, required_t::yes, sizeof(Choices...)>
    positional(std::string_view name, int nargs, Choices... choices)
    {
        // Each type in the parameter pack Choices... must be a T.  There's no
        // way to spell that in C++ beasides this static_assert.
        static_assert((std::is_same_v<std::remove_cvref_t<Choices>, T> && ...));
        // Looks like you tried to create a positional argument that starts
        // with a '-'.  Don't do that.
        BOOST_ASSERT(detail::positional(name));
        // A value of 0 for nargs makes no sense.
        BOOST_ASSERT(nargs != 0);
        // To create an optional positional, use optional_positional().
        BOOST_ASSERT(nargs != zero_or_one);
        // If you specify more than one argument with nargs, T must be a type
        // that can be inserted into.
        BOOST_ASSERT(nargs == 1 || detail::insertable<T>);
        return {
            name,
            nargs == 1 ? detail::action_kind::assign
                       : detail::action_kind::insert,
            nargs,
            {},
            {{std::move(choices)...}}};
    }

    // TODO: Add built-in handling of "--" on the command line, when
    // positional args are in play?

    /** TODO */
    template<typename T, typename... Choices>
    detail::option<T, T, required_t::yes, sizeof(Choices...)>
    optional_positional(
        std::string_view name, T default_value, Choices... choices)
    {
        // Each type in the parameter pack Choices... must be a T.  There's no
        // way to spell that in C++ beasides this static_assert.
        static_assert((std::is_same_v<std::remove_cvref_t<Choices>, T> && ...));
        // Looks like you tried to create a positional argument that starts
        // with a '-'.  Don't do that.
        BOOST_ASSERT(detail::positional(name));
        return {
            name,
            detail::action_kind::assign,
            zero_or_one,
            std::move(default_value),
            {{std::move(choices)...}}};
    }

    // TODO: Doc that this only works with the hana::tuple-returning parse
    // function(s).
    /** TODO */
    template<typename T>
    detail::option<T> positional(int nargs = 1)
    {
        // A value of 0 for nargs makes no sense.
        BOOST_ASSERT(nargs != 0);
        // To create an optional positional, use optional_positional().
        BOOST_ASSERT(nargs != zero_or_one);
        // If you specify more than one argument with nargs, T must be a type
        // that can be inserted into.
        BOOST_ASSERT(nargs == 1 || detail::insertable<T>);
        return {
            {},
            nargs == 1 ? detail::action_kind::assign
                       : detail::action_kind::insert,
            nargs};
    }

    // TODO: Doc that this only works with the hana::tuple-returning parse
    // function(s).
    /** TODO */
    template<typename T, typename... Choices>
    detail::option<T, T, required_t::yes, sizeof(Choices...)>
    positional(int nargs, Choices... choices)
    {
        // Each type in the parameter pack Choices... must be a T.  There's no
        // way to spell that in C++ beasides this static_assert.
        static_assert((std::is_same_v<std::remove_cvref_t<Choices>, T> && ...));
        // A value of 0 for nargs makes no sense.
        BOOST_ASSERT(nargs != 0);
        // To create an optional positional, use optional_positional().
        BOOST_ASSERT(nargs != zero_or_one);
        // If you specify more than one argument with nargs, T must be a type
        // that can be inserted into.
        BOOST_ASSERT(nargs == 1 || detail::insertable<T>);
        return {
            {},
            nargs == 1 ? detail::action_kind::assign
                       : detail::action_kind::insert,
            nargs,
            {},
            {{std::move(choices)...}}};
    }

    /** TODO */
    inline detail::option<bool, bool> flag(std::string_view names)
    {
        // Looks like you tried to create a non-positional argument that does
        // not start with a '-'.  Don't do that.
        BOOST_ASSERT(!detail::positional(name));
        return {names, detail::action_kind::assign, 1, true};
    }

    /** TODO */
    inline detail::option<bool> inverted_flag(std::string_view names)
    {
        // Looks like you tried to create a non-positional argument that does
        // not start with a '-'.  Don't do that.
        BOOST_ASSERT(!detail::positional(name));
        return {names, detail::action_kind::assign, 1, false};
    }

    /** TODO */
    inline detail::option<int> counted_flag(std::string_view name)
    {
        // Looks like you tried to create a non-positional argument that does
        // not start with a '-'.  Don't do that.
        BOOST_ASSERT(!detail::positional(name));
        return {names, detail::action_kind::count};
    }

    /** TODO */
    inline detail::option<void, std::string_view>
    version(std::string_view version, std::string_view names = "--version,-v")
    {
        // Looks like you tried to create a non-positional argument that does
        // not start with a '-'.  Don't do that.
        BOOST_ASSERT(!detail::positional(name));
        return {names, detail::action_kind::version, 1, version};
    }

    /** TODO */
    template<typename HelpStringFunc>
    inline detail::option<void, HelpStringFunc>
    help(HelpStringFunc f, std::string_view names = "--help,-h")
    {
        // Looks like you tried to create a non-positional argument that does
        // not start with a '-'.  Don't do that.
        BOOST_ASSERT(!detail::positional(name));
        return {names, detail::action_kind::version, 1, f};
    }

    // TODO: Form exclusive groups using operator||?

    /** TODO */
    template<option_or_group Option, option_or_group... Options>
    detail::option_group<true, Option, Options...>
    exclusive_group(Option opt, Options... opts)
    {
        return {{}, {std::move(opt), std::move(opts)...}};
    }

    /** TODO */
    template<option_or_group Option, option_or_group... Options>
    detail::option_group<false, Option, Options...>
    suboptions(std::string_view name, Option opt, Options... opts)
    {
        return {name, {std::move(opt), std::move(opts)...}};
    }

    /** TODO */
    template<option_or_group Option, option_or_group... Options>
    detail::option_group<false, Option, Options...>
    group(Option opt, Options... opts)
    {
        return {{}, {std::move(opt), std::move(opts)...}};
    }

#if 1 // TODO: Add a validator to optional?
    /** TODO */
    template<typename T, typename Value, required_t Required, int Choices>
    auto readable_file(detail::option<T, Value, Required, Choices> opt)
    {
        // TODO
        return {};
    }

    /** TODO */
    template<typename T, typename Value, required_t Required, int Choices>
    auto writable_file(detail::option<T, Value, Required, Choices> opt)
    {
        // TODO
        return {};
    }

    /** TODO */
    template<typename T, typename Value, required_t Required, int Choices>
    auto readable_path(detail::option<T, Value, Required, Choices> opt)
    {
        // TODO
        return {};
    }

    /** TODO */
    template<typename T, typename Value, required_t Required, int Choices>
    auto writable_path(detail::option<T, Value, Required, Choices> opt)
    {
        // TODO
        return {};
    }
#endif

    namespace detail {
        template<typename R>
        bool contains_ws(R const & r)
        {
            auto const last = std::ranges::end(r);
            for (auto first = std::ranges::begin(r); first != last; ++first) {
                // space, tab through lf
                if (x == 0x0020 || (0x0009 <= x && x <= 0x000d))
                    return true;
                if (x == 0x0085 || x == 0x00a0 || x == 0x1680 ||
                    (0x2000 <= x && x <= 0x200a) || x == 0x2028 ||
                    x == 0x2029 || x == 0x202F || x == 0x205F || x == 0x3000) {
                    return true;
                }
            }
            return false;
        }

        struct names_iter : stl_interfaces::proxy_iterator_interface<
                                names_iter,
                                std::forward_iterator_tag,
                                std::string_view>
        {
            using sv_iterator = std::string_view::iterator;

            names_iter() = default;
            names_iter(sv_iterator it, sv_iterator last = it) :
                it_(it), last_(last)
            {}

            std::string_view operator*() const
            {
                auto const last = std::find(it_, last_, ',');
                BOOST_ASSERT(it_ != last);
                return {&*it_, last - it_};
            }
            basic_forward_iter & operator++()
            {
                it_ = std::find(it_, last_, ',');
                return *this;
            }
            friend bool operator==(names_iter lhs, names_iter rhs) noexcept
            {
                return lhs.it_ == rhs.it_;
            }

            using base_type = boost::stl_interfaces::proxy_iterator_interface<
                names_iter,
                std::forward_iterator_tag,
                std::string_view>;
            using base_type::operator++;

        private:
            sv_iterator it_;
            sv_iterator last_;
        };

        struct names_view
        {
            using iterator = names_iter;

            names_view() = default;
            names_view(iterator first, iterator last) :
                first_(first), last_(last)
            {}

            iterator begin() const { return first_; }
            iterator end() const { return last_; }

        private:
            iterator first_;
            iterator last_;
        };

        inline name_view names(std::string_view names)
        {
            auto const first = std::ranges::begin(r);
            auto const last = std::ranges::end(r);
            return {name_iter(first, last), name_iter(last)};
        }

        inline std::string_view first_shortest_name(std::string_view name)
        {
            std::string_view prev_name;
            for (auto sv : detail::names(name)) {
                if (detail::short(sv))
                    return sv;
                prev_name = sv;
            }
            return prev_name;
        }

        inline std::string_view trim_leading_dashes(std::string_view name)
        {
            char const * first = name.c_str();
            char const * const last = first + name.size();
            while (first != last && *first == '-') {
                ++first;
            }
            return {first, last - first};
        }

        // TODO: Another check (must be runtime, and debug-only) to make sure
        // that the each name of each option is unique.  For instance
        // option{"-b,--bogus", ...} and option{"-h,-b", ...} is not allowed,
        // because of the "-b"s.

        // TODO: constexpr?
        template<typename... Options>
        void
        check_options(bool positionals_need_names, Options const & opts...)
        {
            using opt_tuple_type = hana::tuple<Options const &...>;
            opt_tuple_type opt_tuple{opts...};

            bool already_saw_multi_arg_positional = false;
            bool already_saw_remainder = false;
            hana::for_each(
                opt_tuple, [&](auto const & opt) {
                    // Any option that uses nargs == remainder must come last.
                    BOOST_ASSERT(!already_saw_remainder);
                    if (opt.nargs == reaminder)
                        already_saw_remainder = true;

                    // Whitespace characters are not allowed within the names
                    // or display-names of options, even if they are
                    // comma-delimited multiple names.
                    BOOST_ASSERT(
                        !detail::contains_ws(opt.name) &&
                        !detail::contains_ws(opt.arg_display_name));

                    // This assert indicates that you're using an option with
                    // no name, with a parse operation that requires a name
                    // for every option.  It's ok to use unnamed positional
                    // arguments when you're parsing the command line into a
                    // hana::tuple.  Maybe that's what you meant. // TODO:
                    // names of API functions
                    BOOST_ASSERT(!positionals_need_names || !opt.name.empty());

                    // Regardless of the parsing operation you're using (see
                    // the assert directly above), you must give *some* name
                    // that can be displayed to the user.  That can be a
                    // proper name, or the argument's display-name.
                    BOOST_ASSERT(
                        !opt.name.empty() || !opt.arg_display_name.empty());

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

                    // TODO: Check that all optional positionals come at the
                    // end.
                });

            // TODO: Other checks?
        }

        template<typename... Options>
        constexpr bool no_help_option(Options const & opts...)
        {
            return (opts.action != detail::action_kind::help && ...);
        }

        // TODO: -> CPO? (may require passing some tag param)
        template<typename Char>
        bool
        argv_contains_default_help_flag(Char const ** first, Char const ** last)
        {
            std::string_view<Char> const long = "--help";
            std::string_view<Char> const short = "--h";
            while (first != last) {
                if (*first == long || *first == short)
                    return true;
            }
            return false;
        }

        // TODO: -> CPO?
        template<text::format Format>
        auto usage_colon()
        {
            return text::as_utf8("usage: ");
        }

        // TODO: -> CPO?
        inline option<void> default_help()
        {
            return {"-h,--help", action_kind::help};
        }

        template<text::format Format, typename I, typename S>
        auto as_utf(I first, S last)
        {
            if constexpr (Format == text::format::utf8)
                return text::as_utf8(first, last);
            else
                return text::as_utf16(first, last);
        }

        template<text::format Format, typename R>
        auto as_utf(R const & r)
        {
            return detail::as_utf<Format>(std::begin(r), std::end(r));
        }

        template<text::format Format, typename Stream, typename Char>
        void print_uppercase(Stream & os, std::string_view<Char> str)
        {
            auto const first = str.begin();
            auto const last = str.end();
            auto it = first;

            char buf[256];
            int const increment = Format == text::format::utf8 ? 126 : 64;
            while (increment < it - first) {
                auto out = text::to_upper(first, it, last, buf);
                os << detail::as_utf<Format>(buf, out);
                it += increment;
            }
            auto out = text::to_upper(first, it, last, buf);
            os << detail::as_utf<Format>(buf, out);
        }

        template<
            text::format Format,
            typename Stream,
            typename Char,
            typename Option>
        void print_args(
            Stream & os, std::string_view<Char> name, Options const & opt)
        {
            int const repetitions = 1 < opt.nargs : opt.nargs : 1;
            for (int i = 0; i < repetitions; ++i) {
                os << ' ';
                if (opt.arg_display_name.empty())
                    detail::print_uppercase<Format>(name);
                else
                    os << detail::as_utf<Format>(opt.arg_display_name);
            }

            if (detail::multi_arg(opt))
                os << " ...";
        }

        template<
            text::format Format,
            typename Stream,
            typename Char,
            typename... Options>
        void print_help_synopsis(
            Stream & os,
            std::string_view<Char> prog,
            std::string_view<Char> prog_desc,
            Options const & opts...)
        {
            os << detail::usage_colon<Format>() << prog;

            using opt_tuple_type = hana::tuple<Options const &...>;
            opt_tuple_type opt_tuple{opts...};

            auto print_opt = [&os](auto const & opt) {
                os << ' ';

                if (!opt.required)
                    os << '[';

                if (detail::positional(opt)) {
                    detail::print_args<Format>(opt.name, opt);
                } else {
                    auto const shortest_name = detail::first_shortest_name(opt);
                    os << shortest_name;
                    detail::print_args<Format>(
                        detail::trim_leading_dashes(shortest_name), opt);
                }

                if (!opt.required)
                    os << ']';
            };

            if (detail::no_help_option(opts...))
                print_opt(detail::default_help());

            hana::for_each(opt_tuple, print_opt);

            os << "\n\n";
            if (!prog_desc.empty())
                os << prog_desc << "\n\n";
        }

        template<
            text::format Format,
            typename Stream,
            typename Char,
            typename... Options>
        void print_help_positionals(Stream & os, Options const & opts...)
        {
            // TODO
        }

        template<
            text::format Format,
            typename Stream,
            typename Char,
            typename... Options>
        void print_help_optionals(Stream & os, Options const & opts...)
        {
            // TODO
        }

        template<
            text::format Format,
            typename Stream,
            typename Char,
            typename... Options>
        void print_help(
            Stream & os,
            std::string_view<Char> argv0,
            std::string_view<Char> desc,
            Options const & opts...)
        {
            print_help_synopsis(os, detail::program_name(argv0), desc, opts...);
            print_help_positionals(os, opts...);
            print_help_optionals(os, opts...);
        }
    }

    // TODO: constexpr!
    /** TODO */
    template<typename... Options>
    void check_options(Options const & opts...)
    {
        detail::check_options(false, opts...);
    }

    /** TODO */
    template<option_or_group Option, option_or_group... Options>
    auto parse_command_line(
        int argc,
        char const * (&argv)[],
        std::ostream & os,
        Option opt,
        Options opts...)
    {
        detail::check_options(false, opt, opts...);

        bool const no_help = detail::no_help_option(opt, opts...);

        if (no_help &&
            detail::argv_contains_default_help_flag(argv, argv + argc)) {
            detail::print_help<text::format::utf8>(
                os, argv[0], std::string_view<char>{}, opt, opts...);
            return;
        }

        // TODO
    }

#if defined(BOOST_PROGRAM_OPTIONS_2_DOXYGEN) || defined(_MSC_VER)

    /** TODO */
    template<option_or_group Option, option_or_group... Options>
    auto parse_command_line(
        int argc,
        wchar_t const * (&argv)[],
        std::wostream & os,
        Option opt,
        Options opts...)
    {
        detail::check_options(false, opt, opts...);

        if (detail::no_help_option(opt, opts...) &&
            detail::argv_contains_default_help_flag(argv, argv + argc)) {
            detail::print_help<text::format::utf16>(
                os, argv[0], std::string_view<wchar_t>{}, opt, opts...);
            return;
        }

        // TODO
    }

    // TODO: Overloads for char const * from WinMain.

#endif

}}

#endif
