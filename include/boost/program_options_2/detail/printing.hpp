// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PROGRAM_OPTIONS_2_PRINTING_HPP
#define BOOST_PROGRAM_OPTIONS_2_PRINTING_HPP

#include <boost/program_options_2/fwd.hpp>
#include <boost/program_options_2/detail/utility.hpp>

#include <boost/parser/parser.hpp>
#include <boost/text/bidirectional.hpp>
#include <boost/text/case_mapping.hpp>
#include <boost/text/estimated_width.hpp>
#include <boost/text/utf.hpp>
#include <boost/text/transcode_view.hpp>

#include <climits>
#include <sstream>


namespace boost { namespace program_options_2 { namespace detail {

    template<typename Char>
    void print_placeholder_string(
        std::basic_ostream<Char> & os,
        std::basic_string_view<Char> placeholder_str,
        std::basic_string_view<Char> inserted_str1,
        std::basic_string_view<Char> inserted_str2,
        bool end_with_newline = true);

    template<typename... Options>
    auto
    make_opt_tuple_for_printing(hana::tuple<Options const &...> const & opts)
    {
        return detail::make_opt_tuple_impl<true>(opts);
    }

    template<typename... Options>
    auto make_opt_tuple_for_printing(Options const &... opts)
    {
        using opts_as_tuple_type = hana::tuple<Options const &...>;
        return detail::make_opt_tuple_for_printing(opts_as_tuple_type{opts...});
    }

    template<typename... Options>
    bool no_response_file_option(Options const &... opts);

    template<typename Option>
    bool no_response_file_option_impl(Option const & opt)
    {
        if constexpr (group_<Option>) {
            return hana::unpack(opt.options, [](auto const &... opts) {
                return detail::no_response_file_option(opts...);
            });
        } else {
            return opt.action != detail::action_kind::response_file;
        }
    }

    template<typename... Options>
    bool no_response_file_option(Options const &... opts)
    {
        return (detail::no_response_file_option_impl(opts) && ...);
    }

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

