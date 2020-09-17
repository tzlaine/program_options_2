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

#include <boost/parser/parser.hpp>
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

#include <climits>


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

    /** TODO help_text_customizable_strings() returns one of these.... */
    struct customizable_strings
    {
        std::string_view usage_text = "usage: ";
        std::string_view positional_section_text = "positional arguments:";
        std::string_view optional_section_text = "optional arguments:";
        std::string_view help_names = "-h,--help";
        std::string_view help_description = "Print this help message and exit";

        std::array<std::string_view, 6> error_strings = {
            {"error: unrecognized argument '{}'",
             "error: wrong number of arguments passed to '{}'",
             "error: cannot parse argument '{}'",
             "error: '{}' is not one of the allowed choices",
             "error: unexpected positional argument '{}'",
             "error: one or more missing positional arguments, starting with "
             "'{}'"}};
    };

    /** TODO */
    template<typename Char>
    struct arg_iter : stl_interfaces::proxy_iterator_interface<
                          arg_iter<Char>,
                          std::random_access_iterator_tag,
                          std::basic_string_view<Char>>
    {
        arg_iter() = default;
        arg_iter(Char const ** ptr) : it_(ptr) { BOOST_ASSERT(ptr); }

        std::basic_string_view<Char> operator*() const { return {*it_}; }

    private:
        friend boost::stl_interfaces::access;
        Char const **& base_reference() noexcept { return it_; }
        Char const ** base_reference() const noexcept { return it_; }
        Char const ** it_;
    };

    /** TODO */
    template<typename Char>
    struct arg_view : stl_interfaces::view_interface<arg_view<Char>>
    {
        using iterator = arg_iter<Char>;

        arg_view() = default;
        arg_view(int argc, Char const ** argv) :
            first_(argv), last_(argv + argc)
        {}

        iterator begin() const { return first_; }
        iterator end() const { return last_; }

    private:
        iterator first_;
        iterator last_;
    };

    /** TODO */
    struct no_value
    {};

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

        enum struct option_kind { positional, argument };

        enum struct required_t { yes, no };

        enum struct action_kind { assign, count, insert, help, version };

        template<
            option_kind Kind,
            typename T,
            typename Value = no_value,
            required_t Required = required_t::no,
            int Choices = 0,
            typename ChoiceType = T>
        struct option
        {
            using type = T;
            using value_type = Value;
            using choice_type = std::conditional_t<
                std::is_same_v<ChoiceType, void>,
                no_value,
                ChoiceType>;

            std::string_view names;
            std::string_view help_text;
            action_kind action;
            int args;
            value_type default_value;
            std::array<choice_type, Choices> choices;
            std::string_view arg_display_name;

            constexpr static bool positional = Kind == option_kind::positional;
            constexpr static bool required = Required == required_t::yes;
            constexpr static int num_choices = Choices;
        };

        template<typename T>
        struct is_option : std::false_type
        {};
        template<
            option_kind Kind,
            typename T,
            typename Value,
            required_t Required,
            int Choices,
            typename ChoiceType>
        struct is_option<option<Kind, T, Value, Required, Choices, ChoiceType>>
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

        inline bool positional(std::string_view name) { return name[0] != '-'; }
        inline bool short_(std::string_view name)
        {
            return name[0] == '-' && name[1] != '-';
        }
        inline bool long_(std::string_view name)
        {
            return name[0] == '-' && name[1] == '-';
        }

        inline bool no_leading_dashes(std::string_view str)
        {
            return str.empty() || str[0] != '-';
        }

        template<
            option_kind Kind,
            typename T,
            typename Value,
            required_t Required,
            int Choices,
            typename ChoiceType>
        bool positional(
            option<Kind, T, Value, Required, Choices, ChoiceType> const & opt)
        {
            return detail::positional(opt.names);
        }

        template<
            option_kind Kind,
            typename T,
            typename Value,
            required_t Required,
            int Choices,
            typename ChoiceType>
        bool optional_arg(
            option<Kind, T, Value, Required, Choices, ChoiceType> const & opt)
        {
            return opt.args == zero_or_one || opt.args == zero_or_more ||
                   opt.args == remainder;
        }

        template<
            option_kind Kind,
            typename T,
            typename Value,
            required_t Required,
            int Choices,
            typename ChoiceType>
        bool multi_arg(
            option<Kind, T, Value, Required, Choices, ChoiceType> const & opt)
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

        template<typename T>
        struct is_optional : std::false_type
        {};
        template<typename T>
        struct is_optional<std::optional<T>> : std::true_type
        {};

        template<
            typename T,
            typename Choice,
            bool AllAssignable,
            bool AllInsertable>
        struct choice_type_impl
        {};
        template<typename T, typename Choice, bool AllInsertable>
        struct choice_type_impl<T, Choice, true, AllInsertable>
        {
            using type = Choice;
        };
        template<typename T, typename Choice>
        struct choice_type_impl<T, Choice, false, true>
        {
            using type = std::ranges::range_value_t<T>;
        };
        template<typename T, typename Choice = T, typename... Choices>
        struct choice_type
            : choice_type_impl<
                  T,
                  Choice,
                  std::is_assignable_v<T &, Choice> &&
                      std::is_constructible_v<T, Choice> &&
                      ((std::is_assignable_v<T &, Choices> &&
                        std::is_constructible_v<T, Choices>) &&...),
                  is_insertable_from<T, Choice>::value &&
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

        template<typename... Options>
        bool no_help_option(Options const &... opts)
        {
            return ((opts.action != detail::action_kind::help) && ...);
        }

        struct default_strings_tag
        {};

        customizable_strings help_text_customizable_strings(default_strings_tag)
        {
            return customizable_strings{};
        }

        template<typename CustomStringsTag, typename Char>
        bool argv_contains_default_help_flag(
            CustomStringsTag tag, arg_view<Char> args)
        {
            customizable_strings const strings =
                help_text_customizable_strings(tag);
            auto const names = names_view(strings.help_names);
            for (auto arg : args) {
                for (auto name : names) {
                    if (std::ranges::equal(arg, name))
                        return true;
                }
            }
            return false;
        }

        template<typename CustomStringsTag>
        option<option_kind::argument, void> default_help(CustomStringsTag tag)
        {
            customizable_strings const strings =
                help_text_customizable_strings(tag);
            return {
                strings.help_names,
                strings.help_description,
                action_kind::help,
                0};
        }

        constexpr inline int max_col_width = 80;
        constexpr inline int max_option_col_width = 24;

        template<typename Stream, typename Char>
        void print_uppercase(Stream & os, std::basic_string_view<Char> str)
        {
            auto const first = str.begin();
            auto const last = str.end();
            auto it = first;

            char buf[256];
            int const increment = 64;
            while (increment < it - first) {
                auto out = text::to_upper(
                    text::as_utf32(it, last), text::utf_32_to_8_out(buf));
                os << text::as_utf8(buf, out.base());
                it += increment;
            }
            auto out = text::to_upper(
                text::as_utf32(it, last), text::utf_32_to_8_out(buf));
            os << text::as_utf8(buf, out.base());
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

        template<typename Stream, typename Char, typename Option>
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
                    detail::print_uppercase(os, name);
                else
                    os << text::as_utf8(opt.arg_display_name);
            }

            if (detail::multi_arg(opt)) {
                char const * ellipsis = " ...";
                os << text::as_utf8(ellipsis);
            }

            if (args_optional)
                os << ']';
        }

        template<typename R>
        int estimated_width(R const & r)
        {
            return text::estimated_width_of_graphemes(text::as_utf32(r));
        }

        // brief=true is used to print the comma-delimited names in the
        // desciptions.
        template<typename Stream, typename Option>
        int print_option(
            Stream & os,
            Option const & opt,
            int first_column,
            int current_width,
            int max_width = max_col_width,
            bool brief = false)
        {
            std::ostringstream oss;

            oss << ' ';

            if (!opt.required && !brief)
                oss << '[';

            if (brief) {
                // This, plus the leading space in the first name printed
                // below, gives us a 2-space indent.
                oss << ' ';
                bool print_separator = false;
                for (auto name : names_view(opt.names)) {
                    if (print_separator)
                        oss << ',' << ' ';
                    print_separator = true;
                    oss << text::as_utf8(name);
                }
            } else if (detail::positional(opt)) {
                detail::print_args(oss, opt.names, opt, false);
            } else if (opt.action == action_kind::count) {
                auto const shortest_name =
                    detail::first_shortest_name(opt.names);
                auto const trimmed_name =
                    detail::trim_leading_dashes(shortest_name);
                char const * close = "...]";
                oss << text::as_utf8(shortest_name) << '['
                    << text::as_utf8(trimmed_name) << text::as_utf8(close);
            } else {
                auto const shortest_name =
                    detail::first_shortest_name(opt.names);
                oss << text::as_utf8(shortest_name);
                detail::print_args(
                    oss, detail::trim_leading_dashes(shortest_name), opt, true);
            }

            if (!opt.required && !brief)
                oss << ']';

            std::string const str(std::move(oss).str());
            auto const str_width = detail::estimated_width(str);
            if (max_width < current_width + str_width) {
                os << '\n';
                for (int i = 0; i < first_column; ++i) {
                    os << ' ';
                }
                current_width = first_column;
            } else {
                current_width += str_width;
            }

            os << text::as_utf8(str);

            return current_width;
        }

        template<
            typename CustomStringsTag,
            typename Stream,
            typename Char,
            typename... Options>
        void print_help_synopsis(
            CustomStringsTag tag,
            Stream & os,
            std::basic_string_view<Char> prog,
            std::basic_string_view<Char> prog_desc,
            Options const &... opts)
        {
            using opt_tuple_type = hana::tuple<Options const &...>;
            opt_tuple_type opt_tuple{opts...};

            customizable_strings const strings =
                help_text_customizable_strings(tag);

            os << text::as_utf8(strings.usage_text) << ' '
               << text::as_utf8(prog);

            int const usage_colon_width =
                detail::estimated_width(strings.usage_text);
            int const prog_width = detail::estimated_width(prog);

            int first_column = usage_colon_width + 1 + prog_width;
            if (detail::max_col_width / 2 < first_column)
                first_column = usage_colon_width;

            int current_width = first_column;

            auto print_opt = [&](auto const & opt) {
                current_width =
                    detail::print_option(os, opt, first_column, current_width);
            };

            hana::for_each(opt_tuple, print_opt);

            if (prog_desc.empty())
                os << '\n';
            else
                os << '\n' << '\n' << text::as_utf8(prog_desc) << '\n';

            // TODO: When there are one or more subcommands in use, print all
            // the non-subcommand options, as above, but then end with
            // "COMMAND [COMMAND-ARGS]".  That string should of course be
            // user-configurable.
        }

        struct printed_names_and_desc
        {
            printed_names_and_desc() = default;
            printed_names_and_desc(
                std::string printed_names,
                std::string_view desc,
                int estimated_width) :
                printed_names(std::move(printed_names)),
                desc(desc),
                estimated_width(estimated_width)
            {}
            std::string printed_names;
            std::string_view desc;
            int estimated_width;
        };

        // This is the minimum allowed gap between the options and their
        // descriptions.
        constexpr int min_help_column_gap = 2;

        template<typename Stream, typename... Options>
        void
        print_wrapped_column(Stream & os, std::string_view str, int min_column)
        {
            bool need_newline = false;
            for (auto range : text::bidirectional_subranges(
                     text::as_utf32(str),
                     max_col_width - min_column,
                     [](auto first, auto last) {
                         return detail::estimated_width(
                             text::as_utf32(first, last));
                     })) {
                if (need_newline) {
                    os << '\n';
                    for (int i = 0; i < min_column; ++i) {
                        os << ' ';
                    }
                    need_newline = false;
                }
                os << text::as_utf32(range);
                if (range.allowed_break())
                    need_newline = true;
            }
        }

        template<typename Stream, typename... Options>
        void print_options_and_descs(
            Stream & os,
            std::vector<printed_names_and_desc> const & names_and_descs,
            int description_column)
        {
            for (auto const & name_and_desc : names_and_descs) {
                os << text::as_utf8(name_and_desc.printed_names);
                int const limit = description_column - min_help_column_gap;
                int const needed_spacing =
                    name_and_desc.estimated_width <= limit
                        ? limit - name_and_desc.estimated_width +
                              min_help_column_gap
                        : description_column;
                if (needed_spacing == description_column)
                    os << '\n';
                for (int i = 0; i < needed_spacing; ++i) {
                    os << ' ';
                }
                detail::print_wrapped_column(
                    os, name_and_desc.desc, description_column);
                os << '\n';
            }
        }

        template<
            typename CustomStringsTag,
            typename Stream,
            typename... Options>
        void print_help_post_synopsis(
            CustomStringsTag tag, Stream & os, Options const &... opts)
        {
            using opt_tuple_type = hana::tuple<Options const &...>;
            opt_tuple_type opt_tuple{opts...};

            int max_option_length = 0;
            std::vector<printed_names_and_desc> printed_positionals;
            std::vector<printed_names_and_desc> printed_arguments;
            hana::for_each(opt_tuple, [&](auto const & opt) {
                std::vector<printed_names_and_desc> & vec =
                    detail::positional(opt) ? printed_positionals
                                            : printed_arguments;
                std::ostringstream oss;
                detail::print_option(oss, opt, 0, 0, INT_MAX, true);
                vec.emplace_back(std::move(oss).str(), opt.help_text, 0);
                auto const opt_width =
                    detail::estimated_width(vec.back().printed_names);
                vec.back().estimated_width = opt_width;
                max_option_length = (std::max)(max_option_length, opt_width);
            });

            // max_option_length includes a 2-space initial sequence, which acts
            // as an indent.
            int const description_column = (std::min)(
                max_option_length + min_help_column_gap, max_option_col_width);

            customizable_strings const strings =
                help_text_customizable_strings(tag);

            if (!printed_positionals.empty()) {
                os << '\n'
                   << text::as_utf8(strings.positional_section_text) << '\n';
                print_options_and_descs(
                    os, printed_positionals, description_column);
            }

            if (!printed_arguments.empty()) {
                os << '\n'
                   << text::as_utf8(strings.optional_section_text) << '\n';
                print_options_and_descs(
                    os, printed_arguments, description_column);
            }
        }

        template<typename CustomStringsTag, typename Char, typename... Options>
        void print_help(
            CustomStringsTag tag,
            std::basic_ostream<Char> & os,
            std::basic_string_view<Char> argv0,
            std::basic_string_view<Char> desc,
            Options const &... opts)
        {
            std::basic_ostringstream<Char> oss;
            print_help_synopsis(
                tag, oss, detail::program_name(argv0), desc, opts...);
            print_help_post_synopsis(tag, oss, opts...);
            auto const str = std::move(oss).str();
            for (auto const & range :
                 text::bidirectional_subranges(text::as_utf32(str))) {
                os << text::as_utf32(range);
            }
        }
    }

    template<typename T>
    concept option_ = detail::is_option<T>::value;
    template<typename T>
    concept group_ = detail::is_group<T>::value;
    template<typename T>
    concept option_or_group = option_<T> || group_<T>;

    namespace detail {
        // TODO: Add a custom error handler facility.
        template<
            typename CustomStringsTag,
            typename Char,
            option_or_group Option,
            option_or_group... Options>
        auto print_help_and_exit(
            int exit_code,
            CustomStringsTag tag,
            std::basic_string_view<Char> program_name,
            std::basic_string_view<Char> program_desc,
            std::basic_ostream<Char> & os,
            bool no_help,
            Option opt,
            Options... opts)
        {
            if (no_help) {
                detail::print_help(
                    tag,
                    os,
                    program_name,
                    program_desc,
                    detail::default_help(tag),
                    opt,
                    opts...);
            } else {
                detail::print_help(
                    tag, os, program_name, program_desc, opt, opts...);
            }
#ifdef BOOST_PROGRAM_OPTIONS_2_TESTING
            throw 0;
#endif
            std::exit(exit_code);
        }

        template<typename Option>
        constexpr bool has_choices()
        {
            return Option::num_choices != 0;
        }
        template<typename Option>
        constexpr bool has_default()
        {
            return !std::is_same_v<typename Option::value_type, no_value>;
        }

        template<class Char>
        struct command_line_arg
        {
            std::basic_string_view<Char> str;
            int original_position;
        };

        template<typename... Options>
        auto make_result_tuple(Options const &... opts)
        {
            using opt_tuple_type = hana::tuple<Options const &...>;
            opt_tuple_type opt_tuple{opts...};
            return hana::transform(opt_tuple, [](auto const & opt) {
                using opt_type = std::remove_cvref_t<decltype(opt)>;
                constexpr bool required_option =
                    opt_type::positional && opt_type::required;
                constexpr bool has_default = detail::has_default<opt_type>();
                using T = typename opt_type::type;
                if constexpr (std::is_same_v<T, void>) {
                    return no_value{};
                } else if constexpr (required_option) {
                    if constexpr (has_default)
                        return T{opt.default_value};
                    else
                        return T{};
                } else {
                    if constexpr (has_default)
                        return std::optional<T>{opt.default_value};
                    else
                        return std::optional<T>{};
                }
            });
        }

        template<typename Char>
        struct string_view_action
        {
            template<typename Context>
            void operator()(Context & ctx) const
            {
                auto const first = _begin(ctx);
                auto const last = _end(ctx);
                if (first == last) {
                    _val(ctx) = std::basic_string_view<Char>{};
                } else {
                    _val(ctx) = std::basic_string_view<Char>{
                        &*first, std::size_t(last - first)};
                }
            }
        };
        parser::rule<struct string_view_parser, std::string_view> const
            sv_rule = "string_view";
        auto const sv_rule_def =
            parser::raw[*parser::char_][string_view_action<char>{}];
        BOOST_PARSER_DEFINE_RULES(sv_rule);
#if defined(_MSC_VER)
        parser::rule<struct wstring_view_parser, std::wstring_view> const
            wsv_rule = "wstring_view";
        auto const wsv_rule_def =
            parser::raw[*parser::char_][string_view_action<wchar_t>{}];
        BOOST_PARSER_DEFINE_RULES(wsv_rule);
#endif

        template<typename Char, typename T>
        auto parser_for()
        {
            if constexpr (std::is_unsigned_v<T>) {
                return parser::uint_parser<T>{};
            } else if constexpr (std::is_signed_v<T>) {
                return parser::int_parser<T>{};
            } else if constexpr (std::is_floating_point_v<T>) {
                return parser::float_parser<T>{};
            } else if constexpr (std::is_same_v<T, bool>) {
                return parser::bool_parser{};
            } else if constexpr (insertable<T>) {
                return detail::
                    parser_for<Char, std::ranges::range_value_t<T>>();
            } else {
#if defined(_MSC_VER)
                if constexpr(std::is_same_v<Char, char>)
                    return sv_rule;
                else
                    return wsv_rule;
#else
                return sv_rule;
#endif
            }
        }

        template<typename T, typename U>
        void assign_or_insert(T & t, U & u)
        {
            if constexpr (std::is_assignable_v<T &, U &>) {
                t = std::move(u);
            } else {
                t.insert(t.end(), std::move(u));
            }
        }

        enum struct parse_option_error {
            none,
            unknown_arg,
            wrong_number_of_args,
            cannot_parse_arg,
            no_such_choice,
            extra_positional,
            missing_positional
        };

        template<typename Char, typename Option, typename Result>
        auto parse_action_for(
            Option const & opt, Result & result, parse_option_error & error)
        {
            if constexpr (detail::has_choices<Option>()) {
                return [&opt, &result, &error](auto & ctx) {
                    auto const & attr = _attr(ctx);
                    // TODO: Might need to be transcoded if the compareds are
                    // basic_string_view<T>s.
                    if (std::find(
                            opt.choices.begin(), opt.choices.end(), attr) ==
                        opt.choices.end()) {
                        _pass(ctx) = false;
                        error = parse_option_error::no_such_choice;
                    } else {
                        detail::assign_or_insert(result, attr);
                    }
                };
            } else {
                // TODO: Grab the validator out of opt here, if support for
                // that is added.
                return [&result](auto & ctx) {
                    auto const & attr = _attr(ctx);
                    detail::assign_or_insert(result, attr);
                };
            }
        }

        template<typename Char, typename Option, typename Result>
        auto parser_for(
            Option const & opt, Result & result, parse_option_error & error)
        {
            if constexpr (detail::has_choices<Option>()) {
                return detail::parser_for<Char, typename Option::choice_type>()
                    [detail::parse_action_for<Char>(opt, result, error)];
            } else {
                return detail::parser_for<Char, typename Option::type>()
                    [detail::parse_action_for<Char>(opt, result, error)];
            }
        }

        struct parse_option_result
        {
            explicit operator bool() const { return keep_parsing; }

            bool keep_parsing = true;
            parse_option_error error = parse_option_error::none;
        };

        // TODO: Should this return a more complicated result -- one that
        // conveys whether the parse is failed, etc.?
        template<
            typename Char,
            typename ArgsIter,
            typename Option,
            typename ResultType,
            typename PositionIndicesTuple>
        parse_option_result parse_option(
            ArgsIter & first,
            ArgsIter last,
            Option const & opt,
            ResultType & result,
            int & next_positional,
            PositionIndicesTuple const & positional_indices)
        {
            if (first == last)
                return {};

            if (!detail::positional(opt)) {
                auto names = names_view(opt.names);
                if (std::find_if(
                        names.begin(), names.end(), [first](auto name) {
                            return std::ranges::equal(
                                text::as_utf8(name), text::as_utf8(*first));
                        }) != names.end()) {
                    return {};
                }
                ++first;

                if (std::is_same_v<typename Option::type, bool> && !opt.args) {
                    if constexpr (detail::has_default<Option>())
                        result = !bool{opt.default_value};
                    return {};
                }
            }

            // It makes no sense to have a non-positional argument that takes
            // no argument, unless the non-positional argument's type T is
            // bool (handled above).  It also makes no sense to have a
            // positional argument of any type that is supposed to occur 0
            // times.
            BOOST_ASSERT(opt.args);

            auto min_reps = opt.args;
            if (min_reps < 0)
                min_reps = min_reps == one_or_more ? 1 : 0;
            auto max_reps = opt.args;
            if (max_reps < 0)
                max_reps = max_reps == zero_or_one ? 1 : INT_MAX;

            if (first == last) {
                return min_reps == 0
                           ? parse_option_result{}
                           : parse_option_result{
                                 false,
                                 parse_option_error::wrong_number_of_args};
            }

            int reps = 0;
            parse_option_error error = parse_option_error::none;
            auto const parser = detail::parser_for<Char>(opt, result, error);
            if (parser::parse(*first, parser)) {
                ++first;
                ++reps;
                for (; reps < max_reps && first != last &&
                       detail::no_leading_dashes(*first);
                     ++reps, ++first) {
                    if (!parser::parse(*first, parser))
                        break;
                }
            }

            if (min_reps <= reps && reps <= max_reps) {
                if (detail::positional(opt))
                    ++next_positional;
                return {};
            }
            if (reps <= max_reps && error != parse_option_error::none)
                return parse_option_result{false, error};
            return parse_option_result{
                false, parse_option_error::wrong_number_of_args};
        }

        template<typename OptTuple>
        int count_positionals(OptTuple const & opt_tuple)
        {
            int retval = 0;
            hana::for_each(opt_tuple, [&](auto const & opt) {
                if (detail::positional(opt))
                    ++retval;
            });
            return retval;
        }

        template<typename OptTuple>
        int count_arguments(OptTuple const & opt_tuple)
        {
            return (int)hana::size(opt_tuple) - count_positionals(opt_tuple);
        }

        template<typename OptTuple>
        std::string_view positional_name(OptTuple const & opt_tuple, int i)
        {
            std::string_view retval;
            hana::for_each(opt_tuple, [&](auto const & opt) {
                if (detail::positional(opt)) {
                    if (!i--)
                        retval = opt.names;
                }
            });
            return retval;
        }

        template<typename CustomStringsTag, typename Char>
        auto print_parse_error(
            CustomStringsTag tag,
            std::basic_ostream<Char> & os,
            parse_option_error error,
            std::basic_string_view<Char> cl_arg_or_opt_name)
        {
            customizable_strings const strings =
                help_text_customizable_strings(tag);
            auto const error_str = strings.error_strings[(int)error - 1];

            using namespace parser::literals;
            auto const kinda_matched_braces =
                parser::omit[*(parser::char_ - '{')] >>
                parser::raw
                    [('{'_l - "{{") >> *(parser::char_ - '}') >>
                     ('}'_l - "}}")];
            auto first = error_str.begin();
            auto const braces =
                parser::parse(first, error_str.end(), kinda_matched_braces);
            auto const open_brace = braces ? braces->begin() : error_str.end();
            auto const close_brace = braces ? braces->end() : error_str.end();

            os << text::as_utf8(error_str.begin(), open_brace);
            if (braces)
                os << text::as_utf8(cl_arg_or_opt_name);
            os << text::as_utf8(close_brace, error_str.end()) << '\n';
        }

        template<
            typename CustomStringsTag,
            typename Char,
            option_or_group Option,
            option_or_group... Options>
        auto parse_options_into_tuple(
            CustomStringsTag tag,
            arg_view<Char> args,
            std::basic_string_view<Char> program_desc,
            std::basic_ostream<Char> & os,
            bool no_help,
            Option opt,
            Options... opts)
        {
            auto fail = [&](parse_option_error error, auto cl_arg_or_opt_name) {
                detail::print_parse_error(tag, os, error, cl_arg_or_opt_name);
                os << '\n';
                detail::print_help_and_exit(
                    1, tag, args[0], program_desc, os, no_help, opt, opts...);
            };

            auto result = detail::make_result_tuple(opt, opts...);

            using opt_tuple_type =
                hana::tuple<Option const &, Options const &...>;
            opt_tuple_type opt_tuple{opt, opts...};

            int positional_index = -1;
            auto const positional_indices =
                hana::transform(opt_tuple, [&](auto const & opt) {
                    if (detail::positional(opt))
                        ++positional_index;
                    return positional_index;
                });

            auto args_first = args.begin() + 1;
            auto const args_last = args.end();
            int next_positional = 0;
            while (args_first != args_last) {
                using namespace hana::literals;
                auto const initial_args_first = args_first;
                int positional_index = -1;
                hana::fold(opt_tuple, 0_c, [&](auto i, auto const & opt) {
                    auto const i_plus_1 = hana::llong_c<decltype(i)::value + 1>;

                    // Skip this option if it is a positional we've already
                    // processed.
                    if (detail::positional(opt)) {
                        ++positional_index;
                        if (positional_index < next_positional)
                            return i_plus_1;
                    }

                    auto const parse_result = detail::parse_option<Char>(
                        args_first,
                        args_last,
                        opt,
                        result[i],
                        next_positional,
                        positional_indices);
                    if (!parse_result) {
                        if (parse_result.error ==
                                parse_option_error::no_such_choice ||
                            parse_result.error ==
                                parse_option_error::extra_positional) {
                            fail(parse_result.error, *args_first);
                        } else {
                            fail(parse_result.error, opt_tuple[i].names);
                        }
                    }
                    return i_plus_1;
                });
                if (args_first == initial_args_first)
                    fail(parse_option_error::unknown_arg, *args_first);
            }

            if (next_positional != detail::count_positionals(opt_tuple)) {
                fail(
                    parse_option_error::missing_positional,
                    detail::positional_name(opt_tuple, next_positional));
            }

            return result;
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
        // TODO BOOST_ASSERT(args != zero_or_one || detail::is_optional<T>::value);
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
    // positional args are in play?

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
        return {names, help_text, detail::action_kind::count};
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
        return {
            names, help_text, detail::action_kind::version, 0, std::move(f)};
    }

    /** TODO */
    inline detail::option<detail::option_kind::argument, void>
    help(std::string_view names, std::string_view help_text)
    {
        // Looks like you tried to create a non-positional argument that does
        // not start with a '-'.  Don't do that.
        BOOST_ASSERT(!detail::positional(names));
        return {names, help_text, detail::action_kind::version, 0};
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

    /** TODO */
    template<
        detail::option_kind Kind,
        typename T,
        detail::required_t Required,
        int Choices,
        typename ChoiceType,
        typename DefaultType>
        // clang-format off
        requires 
            (Choices == 0 ||
             std::equality_comparable_with<DefaultType, ChoiceType>) &&
             ((std::assignable_from<T &, DefaultType>  &&
               std::constructible_from<T, DefaultType>)||
              detail::insertable_from<T, DefaultType>)
    detail::option<Kind, T, DefaultType, Required, Choices, ChoiceType>
    with_default(
        detail::option<Kind, T, no_value, Required, Choices, ChoiceType>
            opt,
        DefaultType default_value)
    // clang-format on
    {
        if constexpr (std::equality_comparable_with<DefaultType, ChoiceType>) {
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
            opt.help_text,
            opt.action,
            opt.args,
            std::move(default_value),
            opt.choices,
            opt.arg_display_name};
    }

    /** TODO */
    template<
        detail::option_kind Kind,
        typename T,
        typename Value,
        detail::required_t Required,
        int Choices,
        typename ChoiceType>
    auto with_display_name(
        detail::option<Kind, T, Value, Required, Choices, ChoiceType> opt,
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
        detail::option_kind Kind,
        typename T,
        typename Value,
        detail::required_t Required,
        int Choices,
        typename ChoiceType>
    auto readable_file(
        detail::option<Kind, T, Value, Required, Choices, ChoiceType> opt)
    {
        // TODO
        return {};
    }

    /** TODO */
    template<
        detail::option_kind Kind,
        typename T,
        typename Value,
        detail::required_t Required,
        int Choices,
        typename ChoiceType>
    auto writable_file(
        detail::option<Kind, T, Value, Required, Choices, ChoiceType> opt)
    {
        // TODO
        return {};
    }

    /** TODO */
    template<
        detail::option_kind Kind,
        typename T,
        typename Value,
        detail::required_t Required,
        int Choices,
        typename ChoiceType>
    auto readable_path(
        detail::option<Kind, T, Value, Required, Choices, ChoiceType> opt)
    {
        // TODO
        return {};
    }

    /** TODO */
    template<
        detail::option_kind Kind,
        typename T,
        typename Value,
        detail::required_t Required,
        int Choices,
        typename ChoiceType>
    auto writable_path(
        detail::option<Kind, T, Value, Required, Choices, ChoiceType> opt)
    {
        // TODO
        return {};
    }
#endif

    /** TODO */
    template<
        typename CustomStringsTag,
        option_or_group Option,
        option_or_group... Options>
    auto parse_command_line(
        CustomStringsTag tag,
        int argc,
        char const ** argv,
        std::string_view program_desc,
        std::ostream & os,
        Option opt,
        Options... opts)
    {
        detail::check_options(false, opt, opts...);

        arg_view const args(argc, argv);

        bool const no_help = detail::no_help_option(opt, opts...);

        if (no_help && detail::argv_contains_default_help_flag(tag, args)) {
            detail::print_help_and_exit(
                0,
                tag,
                std::string_view(argv[0]),
                program_desc,
                os,
                true,
                opt,
                opts...);
        }

        return detail::parse_options_into_tuple(
            tag, args, program_desc, os, no_help, opt, opts...);
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
            detail::default_strings_tag{},
            argc,
            argv,
            program_desc,
            os,
            opt,
            opts...);
    }

#if defined(BOOST_PROGRAM_OPTIONS_2_DOXYGEN) || defined(_MSC_VER)

    /** TODO */
    template<
        typename CustomStringsTag,
        option_or_group Option,
        option_or_group... Options>
    auto parse_command_line(
        CustomStringsTag tag,
        int argc,
        wchar_t const ** argv,
        std::wstring_view program_desc,
        std::wostream & os,
        Option opt,
        Options... opts)
    {
        detail::check_options(false, opt, opts...);

        arg_view const args(argc, argv);

        bool const no_help = detail::no_help_option(opt, opts...);

        if (no_help && detail::argv_contains_default_help_flag(tag, args)) {
            detail::print_help_and_exit(
                0,
                tag,
                std::wstring_view(argv[0]),
                program_desc,
                os,
                true,
                opt,
                opts...);
        }

        return detail::parse_options_into_tuple(
            tag, args, program_desc, os, no_help, opt, opts...);
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
            detail::default_strings_tag{},
            argc,
            argv,
            program_desc,
            os,
            opt,
            opts...);
    }

    // TODO: Overload for char const * from WinMain.

#endif

}}

#endif
