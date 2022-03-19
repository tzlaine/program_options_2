// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PROGRAM_OPTIONS_2_PARSING_HPP
#define BOOST_PROGRAM_OPTIONS_2_PARSING_HPP

#include <boost/program_options_2/concepts.hpp>
#include <boost/program_options_2/arg_view.hpp>
#include <boost/program_options_2/options.hpp>
#include <boost/program_options_2/detail/printing.hpp>

#include <boost/parser/parser.hpp>
#include <boost/type_traits/is_detected.hpp>


namespace boost { namespace program_options_2 { namespace detail {

    template<typename T>
    using type_eraser = decltype(
        std::declval<T &>() = std::declval<void *>(),
        std::declval<T &>() = std::declval<no_value>(),
        std::declval<T &>() =
            std::declval<std::pair<int, std::vector<double> *>>());

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

    template<typename Option, typename T = typename Option::type>
    using result_map_element_t =
        std::conditional_t<std::is_same_v<T, void>, no_value, T>;

    template<typename... Options>
    auto variant_for_tuple(hana::tuple<Options...> const & opts)
    {
        return std::variant<typename Options::type...>{};
    }

    template<typename Option>
    auto make_result_tuple_element()
    {
        constexpr bool required_option = Option::positional || Option::required;
        using T = typename Option::type;
        if constexpr (std::is_same_v<T, void>) {
            return no_value{};
        } else if constexpr (required_option) {
            return T{};
        } else {
            return std::optional<T>{};
        }
    };

    template<typename... Options>
    auto make_result_tuple(hana::tuple<Options...> const & opts)
    {
        return hana::transform(opts, [](auto const & opt) {
            using opt_type = std::remove_cvref_t<decltype(opt)>;
            if constexpr (is_group<opt_type>::value) {
                if constexpr (opt_type::mutually_exclusive)
                    return detail::variant_for_tuple(opt.options);
                else
                    return detail::make_result_tuple(opt.options);
            } else {
                constexpr bool required_option =
                    opt_type::positional || opt_type::required;
                auto retval = detail::make_result_tuple_element<opt_type>();
                if constexpr (required_option && detail::flag<opt_type>())
                    retval = opt.default_value;
                return retval;
            }
        });
    }

