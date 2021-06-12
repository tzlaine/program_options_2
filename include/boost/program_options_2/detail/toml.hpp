// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PROGRAM_OPTIONS_2_DETAIL_TOML_HPP
#define BOOST_PROGRAM_OPTIONS_2_DETAIL_TOML_HPP

#include <boost/program_options_2/fwd.hpp>
#include <boost/program_options_2/detail/parsing.hpp>
//#include <boost/program_options_2/detail/utility.hpp>


namespace boost { namespace program_options_2 { namespace toml_detail {

    // Basic rules

    inline parser::rule<class toml> const toml = "TOML document";
    inline parser::rule<class expression> const expression =
        "key/value pair or table";
    inline parser::rule<class ws_char> const ws_char = "whitespace character";
    inline parser::rule<class newline> const newline = "newline";
    inline parser::rule<class non_ascii> const non_ascii = "non-ASCII character";
    inline parser::rule<class comment> const comment = "comment";


    // Key/value pairs

    inline parser::rule<class keyval> const keyval = "key/value pair";
    inline parser::rule<class key> const key = "key";
    inline parser::rule<class simple_key> const simple_key = "simple-key";
    inline parser::rule<class unquoted_key> const unquoted_key = "unquoted-key";
    inline parser::rule<class quoted_key> const quoted_key = "quoted-key";
    inline parser::rule<class val> const val = "value";


    // Strings

    inline parser::rule<class string> const string = "string";
    inline parser::rule<class basic_string> const basic_string = "basic-string";
    inline parser::rule<class basic_char> const basic_char = "character";
    inline parser::rule<class escaped> const escaped = "character";
    inline parser::rule<class literal_string> const literal_string =
        "literal-string";
    inline parser::rule<class literal_char> const literal_char = "literal-char";


    // Multiline strings

    inline parser::rule<class ml_basic_string> const ml_basic_string =
        "multiline basic-string";
    inline parser::rule<class mlb_content> const mlb_content =
        "multiline basic-string characters";
    inline parser::rule<class mlb_escaped_nl> const mlb_escaped_nl =
        "\\-escaped newline";
    inline parser::rule<class ml_literal_string> const ml_literal_string =
        "multiline literal-string";
    inline parser::rule<class mll_content> const mll_content =
        "multiline literal-string characters";


    // Integer

    inline parser::rule<class integer, std::ptrdiff_t> const integer =
        "integer";
    inline parser::rule<class unsigned_dec_int, std::size_t, std::string> const
        unsigned_dec_int = "unsigned decimal integer";
    inline parser::rule<class unsigned_integer, std::size_t> const
        unsigned_integer = "unsigned integer";
    inline parser::rule<class hex_int, std::size_t, std::string> const hex_int =
        "hexidecimal digits";
    inline parser::rule<class oct_int, std::size_t, std::string> const oct_int =
        "octal digits";
    inline parser::rule<class bin_int, std::size_t, std::string> const bin_int =
        "binary digits";


    // Float

    inline parser::rule<class float_> const float_ = "floating-point number";
    inline parser::rule<class special_float> const special_float =
        "+/-inf or +/-nan";


    // Date-Time

    inline parser::rule<class date_time> const date_time = "date/time";

    // TODO


    // Array

    // TODO


    // Table

    inline parser::rule<class table> const table = "table";


    auto const append_local_char = [](auto & ctx) {
        _locals(ctx).push_back(_attr(ctx));
    };


    // Definitions


    // Basic rules

    inline auto const toml_def = expression % newline;
    inline auto const expression_def = keyval | table;
    inline auto const ws_char_def = parser::lit('\x20') | '\x09';
    inline auto const newline_def = -parser::lit('\x0d') | '\x0a';
    inline auto const non_ascii_def =
        parser::char_(0x80, 0xd7ff) | parser::char_(0xe000, 0x10ffff);
    inline auto const comment_def = '#' >> *(parser::char_(0x80, 0xd7ff) |
                                             '\0x09' | non_ascii);

    // Key/value pair

    inline auto const keyval_def = key >> '=' >> val;
    inline auto const key_def = (quoted_key | unquoted_key) % '.';
    inline auto const unquoted_key_def = parser::lexeme[+(
        parser::char_('A', 'Z') | parser::char_('a', 'z') |
        parser::char_('0', '9') | parser::char_("-_"))];
    inline auto const quoted_key_def = basic_string | literal_string;
    inline auto const val_def =
        string | boolean | array | inline_table | date_time | float_ | integer;

    // String

