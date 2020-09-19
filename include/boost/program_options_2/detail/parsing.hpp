// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PROGRAM_OPTIONS_2_PARSING_HPP
#define BOOST_PROGRAM_OPTIONS_2_PARSING_HPP

#include <boost/program_options_2/fwd.hpp>
#include <boost/program_options_2/detail/printing.hpp>

#include <boost/parser/parser.hpp>


namespace boost { namespace program_options_2 { namespace detail {

    template<typename... Options>
    bool no_help_option(Options const &... opts)
    {
        return ((opts.action != detail::action_kind::help) && ...);
    }

    template<typename Args>
    bool argv_contains_default_help_flag(
        customizable_strings const & strings, Args const & args)
    {
        auto const names = names_view(strings.help_names);
        for (auto arg : args) {
            for (auto name : names) {
                if (std::ranges::equal(arg, name))
                    return true;
            }
        }
        return false;
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
    inline parser::rule<struct string_view_parser, std::string_view> const
        sv_rule = "string_view";
    inline auto const sv_rule_def =
        parser::raw[*parser::char_][string_view_action<char>{}];
    BOOST_PARSER_DEFINE_RULES(sv_rule);
#if defined(_MSC_VER)
    inline parser::rule<struct wstring_view_parser, std::wstring_view> const
        wsv_rule = "wstring_view";
    inline auto const wsv_rule_def =
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
            return detail::parser_for<Char, std::ranges::range_value_t<T>>();
        } else {
#if defined(_MSC_VER)
            if constexpr (std::is_same_v<Char, char>)
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
        if constexpr (std::is_same_v<std::remove_cv_t<T>, no_value>) {
            // no-op
        } else if constexpr (std::is_assignable_v<T &, U &>) {
            t = std::move(u);
        } else {
            t.insert(t.end(), std::move(u));
        }
    }

