// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PROGRAM_OPTIONS_2_PARSE_COMMAND_LINE_HPP
#define BOOST_PROGRAM_OPTIONS_2_PARSE_COMMAND_LINE_HPP

// Silence very verbose warnings about std::is_pod being deprecated.  TODO:
// Remove this if/when Hana accepts releases the fix for this (already on
// develop).
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#include <boost/hana.hpp>
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include <boost/stl_interfaces/iterator_interface.hpp>
#include <boost/stl_interfaces/view_interface.hpp>
#include <boost/text/bidirectional.hpp>
#include <boost/text/case_mapping.hpp>
#include <boost/text/estimated_width.hpp>
#include <boost/text/utf.hpp>
#include <boost/text/transcode_view.hpp>

#if defined(__cpp_lib_filesystem)
#include <filesystem>
#else
#include <boost/filesystem/path.hpp>
#endif

#include <sstream>
#include <string_view>
#include <type_traits>


namespace boost { namespace program_options_2 {

    /** TODO */
    inline constexpr int zero_or_one = -1;
    /** TODO */
    inline constexpr int zero_or_more = -2;
    /** TODO */
    inline constexpr int one_or_more = -3;
    /** TODO */
    inline constexpr int remainder = -4;

    // TODO: Does remainder make sense for arguments?

    namespace detail {
        template<typename BidiIter, typename T>
        BidiIter find_last(BidiIter first, BidiIter last, T const & x)
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
        std::basic_string_view<Char>
        program_name(std::basic_string_view<Char> argv0)
        {
            auto first = detail::find_last(argv0.begin(), argv0.end(), fs_sep);
            if (first == argv0.end())
                return argv0;
            ++first;
            auto const last = argv0.end();
            if (first == last)
                return std::basic_string_view<Char>();
            return std::basic_string_view<Char>(&*first, last - first);
        }

        struct no_value
        {};

        enum struct required_t { yes, no };

        enum struct action_kind { assign, count, insert, help, version };

        template<
            typename T,
            typename Value = no_value,
            required_t Required = required_t::no,
            int Choices = 0,
            typename ChoiceType = T>
        struct option
        {
            std::string_view names; // a single name, or --a,-b,--c,...
            action_kind action;
            int args;
            Value value; // argparse's "const" or "default"
            using choice_type = std::conditional_t<
                std::is_same_v<ChoiceType, void>,
                no_value,
                ChoiceType>;
            std::array<choice_type, Choices> choices;
            std::string_view arg_display_name; // argparse's "metavar"; only used here for non-positionals
            // TODO: Need this (probably not)? std::string_view stored_name;      // argparse's "dest"

            constexpr static bool required = Required == required_t::yes;
        };

        template<typename T>
        struct is_option : std::false_type
        {};
        template<
            typename T,
            typename Value,
            required_t Required,
            int Choices,
            typename ChoiceType>
        struct is_option<option<T, Value, Required, Choices, ChoiceType>>
            : std::true_type
        {};

        template<bool MutuallyExclusive, typename... Options>
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
        template<bool MutuallyExclusive, typename... Options>
        struct is_group<option_group<MutuallyExclusive, Options...>>
            : std::true_type
        {};

        inline bool positional(std::string_view name)
        {
            return name[0] != '-';
        }
        inline bool short_(std::string_view name)
        {
            return name[0] == '-' && name[1] != '-';
        }
        inline bool long_(std::string_view name)
        {
            return name[0] == '-' && name[1] == '-';
        }

        template<
            typename T,
            typename Value,
            required_t Required,
            int Choices,
            typename ChoiceType>
        bool
        positional(option<T, Value, Required, Choices, ChoiceType> const & opt)
        {
            return detail::positional(opt.names);
        }

        template<
            typename T,
            typename Value,
            required_t Required,
            int Choices,
            typename ChoiceType>
        bool optional_arg(
            option<T, Value, Required, Choices, ChoiceType> const & opt)
        {
            return opt.args == zero_or_one || opt.args == zero_or_more ||
                   opt.args == remainder;
        }

