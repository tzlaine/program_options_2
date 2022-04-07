// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PROGRAM_OPTIONS_2_FWD_HPP
#define BOOST_PROGRAM_OPTIONS_2_FWD_HPP

#include <boost/program_options_2/config.hpp>
#include <boost/program_options_2/tag_invoke.hpp>

#include <boost/text/transcode_view.hpp>

#include <boost/container/small_vector.hpp>

#include <boost/any.hpp>

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

#include <any>
#include <array>
#include <map>
#include <string_view>
#include <type_traits>


namespace boost { namespace program_options_2 {

    /** The constant used to specify an option that has an optional following
        arguments. */
    inline constexpr int zero_or_one = -1;
    /** The constant used to specify an option that has zero or more following
        arguments. */
    inline constexpr int zero_or_more = -2;
    /** The constant used to specify an option that has one or more following
        arguments. */
    inline constexpr int one_or_more = -3;

    /** TODO */
    struct customizable_strings
    {
        std::string_view usage_text = "usage: ";
        std::string_view top_subcommand_placeholder_text = "COMMAND";
        std::string_view next_subcommand_placeholder_text = "SUB-COMMAND";
        std::string_view positional_section_text = "positional arguments:";
        std::string_view optional_section_text = "optional arguments:";
        std::string_view commands_section_text = "commands:";
        std::string_view default_help_names = "-h,--help";
        std::string_view help_description = "Print this help message and exit";
        std::string_view command_help_note =
            "\nUse '{} CMD {}' for help on command CMD.";
        std::string_view response_file_note =
            "response files:\n  Use '@file' to load a file containing "
            "command line arguments.";
        // TODO: use the epilog in printing.
        std::string_view epilog = "";

        std::string_view mutually_exclusive_begin = " (may not be used with '{}'";
        std::string_view mutually_exclusive_continue = ", '{}'";
        std::string_view mutually_exclusive_continue_final = " or '{}'";
        std::string_view mutually_exclusive_end = ")";

        std::string_view short_option_prefix = "-";
        std::string_view long_option_prefix = "--";
        std::string_view response_file_prefix = "@";

        std::array<std::string_view, 7> parse_errors = {
            {"error: unrecognized argument '{}'",
             "error: wrong number of arguments for '{}'",
             "error: cannot parse argument '{}'",
             "error: '{}' is not one of the allowed choices for '{}'",
             "error: unexpected positional argument '{}'",
             "error: one or more missing positional arguments, starting with "
             "'{}'",
             "error: '{}' may not be used with '{}'"}};

        // validation errors
        std::string_view path_not_found = "error: path '{}' not found";
        std::string_view file_not_found = "error: file '{}' not found";
        std::string_view directory_not_found =
            "error: directory '{}' not found";
        std::string_view found_file_not_directory =
            "error: '{}' is a file, but a directory was expected";
        std::string_view found_directory_not_file =
            "error: '{}' is a directory, but a file was expected";
        std::string_view cannot_read = "error: cannot open '{}' for reading";
    };

    /** The type that must be returned from any invocable that can be used as
        a validator. */
    struct validation_result
    {
        bool valid = true;
        std::string_view error;
    };

    /** Represents the absence of a type in numerous places in
        program_options_2. */
    struct no_value
    {};

    /** A `std::map` of `std::string`s to `std::any`s.  This is a type
        appropriate for parsing options into. */
    using string_any_map = std::map<std::string, std::any>;

    /** A `std::map` of `std::string`s to `std::any`s.  This is a type that
        may be appropriate for parsing options into.  Note that if you parse
        response files and/or parse UTF-16 command line arguments, this type
        will not work. */
    using string_view_any_map = std::map<std::string_view, std::any>;

    /** An invocable that returns true iff the given `any`.  It has built-in
        support for `boost::any` and `std::any`, and uses `tag_invoke` to
        allow users to customize its behavior for their own types. */
    inline constexpr struct any_empty_fn
    {
        template<typename Any>
        bool operator()(Any && a) const
        {
            if constexpr (program_options_2::tag_invocable<any_empty_fn, Any>) {
                return program_options_2::tag_invoke(*this, (Any &&) a);
            } else if constexpr (std::is_same_v<
                                     std::remove_cvref_t<Any>,
                                     boost::any>) {
                return a.empty();
            } else {
                return !a.has_value();
            }
        }
    } any_empty;

