// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PROGRAM_OPTIONS_2_FWD_HPP
#define BOOST_PROGRAM_OPTIONS_2_FWD_HPP

#include <boost/program_options_2/config.hpp>
#include <boost/program_options_2/tag_invoke.hpp>

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

    /** TODO */
    inline constexpr int zero_or_one = -1;
    /** TODO */
    inline constexpr int zero_or_more = -2;
    /** TODO */
    inline constexpr int one_or_more = -3;

    // TODO: Allow replacement of '-' , '@', ',', etc.?

    /** TODO */
    struct customizable_strings
    {
        std::string_view usage_text = "usage: ";
        std::string_view positional_section_text = "positional arguments:";
        std::string_view optional_section_text = "optional arguments:";
        std::string_view help_names = "-h,--help";
        std::string_view help_description = "Print this help message and exit";
        std::string_view response_file_description =
            "response files:\n  Write '@file' to load a file containing "
            "command line arguments.";

        std::array<std::string_view, 6> parse_errors = {
            {"error: unrecognized argument '{}'",
             "error: wrong number of arguments for '{}'",
             "error: cannot parse argument '{}'",
             "error: '{}' is not one of the allowed choices for '{}'",
             "error: unexpected positional argument '{}'",
             "error: one or more missing positional arguments, starting with "
             "'{}'"}};

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

    /** TODO */
    struct validation_result
    {
        bool valid = true;
        std::string_view error;
    };

    /** TODO */
    struct no_value
    {};

    /** TODO */
    using string_any_map = std::map<std::string, std::any>;

    /** TODO */
    using string_view_any_map = std::map<std::string_view, std::any>;

    /** TODO */
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

    /** TODO */
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

    /** TODO */
    template<typename T>
    inline constexpr any_cast_fn<T> any_cast;

    namespace detail {
        enum struct option_kind { positional, argument };

        enum struct required_t { yes, no };

        enum struct action_kind {
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

        template<
            exclusive_t MutuallyExclusive,
            subcommand_t Subcommand,
            typename... Options>
        struct option_group
        {
            // name is only nonempty when this is a group gated by some verb,
            // e.g. "push" in "git push arg arg".
            std::string_view names;
            std::string_view help_text;
            hana::tuple<Options...> options;

            constexpr static bool mutually_exclusive =
                MutuallyExclusive == exclusive_t::yes;
            constexpr static bool subcommand = Subcommand == subcommand_t::yes;
        };

        template<typename T>
        struct is_group : std::false_type
        {};
        template<
            exclusive_t MutuallyExclusive,
            subcommand_t Subcommand,
            typename... Options>
        struct is_group<option_group<MutuallyExclusive, Subcommand, Options...>>
            : std::true_type
        {};

        template<typename T>
        struct is_command : std::false_type
        {};
        template<typename... Options>
        struct is_command<
            option_group<exclusive_t::no, subcommand_t::yes, Options...>>
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

        inline bool leading_dash(std::string_view str)
        {
            return !str.empty() && str[0] == '-';
        }

        template<
            option_kind Kind,
            typename T,
            typename Value,
            required_t Required,
            int Choices,
            typename ChoiceType,
            typename Validator>
        bool positional(option<
                        Kind,
                        T,
                        Value,
                        Required,
                        Choices,
                        ChoiceType,
                        Validator> const & opt)
        {
            return detail::positional(opt.names);
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
    }

    /** TODO */
    template<
        detail::option_kind Kind,
        typename T,
        typename Value,
        detail::required_t Required,
        int Choices,
        typename ChoiceType,
        typename Validator>
    std::string_view storage_name(detail::option<
                                  Kind,
                                  T,
                                  Value,
                                  Required,
                                  Choices,
                                  ChoiceType,
                                  Validator> const & opt);

}}

#endif