        template<
            typename T,
            typename Value,
            required_t Required,
            int Choices,
            typename ChoiceType>
        bool
        multi_arg(option<T, Value, Required, Choices, ChoiceType> const & opt)
        {
            return opt.args == zero_or_more || opt.args == one_or_more ||
                   opt.args == remainder;
        }

        // clang-format off
        template<typename T>
        concept insertable = requires(T t) {
            t.insert(t.end(), *t.begin());
        };
        template<typename T, typename U>
        concept insertable_from = requires(T t, U u) {
            t.insert(t.end(), u);
        };
        // clang-format on

        template<typename T, typename U, typename = hana::when<true>>
        struct is_insertable_from : std::false_type
        {};
        template<typename T, typename U>
        struct is_insertable_from<
            T,
            U,
            hana::when_valid<decltype(std::declval<T &>().insert(
                std::declval<T &>().end(), std::declval<U &>()))>>
            : std::true_type
        {};

        template<typename T, bool AllAssignable, bool AllInsertable>
        struct choice_type_impl
        {};
        template<typename T, bool AllInsertable>
        struct choice_type_impl<T, true, AllInsertable>
        {
            using type = T;
        };
        template<typename T>
        struct choice_type_impl<T, false, true>
        {
            using type = std::ranges::range_value_t<T>;
        };
        template<typename T, typename... Choices>
        struct choice_type : choice_type_impl<
                                 T,
                                 (std::is_assignable_v<T &, Choices> && ...),
                                 (is_insertable_from<T, Choices>::value && ...)>
        {};
        template<typename T, typename... Choices>
        using choice_type_t = typename choice_type<T, Choices...>::type;

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

        struct names_iter : stl_interfaces::proxy_iterator_interface<
                                names_iter,
                                std::forward_iterator_tag,
                                std::string_view>
        {
            using sv_iterator = std::string_view::iterator;

            names_iter() = default;
            names_iter(sv_iterator it) : it_(it), last_(it) {}
            names_iter(sv_iterator it, sv_iterator last) : it_(it), last_(last)
            {}

            std::string_view operator*() const
            {
                sv_iterator last = std::find(it_, last_, ',');
                BOOST_ASSERT(it_ != last_);
                return {&*it_, std::size_t(last - it_)};
            }
            names_iter & operator++()
            {
                it_ = std::find(it_, last_, ',');
                if (it_ != last_)
                    ++it_;
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
            names_view(std::string_view names) :
                first_(names_iter(names.begin(), names.end())),
                last_(names_iter(names.end()))
            {}

            iterator begin() const { return first_; }
            iterator end() const { return last_; }

        private:
            iterator first_;
            iterator last_;
        };

        inline std::string_view first_shortest_name(std::string_view name)
        {
            std::string_view prev_name;
            for (auto sv : names_view(name)) {
                if (detail::short_(sv))
                    return sv;
                prev_name = sv;
            }
            return prev_name;
        }

        inline std::string_view trim_leading_dashes(std::string_view name)
        {
            char const * first = name.data();
            char const * const last = first + name.size();
            while (first != last && *first == '-') {
                ++first;
            }
            return {first, std::size_t(last - first)};
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
                // TODO: Check that the characters in trimmed_name are
                // "letters".  Use the Unicode word breaking algorithm code
                // point classes (numbers are ok, too).
            }
            return true;
        }

        // TODO: Another check (must be runtime, and debug-only) to make sure
        // that the each name of each option is unique.  For instance
        // option{"-b,--bogus", ...} and option{"-h,-b", ...} is not allowed,
        // because of the "-b"s.  This should include display names.

        // TODO: Another runtime check: make sure that the choices within an
        // options are unique.

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

                // TODO: Check that all optional positionals come at the
                // end.
            });