    /** An invocable that returns the result of an `any_cast` on the given
        `any`.  It has built-in support for `boost::any` and `std::any`, and
        uses `tag_invoke` to allow users to customize its behavior for their
        own types. */
    template<typename T>
    struct any_cast_fn
    {
        template<typename Any>
        decltype(auto) operator()(Any && a) const
        {
            if constexpr (program_options_2::
                              tag_invocable<any_cast_fn, Any, type_<T>>) {
                return program_options_2::tag_invoke(
                    *this, (Any &&) a, type_c<T>);
            } else if constexpr (std::is_same_v<
                                     std::remove_cvref_t<Any>,
                                     boost::any>) {
                return boost::any_cast<T>(a);
            } else {
                return std::any_cast<T>(a);
            }
        }
    };

    /** An inline variable for `any_cast_fn`. */
    template<typename T>
    inline constexpr any_cast_fn<T> any_cast;

    namespace detail {
        enum struct option_kind { positional, argument };

        enum struct required_t { yes, no };

        enum struct action_kind {
            none,
            assign,
            count,
            insert,
            help,
            version,
            response_file
        };

        template<
            option_kind Kind,
            typename T,
            typename Value = no_value,
            required_t Required = required_t::no,
            int Choices = 0,
            typename ChoiceType = T,
            typename Validator = no_value>
        struct option
        {
            using type = T;
            using value_type = Value;
            using choice_type = std::conditional_t<
                std::is_same_v<ChoiceType, void>,
                no_value,
                ChoiceType>;
            using validator_type = Validator;

            constexpr static bool positional = Kind == option_kind::positional;
            constexpr static bool required = Required == required_t::yes;
            constexpr static int num_choices = Choices;

            std::string_view names;
            std::string_view help_text;
            action_kind action;
            int args;
            value_type default_value;
            std::array<choice_type, num_choices> choices;
            std::string_view arg_display_name;
            mutable validator_type validator;
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
            typename ChoiceType,
            typename Validator>
        struct is_option<
            option<Kind, T, Value, Required, Choices, ChoiceType, Validator>>
            : std::true_type
        {};

        enum class exclusive_t { yes, no };
        enum class subcommand_t { yes, no };
        enum class named_group_t { yes, no };

        struct no_func
        {
            template<typename T>
            void operator()(T &&)
            {}
        };

        template<
            exclusive_t MutuallyExclusive,
            subcommand_t Subcommand,
            required_t Required,
            named_group_t NamedGroup,
            typename Func,
            typename... Options>
        struct option_group
        {
            using type = no_value;
            using value_type = no_value;
            using choice_type = no_value;
            using validator_type = no_value;
            using func_type = Func;

            std::string_view names;
            std::string_view help_text;
            hana::tuple<Options...> options;
            Func func;
            action_kind action = action_kind::none;

            constexpr static bool mutually_exclusive =
                MutuallyExclusive == exclusive_t::yes;
            constexpr static bool subcommand = Subcommand == subcommand_t::yes;
            constexpr static bool named_group =
                NamedGroup == named_group_t::yes;
            constexpr static bool positional = false;
            constexpr static bool required = Required == required_t::yes;
            constexpr static int num_choices = 0;
            constexpr static bool has_func =
                !std::is_same_v<func_type, no_func>;

            constexpr static bool flatten_during_printing =
                !mutually_exclusive && !subcommand && !named_group;
        };

        template<typename T>
        struct is_group : std::false_type
        {};
        template<
            exclusive_t MutuallyExclusive,
            subcommand_t Subcommand,
            required_t Required,
            named_group_t NamedGroup,
            typename Func,
            typename... Options>
        struct is_group<option_group<
            MutuallyExclusive,
            Subcommand,
            Required,
            NamedGroup,
            Func,
            Options...>> : std::true_type
        {};

        template<typename T>
        struct is_command : std::false_type
        {};
        template<typename... Options>
        struct is_command<option_group<
            exclusive_t::no,
            subcommand_t::yes,
            required_t::no,
            named_group_t::yes,
            Options...>> : std::true_type
        {};

        template<typename R1, typename R2>
        constexpr bool starts_with(R1 const & str, R2 const & substr)
        {
            auto const [_, substr_it] = std::ranges::mismatch(str, substr);
            return substr_it == std::end(substr);
        }

        template<typename R1, typename R2>
        constexpr bool transcoded_starts_with(R1 const & str, R2 const & substr)
        {
            return detail::starts_with(
                text::as_utf32(str), text::as_utf32(substr));
        }

        // Defined in utility.hpp.
        inline bool
        positional(std::string_view name, customizable_strings const & strings);

