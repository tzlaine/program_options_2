// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PROGRAM_OPTIONS_2_STORAGE_HPP
#define BOOST_PROGRAM_OPTIONS_2_STORAGE_HPP

#include <boost/program_options_2/fwd.hpp>
#include <boost/program_options_2/detail/parsing.hpp>
#include <boost/program_options_2/detail/utility.hpp>

#include <boost/throw_exception.hpp>
#include <boost/type_traits/is_detected.hpp>

#include <exception>


namespace boost { namespace program_options_2 {

    /** Returns the name used for a value associated with the given non-group
        option when storing the value in a map. */
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

    /** The kinds of results that can occur when saving options. */
    enum struct save_result {
        /** The save was successful. */
        success = 0,

        /** The save failed because the output file could not be opened for
            writing. */
        could_not_open_file_for_writing,

        /** The save failed because an `any_cast` failed on one of the map
            elements being saved. */
        bad_any_cast
    };

    /** The kinds of results that can occur when loading options. */
    enum struct load_result {
        /** The load was successful. */
        success = 0,

        /** The load failed because the input file could not be opened for
            reading. */
        could_not_open_file_for_reading,

        /** An argument was loaded, for which no associated option could be
            found. */
        unknown_arg = (int)detail::parse_option_error::unknown_arg,

        /** There were too many or too few values given for the associated
            option. */
        wrong_number_of_args =
            (int)detail::parse_option_error::wrong_number_of_args,

        /** At least one value could not be parsed as the type specified in
            its associated option. */
        cannot_parse_arg = (int)detail::parse_option_error::cannot_parse_arg,

        /** A value for at least one value that was loaded was not one of the
            choices specified for its associated option. */
        no_such_choice = (int)detail::parse_option_error::no_such_choice,

        /** At least one value that was loaded failed a validation check for
            its associated option. */
        validation_error = (int)detail::parse_option_error::validation_error,

        /** Some of the input was JSON, and the JSON could not be parsed. */
        malformed_json
    };

    /** The exception type thrown when an options-saving function fails. */
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

    /** The exception type thrown when an options-loading function fails. */
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
        std::string const & str() const { return str_; }

    private:
        load_result error_;
        std::string str_;
    };

    namespace detail {
        template<typename T>
        using streamable =
            decltype(std::declval<std::ofstream>() << std::declval<T>());
        template<typename T>
        using quotable = decltype(std::quoted(std::declval<T>()));
    }

    /** Saves the options in `m`, expecting to find the options in `opts`,
        writing to file `filename`.

        \throws `save_error` on failure */
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

        ofs << std::boolalpha;