    template<typename Char, typename Option, typename Attr>
    void validate(
        Option const & opt,
        Attr const & attr,
        std::basic_string_view<Char> & validation_error)
    {
        if constexpr (!std::is_same_v<
                          typename Option::validator_type,
                          no_value>) {
            validation_result const validation = opt.validator(attr);
            if (!validation.valid)
                validation_error = validation.error;
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

    template<typename Char>
    auto print_parse_error(
        customizable_strings const & strings,
        std::basic_ostream<Char> & os,
        parse_option_error error,
        std::basic_string_view<Char> cl_arg_or_opt_name)
    {
        auto const error_str = strings.parse_errors[(int)error - 1];

        using namespace parser::literals;
        auto const kinda_matched_braces =
            parser::omit[*(parser::char_ - '{')] >>
            parser::raw
                [('{'_l - "{{") >> *(parser::char_ - '}') >> ('}'_l - "}}")];
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

    template<typename Char, typename Option, typename Result>
    auto parse_action_for(
        Option const & opt,
        Result & result,
        parse_option_error & error,
        std::basic_string_view<Char> & validation_error)
    {
        if constexpr (detail::has_choices<Option>()) {
            return [&opt, &result, &error](auto & ctx) {
                auto const & attr = _attr(ctx);
                // TODO: Might need to be transcoded if the compareds are
                // basic_string_view<T>s.
                if (std::find(opt.choices.begin(), opt.choices.end(), attr) ==
                    opt.choices.end()) {
                    _pass(ctx) = false;
                    error = parse_option_error::no_such_choice;
                } else {
                    detail::assign_or_insert(result, attr);
                }
            };
        } else {
            return [&opt, &result, &validation_error](auto & ctx) {
                auto const & attr = _attr(ctx);
                detail::validate(opt, attr, validation_error);
                detail::assign_or_insert(result, attr);
            };
        }
    }

    template<typename Char, typename Option, typename Result>
    auto parser_for(
        Option const & opt,
        Result & result,
        parse_option_error & error,
        std::basic_string_view<Char> & validation_error)
    {
        if constexpr (detail::has_choices<Option>()) {
            return detail::parser_for<Char, typename Option::choice_type>()
                [detail::parse_action_for<Char>(
                    opt, result, error, validation_error)];
        } else {
            return detail::parser_for<Char, typename Option::type>()
                [detail::parse_action_for<Char>(
                    opt, result, error, validation_error)];
        }
    }

    template<typename Char, typename HelpOption, typename... Options>
    auto handle_help_option(
        customizable_strings const & strings,
        std::basic_string_view<Char> argv0,
        std::basic_string_view<Char> program_desc,
        std::basic_ostream<Char> & os,
        bool no_help,
        HelpOption const & help_opt,
        Options const &... opts)
    {
        if constexpr (std::invocable<typename HelpOption::value_type>) {
            os << text::as_utf8(help_opt.default_value());
#ifdef BOOST_PROGRAM_OPTIONS_2_TESTING
            throw 0;
#endif
            std::exit(0);
        } else {
            detail::print_help_and_exit(
                0, strings, argv0, program_desc, os, no_help, opts...);
        }
    }

    template<typename Char, typename Option>
    auto
    handle_version_option(std::basic_ostream<Char> & os, Option const & opt)
    {
        if constexpr (std::is_same_v<
                          typename Option::value_type,
                          std::string_view>) {
            os << text::as_utf8(opt.default_value);
        }
#ifdef BOOST_PROGRAM_OPTIONS_2_TESTING
        throw 0;
#endif
        std::exit(0);
    }

    template<typename Char, typename... Options>
    auto handle_validation_error(
        customizable_strings const & strings,
        std::basic_string_view<Char> argv0,
        std::basic_string_view<Char> program_desc,
        std::basic_ostream<Char> & os,
        bool no_help,
        std::basic_string_view<Char> error,
        Options const &... opts)
    {
        os << text::as_utf8(error);
        os << '\n';
        detail::print_help_and_exit(
            1, strings, argv0, program_desc, os, no_help, opts...);
    }

    struct parse_option_result
    {
        enum next_t { match_keep_parsing, no_match_keep_parsing, stop_parsing };

        explicit operator bool() const { return next != stop_parsing; }

        next_t next = no_match_keep_parsing;
        parse_option_error error = parse_option_error::none;
    };

    template<
        typename Char,
        typename ArgsIter,
        typename Option,
        typename ResultType,
        typename... Options>
    parse_option_result parse_option(
        customizable_strings const & strings,
        std::basic_string_view<Char> argv0,
        std::basic_string_view<Char> program_desc,
        std::basic_ostream<Char> & os,
        bool no_help,
        ArgsIter & first,
        ArgsIter last,
        Option const & opt,
        ResultType & result,
        int & next_positional,
        Options const &... opts)
    {
        if (first == last)
            return {};

        auto next = parse_option_result::no_match_keep_parsing;
        if (!detail::positional(opt)) {
            if (opt.action == action_kind::count) {
                // Special case: parse the repetitions of a counted flag
                // differently.
                auto short_flag = detail::first_shortest_name(opt.names);
                BOOST_ASSERT(short_flag.size() == 2u);
                if (!first->empty() && first->front() == '-') {
                    int const count = std::count(
                        first->begin() + 1, first->end(), short_flag[1]);
                    if (count + 1 == (int)first->size()) {
                        if constexpr (std::is_assignable_v<ResultType &, int>) {
                            result = count;
                        }
                        ++first;
                        return {parse_option_result::match_keep_parsing};
                    }
                }
            }

            auto const names = names_view(opt.names);
            auto const arg = *first;
            if (std::find_if(names.begin(), names.end(), [arg](auto name) {
                    return std::ranges::equal(
                        text::as_utf8(name), text::as_utf8(arg));
                }) == names.end()) {
                return {};
            }
            ++first;
            next = parse_option_result::match_keep_parsing;

            if constexpr (std::is_same_v<typename Option::type, bool>) {
                if (!opt.args) {
                    if constexpr (detail::has_default<Option>())
                        result = !bool{opt.default_value};
                    return {next};
                }
            }

            // Special case: if we just found a help or version
            // option, handle this now and exit.
            if (opt.action == action_kind::help) {
                detail::handle_help_option(
                    strings, argv0, program_desc, os, no_help, opt, opts...);
            } else if (opt.action == action_kind::version) {
                detail::handle_version_option(os, opt);
            }

            // Special case: early return after matching a long counted
            // flag.
            if (opt.action == action_kind::count) {
                if constexpr (std::is_assignable_v<ResultType &, int>)
                    result = 1;
                return {next};
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
                       ? parse_option_result{next}
                       : parse_option_result{
                             parse_option_result::stop_parsing,
                             parse_option_error::wrong_number_of_args};
        }

        auto handle_validation_error_ =
            [&](std::basic_string_view<Char> validation_error) {
                detail::handle_validation_error(
                    strings,
                    argv0,
                    program_desc,
                    os,
                    no_help,
                    validation_error,
                    opts...);
            };

        int reps = 0;
        parse_option_error error = parse_option_error::none;
        std::basic_string_view<Char> validation_error;
        auto const parser =
            detail::parser_for<Char>(opt, result, error, validation_error);
        if (parser::parse(*first, parser)) {
            if (!validation_error.empty())
                handle_validation_error_(validation_error);
            ++first;
            ++reps;
            for (; reps < max_reps && first != last &&
                   detail::no_leading_dashes(*first);
                 ++reps, ++first) {
                if (!parser::parse(*first, parser))
                    break;
                if (!validation_error.empty())
                    handle_validation_error_(validation_error);
            }
        }

        if (min_reps <= reps && reps <= max_reps) {
            next = parse_option_result::match_keep_parsing;
            if (detail::positional(opt))
                ++next_positional;
            return {next};
        }
        if (reps <= max_reps && error != parse_option_error::none) {
            return parse_option_result{
                parse_option_result::stop_parsing, error};
        }
        return parse_option_result{
            parse_option_result::stop_parsing,
            parse_option_error::wrong_number_of_args};
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

    template<typename Char, typename Args, typename... Options>
    auto parse_options_into_tuple(
        customizable_strings const & strings,
        Args const & args,
        std::basic_string_view<Char> program_desc,
        std::basic_ostream<Char> & os,
        bool no_help,
        Options... opts)
    {
        auto const argv0 = *args.begin();
        auto fail = [&](parse_option_error error, auto cl_arg_or_opt_name) {
            detail::print_parse_error(strings, os, error, cl_arg_or_opt_name);
            os << '\n';
            detail::print_help_and_exit(
                1, strings, argv0, program_desc, os, no_help, opts...);
        };

        auto result = detail::make_result_tuple(opts...);

        using opt_tuple_type = hana::tuple<Options const &...>;
        opt_tuple_type opt_tuple{opts...};

        auto parse_option_ = [&](auto & first,
                                 auto last,
                                 auto const & opt,
                                 auto & result,
                                 int & next_positional) {
            return detail::parse_option<Char>(
                strings,
                argv0,
                program_desc,
                os,
                no_help,
                first,
                last,
                opt,
                result,
                next_positional,
                opts...);
        };

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

                auto const parse_result = parse_option_(
                    args_first, args_last, opt, result[i], next_positional);
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

}}}

#endif
