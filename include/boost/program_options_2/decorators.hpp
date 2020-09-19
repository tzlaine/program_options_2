// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PROGRAM_OPTIONS_2_DECORATORS_HPP
#define BOOST_PROGRAM_OPTIONS_2_DECORATORS_HPP

#include <boost/program_options_2/fwd.hpp>
#include <boost/program_options_2/options.hpp>
#include <boost/program_options_2/concepts.hpp>


namespace boost { namespace program_options_2 {

    /** TODO */
    template<
        detail::option_kind Kind,
        typename T,
        detail::required_t Required,
        int Choices,
        typename ChoiceType,
        typename Validator,
        typename DefaultType>
        // clang-format off
        requires
            (Choices == 0 ||
             std::equality_comparable_with<DefaultType, ChoiceType>) &&
             ((std::assignable_from<T &, DefaultType>  &&
               std::constructible_from<T, DefaultType>)||
              detail::insertable_from<T, DefaultType>)
    detail::option<Kind, T, DefaultType, Required, Choices, ChoiceType, Validator>
    with_default(
        detail::option<
            Kind,
            T,
            no_value,
            Required,
            Choices,
            ChoiceType,
            Validator> opt,
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
            opt.arg_display_name,
            std::move(opt.validator)};
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
    auto with_display_name(
        detail::option<Kind, T, Value, Required, Choices, ChoiceType, Validator>
            opt,
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

    /** TODO */
    template<
        detail::option_kind Kind,
        typename T,
        typename Value,
        detail::required_t Required,
        int Choices,
        typename ChoiceType,
        validator<T> Validator>
    detail::option<Kind, T, Value, Required, Choices, ChoiceType, Validator>
    with_validator(
        detail::option<Kind, T, Value, Required, Choices, ChoiceType, no_value>
            opt,
        Validator validator)
    {
        return {
            opt.names,
            opt.help_text,
            opt.action,
            opt.args,
            std::move(opt.default_value),
            opt.choices,
            opt.arg_display_name,
            std::move(validator)};
    }

    /** TODO */
    template<
        detail::option_kind Kind,
        typename T,
        typename Value,
        detail::required_t Required,
        int Choices,
        typename ChoiceType>
    auto path(
        detail::option<Kind, T, Value, Required, Choices, ChoiceType, no_value>
            opt,
        customizable_strings const & strings = customizable_strings{})
    {
        auto error_str = strings.path_not_found;
        auto f = [error_str](auto sv) {
#if BOOST_PROGRAM_OPTIONS_2_USE_STD_FILESYSTEM
            namespace fs = std::filesystem;
#else
            namespace fs = filesystem;
#endif
            fs::path p(sv);
            detail::error_code ec;
            auto const status = fs::status(p, ec);
            if (ec || !fs::exists(status) || status.type() != fs::status_error)
                return validation_result{false, error_str};
            return validation_result{};
        };
        return program_options_2::with_validator(opt, f);
    }

    /** TODO */
    template<
        detail::option_kind Kind,
        typename T,
        typename Value,
        detail::required_t Required,
        int Choices,
        typename ChoiceType>
    auto file(
        detail::option<Kind, T, Value, Required, Choices, ChoiceType, no_value>
            opt,
        customizable_strings const & strings = customizable_strings{})
    {
        auto not_found_str = strings.file_not_found;
        auto not_a_file_str = strings.file_not_found;
        auto f = [not_found_str, not_a_file_str](auto sv) {
#if BOOST_PROGRAM_OPTIONS_2_USE_STD_FILESYSTEM
            namespace fs = std::filesystem;
#else
            namespace fs = filesystem;
#endif
            fs::path p(sv);
            detail::error_code ec;
            auto const status = fs::status(p, ec);
            if (ec || !fs::exists(status) || status.type() != fs::status_error)
                return validation_result{false, not_found_str};
            if (fs::is_directory(status))
                return validation_result{false, not_a_file_str};
            return validation_result{};
        };
        return program_options_2::with_validator(opt, f);
    }

    /** TODO */
    template<
        detail::option_kind Kind,
        typename T,
        typename Value,
        detail::required_t Required,
        int Choices,
        typename ChoiceType>
    auto directory(
        detail::option<Kind, T, Value, Required, Choices, ChoiceType, no_value>
            opt,
        customizable_strings const & strings = customizable_strings{})
    {
        auto not_found_str = strings.file_not_found;
        auto not_a_dir_str = strings.file_not_found;
        auto f = [not_found_str, not_a_dir_str](auto sv) {
#if BOOST_PROGRAM_OPTIONS_2_USE_STD_FILESYSTEM
            namespace fs = std::filesystem;
#else
            namespace fs = filesystem;
#endif
            fs::path p(sv);
            detail::error_code ec;
            auto const status = fs::status(p, ec);
            if (ec || !fs::exists(status) || status.type() != fs::status_error)
                return validation_result{false, not_found_str};
            if (!fs::is_directory(status))
                return validation_result{false, not_a_dir_str};
            return validation_result{};
        };
        return program_options_2::with_validator(opt, f);
    }

}}

#endif