    template<typename... Options>
    auto make_result_tuple(Options const &... opts)
    {
        auto const opt_tuple = detail::make_opt_tuple(opts...);
        return detail::make_result_tuple(opt_tuple);
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

    inline parser::rule<struct string_parser, std::string> const s_rule =
        "string";
    inline auto const s_rule_def =
        parser::raw[*parser::char_][string_view_action<char>{}];
    inline parser::rule<struct string_view_parser, std::string_view> const
        sv_rule = "string_view";
    inline auto const sv_rule_def =
        parser::raw[*parser::char_][string_view_action<char>{}];
    BOOST_PARSER_DEFINE_RULES(s_rule, sv_rule);
#if defined(_MSC_VER)
    inline parser::rule<struct wstring_parser, std::wstring> const ws_rule =
        "wstring";
    inline auto const ws_rule_def =
        parser::raw[*parser::char_][string_view_action<wchar_t>{}];
    inline parser::rule<struct wstring_view_parser, std::wstring_view> const
        wsv_rule = "wstring_view";
    inline auto const wsv_rule_def =
        parser::raw[*parser::char_][string_view_action<wchar_t>{}];
    BOOST_PARSER_DEFINE_RULES(ws_rule, wsv_rule);
#endif

    template<typename T>
    struct is_string : std::false_type
    {};
    template<typename Char>
    struct is_string<std::basic_string<Char>> : std::true_type
    {};

    template<typename Char, typename T>
    auto parser_for()
    {
        if constexpr (is_optional<T>::value) {
            return detail::parser_for<Char, typename T::value_type>();
        } else if constexpr (is_string<T>::value) {
#if defined(_MSC_VER)
            if constexpr (std::is_same_v<Char, wchar_t>)
                return ws_rule;
            else
#else
            return s_rule;
#endif
        } else if constexpr (insertable<T>) {
            return detail::parser_for<Char, std::ranges::range_value_t<T>>();
        } else if constexpr (std::is_same_v<T, bool>) {
            return parser::parser_interface<parser::bool_parser>{};
        } else if constexpr (std::is_floating_point_v<T>) {
            return parser::parser_interface<parser::float_parser<T>>{};
        } else if constexpr (std::is_unsigned_v<T>) {
            return parser::parser_interface<parser::uint_parser<T>>{};
        } else if constexpr (std::is_signed_v<T>) {
            return parser::parser_interface<parser::int_parser<T>>{};
        } else {
#if defined(_MSC_VER)
            if constexpr (std::is_same_v<Char, wchar_t>)
                return wsv_rule;
            else
#else
            return sv_rule;
#endif
        }
    }

    template<bool UnwrapOptionals = true, typename T, typename U>
    void assign_or_insert_impl(T & t, U & u)
    {
        if constexpr (std::is_same_v<std::remove_cv_t<T>, no_value>) {
            // no-op
        } else if constexpr (
            UnwrapOptionals && is_optional<std::remove_cv_t<T>>::value) {
            using value_type = typename T::value_type;
            if (!t)
                t = value_type{};
            auto & t_value = *t;
            if constexpr (std::is_assignable_v<value_type &, U &>)
                t_value = std::move(u);
            else
                t_value.insert(t_value.end(), std::move(u));
        } else {
            if constexpr (std::is_assignable_v<T &, U &>)
                t = std::move(u);
            else
                t.insert(t.end(), std::move(u));
        }
    }

    template<typename T>
    struct is_erased_type
        : std::integral_constant<
              bool,
              boost::is_detected<type_eraser, std::remove_cv_t<T>>::value>
    {};
    template<typename Option, typename T>
    struct inserting_into_any
        : std::integral_constant<
              bool,
              is_erased_type<T>::value &&
                  !std::is_same_v<typename Option::type, std::remove_cv_t<T>>>
    {};

    template<typename Option, typename T, typename U>
    void assign_or_insert(T & t, U && u)
    {
        // The calls below to assign_or_insert_impl<false>() use false because
        // assign_or_insert() should only unwrap the top-level optionality,
        // whether that comes in the form of a std::optional<> or an erased
        // type.
        if constexpr (inserting_into_any<Option, T>::value) {
            using stored_type = result_map_element_t<Option>;
            if (program_options_2::any_empty(t)) {
                stored_type temp;
                detail::assign_or_insert_impl<false>(temp, u);
                t = temp;
            } else {
                detail::assign_or_insert_impl<false>(
                    program_options_2::any_cast<stored_type &>(t), u);
            }
        } else {
            detail::assign_or_insert_impl(t, u);
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
        missing_positional,
        validation_error
    };

    template<typename Char>
    void print_parse_error(
        customizable_strings const & strings,
        std::basic_ostream<Char> & os,
        parse_option_error error,
        std::basic_string_view<Char> cl_arg_or_opt_name)
    {
        auto const error_str = strings.parse_errors[(int)error - 1];
        detail::print_placeholder_string(os, error_str, cl_arg_or_opt_name);
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
                bool const consumed_all = _where(ctx).end() == _end(ctx);
                // TODO: Might need to be transcoded if the compareds are
                // basic_string_view<T>s.
                if (!consumed_all) {
                    _pass(ctx) = false;
                } else if (
                    std::find(opt.choices.begin(), opt.choices.end(), attr) ==
                    opt.choices.end()) {
                    _pass(ctx) = false;
                    error = parse_option_error::no_such_choice;
                } else {
                    detail::assign_or_insert<Option>(result, attr);
                }
            };
        } else {
            return [&opt, &result, &validation_error](auto & ctx) {
                auto const & attr = _attr(ctx);
                bool const consumed_all = _where(ctx).end() == _end(ctx);
                if (!consumed_all) {
                    _pass(ctx) = false;
                } else {
                    detail::validate(opt, attr, validation_error);
                    detail::assign_or_insert<Option>(result, attr);
                }
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
    void handle_help_option(
        customizable_strings const & strings,
        bool deserializing,
        std::basic_string_view<Char> argv0,
        std::basic_string_view<Char> program_desc,
        std::basic_ostream<Char> & os,
        bool no_help,
        HelpOption const & help_opt,
        Options const &... opts)
    {
        if (deserializing)
            return;
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
    void handle_version_option(
        bool deserializing, std::basic_ostream<Char> & os, Option const & opt)
    {
        if (deserializing)
            return;
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
    void handle_validation_error(
        customizable_strings const & strings,
        bool deserializing,
        std::basic_string_view<Char> argv0,
        std::basic_string_view<Char> program_desc,
        std::basic_ostream<Char> & os,
        bool no_help,
        std::basic_string_view<Char> error,
        Options const &... opts)
    {
        if (deserializing)
            return;
        os << text::as_utf8(error);
        os << '\n';
        detail::print_help_and_exit(
            1, strings, argv0, program_desc, os, no_help, opts...);
    }

    struct parse_option_result
    {
        enum next_t {
            match_keep_parsing,
            no_match_keep_parsing,
            stop_parsing,

            // In this one case, the current arg is the response file.  That
            // is, it is not consumed within parse_option().
            response_file
        };

        explicit operator bool() const { return next != stop_parsing; }

        next_t next = no_match_keep_parsing;
        parse_option_error error = parse_option_error::none;
    };

    template<typename Char, typename Option>
    bool matches_dashed_argument(
        std::basic_string_view<Char> arg, Option const & opt)
    {
        auto const names = names_view(opt.names);
        if (std::find_if(names.begin(), names.end(), [arg](auto name) {
                return std::ranges::equal(
                    text::as_utf8(name), text::as_utf8(arg));
            }) != names.end()) {
            return true;
        }
        return false;
    }

    template<typename Char, typename... Options>
    bool known_dashed_argument(
        std::basic_string_view<Char> str, Options const &... opts)
    {
        if (!detail::leading_dash(str))
            return false;
        return (detail::matches_dashed_argument(str, opts) || ...);
    }

    template<
        typename Char,
        typename ArgsIter,
        typename Option,
        typename ResultType,
        typename... Options>
    parse_option_result parse_option(
        customizable_strings const & strings,
        bool deserializing,
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
                auto short_flag = detail::first_short_name(opt.names);
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

            if (!detail::matches_dashed_argument(*first, opt))
                return {};

            ++first;
            next = parse_option_result::match_keep_parsing;

            // Special case: if we just found a flag option, assign the right
            // value and return.
            if constexpr (detail::flag<Option>()) {
                result = !opt.default_value;
                return {next};
            }

            using option_result_type =
                decltype(detail::make_result_tuple_element<Option>());
            if constexpr (
                !Option::required && is_optional<option_result_type>::value) {
                if (opt.args == zero_or_one || opt.args == zero_or_more) {
                    detail::assign_or_insert<Option>(
                        result, typename Option::type{});
                }
            }

            // Special case: if we just found a help or version
            // option, handle this now and exit.
            if (opt.action == action_kind::help) {
                detail::handle_help_option(
                    strings,
                    deserializing,
                    argv0,
                    program_desc,
                    os,
                    no_help,
                    opt,
                    opts...);
            } else if (opt.action == action_kind::version) {
                detail::handle_version_option(deserializing, os, opt);
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
                    deserializing,
                    argv0,
                    program_desc,
                    os,
                    no_help,
                    validation_error,
                    opts...);
            };

        // Special case: If we just parsed the dashed arg that precedes a
        // response file, return immediately, so that the caller can process
        // the file.
        if (opt.action == action_kind::response_file && !first->empty()) {
            if constexpr (!std::is_same_v<
                              typename Option::validator_type,
                              no_value>) {
                validation_result const validation = opt.validator(*first);
                if (!validation.valid)
                    handle_validation_error_(validation.error);
            }
            return {parse_option_result::response_file};
        }

        int reps = 0;
        parse_option_error error = parse_option_error::none;
        std::basic_string_view<Char> validation_error;
        auto const parser =
            detail::parser_for<Char>(opt, result, error, validation_error);
        if (!detail::known_dashed_argument(*first, opts...) &&
            parser::parse(*first, parser)) {
            if (!validation_error.empty()) {
                handle_validation_error_(validation_error);
                return {
                    parse_option_result::stop_parsing,
                    parse_option_error::validation_error};
            }
            ++first;
            ++reps;
            for (; reps < max_reps && first != last &&
                   !detail::known_dashed_argument(*first, opts...);
                 ++reps, ++first) {
                if (!parser::parse(*first, parser)) {
                    if (error == parse_option_error::none)
                        error = parse_option_error::cannot_parse_arg;
                    break;
                }
                if (!validation_error.empty()) {
                    handle_validation_error_(validation_error);
                    return {
                        parse_option_result::stop_parsing,
                        parse_option_error::validation_error};
                }
            }
        } else {
            if (error == parse_option_error::none)
                error = parse_option_error::cannot_parse_arg;
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
            if (opt.positional && opt.required)
                ++retval;
        });
        return retval;
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

    template<
        typename Accessor,
        typename Char,
        typename Args,
        typename... Options>
    parse_option_result parse_options_into(
        Accessor accessor,
        int & next_positional,
        customizable_strings const & strings,
        bool deserializing,
        Args const & args,
        bool skip_first,
        std::basic_string_view<Char> program_desc,
        std::basic_ostream<Char> & os,
        bool no_help,
        Options const &... opts)
    {
        // This dance is here to support the case where the values returned by
        // args are temporaries -- args may have an underlying proxy iterator.
        std::basic_string<Char> const argv0_str(
            args.begin()->begin(), args.begin()->end());
        std::basic_string_view<Char> argv0 = argv0_str;

        auto fail = [&](parse_option_error error,
                        std::basic_string_view<Char> cl_arg_or_opt_name) {
            if (deserializing)
                return;
            detail::print_parse_error(strings, os, error, cl_arg_or_opt_name);
            os << '\n';
            detail::print_help_and_exit(
                1, strings, argv0, program_desc, os, no_help, opts...);
        };

        auto parse_option_ = [&](auto & first,
                                 auto last,
                                 auto const & opt,
                                 auto & result) {
            return detail::parse_option<Char>(
                strings,
                deserializing,
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

        auto process_response_file = [&](auto & sv_it) {
            auto const arg_utf8 = text::as_utf8(*sv_it);
            std::string arg(arg_utf8.begin(), arg_utf8.end());
            if (arg.front() == '@')
                arg.erase(arg.begin());
            std::ifstream ifs(arg.c_str());
            ifs.unsetf(ifs.skipws);
            detail::parse_options_into(
                accessor,
                next_positional,
                strings,
                deserializing,
                detail::response_file_arg_view(ifs),
                false,
                program_desc,
                os,
                no_help,
                opts...);
            ++sv_it;
        };

        using namespace hana::literals;

        auto const opt_tuple = detail::make_opt_tuple(opts...);

        auto args_first = args.begin();
        if (skip_first)
            ++args_first;
        auto const args_last = args.end();
        while (args_first != args_last) {
            // Special case: an arg starting with @ names a response file.
            if (!strings.response_file_description.empty() &&
                !args_first->empty() && args_first->front() == '@') {
                auto const response_file_opt =
                    program_options_2::response_file("-d", "Dummy.", strings);
                validation_result const validation =
                    response_file_opt.validator(args_first->substr(1));
                if (!validation.valid) {
                    detail::handle_validation_error(
                        strings,
                        deserializing,
                        argv0,
                        program_desc,
                        os,
                        no_help,
                        validation.error,
                        opts...);
                    return {
                        parse_option_result::stop_parsing,
                        parse_option_error::validation_error};
                }
                process_response_file(args_first);
                continue;
            }

            auto const initial_args_first = args_first;
            int positional_index = -1;
            parse_option_result parse_result;
            hana::fold(opt_tuple, 0_c, [&](auto i, auto const & opt) {
                auto const i_plus_1 = hana::llong_c<decltype(i)::value + 1>;

                if (!parse_result)
                    return i_plus_1;

                // Skip this option if it is a positional we've already
                // processed.
                if (detail::positional(opt)) {
                    ++positional_index;
                    if (positional_index < next_positional)
                        return i_plus_1;
                }

                parse_result =
                    parse_option_(args_first, args_last, opt, accessor(opt, i));

                // Special case: if we just parsed a response_file opt
                // successfully, process the file.
                if (parse_result.next == parse_option_result::response_file) {
                    process_response_file(args_first);
                    return i_plus_1;
                }

                if (!parse_result) {
                    if (parse_result.error ==
                            parse_option_error::cannot_parse_arg ||
                        parse_result.error ==
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

            if (!parse_result)
                return parse_result;
            if (args_first == initial_args_first) {
                fail(parse_option_error::unknown_arg, *args_first);
                return {
                    parse_option_result::stop_parsing,
                    parse_option_error::unknown_arg};
            }
        }

        // Partial sets of positionals are ok when deserializing.
        if (!deserializing &&
            next_positional < detail::count_positionals(opt_tuple)) {
            std::basic_ostringstream<Char> oss;
            detail::print_uppercase(
                oss, detail::positional_name(opt_tuple, next_positional));
            fail(parse_option_error::missing_positional, oss.str());
        }

        auto empty = [](auto const & result_i) {
            using type = std::remove_cvref_t<decltype(result_i)>;
            if constexpr (is_erased_type<type>::value) {
                return program_options_2::any_empty(result_i);
            } else {
                return !result_i;
            }
        };

        hana::fold(opt_tuple, 0_c, [&](auto i, auto const & opt) {
            auto const i_plus_1 = hana::llong_c<decltype(i)::value + 1>;
            using opt_type = std::remove_cvref_t<decltype(opt)>;
            auto & result_i = accessor(opt, i);
            using result_type = std::remove_cvref_t<decltype(result_i)>;
            if constexpr (
                !opt_type::required && detail::has_default<opt_type>() &&
                !std::is_same_v<result_type, no_value>) {
                if (empty(result_i)) {
                    detail::assign_or_insert<opt_type>(
                        result_i, opt.default_value);
                }
            }
            return i_plus_1;
        });

        return {
            parse_option_result::match_keep_parsing, parse_option_error::none};
    }

    template<typename Char, typename Args, typename... Options>
    auto parse_options_as_tuple(
        customizable_strings const & strings,
        Args const & args,
        std::basic_string_view<Char> program_desc,
        std::basic_ostream<Char> & os,
        bool no_help,
        Options const &... opts)
    {
        auto result = detail::make_result_tuple(opts...);
        int next_positional = 0;
        detail::parse_options_into(
            [&](auto const & opt, auto i) -> decltype(auto) {
                return result[i];
            },
            next_positional,
            strings,
            false,
            args,
            true,
            program_desc,
            os,
            no_help,
            opts...);
        return result;
    }

    template<typename Map, typename Key = map_key_t<Map>>
    struct map_lookup;

    template<typename Map>
    struct map_lookup<Map, std::string>
    {
        map_lookup(Map & m) : m_(m) {}
        template<typename Option>
        auto find(Option const & opt)
        {
            scratch_ = program_options_2::storage_name(opt);
            return m_.find(scratch_);
        }
        template<typename Option, long long I>
        decltype(auto) operator()(Option const & opt, hana::llong<I>)
        {
            scratch_ = program_options_2::storage_name(opt);
            return m_[scratch_];
        }

    private:
        Map & m_;
        std::string scratch_;
    };

    template<typename Map>
    struct map_lookup<Map, std::string_view>
    {
        map_lookup(Map & m) : m_(m) {}
        template<typename Option>
        auto find(Option const & opt)
        {
            return m_.find(program_options_2::storage_name(opt));
        }
        template<typename Option, long long I>
        decltype(auto) operator()(Option const & opt, hana::llong<I>)
        {
            return m_[program_options_2::storage_name(opt)];
        }

    private:
        Map & m_;
    };

    template<
        typename OptionsMap,
        typename Char,
        typename Args,
        typename... Options>
    parse_option_result parse_options_into_map(
        OptionsMap & result,
        customizable_strings const & strings,
        bool deserializing,
        Args const & args,
        std::basic_string_view<Char> program_desc,
        std::basic_ostream<Char> & os,
        bool no_help,
        bool skip_first,
        Options const &... opts)
    {
        int next_positional = 0;
        auto const retval = detail::parse_options_into(
            map_lookup<OptionsMap>(result),
            next_positional,
            strings,
            deserializing,
            args,
            skip_first,
            program_desc,
            os,
            no_help,
            opts...);
        auto it = result.begin();
        auto prev_it = it;
        auto const last = result.end();
        while (it != last) {
            if (program_options_2::any_empty(it->second)) {
                prev_it = it;
                ++it;
                result.erase(prev_it);
            } else {
                ++it;
            }
        }
        return retval;
    }

}}}

#endif