            // TODO: Other checks?
        }

        template<typename... Options>
        bool no_help_option(Options const &... opts)
        {
            return ((opts.action != detail::action_kind::help) && ...);
        }

        // TODO: -> CPO? (may require passing some tag param)
        template<typename Char>
        bool
        argv_contains_default_help_flag(Char const ** first, Char const ** last)
        {
            // TODO: Need some kind of customization point for these strings.
            std::string_view const long_form = "--help";
            std::string_view const short_form = "-h";
            for (; first != last; ++first) {
                auto const str = std::basic_string_view<Char>(*first);
                if (std::ranges::equal(str, long_form) ||
                    std::ranges::equal(str, short_form)) {
                    return true;
                }
            }
            return false;
        }

        // TODO: -> CPO?
        template<text::format Format>
        auto usage_colon()
        {
            char const * usage_str = "usage: ";
            return text::as_utf8(usage_str);
        }

        inline option<void> default_help()
        {
            return {"-h,--help", action_kind::help, 0};
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

        constexpr inline int max_col_width = 80;

        template<text::format Format, typename Stream, typename Char>
        void print_uppercase(Stream & os, std::basic_string_view<Char> str)
        {
            auto const first = str.begin();
            auto const last = str.end();
            auto it = first;

            char buf[256];
            int const increment = Format == text::format::utf8 ? 126 : 64;
            while (increment < it - first) {
                auto out = text::to_upper(
                    text::as_utf32(it, last), text::utf_32_to_8_out(buf));
                os << detail::as_utf<Format>(buf, out.base());
                it += increment;
            }
            auto out = text::to_upper(
                text::as_utf32(it, last), text::utf_32_to_8_out(buf));
            os << detail::as_utf<Format>(buf, out.base());
        }

        template<typename Stream, typename T, typename = hana::when<true>>
        struct is_printable : std::false_type
        {};
        template<typename Stream, typename T>
        struct is_printable<
            Stream,
            T,
            hana::when_valid<decltype(
                std::declval<Stream &>() << std::declval<T>())>>
            : std::true_type
        {};

        template<typename Stream, typename Option>
        bool print_choices(Stream & os, Option const & opt)
        {
            if (opt.choices.empty())
                return false;

            os << '{';
            bool print_comma = false;
            for (auto const & choice : opt.choices) {
                if (print_comma)
                    os << ',';
                print_comma = true;
                os << choice;
            }
            os << '}';

            return true;
        }

        template<
            text::format Format,
            typename Stream,
            typename Char,
            typename Option>
        void print_args(
            Stream & os,
            std::basic_string_view<Char> name,
            Option const & opt,
            bool print_leading_space)
        {
            bool const args_optional = detail::optional_arg(opt);
            if (args_optional) {
                if (print_leading_space)
                    os << ' ';
                os << '[';
                print_leading_space = false;
            }

            int repetitions = opt.args ? opt.args : 0;
            if (repetitions < 0)
                repetitions = 1;
            for (int i = 0; i < repetitions; ++i) {
                if (print_leading_space)
                    os << ' ';
                print_leading_space = true;

                if constexpr (is_printable<
                                  Stream,
                                  typename Option::choice_type const &>::
                                  value) {
                    if (detail::print_choices(os, opt))
                        continue;
                }

                if (opt.arg_display_name.empty())
                    detail::print_uppercase<Format>(os, name);
                else
                    os << detail::as_utf<Format>(opt.arg_display_name);
            }

            if (detail::multi_arg(opt))
                os << " ...";

            if (args_optional)
                os << ']';
        }

        template<text::format Format, typename Stream, typename Option>
        int print_option(
            Stream & os,
            Option const & opt,
            int first_column,
            int current_width)
        {
            std::ostringstream oss;

            oss << ' ';

            if (!opt.required)
                oss << '[';

            if (detail::positional(opt)) {
                detail::print_args<Format>(oss, opt.names, opt, false);
            } else if (opt.action == action_kind::count) {
                auto const shortest_name =
                    detail::first_shortest_name(opt.names);
                auto const trimmed_name =
                    detail::trim_leading_dashes(shortest_name);
                oss << shortest_name << '[' << trimmed_name << "...]";
            } else {
                auto const shortest_name =
                    detail::first_shortest_name(opt.names);
                oss << shortest_name;
                detail::print_args<Format>(
                    oss, detail::trim_leading_dashes(shortest_name), opt, true);
            }

            if (!opt.required)
                oss << ']';

            std::string const str(std::move(oss).str());
            auto const str_width =
                text::estimated_width_of_graphemes(text::as_utf32(str));
            if (detail::max_col_width < current_width + str_width) {
                os << '\n' << std::string(first_column, ' ');
            }

            os << str;

            return current_width;
        }

        template<
            text::format Format,
            typename Stream,
            typename Char,
            typename... Options>
        void print_help_synopsis(
            Stream & os,
            std::basic_string_view<Char> prog,
            std::basic_string_view<Char> prog_desc,
            Options const &... opts)
        {
            using opt_tuple_type = hana::tuple<Options const &...>;
            opt_tuple_type opt_tuple{opts...};

            os << detail::usage_colon<Format>() << ' ' << prog;

            int const usage_colon_width = text::estimated_width_of_graphemes(
                text::as_utf32(detail::usage_colon<Format>()));
            int const prog_width =
                text::estimated_width_of_graphemes(text::as_utf32(prog));

            int first_column = usage_colon_width + 1 + prog_width;
            if (detail::max_col_width / 2 < first_column)
                first_column = usage_colon_width;

            int current_width = first_column;

            auto print_opt = [&](auto const & opt) {
                current_width = detail::print_option<Format>(
                    os, opt, first_column, current_width);
            };

            if (detail::no_help_option(opts...))
                print_opt(detail::default_help());

            hana::for_each(opt_tuple, print_opt);

            os << "\n\n";
            if (!prog_desc.empty())
                os << prog_desc << "\n\n";
        }

        template<text::format Format, typename Stream, typename... Options>
        void print_help_positionals(Stream & os, Options const &... opts)
        {
            // TODO
        }

        template<text::format Format, typename Stream, typename... Options>
        void print_help_optionals(Stream & os, Options const &... opts)
        {
            // TODO
        }

        template<text::format Format, typename Char, typename... Options>
        void print_help(
            std::basic_ostream<Char> & os,
            std::basic_string_view<Char> argv0,
            std::basic_string_view<Char> desc,
            Options const &... opts)
        {
            std::basic_ostringstream<Char> oss;
            print_help_synopsis<Format>(
                oss, detail::program_name(argv0), desc, opts...);
            print_help_positionals<Format>(oss, opts...);
            print_help_optionals<Format>(oss, opts...);
            auto const str = std::move(oss).str();
            for (auto range :
                 text::bidirectional_subranges(text::as_utf32(str))) {
                for (auto grapheme : range) {
                    os << grapheme;
                }
                if (range.hard_break())
                    os << "\n";
            }
        }
    }

    template<typename T>
    concept option_ = detail::is_option<T>::value;
    template<typename T>
    concept group_ = detail::is_group<T>::value;
    template<typename T>
    concept option_or_group = option_<T> || group_<T>;

    /** TODO */
    template<typename T>
    detail::option<T> argument(std::string_view names)
    {
        // There's something wrong with the argument names in "names".  Either
        // it contains whitespace, or it contains at least one name that is
        // not of the form "-<name>" or "--<name>".
        BOOST_ASSERT(detail::valid_nonpositional_names(names));
        return {names, detail::action_kind::assign, 1};
    }

    /** TODO */
    template<typename T, typename... Choices>
        // clang-format off
        requires
            (std::assignable_from<T &, Choices> && ...) ||
            (detail::insertable_from<T, Choices> && ...)
    detail::option<
        T ,
        detail::no_value,
        detail::required_t::no,
        sizeof...(Choices),
        detail::choice_type_t<T, Choices...>>
    argument(std::string_view names, int args, Choices... choices)
    // clang-format on
    {
#if !BOOST_PROGRAM_OPTIONS_2_USE_CONCEPTS
        // Each type in the parameter pack Choices... must be assignable to T,
        // or insertable into T.
        static_assert(
            (std::is_assignable_v<T &, Choices> && ...) ||
            (detail::is_insertable_from<T, Choices>::value &&
             ...));
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
        return {
            names,
            args == 1 || args == zero_or_one ? detail::action_kind::assign
                                             : detail::action_kind::insert,
            args,
            {},
            {{std::move(choices)...}}};
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
        // clang-format off
        requires
            (std::assignable_from<T &, Choices> && ...) ||
            (detail::insertable_from<T, Choices> && ...)
    detail::option<
        T ,
        detail::no_value,
        detail::required_t::yes,
        sizeof...(Choices),
        detail::choice_type_t<T, Choices...>>
    positional(std::string_view name, int args, Choices... choices)
    // clang-format on
    {
#if !BOOST_PROGRAM_OPTIONS_2_USE_CONCEPTS
        // Each type in the parameter pack Choices... must be assignable to T,
        // or insertable into T.
        static_assert(
            (std::is_assignable_v<T &, Choices> && ...) ||
            (detail::is_insertable_from<T, Choices>::value &&
             ...));
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
        return {
            name,
            args == 1 || args == zero_or_one ? detail::action_kind::assign
                                             : detail::action_kind::insert,
            args,
            {},
            {{std::move(choices)...}}};
    }

    // TODO: Add built-in handling of "--" on the command line, when
    // positional args are in play?

    /** TODO */
    inline detail::option<bool, bool> flag(std::string_view names)
    {
        // Looks like you tried to create a non-positional argument that does
        // not start with a '-'.  Don't do that.
        BOOST_ASSERT(!detail::positional(names));
        return {names, detail::action_kind::assign, 0, true};
    }

    /** TODO */
    inline detail::option<bool, bool> inverted_flag(std::string_view names)
    {
        // Looks like you tried to create a non-positional argument that does
        // not start with a '-'.  Don't do that.
        BOOST_ASSERT(!detail::positional(names));
        return {names, detail::action_kind::assign, 0, false};
    }

    /** TODO */
    inline detail::option<int> counted_flag(std::string_view names)
    {
        // Looks like you tried to create a non-positional argument that does
        // not start with a '-'.  Don't do that.
        BOOST_ASSERT(!detail::positional(names));
        // For a counted flag, the first short name in names must be of the
        // form "-<name>", where "<name>" is a single character.
        BOOST_ASSERT(
            detail::short_(detail::first_shortest_name(names)) &&
            detail::first_shortest_name(names).size() == 2u);
        return {names, detail::action_kind::count};
    }

    /** TODO */
    inline detail::option<void, std::string_view>
    version(std::string_view version, std::string_view names = "--version,-v")
    {
        // Looks like you tried to create a non-positional argument that does
        // not start with a '-'.  Don't do that.
        BOOST_ASSERT(!detail::positional(names));
        return {names, detail::action_kind::version, 0, version};
    }

    /** TODO */
    template<std::invocable HelpStringFunc>
    detail::option<void, HelpStringFunc>
    help(HelpStringFunc f, std::string_view names = "--help,-h")
    {
        // Looks like you tried to create a non-positional argument that does
        // not start with a '-'.  Don't do that.
        BOOST_ASSERT(!detail::positional(names));
        return {names, detail::action_kind::version, 0, std::move(f)};
    }

    /** TODO */
    inline detail::option<void> help(std::string_view names)
    {
        // Looks like you tried to create a non-positional argument that does
        // not start with a '-'.  Don't do that.
        BOOST_ASSERT(!detail::positional(names));
        return {names, detail::action_kind::version, 0};
    }

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
    subcommand_options(std::string_view name, Option opt, Options... opts)
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

    /** TODO */
    template<
        typename T,
        detail::required_t Required,
        int Choices,
        typename ChoiceType,
        typename DefaultType>
        // clang-format off
        requires 
            (Choices == 0 ||
             std::equality_comparable_with<DefaultType, ChoiceType>) &&
            (std::assignable_from<T &, DefaultType> ||
             detail::insertable_from<T, DefaultType>)
    detail::option<T, DefaultType, Required, Choices, ChoiceType> with_default(
        detail::option<T, detail::no_value, Required, Choices, ChoiceType> opt,
        DefaultType default_value)
    // clang-format on
    {
        if constexpr (std::equality_comparable_with<DefaultType, ChoiceType>){
            // If there are choices specified, the default must be one of the
            // choices.
            BOOST_ASSERT(
                opt.choices.empty() ||
                std::find(
                    opt.choices.begin(), opt.choices.end(), default_value) !=
                    opt.choices.end());
        }
        return {
            opt.names,
            opt.action,
            opt.args,
            std::move(default_value),
            opt.choices,
            opt.arg_display_name};
    }

    /** TODO */
    template<
        typename T,
        typename Value,
        detail::required_t Required,
        int Choices,
        typename ChoiceType>
    auto with_display_name(
        detail::option<T, Value, Required, Choices, ChoiceType> opt,
        std::string_view name)
    {
        // A display name for a flag or other option with no arguments will
        // never be displayed.
        BOOST_ASSERT(opt.args != 0);
        // A display name for an option with choices will never be displayed.
        BOOST_ASSERT(opt.choices.size() == 0);
        opt.arg_display_name = name;
        return opt;
    }

#if 1 // TODO: Add a validator to option?
    /** TODO */
    template<
        typename T,
        typename Value,
        detail::required_t Required,
        int Choices,
        typename ChoiceType>
    auto
    readable_file(detail::option<T, Value, Required, Choices, ChoiceType> opt)
    {
        // TODO
        return {};
    }

    /** TODO */
    template<
        typename T,
        typename Value,
        detail::required_t Required,
        int Choices,
        typename ChoiceType>
    auto
    writable_file(detail::option<T, Value, Required, Choices, ChoiceType> opt)
    {
        // TODO
        return {};
    }

    /** TODO */
    template<
        typename T,
        typename Value,
        detail::required_t Required,
        int Choices,
        typename ChoiceType>
    auto
    readable_path(detail::option<T, Value, Required, Choices, ChoiceType> opt)
    {
        // TODO
        return {};
    }

    /** TODO */
    template<
        typename T,
        typename Value,
        detail::required_t Required,
        int Choices,
        typename ChoiceType>
    auto
    writable_path(detail::option<T, Value, Required, Choices, ChoiceType> opt)
    {
        // TODO
        return {};
    }
#endif

    /** TODO */
    template<option_or_group Option, option_or_group... Options>
    auto parse_command_line(
        int argc,
        char const ** argv,
        std::ostream & os,
        Option opt,
        Options... opts)
    {
        detail::check_options(false, opt, opts...);

        bool const no_help = detail::no_help_option(opt, opts...);

        if (no_help &&
            detail::argv_contains_default_help_flag(argv, argv + argc)) {
            detail::print_help<text::format::utf8>(
                os,
                std::string_view(argv[0]),
                std::string_view{},
                opt,
                opts...);
            std::exit(0);
        }

        // TODO
    }

#if defined(BOOST_PROGRAM_OPTIONS_2_DOXYGEN) || defined(_MSC_VER)

    /** TODO */
    template<option_or_group Option, option_or_group... Options>
    auto parse_command_line(
        int argc,
        wchar_t const ** argv,
        std::wostream & os,
        Option opt,
        Options... opts)
    {
        detail::check_options(false, opt, opts...);

        if (detail::no_help_option(opt, opts...) &&
            detail::argv_contains_default_help_flag(argv, argv + argc)) {
            detail::print_help<text::format::utf16>(
                os, argv[0], std::basic_string_view<wchar_t>{}, opt, opts...);
            std::exit(0);
        }

        // TODO
    }

    // TODO: Overload for char const * from WinMain.

#endif

}}

#endif
