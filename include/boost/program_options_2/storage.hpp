// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PROGRAM_OPTIONS_2_STORAGE_HPP
#define BOOST_PROGRAM_OPTIONS_2_STORAGE_HPP

#include <boost/program_options_2/fwd.hpp>
#include <boost/program_options_2/detail/detection.hpp>
#include <boost/program_options_2/detail/parsing.hpp>
#include <boost/program_options_2/detail/utility.hpp>

#include <boost/throw_exception.hpp>

#include <exception>


namespace boost { namespace program_options_2 {

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
                                  Validator> const & opt)
    {
        auto const name =
            detail::first_name_prefer(opt.names, [](std::string_view sv) {
                return detail::positional(sv) || detail::long_(sv);
            });
        return detail::trim_leading_dashes(name);
    }

    /** TODO */
    enum struct save_result {
        success = 0,
        could_not_open_file_for_writing,
        bad_any_cast
    };

    /** TODO */
    enum struct load_result {
        success = 0,
        could_not_open_file_for_reading,
        unknown_arg = (int)detail::parse_option_error::unknown_arg,
        wrong_number_of_args =
            (int)detail::parse_option_error::wrong_number_of_args,
        cannot_parse_arg = (int)detail::parse_option_error::cannot_parse_arg,
        no_such_choice = (int)detail::parse_option_error::no_such_choice,
        validation_error = (int)detail::parse_option_error::validation_error
    };

    /** TODO */
    struct save_error : std::exception
    {
        save_error() = default;
        save_error(save_result error, std::string_view str) :
            error_(error), str_(str)
        {}
        virtual char const * what()
        {
            return "Failed to save command line arguments to storage.";
        }
        save_result error() const { return error_; }
        std::string_view str() const { return str_; }

    private:
        save_result error_;
        std::string_view str_;
    };

    /** TODO */
    struct load_error : std::exception
    {
        load_error() = default;
        load_error(load_result error, std::string_view str) :
            error_(error), str_(str)
        {}
        virtual char const * what()
        {
            return "Failed to load command line arguments from storage.";
        }
        load_result error() const { return error_; }
        std::string_view str() const { return str_; }

    private:
        load_result error_;
        std::string_view str_;
    };

    namespace detail {
        template<typename T>
        using streamable =
            decltype(std::declval<std::ofstream>() << std::declval<T>());
    }

    /** TODO */
    template<options_map OptionsMap, typename... Options>
    void save_response_file(
        std::string_view filename,
        OptionsMap const & m,
        Options const &... opts)
    {
        std::ofstream ofs(filename.data());
        if (!ofs) {
            BOOST_THROW_EXCEPTION(save_error(
                save_result::could_not_open_file_for_writing, filename));
        }

        auto const opt_tuple = detail::make_opt_tuple(opts...);
        hana::for_each(opt_tuple, [&](auto const & opt) {
            using opt_type = std::remove_cvref_t<decltype(opt)>;
            using type = typename opt_type::type;

            auto const name = program_options_2::storage_name(opt);
            auto it = m.find(name);
            if (it == m.end() || it->second.empty())
                return;

            try {
                type const & value =
                    program_options_2::any_cast<type const &>(it->second);
                ofs << detail::first_long_name(opt.names);
                if constexpr (insertable<type>) {
                    for (auto const & x : value) {
                        // To use save_response_file(), all options must have
                        // a type that can be written to file using
                        // operator<<().
                        static_assert(detail::is_detected<
                                      detail::streamable,
                                      decltype(x)>::value);
                        ofs << ' ' << x;
                    }
                } else {
                    // To use save_response_file(), all options must have a
                    // type that can be written to file using operator<<().
                    static_assert(
                        detail::is_detected<detail::streamable, type>::value);
                    ofs << ' ' << value;
                }
                ofs << '\n';
            } catch (...) {
                BOOST_THROW_EXCEPTION(
                    save_error(save_result::bad_any_cast, filename));
            }
        });
    }

    /** TODO */
    template<options_map OptionsMap, typename... Options>
    void load_response_file(
        std::string_view filename, OptionsMap & m, Options const &... opts)
    {
        std::ifstream ifs(filename.data());
        ifs.unsetf(ifs.skipws);
        if (!ifs) {
            BOOST_THROW_EXCEPTION(load_error(
                load_result::could_not_open_file_for_reading, filename));
        }

        std::ostringstream oss;
        auto const parse_result = detail::parse_options_into_map(
            m,
            customizable_strings{},
            true,
            detail::response_file_arg_view(ifs),
            std::string_view{},
            oss,
            false,
            false,
            opts...);

        if (!parse_result) {
            BOOST_THROW_EXCEPTION(
                load_error((load_result)parse_result.error, filename));
        }
    }

}}

#endif