    inline option<option_kind::argument, void>
    default_help(customizable_strings const & strings)
    {
        return {
            strings.help_names, strings.help_description, action_kind::help, 0};
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
            std::declval<Stream &>() << std::declval<T>())>> : std::true_type
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
        bool const args_optional = detail::optional_arg(opt) && !opt.positional;
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
                              typename Option::choice_type const &>::value) {
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

#if 0
    template<
        typename Stream,
        typename Char,
        exclusive_t MutuallyExclusive,
        subcommand_t Subcommand,
        required_t Required,
        named_group_t NamedGroup,
        typename Func,
        typename... Options>
    void print_args(
        Stream & os,
        std::basic_string_view<Char> name,
        option_group<
            MutuallyExclusive,
            Subcommand,
            Required,
            NamedGroup,
            Func,
            Options...> const & group,
        bool print_leading_space)
    {
        // TODO
    }
#endif

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

        if ((!opt.required || detail::flag<Option>()) && !brief)
            oss << '[';

        if (brief) {
            // This, plus the leading space in the first name printed
            // below, gives us a 2-space indent.
            oss << ' ';
            if (opt.positional && !opt.arg_display_name.empty()) {
                oss << text::as_utf8(opt.arg_display_name);
            } else {
                bool print_separator = false;
                for (auto name : detail::names_view(opt.names)) {
                    if (print_separator)
                        oss << ',' << ' ';
                    print_separator = true;
                    oss << text::as_utf8(name);
                }
            }
        } else if (detail::positional(opt)) {
            detail::print_args(oss, opt.names, opt, false);
        } else if (opt.action == action_kind::count) {
            auto const shortest_name = detail::first_short_name(opt.names);
            auto const trimmed_name =
                detail::trim_leading_dashes(shortest_name);
            char const * close = "...]";
            oss << text::as_utf8(shortest_name) << '['
                << text::as_utf8(trimmed_name) << text::as_utf8(close);
        } else {
            auto const shortest_name = detail::first_short_name(opt.names);
            oss << text::as_utf8(shortest_name);
            detail::print_args(
                oss, detail::trim_leading_dashes(shortest_name), opt, true);
        }

        if ((!opt.required || detail::flag<Option>()) && !brief)
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
        typename Stream,
        exclusive_t MutuallyExclusive,
        subcommand_t Subcommand,
        required_t Required,
        named_group_t NamedGroup,
        typename Func,
        typename... Options>
    int print_option(
        Stream & os,
        option_group<
            MutuallyExclusive,
            Subcommand,
            Required,
            NamedGroup,
            Func,
            Options...> const & opt,
        int first_column,
        int current_width,
        int max_width = max_col_width,
        bool brief = false)
    {
        if constexpr (opt.subcommand) {
            // TODO
        } else if constexpr (opt.mutually_exclusive) {
            hana::for_each(opt.options, [&](auto const & opt) {
                current_width =
                    detail::print_option(os, opt, first_column, current_width);
            });
        } else { // named group
            hana::for_each(opt.options, [&](auto const & opt) {
                current_width =
                    detail::print_option(os, opt, first_column, current_width);
            });
        }
        return current_width;
    }

    template<typename Stream, typename Char, typename... Options>
    void print_help_synopsis(
        customizable_strings const & strings,
        Stream & os,
        std::basic_string_view<Char> prog,
        std::basic_string_view<Char> prog_desc,
        Options const &... opts)
    {
        auto const opt_tuple = detail::make_opt_tuple_for_printing(opts...);

        os << text::as_utf8(strings.usage_text) << ' ' << text::as_utf8(prog);

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

        // TODO: When there are one or more subcommands in use, print
        // all the non-subcommand options, as above, but then end with
        // "COMMAND [COMMAND-ARGS]".  That string should of course be
        // user-configurable.
    }

    struct printed_names_and_desc
    {
        printed_names_and_desc() = default;
        printed_names_and_desc(
            std::string printed_names, std::string desc, int estimated_width) :
            printed_names(std::move(printed_names)),
            desc(desc),
            estimated_width(estimated_width)
        {}
        std::string printed_names;
        std::string desc;
        int estimated_width;
    };

    // This is the minimum allowed gap between the options and their
    // descriptions.
    constexpr int min_help_column_gap = 2;

    template<typename Stream, typename... Options>
    void print_wrapped_column(Stream & os, std::string_view str, int min_column)
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
            int const needed_spacing = name_and_desc.estimated_width <= limit
                                           ? limit -
                                                 name_and_desc.estimated_width +
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

    template<typename Stream, typename... Options>
    void print_help_post_synopsis(
        customizable_strings const & strings,
        Stream & os,
        Options const &... opts)
    {
        auto const opt_tuple = detail::make_opt_tuple_for_printing(opts...);

        int max_option_length = 0;
        std::vector<std::pair<std::string, std::vector<printed_names_and_desc>>>
            printed_sections(2);
        hana::for_each(opt_tuple, [&](auto const & opt) {
            auto process_single_opt = [&](auto const & curr_opt,
                                          auto const & parent_opt,
                                          int child_index,
                                          int first_column) {
                constexpr bool no_parent = std::is_same_v<
                    std::remove_cvref_t<decltype(parent_opt)>,
                    no_value>;

                std::vector<printed_names_and_desc> * vec_ptr =
                    detail::positional(curr_opt) ? &printed_sections[0].second
                                                 : &printed_sections[1].second;
                if constexpr (!no_parent) {
                    if constexpr (parent_opt.named_group) {
                        vec_ptr = &printed_sections.back().second;
                    }
                }
                std::vector<printed_names_and_desc> & vec = *vec_ptr;

                std::ostringstream names_oss;
                detail::print_option(
                    names_oss, curr_opt, first_column, 0, INT_MAX, true);
                std::ostringstream desc_oss;
                desc_oss << curr_opt.help_text;

                // Special case: Print an option from an exclusive group.
                if constexpr (!no_parent) {
                    if constexpr (
                        parent_opt.mutually_exclusive &&
                        !parent_opt.subcommand) {
                        using namespace hana::literals;
                        bool first = true;
                        unsigned num_printed = 0;
                        hana::fold(
                            parent_opt.options,
                            0_c,
                            [&](auto i, auto const & opt) {
                                auto const i_plus_1 =
                                    hana::llong_c<decltype(i)::value + 1>;
                                if (i == child_index)
                                    return i_plus_1;
                                if (first) {
                                    detail::print_placeholder_string(
                                        desc_oss,
                                        strings.mutually_exclusive_begin,
                                        program_options_2::storage_name(opt),
                                        {},
                                        false);
                                    first = false;
                                } else if (
                                    num_printed + 2 ==
                                    hana::size(parent_opt.options)) {
                                    detail::print_placeholder_string(
                                        desc_oss,
                                        strings
                                            .mutually_exclusive_continue_final,
                                        program_options_2::storage_name(opt),
                                        {},
                                        false);
                                } else {
                                    detail::print_placeholder_string(
                                        desc_oss,
                                        strings.mutually_exclusive_continue,
                                        program_options_2::storage_name(opt),
                                        {},
                                        false);
                                }
                                ++num_printed;
                                return i_plus_1;
                            });
                        desc_oss << strings.mutually_exclusive_end;
                    }
                }

                vec.emplace_back(
                    std::move(names_oss).str(), std::move(desc_oss).str(), 0);
                auto const opt_width =
                    detail::estimated_width(vec.back().printed_names);
                vec.back().estimated_width = opt_width;
                max_option_length = (std::max)(max_option_length, opt_width);
            };

            if constexpr (group_<std::remove_cvref_t<decltype(opt)>>) {
                if constexpr (opt.mutually_exclusive && !opt.subcommand) {
                    auto const exclusive_opts =
                        detail::make_opt_tuple_for_printing(
                            detail::to_ref_tuple(opt.options));
                    auto const sub_opts = hana::size(opt.options);
                    auto curr_opt_index = 0;
                    hana::for_each(exclusive_opts, [&](auto const & curr_opt) {
                        process_single_opt(curr_opt, opt, curr_opt_index, 0);
                        ++curr_opt_index;
                    });
                } else if constexpr (opt.subcommand) {
                    // TODO
                } else { // named group
                    std::ostringstream oss;
                    oss << opt.names << ":";
                    if (!opt.help_text.empty()) {
                        oss << "\n  " << opt.help_text << '\n';
                    }
                    printed_sections.emplace_back(
                        std::move(oss).str(),
                        std::vector<printed_names_and_desc>{});
                    auto const group_opts = detail::make_opt_tuple_for_printing(
                        detail::to_ref_tuple(opt.options));
                    hana::for_each(group_opts, [&](auto const & curr_opt) {
                        process_single_opt(curr_opt, opt, 0, 2);
                    });
                }
            } else {
                process_single_opt(opt, no_value{}, 0, 0);
            }
        });

        // max_option_length includes a 2-space initial sequence, which
        // acts as an indent.
        int const description_column = (std::min)(
            max_option_length + min_help_column_gap, max_option_col_width);

        std::vector<printed_names_and_desc> & printed_positionals =
            printed_sections[0].second;
        if (!printed_positionals.empty()) {
            os << '\n'
               << text::as_utf8(strings.positional_section_text) << '\n';
            detail::print_options_and_descs(
                os, printed_positionals, description_column);
        }

        std::vector<printed_names_and_desc> & printed_arguments =
            printed_sections[1].second;
        if (!printed_arguments.empty()) {
            os << '\n' << text::as_utf8(strings.optional_section_text) << '\n';
            detail::print_options_and_descs(
                os, printed_arguments, description_column);
        }

        for (auto const & [name_and_desc, printed_opts] :
             std::ranges::drop_view{printed_sections, 2}) {
            os << '\n' << name_and_desc << '\n';
            detail::print_options_and_descs(
                os, printed_opts, description_column);
        }

        if (!strings.response_file_description.empty() &&
            detail::no_response_file_option(opts...)) {
            os << '\n'
               << text::as_utf8(strings.response_file_description) << '\n';
        }
    }

    template<typename Char, typename... Options>
    void print_help(
        customizable_strings const & strings,
        std::basic_ostream<Char> & os,
        std::basic_string_view<Char> argv0,
        std::basic_string_view<Char> desc,
        Options const &... opts)
    {
        std::basic_ostringstream<Char> oss;
        print_help_synopsis(
            strings, oss, detail::program_name(argv0), desc, opts...);
        print_help_post_synopsis(strings, oss, opts...);
        auto const str = std::move(oss).str();
        for (auto const & range :
             text::bidirectional_subranges(text::as_utf32(str))) {
            os << text::as_utf32(range);
        }
    }

    template<typename Char, typename... Options>
    void print_help_and_exit(
        int exit_code,
        customizable_strings const & strings,
        std::basic_string_view<Char> argv0,
        std::basic_string_view<Char> program_desc,
        std::basic_ostream<Char> & os,
        bool no_help,
        Options... opts)
    {
        if (no_help) {
            detail::print_help(
                strings,
                os,
                argv0,
                program_desc,
                detail::default_help(strings),
                opts...);
        } else {
            detail::print_help(strings, os, argv0, program_desc, opts...);
        }
#ifdef BOOST_PROGRAM_OPTIONS_2_TESTING
        throw 0;
#endif
        std::exit(exit_code);
    }

    template<typename Char>
    void print_placeholder_string(
        std::basic_ostream<Char> & os,
        std::basic_string_view<Char> placeholder_str,
        std::basic_string_view<Char> inserted_str1,
        std::basic_string_view<Char> inserted_str2,
        bool end_with_newline)
    {
        using namespace parser::literals;
        auto const before_brace = parser::omit[*(parser::char_ - '{')];
        auto const brace = parser::raw
            [('{'_l - "{{") >> *(parser::char_ - '}') >> ('}'_l - "}}")];
        auto const kinda_matched_braces =
            before_brace >> brace >> -(before_brace >> brace);
        auto first = placeholder_str.begin();
        auto const braces =
            parser::parse(first, placeholder_str.end(), kinda_matched_braces);
        auto const open_brace =
            braces ? (*braces)[0].begin() : placeholder_str.end();
        auto const close_brace =
            braces ? (*braces)[0].end() : placeholder_str.end();
        auto const second_open_brace = braces && 1u < braces->size()
                                           ? (*braces)[1].begin()
                                           : placeholder_str.end();
        auto const second_close_brace = braces && 1u < braces->size()
                                            ? (*braces)[1].end()
                                            : placeholder_str.end();

        os << text::as_utf8(placeholder_str.begin(), open_brace);
        if (braces)
            os << text::as_utf8(inserted_str1);
        os << text::as_utf8(close_brace, second_open_brace);
        if (braces && 1u < braces->size())
            os << text::as_utf8(inserted_str2);
        os << text::as_utf8(second_close_brace, placeholder_str.end());
        if (end_with_newline)
            os << '\n';
    }

}}}

#endif