        inline bool
        long_(std::string_view name, customizable_strings const & strings)
        {
            return detail::transcoded_starts_with(
                name, strings.long_option_prefix);
        }
        inline bool
        short_(std::string_view name, customizable_strings const & strings)
        {
            return detail::transcoded_starts_with(
                       name, strings.short_option_prefix) &&
                   !detail::long_(name, strings);
        }

        inline bool
        leading_dash(std::string_view str, customizable_strings const & strings)
        {
            return detail::short_(str, strings) || detail::long_(str, strings);
        }

        template<
            option_kind Kind,
            typename T,
            typename Value,
            required_t Required,
            int Choices,
            typename ChoiceType,
            typename Validator>
        bool positional(
            option<
                Kind,
                T,
                Value,
                Required,
                Choices,
                ChoiceType,
                Validator> const & opt,
            customizable_strings const & strings)
        {
            return detail::positional(opt.names, strings);
        }

        template<
            exclusive_t MutuallyExclusive,
            subcommand_t Subcommand,
            required_t Required,
            named_group_t NamedGroup,
            typename Func,
            typename... Options>
        bool positional(
            option_group<
                MutuallyExclusive,
                Subcommand,
                Required,
                NamedGroup,
                Func,
                Options...> const &,
            customizable_strings const &)
        {
            return false;
        }

        template<typename Option>
        constexpr bool flag()
        {
            return std::is_same_v<
                Option,
                option<option_kind::argument, bool, bool, required_t::yes>>;
        }

        template<
            option_kind Kind,
            typename T,
            typename Value,
            required_t Required,
            int Choices,
            typename ChoiceType,
            typename Validator>
        bool optional_arg(option<
                          Kind,
                          T,
                          Value,
                          Required,
                          Choices,
                          ChoiceType,
                          Validator> const & opt)
        {
            return opt.args == zero_or_one || opt.args == zero_or_more;
        }

        template<
            option_kind Kind,
            typename T,
            typename Value,
            required_t Required,
            int Choices,
            typename ChoiceType,
            typename Validator>
        bool multi_arg(option<
                       Kind,
                       T,
                       Value,
                       Required,
                       Choices,
                       ChoiceType,
                       Validator> const & opt)
        {
            return opt.args == zero_or_more || opt.args == one_or_more;
        }

        enum struct parse_option_error {
            none,

            unknown_arg,
            wrong_number_of_args,
            cannot_parse_arg,
            no_such_choice,
            extra_positional,
            missing_positional,
            too_many_mutally_exclusives,

            // This one must come last, to match
            // customizable_strings::parse_errors.
            validation_error
        };

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

        struct printed_names_and_desc
        {
            printed_names_and_desc() = default;
            printed_names_and_desc(
                std::string printed_names,
                std::string desc,
                int estimated_width) :
                printed_names(std::move(printed_names)),
                desc(desc),
                estimated_width(estimated_width)
            {}
            std::string printed_names;
            std::string desc;
            int estimated_width;
        };

        using printed_section_vec =
            boost::container::small_vector<printed_names_and_desc, 32>;

        using all_printed_sections = boost::container::
            small_vector<std::pair<std::string, printed_section_vec>, 4>;

        struct cmd_parse_ctx
        {
            std::string name_used_;
            std::string_view help_text_;
            std::function<parse_option_result(int &)> parse_;
            std::function<int(std::ostringstream &, int, int)> print_synopsis_;
            std::function<void(bool, all_printed_sections &, int &, bool &)>
                print_post_synopsis_;
            std::string commands_synopsis_text_;
            bool has_subcommands_ = false;
        };

        using parse_contexts_vec =
            boost::container::small_vector<cmd_parse_ctx, 8>;

        constexpr inline int max_col_width = 80;
        constexpr inline int max_option_col_width = 24;

        template<typename Stream, typename Option>
        int print_option(
            customizable_strings const & strings,
            Stream & os,
            Option const & opt,
            int first_column,
            int current_width,
            int max_width = max_col_width,
            bool for_post_synopsis = false);

        template<
            typename Stream,
            exclusive_t MutuallyExclusive,
            subcommand_t Subcommand,
            required_t Required,
            named_group_t NamedGroup,
            typename Func,
            typename... Options>
        int print_option(
            customizable_strings const & strings,
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
            bool for_post_synopsis = false);
    }

    template<
        detail::option_kind Kind,
        typename T,
        typename Value,
        detail::required_t Required,
        int Choices,
        typename ChoiceType,
        typename Validator>
    std::string_view storage_name(
        detail::option<
            Kind,
            T,
            Value,
            Required,
            Choices,
            ChoiceType,
            Validator> const & opt,
        customizable_strings const & strings = customizable_strings{});

}}

#endif
