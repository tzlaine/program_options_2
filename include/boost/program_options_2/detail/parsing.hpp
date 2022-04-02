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

#include <boost/container/flat_map.hpp>
#include <boost/parser/parser.hpp>
#include <boost/text/string_utility.hpp>
#include <boost/type_traits/is_detected.hpp>


namespace boost { namespace program_options_2 { namespace detail {

    template<typename... Options>
    bool no_help_option(Options const &... opts)
    {
        return !detail::help_option(opts...);
    }

    template<typename Args>
    bool argv_contains_default_help_flag(
        customizable_strings const & strings, Args const & args)
    {
        auto const names = names_view(strings.default_help_names);
        for (auto arg : args) {
            for (auto name : names) {
                if (std::ranges::equal(text::as_utf8(arg), text::as_utf8(name)))
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

    template<typename... Ts>
    auto to_variant(hana::set<Ts...> const &)
    {
        return std::variant<typename Ts::type...>{};
    }

    template<typename Option1, typename... Options>
    auto variant_for_tuple(hana::tuple<Option1, Options...> const & opts)
    {
        // TODO: These should probably be conditionally optional<>s.
        if constexpr ((std::is_same_v<
                           typename Option1::type,
                           typename Options::type> &&
                       ...)) {
            return typename Option1::type{};
        } else {
            auto const types = hana::transform(opts, [](auto const & opt) {
                return hana::type_c<
                    typename std::remove_cvref_t<decltype(opt)>::type>;
            });
            auto const unique_types = hana::to_set(types);
            return detail::to_variant(unique_types);
        }
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
        auto unflattened = hana::transform(opts, [](auto const & opt) {
            using opt_type = std::remove_cvref_t<decltype(opt)>;
            if constexpr (is_group<opt_type>::value) {
                if constexpr (opt_type::mutually_exclusive)
                    return hana::make_tuple(
                        detail::variant_for_tuple(opt.options));
                else
                    return detail::make_result_tuple(opt.options);
            } else {
                constexpr bool required_option =
                    opt_type::positional || opt_type::required;
                auto retval = detail::make_result_tuple_element<opt_type>();
                if constexpr (required_option && detail::flag<opt_type>())
                    retval = opt.default_value;
                return hana::make_tuple(retval);
            }
        });
        return hana::flatten(unflattened);
    }

    template<typename... Options>
    auto make_result_tuple(Options const &... opts)
    {
        using opts_as_tuple_type = hana::tuple<Options const &...>;
        return detail::make_result_tuple(opts_as_tuple_type{opts...});
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

    template<
        typename OptionType,
        bool UnwrapOptionals = true,
        typename T,
        typename U>
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
            if constexpr (std::is_assignable_v<T &, U &>) {
                // See note at top of assign_or_insert() that explains this
                // logic.
                if constexpr (!std::is_same_v<OptionType, std::remove_cv_t<U>>)
                    t = static_cast<OptionType>(u);
                else
                    t = std::move(u);
            } else {
                t.insert(t.end(), std::move(u));
            }
        }
    }

    template<typename T>
    struct is_erased_type
        : std::integral_constant<bool, erased_type<std::remove_cv_t<T>>>
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
        // This is needed for cases like exclusive(option<int, ...>,
        // option<double, ...>), where the options have choices associated
        // with then.  In such a case, the result is T =
        // std::variant<int,double>, and the parser will parse whatever type
        // the choices are (say, U = int).  We need the choices generated by
        // the parser for the second option to be assigned as doubles, not
        // ints!
        using opt_type = typename Option::type;

        // The calls below to assign_or_insert_impl<false>() use false because
        // assign_or_insert() should only unwrap the top-level optionality,
        // whether that comes in the form of a std::optional<> or an erased
        // type.
        if constexpr (inserting_into_any<Option, T>::value) {
            using stored_type = result_map_element_t<Option>;
            if (program_options_2::any_empty(t)) {
                stored_type temp;
                detail::assign_or_insert_impl<opt_type, false>(temp, u);
                t = temp;
            } else {
                detail::assign_or_insert_impl<opt_type, false>(
                    program_options_2::any_cast<stored_type &>(t), u);
            }
        } else {
            detail::assign_or_insert_impl<opt_type>(t, u);
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

    template<typename Char>
    void print_parse_error(
        customizable_strings const & strings,
        std::basic_ostream<Char> & os,
        parse_option_error error,
        std::basic_string_view<Char> cl_arg_or_opt_name,
        std::basic_string_view<Char> opt_name = {})
    {
        auto const error_str = strings.parse_errors[(int)error - 1];
        detail::print_placeholder_string(
            os, error_str, cl_arg_or_opt_name, opt_name);
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
                    std::ranges::find(opt.choices, attr) == opt.choices.end()) {
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
            parse_contexts_vec const parse_contexts;
            detail::print_help_and_exit(
                0,
                strings,
                argv0,
                program_desc,
                os,
                no_help,
                parse_contexts,
                opts...);
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
        parse_contexts_vec const & parse_contexts,
        Options const &... opts)
    {
        if (deserializing)
            return;
        os << text::as_utf8(error);
        os << '\n';
        detail::print_help_and_exit(
            1,
            strings,
            argv0,
            program_desc,
            os,
            no_help,
            parse_contexts,
            opts...);
    }

    template<typename Char>
    bool matches_view(std::basic_string_view<Char> arg, names_view names)
    {
        if (std::ranges::find_if(names, [&](auto name) {
                return std::ranges::equal(
                    text::as_utf8(arg), text::as_utf8(name));
            }) != names.end()) {
            return true;
        }
        return false;
    }

    template<typename Char>
    bool matches_names(std::basic_string_view<Char> arg, std::string_view str)
    {
        auto const names = names_view(str);
        return detail::matches_view(arg, names);
    }

    template<typename Char>
    struct known_dashed_argument_impl
    {
        template<typename... Options>
        bool operator()(Options const &... opts)
        {
            if (!detail::leading_dash(arg))
                return false;
            return (single(opts) || ...);
        }

        template<typename Option>
        bool single(Option const & opt)
        {
            if constexpr (group_<Option>)
                return hana::unpack(opt.options, *this);
            else
                return detail::matches_names(arg, opt.names);
        }

        std::basic_string_view<Char> arg;
    };

    // TODO: This should maybe use a trie (pending perf testing, of course).
    template<typename Char, typename... Options>
    bool known_dashed_argument(
        std::basic_string_view<Char> arg, Options const &... opts)
    {
        known_dashed_argument_impl impl{arg};
        return impl(opts...);
    }

    template<typename Char>
    using exclusives_map =
        boost::container::flat_map<int, std::basic_string<Char>>;

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
        exclusives_map<Char> & exclusives_seen,
        int exclusives_group,
        parse_contexts_vec const & parse_contexts,
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

            if (!detail::known_dashed_argument(*first, opt))
                return {};

            if (0 <= exclusives_group) {
                if (exclusives_seen.count(exclusives_group)) {
                    return {
                        parse_option_result::stop_parsing,
                        parse_option_error::too_many_mutally_exclusives};
                } else {
                    exclusives_seen[exclusives_group] = *first;
                }
            }

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
                    parse_contexts,
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
        typename ArgsIter,
        typename FailFunc,
        typename OptTuple,
        typename... Options>
    parse_option_result parse_options_into_impl(
        Accessor accessor,
        int & next_positional,
        customizable_strings const & strings,
        bool deserializing,
        ArgsIter & first,
        ArgsIter last,
        std::basic_string_view<Char> argv0,
        std::basic_string_view<Char> program_desc,
        std::basic_ostream<Char> & os,
        bool no_help,
        FailFunc const & fail,
        exclusives_map<Char> & exclusives_seen,
        int exclusives_group,
        OptTuple const & opt_tuple,
        parse_contexts_vec const & parse_contexts,
        Options const &... opts)
    {
        auto parse_option_ =
            [&](auto & first, auto last, auto const & opt, auto & result) {
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
                    exclusives_seen,
                    exclusives_group,
                    parse_contexts,
                    opts...);
            };

        auto process_response_file = [&](auto & sv_it) {
            auto const arg_utf8 = text::as_utf8(*sv_it);
            std::string arg(arg_utf8.begin(), arg_utf8.end());
            if (arg.front() == '@')
                arg.erase(arg.begin());
            std::ifstream ifs(arg.c_str());
            ifs.unsetf(ifs.skipws);
            auto file_args = detail::response_file_arg_view(ifs);
            auto file_first = file_args.begin();
            auto const file_last = file_args.end();
            detail::parse_options_into_impl(
                accessor,
                next_positional,
                strings,
                deserializing,
                file_first,
                file_last,
                argv0,
                program_desc,
                os,
                no_help,
                fail,
                exclusives_seen,
                -1,
                opt_tuple,
                parse_contexts,
                opts...);
            ++sv_it;
        };

        using namespace hana::literals;

        while (first != last) {
            // Special case: an arg starting with @ names a response file.
            if (!strings.response_file_note.empty() && !first->empty() &&
                first->front() == '@') {
                auto const response_file_opt =
                    program_options_2::response_file("-d", "Dummy.", strings);
                validation_result const validation =
                    response_file_opt.validator(first->substr(1));
                if (!validation.valid) {
                    detail::handle_validation_error(
                        strings,
                        deserializing,
                        argv0,
                        program_desc,
                        os,
                        no_help,
                        validation.error,
                        parse_contexts,
                        opts...);
                    return {
                        parse_option_result::stop_parsing,
                        parse_option_error::validation_error};
                }
                process_response_file(first);
                continue;
            }

            auto const initial_first = first;
            int positional_index = -1;
            parse_option_result parse_result;

            // Parse a typical option.
            auto parse_leaf_opt = [&](auto i, auto const & opt) {
                // Skip this option if it is a positional we've already
                // processed.
                if (detail::positional(opt)) {
                    ++positional_index;
                    if (positional_index < next_positional)
                        return;
                }

                parse_result =
                    parse_option_(first, last, opt, accessor(opt, i));

                // Special case: if we just parsed a response_file opt
                // successfully, process the file.
                if (parse_result.next == parse_option_result::response_file) {
                    process_response_file(first);
                    return;
                }

                if (!parse_result) {
                    if (parse_result.error ==
                            parse_option_error::cannot_parse_arg ||
                        parse_result.error ==
                            parse_option_error::extra_positional) {
                        fail(parse_result.error, *first);
                    } else if (
                        parse_result.error ==
                        parse_option_error::no_such_choice) {
                        fail(parse_result.error, *first, opt_tuple[i].names);
                    } else if (
                        parse_result.error ==
                        parse_option_error::too_many_mutally_exclusives) {
                        BOOST_ASSERT(0 <= exclusives_group);
                        fail(
                            parse_result.error,
                            exclusives_seen[exclusives_group],
                            *first);
                    } else {
                        fail(parse_result.error, opt_tuple[i].names);
                    }
                }
            };

            hana::fold(opt_tuple, 0_c, [&](auto i, auto const & opt) {
                auto const i_plus_1 = hana::llong_c<decltype(i)::value + 1>;

                if (!parse_result)
                    return i_plus_1;

                using opt_type = std::remove_cvref_t<decltype(opt)>;
                if constexpr (group_<opt_type>) {
                    if constexpr (opt.subcommand) {
                        // Intentionally skipped; commands are parsed in a
                        // pre-pass.
                    } else if constexpr (opt.mutually_exclusive) {
                        parse_result = detail::parse_options_into_impl(
                            [&](auto const & opt, auto j) -> decltype(auto) {
                                return accessor(opt, i);
                            },
                            next_positional,
                            strings,
                            deserializing,
                            first,
                            last,
                            argv0,
                            program_desc,
                            os,
                            no_help,
                            fail,
                            exclusives_seen,
                            (int)i,
                            detail::make_opt_tuple(
                                detail::to_ref_tuple(opt.options)),
                            parse_contexts,
                            opts...);
                    } else {
                        parse_leaf_opt(i, opt);
                    }
                } else {
                    parse_leaf_opt(i, opt);
                }

                return i_plus_1;
            });

            if (!parse_result)
                return parse_result;
            if (first == initial_first) {
                fail(parse_option_error::unknown_arg, *first);
                return {
                    parse_option_result::stop_parsing,
                    parse_option_error::unknown_arg};
            }
        }

        return {
            parse_option_result::match_keep_parsing, parse_option_error::none};
    }

    template<
        typename Accessor,
        typename Char,
        typename ArgsIter,
        typename OptTuple,
        typename... Options>
    parse_option_result parse_options_into(
        Accessor accessor,
        int & next_positional,
        customizable_strings const & strings,
        bool deserializing,
        std::basic_string_view<Char> argv0,
        ArgsIter & first,
        ArgsIter last,
        bool skip_first,
        std::basic_string_view<Char> program_desc,
        std::basic_ostream<Char> & os,
        bool no_help,
        OptTuple const & opt_tuple,
        parse_contexts_vec const & parse_contexts,
        Options const &... opts)
    {
        exclusives_map<Char> exclusives_seen;

        if (skip_first)
            ++first;

        auto fail = [&](parse_option_error error,
                        std::basic_string_view<Char> cl_arg_or_opt_name,
                        std::basic_string_view<Char> opt_name = {}) {
            if (deserializing)
                return;
            detail::print_parse_error(
                strings, os, error, cl_arg_or_opt_name, opt_name);
            os << '\n';
            detail::print_help_and_exit(
                1,
                strings,
                argv0,
                program_desc,
                os,
                no_help,
                parse_contexts,
                opts...);
        };

        auto const impl_result = detail::parse_options_into_impl(
            accessor,
            next_positional,
            strings,
            deserializing,
            first,
            last,
            argv0,
            program_desc,
            os,
            no_help,
            fail,
            exclusives_seen,
            -1,
            opt_tuple,
            parse_contexts,
            opts...);
        if (!impl_result)
            return impl_result;

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

        using namespace hana::literals;

        hana::fold(opt_tuple, 0_c, [&](auto i, auto const & opt) {
            auto const i_plus_1 = hana::llong_c<decltype(i)::value + 1>;
            using opt_type = std::remove_cvref_t<decltype(opt)>;
            if constexpr (!is_command<opt_type>::value) {
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
        auto const opt_tuple = detail::make_opt_tuple(opts...);

        auto first = args.begin();
        auto const last = args.end();

        // This dance is here to support the case where the values returned by
        // args are temporaries -- args may have an underlying proxy iterator.
        std::basic_string<Char> const argv0_str(first->begin(), first->end());
        std::basic_string_view<Char> argv0 = argv0_str;

        parse_contexts_vec const parse_contexts;
        detail::parse_options_into(
            [&](auto const & opt, auto i) -> decltype(auto) {
                return result[i];
            },
            next_positional,
            strings,
            false,
            argv0,
            first,
            last,
            true,
            program_desc,
            os,
            no_help,
            opt_tuple,
            parse_contexts,
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

    template<typename OptionsMap>
    void parse_into_map_cleanup(OptionsMap & map)
    {
        auto it = map.begin();
        auto prev_it = it;
        auto const last = map.end();
        while (it != last) {
            if (program_options_2::any_empty(it->second)) {
                prev_it = it;
                ++it;
                map.erase(prev_it);
            } else {
                ++it;
            }
        }
    }

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
        auto const opt_tuple = detail::make_opt_tuple(opts...);

        auto first = args.begin();
        auto const last = args.end();

        // This dance is here to support the case where the values returned by
        // args are temporaries -- args may have an underlying proxy iterator.
        std::basic_string<Char> const argv0_str(first->begin(), first->end());
        std::basic_string_view<Char> argv0 = argv0_str;

        parse_contexts_vec const parse_contexts;
        auto const retval = detail::parse_options_into(
            map_lookup<OptionsMap>(result),
            next_positional,
            strings,
            deserializing,
            argv0,
            first,
            last,
            skip_first,
            program_desc,
            os,
            no_help,
            opt_tuple,
            parse_contexts,
            opts...);
        detail::parse_into_map_cleanup(result);
        return retval;
    }

#define BOOST_PROGRAM_OPTIONS_2_INSTRUMENT_COMMAND_PARSING 1

    template<
        typename Char,
        typename ArgsIter,
        typename OptionsMap,
        typename... Options,
        typename... Options2>
    parse_option_result parse_commands_in_tuple(
        OptionsMap & map,
        customizable_strings const & strings,
        names_view help_names_view,
        std::basic_string_view<Char> argv0,
        ArgsIter & first,
        ArgsIter last,
        std::basic_string_view<Char> program_desc,
        std::basic_ostream<Char> & os,
        bool no_help,
        hana::tuple<Options...> const & opt_tuple,
        parse_contexts_vec & parse_contexts,
        std::function<void()> & func,
        Options2 const &... opts)
    {
        if (first == last) {
            // TODO: Report that a command is missing.
        }

#if BOOST_PROGRAM_OPTIONS_2_INSTRUMENT_COMMAND_PARSING
        std::cout << "parse_commands_in_tuple(): matching arg '" << *first
                  << "'" << std::endl;
#endif

        auto const & arg = *first;

        if (detail::matches_view(arg, help_names_view)) {
#if BOOST_PROGRAM_OPTIONS_2_INSTRUMENT_COMMAND_PARSING
            std::cout << "parse_commands_in_tuple(): "
                         "detail::print_help_for_command_and_exit()"
                      << std::endl;
#endif
            detail::print_help_and_exit(
                0,
                strings,
                argv0,
                program_desc,
                os,
                no_help,
                parse_contexts,
                opts...);
        }

        bool matched_command = false;
        parse_option_result child_result;
        hana::for_each(opt_tuple, [&]<typename Option>(Option const & opt) {
            if (matched_command)
                return;

            if constexpr (group_<Option>) {
                if constexpr (opt.subcommand) {
#if BOOST_PROGRAM_OPTIONS_2_INSTRUMENT_COMMAND_PARSING
                    std::cout << "parse_commands_in_tuple(): at subcommand '"
                              << opt.names << "'" << std::endl;
#endif
                    if (detail::matches_names(arg, opt.names)) {
                        matched_command = true;
                        ++first;
#if BOOST_PROGRAM_OPTIONS_2_INSTRUMENT_COMMAND_PARSING
                        std::cout << "parse_commands_in_tuple(): it's a match!"
                                  << std::endl;
#endif

                        auto options_tuple = hana::unpack(
                            opt.options, [](auto const &... opts2) {
                                return detail::make_opt_tuple(opts2...);
                            });
                        bool const has_subcommands =
                            (is_command<Options>::value || ...);
                        auto const utf_arg = text::as_utf8(arg);
                        parse_contexts.push_back(
                            {text::to_string(text::as_utf32(arg)),
                             [&,
                              argv0,
                              last,
                              program_desc,
                              no_help,
                              options_tuple](int & next_positional) {
                                 return detail::parse_options_into(
                                     map_lookup<OptionsMap>(map),
                                     next_positional,
                                     strings,
                                     false,
                                     argv0,
                                     first,
                                     last,
                                     false,
                                     program_desc,
                                     os,
                                     no_help,
                                     options_tuple,
                                     parse_contexts,
                                     opts...);
                             },
                             [&strings, options_tuple](
                                 std::ostringstream & oss,
                                 int first_column,
                                 int current_width) {
                                 hana::for_each(
                                     options_tuple, [&](auto const & opt) {
                                         current_width = detail::print_option(
                                             strings,
                                             oss,
                                             opt,
                                             first_column,
                                             current_width);
                                     });
                                 return current_width;
                             },
                             has_subcommands
                                 ? std::string(
                                       strings.next_subcommand_placeholder_text)
                                 : std::string(),
                             has_subcommands});
                        if (parse_contexts.size() == 2u) {
                            // To understand this, see the KLUDGE note in
                            // parse_commands() below.
                            parse_contexts[0].commands_synopsis_text_ += ' ';
                            parse_contexts[0].commands_synopsis_text_ +=
                                strings.next_subcommand_placeholder_text;
                            parse_contexts[0].has_subcommands_ = true;
                        }
#if BOOST_PROGRAM_OPTIONS_2_INSTRUMENT_COMMAND_PARSING
                        std::cout << "parse_commands_in_tuple(): "
                                     "parse_contexts.size()="
                                  << parse_contexts.size() << std::endl;
#endif
                        if constexpr (opt.has_func) {
                            func = [&map, f = opt.func]() { f(map); };
                        }
                        if (!func) {
                            auto opt_tuple = hana::unpack(
                                opt.options, [](auto const &... opts2) {
                                    return detail::make_opt_tuple(opts2...);
                                });
                            child_result = detail::parse_commands_in_tuple(
                                map,
                                strings,
                                help_names_view,
                                argv0,
                                first,
                                last,
                                program_desc,
                                os,
                                no_help,
                                opt_tuple,
                                parse_contexts,
                                func,
                                opts...);
                        }
                    }
#if BOOST_PROGRAM_OPTIONS_2_INSTRUMENT_COMMAND_PARSING
                    else
                        std::cout << "parse_commands_in_tuple(): no match"
                                  << std::endl;
#endif
                }
            }
        });

        if (!child_result)
            return child_result;

        if (!matched_command) {
            // TODO: Report that all commands must be given before all
            // other args.
        }

        if (!func) {
            // TODO: Report that a command is missing.
        }

        return {};
    }

    template<
        typename OptionsMap,
        typename Args,
        typename Char,
        typename... Options>
    parse_option_result parse_commands(
        OptionsMap & map,
        customizable_strings const & strings,
        Args const & args,
        std::basic_string_view<Char> program_desc,
        std::basic_ostream<Char> & os,
        bool skip_first,
        Options const &... opts)
    {
        auto opt_tuple = detail::make_opt_tuple(opts...);

        auto const default_help_names_view =
            names_view(strings.default_help_names);
        auto const opt_names_view = detail::help_option(opts...);
        auto const help_names_view =
            opt_names_view ? *opt_names_view : default_help_names_view;
        bool const no_help = !opt_names_view;

        auto first = args.begin();
        auto const last = args.end();

        // This dance is here to support the case where the values returned by
        // args are temporaries -- args may have an underlying proxy iterator.
        std::basic_string<Char> const argv0_str(first->begin(), first->end());
        std::basic_string_view<Char> argv0 = argv0_str;

        if (skip_first)
            ++first;

        std::function<void()> func;
        parse_contexts_vec parse_contexts;
        // This is the top-level context, outsided any commands.
        parse_contexts.push_back(
            {std::string{},
             [&, argv0, last, program_desc, no_help](int & next_positional) {
                 return detail::parse_options_into(
                     map_lookup<OptionsMap>(map),
                     next_positional,
                     strings,
                     false,
                     argv0,
                     first,
                     last,
                     false,
                     program_desc,
                     os,
                     no_help,
                     opt_tuple,
                     parse_contexts,
                     opts...);
             },
             [&strings, &opt_tuple](
                 std::ostringstream & oss,
                 int first_column,
                 int current_width) {
                 hana::for_each(opt_tuple, [&](auto const & opt) {
                     current_width = detail::print_option(
                         strings, oss, opt, first_column, current_width);
                 });
                 return current_width;
             },
             std::string(strings.top_subcommand_placeholder_text),
             // KLUDGE: This false is here to indicate that we have not yet
             // appended strings.next_subcommand_placeholder_text above.  If we
             // find ourselves deep enough in subcommands, we will, and we'll
             // also set this false to true.
             false});

        parse_option_result parse_result = detail::parse_commands_in_tuple(
            map,
            strings,
            help_names_view,
            argv0,
            first,
            last,
            program_desc,
            os,
            no_help,
            opt_tuple,
            parse_contexts,
            func,
            opts...);
        if (!parse_result)
            return parse_result;

        int next_positional = 0;
        for (auto const & ctx : parse_contexts) {
            parse_result = ctx.parse_(next_positional);
            if (!parse_result) {
                // TODO: Report error, if necessary.
                return parse_result;
            }
        }

        detail::parse_into_map_cleanup(map);
        if (parse_result)
            func();

        return parse_result;
    }

}}}

#endif
