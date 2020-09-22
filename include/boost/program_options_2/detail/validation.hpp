// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PROGRAM_OPTIONS_2_DETAIL_VALIDATION_HPP
#define BOOST_PROGRAM_OPTIONS_2_DETAIL_VALIDATION_HPP

#include <boost/program_options_2/fwd.hpp>
#include <boost/program_options_2/detail/printing.hpp>


namespace boost { namespace program_options_2 { namespace detail {

    template<
        detail::option_kind Kind,
        typename T,
        typename Value,
        detail::required_t Required,
        int Choices,
        typename ChoiceType,
        typename Validator>
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

    template<typename Char>
    validation_result validation_error(
        std::string_view error_str,
        std::basic_string_view<Char> sv,
        std::string & scratch)
    {
        std::ostringstream oss;
        auto const utf8_sv = text::as_utf8(sv);
        scratch.assign(utf8_sv.begin(), utf8_sv.end());
        detail::print_placeholder_string(
            oss, error_str, std::basic_string_view<Char>(scratch));
        scratch = oss.str();
        return {false, std::string_view(scratch)};
    }

}}}

#endif