        detail::map_lookup<OptionsMap const> lookup(m);
        auto const opt_tuple = detail::make_opt_tuple(opts...);
        hana::for_each(opt_tuple, [&](auto const & opt) {
            using opt_type = std::remove_cvref_t<decltype(opt)>;
            using type = typename opt_type::type;

            auto it = lookup.find(opt);
            if (it == m.end() || any_empty(it->second))
                return;

            try {
                type const & value =
                    program_options_2::any_cast<type const &>(it->second);
                if (!detail::positional(opt))
                    ofs << detail::first_long_name(opt.names) << ' ';
                if constexpr (insertable<type>) {
                    bool first = true;
                    for (auto const & x : value) {
                        static_assert(
                            boost::is_detected<
                                detail::streamable,
                                decltype(x)>::value,
                            "To use save_response_file(), all options must "
                            "have a type that can be written to file using "
                            "operator<<().");
                        if (!first)
                            ofs << ' ';
                        first = false;
                        if constexpr (boost::is_detected<
                                          detail::quotable,
                                          decltype(x)>::value) {
                            ofs << std::quoted(x);
                        } else {
                            ofs << x;
                        }
                    }
                } else {
                    static_assert(
                        boost::is_detected<detail::streamable, type>::value,
                        "To use save_response_file(), all options must have a "
                        "type that can be written to file using operator<<().");
                    ofs << value;
                }
                ofs << '\n';
            } catch (...) {
                BOOST_THROW_EXCEPTION(
                    save_error(save_result::bad_any_cast, filename));
            }
        });
    }

    /** Loads the options in file `filename`, expecting to find the options in
        `opts`, and putting the results into `m`.

        \throws `load_error` on failure */
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

    namespace detail {
        inline parser::rule<class ws> const ws = "whitespace";
        inline parser::rule<class single_escaped_char, uint32_t> const
            single_escaped_char = "'\"' or '\\'";
        inline parser::rule<class string_char, uint32_t> const string_char =
            "code point (code points must be > U+001F)";
        inline parser::callback_rule<class string_tag, std::string_view> const
            string = "string";
        inline parser::callback_rule<
            class object_element_key_tag,
            std::string_view> const object_element_key = "string";
        inline parser::rule<class object_element_tag> const object_element =
            "object-element";

        inline parser::callback_rule<class object_open_tag> const object_open =
            "'{'";
        inline parser::callback_rule<class object_close_tag> const
            object_close = "'}'";
        inline parser::rule<class object_tag> const object = "object";

        inline parser::callback_rule<class array_open_tag> const array_open =
            "'['";
        inline parser::callback_rule<class array_close_tag> const array_close =
            "']'";
        inline parser::rule<class array_tag> const array = "array";

        inline parser::rule<class value_tag> const value = "value";


        class string_tag
        {};
        class object_element_key_tag
        {};
        class object_open_tag
        {};
        class object_close_tag
        {};
        class array_open_tag
        {};
        class array_close_tag
        {};


        inline auto const ws_def =
            parser::lit('\x09') | '\x0a' | '\x0d' | '\x20';

        inline auto const
            single_escaped_char_def = '\"' >> parser::attr(0x0022u) |
                                      '\\' >> parser::attr(0x005cu);

        inline auto const string_char_def =
            ('\\' > single_escaped_char) |
            (parser::cp - parser::char_(0x0000u, 0x001fu));

        struct json_string_view_action
        {
            template<typename Context>
            void operator()(Context & ctx) const
            {
                auto const where = _where(ctx);
                auto const first = ++where.begin();
                auto const last = --where.end();
                _val(ctx) =
                    std::string_view{&*first, std::size_t(last - first)};
            }
        };

        inline auto const string_def =
            parser::raw[parser::lexeme['"' >> *(string_char - '"') > '"']]
                       [json_string_view_action{}];

        inline auto const object_element_key_def = string_def;

        inline auto const object_element_def = object_element_key > ':' > value;

        inline auto const object_open_def = parser::lit('{');
        inline auto const object_close_def = parser::lit('}');
        inline auto const object_def = object_open >>
                                       -(object_element % ',') > object_close;

        inline auto const array_open_def = parser::lit('[');
        inline auto const array_close_def = parser::lit(']');
        inline auto const array_def = array_open >>
                                      -(value % ',') > array_close;

        inline auto const value_def = string | array | object;

        inline auto const skipper =
            parser::ws |
            ('#' >> *(parser::char_ - parser::eol) >> -parser::eol);

        BOOST_PARSER_DEFINE_RULES(
            ws,
            single_escaped_char,
            string_char,
            string,
            object_element_key,
            object_element,
            object_open,
            object_close,
            object,
            array_open,
            array_close,
            array,
            value);

        struct json_callbacks
        {
            json_callbacks(std::vector<std::string_view> & result) :
                result_(result)
            {}

            void operator()(string_tag, std::string_view s) const
            {
                result_.push_back(s);
            }
            void operator()(object_element_key_tag, std::string_view key) const
            {
                result_.push_back(key);
            }
            void operator()(object_open_tag) const {}
            void operator()(object_close_tag) const {}
            void operator()(array_open_tag) const {}
            void operator()(array_close_tag) const {}

        private:
            std::vector<std::string_view> & result_;
        };

        inline std::string file_slurp(std::ifstream & ifs)
        {
            std::string retval;
            while (ifs) {
                char const c = ifs.get();
                retval += c;
            }
            if (!retval.empty() && retval.back() == -1)
                retval.pop_back();
            return retval;
        }
    }

    /** Saves the options in `m`, expecting to find the options in `opts`,
        writing JSON-formatted output to file `filename`.

        \throws `save_error` on failure */
    template<options_map OptionsMap, typename... Options>
    void save_json_file(
        std::string_view filename,
        OptionsMap const & m,
        Options const &... opts)
    {
        std::ofstream ofs(filename.data());
        if (!ofs) {
            BOOST_THROW_EXCEPTION(save_error(
                save_result::could_not_open_file_for_writing, filename));
        }

        ofs << std::boolalpha;
        ofs << "{\n";

        detail::map_lookup<OptionsMap const> lookup(m);
        auto const opt_tuple = detail::make_opt_tuple(opts...);
        bool first_opt = true;
        hana::for_each(opt_tuple, [&](auto const & opt) {
            using opt_type = std::remove_cvref_t<decltype(opt)>;
            using type = typename opt_type::type;

            auto it = lookup.find(opt);
            if (it == m.end() || any_empty(it->second))
                return;

            try {
                type const & value =
                    program_options_2::any_cast<type const &>(it->second);
                if (!first_opt)
                    ofs << ",\n";
                first_opt = false;
                ofs << "    \"" << detail::first_long_name(opt.names) << "\":";
                if constexpr (insertable<type>) {
                    bool first_arg = true;
                    ofs << " [";
                    for (auto const & x : value) {
                        static_assert(
                            boost::is_detected<
                                detail::streamable,
                                decltype(x)>::value,
                            "To use save_json_file(), all options must have a "
                            "type that can be written to file using "
                            "operator<<().");
                        if (!first_arg)
                            ofs << ',';
                        first_arg = false;
                        ofs << " \"" << x << '"';
                    }
                    ofs << " ]";
                } else {
                    static_assert(
                        boost::is_detected<detail::streamable, type>::value,
                        "To use save_json_file(), all options must have a type "
                        "that can be written to file using operator<<().");
                    ofs << " \"" << value << '"';
                }
            } catch (...) {
                BOOST_THROW_EXCEPTION(
                    save_error(save_result::bad_any_cast, filename));
            }
        });

        ofs << "\n}\n";
    }

    /** Loads the options in the JSON-formatted file `filename`, expecting to
        find the options in `opts`, and putting the results into `m`.

        \throws `load_error` on failure */
    template<options_map OptionsMap, typename... Options>
    void load_json_file(
        std::string_view filename, OptionsMap & m, Options const &... opts)
    {
        std::ifstream ifs(filename.data());
        if (!ifs) {
            BOOST_THROW_EXCEPTION(save_error(
                save_result::could_not_open_file_for_writing, filename));
        }

        std::string error_message;
        auto record_error = [&](std::string const & error) {
            error_message = error;
        };
        parser::callback_error_handler error_callbacks(
            record_error, record_error, filename);

        std::vector<std::string_view> args;
        detail::json_callbacks callbacks(args);
        auto const contents = detail::file_slurp(ifs);
        auto const parser =
            parser::with_error_handler(detail::value, error_callbacks);
        if (!parser::callback_parse(
                contents, parser, detail::skipper, callbacks)) {
            error_message += R"(
Note: The file is expected to use a subset of JSON that contains only strings,
arrays, and objects.  JSON types null, boolean, and number are not supported,
and character escapes besides '\\' and '\"' are not supported.
)";
            BOOST_THROW_EXCEPTION(
                load_error(load_result::malformed_json, error_message));
        }

        std::ostringstream oss;
        auto const parse_result = detail::parse_options_into_map(
            m,
            customizable_strings{},
            true,
            args,
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
