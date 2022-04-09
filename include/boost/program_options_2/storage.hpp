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
        option when storing the value in a map.

        \note When you customize the `short_option_prefix`,
        `long_option_prefix`, or `response_file_prefix` members of
        `customizable_strings`, you must pass your custom
        `customizable_strings` here, even thought it is possible to call this
        function without it.  If you fail to do this, the value returned by
        this function is very likely to be wrong.*/
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
        customizable_strings const & strings)
    {
        auto const name = detail::first_name_prefer(
            opt.names, [&strings](std::string_view sv) {
                return detail::positional(sv, strings) ||
                       detail::long_(sv, strings);
            });
        return detail::trim_leading_dashes(name, strings);
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
        customizable_strings const & strings,
        OptionsMap const & m,
        Options const &... opts)
    {
        std::ofstream ofs(filename.data());
        if (!ofs) {
            BOOST_THROW_EXCEPTION(save_error(
                save_result::could_not_open_file_for_writing, filename));
        }

        ofs << std::boolalpha;

        detail::map_lookup<OptionsMap const> lookup(m, strings);
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
                if (!detail::positional(opt, strings))
                    ofs << detail::first_long_name(opt.names, strings) << ' ';
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

        template<typename Iter>
        struct excessive_nesting : std::runtime_error
        {
            excessive_nesting(Iter it) :
                runtime_error("excessive_nesting"), iter(it)
            {}
            Iter iter;
        };

        struct global_state
        {
            int recursive_open_count = 0;
            int max_recursive_open_count = 0;
        };

        struct double_escape_locals
        {
            int first_surrogate = 0;
        };

        inline parser::rule<class ws> const ws = "whitespace";

        inline parser::rule<class string_char, uint32_t> const string_char =
            "code point (code points <= U+001F must be escaped)";
        inline parser::rule<class four_hex_digits, uint32_t> const hex_4 =
            "four hexidecimal digits";
        inline parser::rule<class escape_seq, uint32_t> const escape_seq =
            "\\uXXXX hexidecimal escape sequence";
        parser::
            rule<class escape_double_seq, uint32_t, double_escape_locals> const
                escape_double_seq = "\\uXXXX hexidecimal escape sequence";
        inline parser::rule<class single_escaped_char, uint32_t> const
            single_escaped_char = "'\"', '\\', '/', 'b', 'f', 'n', 'r', or 't'";

        inline parser::callback_rule<class null_tag> const null = "null";

        inline parser::callback_rule<class bool_tag, bool> const bool_p =
            "boolean";

        inline parser::callback_rule<class string_tag, std::string> const
            string = "string";
        inline parser::callback_rule<class number_tag, double> const number =
            "number";

        inline parser::callback_rule<
            class object_element_key_tag,
            std::string> const object_element_key = "string";
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


        // Since we use these tag types as function parameters in the callbacks,
        // they need to be complete types.
        class null_tag
        {};
        class bool_tag
        {};
        class string_tag
        {};
        class number_tag
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

        inline auto first_hex_escape = [](auto & ctx) {
            auto & locals = _locals(ctx);
            uint32_t const cu = _attr(ctx);
            if (!boost::parser::detail::text::high_surrogate(cu))
                _pass(ctx) = false;
            else
                locals.first_surrogate = cu;
        };
        inline auto second_hex_escape = [](auto & ctx) {
            auto & locals = _locals(ctx);
            uint32_t const cu = _attr(ctx);
            if (!boost::parser::detail::text::low_surrogate(cu)) {
                _pass(ctx) = false;
            } else {
                uint32_t const high_surrogate_min = 0xd800;
                uint32_t const low_surrogate_min = 0xdc00;
                uint32_t const surrogate_offset =
                    0x10000 - (high_surrogate_min << 10) - low_surrogate_min;
                uint32_t const first_cu = locals.first_surrogate;
                _val(ctx) = (first_cu << 10) + cu + surrogate_offset;
            }
        };

        inline parser::parser_interface<
            parser::uint_parser<uint32_t, 16, 4, 4>> const hex_4_def;

        inline auto const escape_seq_def = "\\u" > hex_4;

        inline auto const escape_double_seq_def =
            escape_seq[first_hex_escape] >> escape_seq[second_hex_escape];

        inline parser::symbols<uint32_t> const single_escaped_char_def = {
            {"\"", 0x0022u},
            {"\\", 0x005cu},
            {"/", 0x002fu},
            {"b", 0x0008u},
            {"f", 0x000cu},
            {"n", 0x000au},
            {"r", 0x000du},
            {"t", 0x0009u}};

        inline auto const string_char_def =
            escape_double_seq | escape_seq |
            (parser::lit('\\') > single_escaped_char) |
            (parser::cp - parser::char_(0x0000u, 0x001fu));

        inline auto const null_def = parser::lit("null");

        inline auto const bool_p_def = parser::bool_;

        inline auto const string_def =
            parser::lexeme['"' >> *(string_char - '"') > '"'];

        inline auto parse_double = [](auto & ctx) {
            auto const cp_range = _attr(ctx);
            auto cp_first = cp_range.begin();
            auto const cp_last = cp_range.end();

            auto const result =
                parser::parse(cp_first, cp_last, parser::double_);
            if (result) {
                _val(ctx) = *result;
            } else {
                boost::container::small_vector<char, 128> chars(
                    cp_first, cp_last);
                auto const chars_first = &*chars.begin();
                auto chars_last = chars_first + chars.size();
                _val(ctx) = std::strtod(chars_first, &chars_last);
            }
        };

        inline auto const number_def =
            parser::raw[parser::lexeme
                            [-parser::char_('-') >>
                             (parser::char_('1', '9') >> *parser::ascii::digit |
                              parser::char_('0')) >>
                             -(parser::char_('.') >> +parser::ascii::digit) >>
                             -(parser::char_("eE") >> -parser::char_("+-") >>
                               +parser::ascii::digit)]][parse_double];

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

        inline auto const value_def =
            number | bool_p | null | string | array | object;

        inline auto const skipper =
            parser::ws |
            ('#' >> *(parser::char_ - parser::eol) >> -parser::eol);

        BOOST_PARSER_DEFINE_RULES(
            ws,
            hex_4,
            escape_seq,
            escape_double_seq,
            single_escaped_char,
            string_char,
            null,
            bool_p,
            string,
            number,
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
            json_callbacks(std::vector<std::string> & result) : result_(result)
            {}

            void operator()(null_tag) const { result_.push_back("null"); }
            void operator()(bool_tag, bool b) const
            {
                result_.push_back(b ? "true" : "false");
            }
            void operator()(string_tag, std::string s) const
            {
                result_.push_back(std::move(s));
            }
            void operator()(number_tag, double d) const
            {
                result_.push_back(std::to_string(d));
            }
            void operator()(object_element_key_tag, std::string key) const
            {
                result_.push_back(std::move(key));
            }
            void operator()(object_open_tag) const {}
            void operator()(object_close_tag) const {}
            void operator()(array_open_tag) const {}
            void operator()(array_close_tag) const {}

        private:
            std::vector<std::string> & result_;
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

        \note When you customize the `short_option_prefix`,
        `long_option_prefix`, or `response_file_prefix` members of
        `customizable_strings`, you must pass your custom
        `customizable_strings` here, even thought it is possible to call
        another overload of this function without it.  If you fail to do this,
        the values saved by this function are very likely to be unparseable by
        the rest of your program.

        \throws `save_error` on failure */
    template<options_map OptionsMap, typename... Options>
    void save_json_file(
        std::string_view filename,
        OptionsMap const & m,
        customizable_strings const & strings,
        Options const &... opts)
    {
        std::ofstream ofs(filename.data());
        if (!ofs) {
            BOOST_THROW_EXCEPTION(save_error(
                save_result::could_not_open_file_for_writing, filename));
        }

        ofs << std::boolalpha;
        ofs << "{\n";

        detail::map_lookup<OptionsMap const> lookup(m, strings);
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
                ofs << "    \"" << detail::first_long_name(opt.names, strings)
                    << "\":";
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

    /** Saves the options in `m`, expecting to find the options in `opts`,
        writing JSON-formatted output to file `filename`.

        \note When you customize the `short_option_prefix`,
        `long_option_prefix`, or `response_file_prefix` members of
        `customizable_strings`, you must call the overload of this function
        that takes a `customizable_strings`, and pass your custom
        `customizable_strings` there.  If you fail to do this, the values
        saved by this function are very likely to be unparseable by the rest
        of your program.

        \throws `save_error` on failure */
    template<options_map OptionsMap, typename... Options>
    void save_json_file(
        std::string_view filename,
        OptionsMap const & m,
        Options const &... opts)
    {
        program_options_2::save_json_file(
            filename, customizable_strings{}, m, opts...);
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

        int const max_recursion = 512;
        detail::global_state globals{0, max_recursion};

        std::vector<std::string> args;
        detail::json_callbacks callbacks(args);
        auto const contents = detail::file_slurp(ifs);
        using iter_t = decltype(contents.begin());

        auto const parser = parser::with_error_handler(
            parser::with_globals(detail::value, globals), error_callbacks);
        try {
            if (!parser::callback_parse(
                    contents, parser, detail::skipper, callbacks)) {
                BOOST_THROW_EXCEPTION(
                    load_error(load_result::malformed_json, error_message));
            }
        } catch (detail::excessive_nesting<iter_t> const & e) {
            std::ostringstream os;
            parser::write_formatted_message(
                os,
                filename,
                contents.begin(),
                e.iter,
                contents.end(),
                "error: Exceeded maximum number (" +
                    std::to_string(max_recursion) +
                    ") of open arrays and/or objects");
            BOOST_THROW_EXCEPTION(
                load_error(load_result::malformed_json, os.str()));
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