    inline parser::uint_parser<unsigned int, 16, 4, 4> const hex4;
    inline parser::uint_parser<unsigned int, 16, 8, 8> const hex8;

    inline auto const escaped_def = parser::lexeme
        ['\\' >>
         ('\"' >> parser::attr(0x0022u) | '\\' >> parser::attr(0x005cu) |
          'b' >> parser::attr(0x0008u) | 'f' >> parser::attr(0x000cu) |
          'n' >> parser::attr(0x000au) | 'r' >> parser::attr(0x000du) |
          't' >> parser::attr(0x0009u) | 'u' >> hex4 | 'U' >> hex8)];

    inline auto const string_def =
        ml_basic_string | basic_string | ml_literal_string | literal_string;

    // Basic string

    inline auto const basic_string_def = parser::lit('"') >> *basic_char >>
                                         parser::lit('"');

    inline auto const basic_char_def =
        parser::char_(" \t!") | parser::char_(0x23, 0x5b) |
        parser::char_(0x5d, 0x7e) | non_ascii | escaped;

    // Multiline string

    inline auto const ml_basic_string_def = parser::lit("\"\"\"") >> -newline >>
                                            *mlb_content >>
                                            parser::lit("\"\"\"");

    inline auto const mlb_content_def =
        (basic_char | newline | mlb_escaped_nl) - parser::lit("\"\"\"");

    inline auto const mlb_escaped_nl_def = parser::lit('\\') >> *ws_char >>
                                           newline >> *(ws_char | newline);

    // Literal string

    inline auto const literal_string_def = parser::lit("'") >> *literal_char >>
                                           parser::lit("'");

    inline auto const literal_char_def = parser::char_(0x09) |
                                         parser::char_(0x20, 0x26) |
                                         parser::char_(0x28, 0x7e) | non_ascii;
    // Multiline literal string

    inline auto const ml_literal_string_def = parser::lit("'''") >> -newline >>
                                              *mll_content >>
                                              parser::lit("'''");

    inline auto const mll_content_def =
        (literal_char | newline_nl) - parser::lit("'''");

    // Integer

    auto const local_str_to_dec_size_t = [](auto & ctx) {
        parser::uint_parser<std::size_t> const p;
        parser::parse(_locals(ctx), p, _val(ctx));
    };
    auto const local_str_to_hex_size_t = [](auto & ctx) {
        parser::uint_parser<std::size_t, 16> const p;
        parser::parse(_locals(ctx), p, _val(ctx));
    };
    auto const local_str_to_oct_size_t = [](auto & ctx) {
        parser::uint_parser<std::size_t, 8> const p;
        parser::parse(_locals(ctx), p, _val(ctx));
    };
    auto const local_str_to_bin_size_t = [](auto & ctx) {
        parser::uint_parser<std::size_t, 2> const p;
        parser::parse(_locals(ctx), p, _val(ctx));
    };

    inline auto const integer_def = ;
    inline auto const unsigned_dec_int_def =
        '0' >> parser::attr(std::size_t{0}) |
        parser::lexeme
            [parser::char_('1', '9')[append_local_char] >>
             *(-parser::lit('_') >> parser::char_('0', '9')[append_local_char])]
            [local_str_to_dec_size_t];
    inline auto const unsigned_integer_def =
        unsigned_dec_int | hex_int | oct_int |
        bin_int; // TODO: Need to assign the attribute here....  Should that
                 // be automatic?
    inline auto const hex_digit =
        parser::char_('0', '1') | parser::char_('A', 'F');
    inline auto const hex_def =
        "0x" >> parser::lexeme
                    [hex_digit[append_local_char] >>
                     *(-parser::lit('_') >> hex_digit[append_local_char])]
                    [local_str_to_hex_size_t];
    inline auto const oct_def =
        "0o" >>
        parser::lexeme
            [parser::char_('0', '7')[append_local_char] >>
             *(-parser::lit('_') >> parser::char_('0', '7')[append_local_char])]
            [local_str_to_oct_size_t];
    inline auto const bin_def =
        "0b" >>
        parser::lexeme
            [parser::char_('0', '1')[append_local_char] >>
             *(-parser::lit('_') >> parser::char_('0', '1')[append_local_char])]
            [local_str_to_bin_size_t];

    // Float

    // TODO

    inline parser::bool_ const boolean;

    // Date/time

    // TODO

    // Array

    // TODO

    // Table

    // TODO

    // Array Table

    // TODO

#if 0 // TODO
    BOOST_PARSER_DEFINE_RULES(
);

    struct callbacks
    {
        callbacks(std::vector<std::string_view> & result) : result_(result) {}

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
#endif

}}}
