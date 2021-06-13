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

#inlcude <cmath>
#include <limits>
#include <optional>


namespace boost { namespace program_options_2 { namespace toml_detail {

#if defined(__cpp_lib_chrono) && 201907 <= __cpp_lib_chrono
    using year_t = std::chrono::year;
    using month_t = std::chrono::month;
    using day_t = std::chrono::day;
    using year_month_day = std::chrono::year_month_day;
#else
    using year_t = std::chrono::
        duration<std::chrono::milliseconds::rep, std::ratio<31556952>>;
    using month_t = std::chrono::
        duration<std::chrono::milliseconds::rep, std::ratio<2629746>>;
    using day_t = std::chrono::
        duration<std::chrono::milliseconds::rep, std::ratio<86400>>;

    struct year_month_day
    {
        year_month_day() = default;
        constexpr year_month_day(year_t y, month_t m, day_t d) noexcept :
            y_(y), m_(m), d_(d)
        {}

        constexpr year_t year() const noexcept { return y_; }
        constexpr month_t month() const noexcept { return m_; }
        constexpr day_t day() const noexcept { return d_; }

    private:
        year y_;
        month m_;
        day d_;
    };
#endif

    struct time_tz
    {
        using time_point = std::chrono::
            time_point<std::chrono::system_clock, std::chrono::milliseconds>;
        using tz_duration = std::chrono::milliseconds;

        time_tz() = default;
        constexpr time_tz(time_point t) : t_(t), tz_() {}
        constexpr time_tz(time_point t, tz_duration tz) : t_(t), tz_(tz) {}

        constexpr time_point time() const noexcept { return t_; }
        constexpr std::optional<tz_duration> tz_offset() const noexcept
        {
            return tz_;
        }

    private:
        time_point t_;
        std::optional<tz_duration> tz_;
    };

    struct date_time
    {
        date_time() = default;
        constexpr date_time(year_month_day date, time_tz time) :
            d_(date), t_(time)
        {}

        constexpr year_month_day date() const noexcept { return d_; }
        constexpr time_tz time() const noexcept { return t_; }

    private:
        year_month_day d_;
        time_tz t_;
    };



    // Basic rules

    inline parser::rule<class toml> const toml = "TOML document";
    inline parser::rule<class expression> const expression =
        "key/value pair or table";
    inline parser::rule<class ws_char> const ws_char = "whitespace character";
    inline parser::rule<class ws> const ws = "whitespace";
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
    inline parser::rule<class dec_int, std::ptrdiff_t, std::string> const
        dec_int = "decimal integer";
    inline parser::rule<class unsigned_dec_int, std::size_t, std::string> const
        unsigned_dec_int = "unsigned decimal integer";
    inline parser::rule<class dec_int_impl, std::ptrdiff_t, std::string> const
        dec_int_impl = "decimal integer";
    inline parser::
        rule<class unsigned_dec_int_impl, std::size_t, std::string> const
            unsigned_dec_int_impl = "unsigned decimal integer";
    inline parser::rule<class hex_int, std::size_t, std::string> const hex_int =
        "unsigned hexidecimal integer";
    inline parser::rule<class oct_int, std::size_t, std::string> const oct_int =
        "unsigned octal integer";
    inline parser::rule<class bin_int, std::size_t, std::string> const bin_int =
        "unsigned binary integer";


    // Float

    inline parser::rule<class float_> const float_ = "floating-point number";
    inline parser::rule<class float_impl> const float_impl =
        "floating-point number";
    inline parser::rule<class zero_prefixable_int_impl> const
        zero_prefixable_int_impl = "floating-point number";
    inline parser::rule<class exp_impl> const exp_impl =
        "floating-point exponent";
    inline parser::rule<class frac_impl> const frac_impl =
        "floating-point frcational part";
    inline parser::rule<class special_float> const special_float =
        "+/-inf or +/-nan";


    // Date-Time

    inline parser::rule<class date_time> const date_time = "date/time";

    inline parser::rule<class date_month> const date_month = "month (01-12)";
    inline parser::rule<class date_mday> const date_mday =
        "day of the month (01-31)";
    inline parser::rule<class time_hour> const time_hour = "hours (00-23)";
    inline parser::rule<class time_minute> const time_minute =
        "minutes (00-59)";
    inline parser::rule<class time_second> const time_second =
        "seconds (00-60)";

    inline parser::rule<class partial_time> const partial_time = "time";
    inline parser::rule<class time_offset> const time_offset =
        "time zone offset";
    inline parser::rule<class full_date> const full_date = "year/month/day";
    inline parser::rule<class full_time> const full_time =
        "time with time zone offset";

    inline parser::rule<class date_time> const offset_date_time =
        "offset date/time";
    inline parser::rule<class date_time> const local_date_time =
        "local date/time";
    inline parser::rule<class date_time> const local_date = "date";
    inline parser::rule<class date_time> const local_time = "local time";


    // Array

    inline parser::rule<class array> const array = "array";
    inline parser::rule<class ws_comment_newline> const ws_comment_newline =
        "ws comment newline";


    // Table

    inline parser::rule<class table> const table = "table";
    inline parser::rule<class std_table> const std_table = "non-inline table";


    // Inline

    inline parser::rule<class inline_table> const inline_table = "inline table";


    // Array

    inline parser::rule<class array_table> const array_table = "array table";


    auto const append_to_local = [](auto & ctx) { _locals(ctx) += _attr(ctx); };
    auto const negate_and_assign = [](auto & ctx) { _val(ctx) = -_attr(ctx); };
    auto const assign = [](auto & ctx) { _val(ctx) = _attr(ctx); };
    auto const append = [](auto & ctx) { _val(ctx) += _attr(ctx); };


    // Rule definitions


    // Basic rules

    inline auto const toml_def = expression % newline;
    inline auto const expression_def = ws >> -(keyval | table) >> ws >>
                                       -comment;
    inline auto const ws_char_def = parser::lit('\x20') | '\x09';
    inline auto const ws_def = *ws_char;
    inline auto const newline_def = -parser::lit('\x0d') | '\x0a';
    inline auto const non_ascii_def =
        parser::char_(0x80, 0xd7ff) | parser::char_(0xe000, 0x10ffff);
    inline auto const non_eol_def =
        parser::char_(0x09) | parser::char_(0x20, 0x7f) | non_ascii;
    inline auto const comment_def = '#' >> *non_eol;

    // Key/value pair

    inline auto const keyval_def = key >> ws >> '=' >> ws >> val;
    inline auto const key_def = (quoted_key | unquoted_key) % (ws >> '.' >> ws);
    inline auto const unquoted_key_def =
        +(parser::char_('A', 'Z') | parser::char_('a', 'z') |
          parser::char_('0', '9') | parser::char_("-_"));
    inline auto const quoted_key_def = basic_string | literal_string;
    inline auto const val_def =
        string | boolean | array | inline_table | date_time | float_ | integer;

    // String

    inline parser::uint_parser<unsigned int, 16, 4, 4> const hex4;
    inline parser::uint_parser<unsigned int, 16, 8, 8> const hex8;

    inline auto const escaped_def =
        '\\' >> ('\"' >> parser::attr(0x0022u) | '\\' >> parser::attr(0x005cu) |
                 'b' >> parser::attr(0x0008u) | 'f' >> parser::attr(0x000cu) |
                 'n' >> parser::attr(0x000au) | 'r' >> parser::attr(0x000du) |
                 't' >> parser::attr(0x0009u) | 'u' >> hex4 | 'U' >> hex8);

    inline auto const string_def =
        ml_basic_string | basic_string | ml_literal_string | literal_string;

    // Basic string

    inline auto const basic_string_def = parser::lit('"') >> *basic_char >>
                                         parser::lit('"');

    inline auto const basic_char_def =
        ws_char | '!' | parser::char_(0x23, 0x5b) | parser::char_(0x5d, 0x7e) |
        non_ascii | escaped;

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
        (literal_char | newline) - parser::lit("'''");

    // Integer

    auto const attr_str_to_dec_size_t = [](auto & ctx) {
        parser::uint_parser<std::size_t> const p;
        parser::parse(_attr(ctx), p, _val(ctx));
    };
    auto const attr_str_to_ptrdiff_t = [](auto & ctx) {
        parser::int_parser<std::ptrdiff_t> const p;
        parser::parse(_attr(ctx), p, _val(ctx));
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

    inline auto const integer_def = dec_int | hex_int | oct_int | bin_int;
    inline auto const dec_int_def = dec_int_impl[attr_str_to_ptrdiff_t];
    inline auto const
        dec_int_impl_def = parser::char_('-')[append] >>
                               unsigned_dec_int_impl[append] |
                           -parser::lit('+') >> unsigned_dec_int_impl[append];
    inline auto const unsigned_dec_int_def =
        unsigned_dec_int_impl[attr_str_to_dec_size_t];
    inline auto const unsigned_dec_int_impl_def =
        parser::char_('0')[append] |
        parser::char_('1', '9')[append] >>
            *(-parser::lit('_') >> parser::char_('0', '9')[append]);
    inline auto const hex_digit =
        parser::char_('0', '1') | parser::char_('A', 'F');
    inline auto const hex_def = "0x" >>
                                (hex_digit[append_to_local] %
                                 -parser::lit('_'))[local_str_to_hex_size_t];
    inline auto const oct_def = "0o" >>
                                (parser::char_('0', '7')[append_to_local] %
                                 -parser::lit('_'))[local_str_to_oct_size_t];
    inline auto const bin_def = "0b" >>
                                (parser::char_('0', '1')[append_to_local] %
                                 -parser::lit('_'))[local_str_to_bin_size_t];

    // Float

    auto const local_str_to_double = [](auto & ctx) {
        parser::parse(_locals(ctx), parser::double_, _val(ctx));
    };

    inline auto const float_def = '0' >> parser::attr(0.0) | float_impl
                                  | special_float_def;
    inline auto const float_impl_def =
        (dec_int_impl[append_to_local] >>
         (exp[append_to_local] |
          frac[append_to_local] >> -exp[append_to_local]))[local_str_to_double];
    inline auto const zero_prefixable_int_def =
        parser::char_('0', '9')[append] % -parser::lit('_');
    inline auto const exp_def = parser::char_('e')[append] >>
                                (parser::char_('-')[append] |
                                 parser::char_('+')[append]) >>
                                zero_prefixable_int[append];
    inline auto const frac_def =
        parser::char_('.')[append] >> zero_prefixable_int[append];
    inline auto const special_float_def =
        parser::lit('-') >> parser::lit("inf") >>
            parser::attr(-std::numeric_limits<double>::infinity()) |
        -parser::lit('+') >> parser::lit("inf") >>
            parser::attr(std::numeric_limits<double>::infinity()) |
        -(parser::lit('-') | parser::lit('+')) >> parser::lit("nan") >>
            parser::attr(nan(""));

    // Boolean

    inline parser::bool_ const boolean;

    // Date/time

    auto const parse_hour_into_locals = [](auto & ctx) {
        parser::parse(_attr(ctx), parser::uint_, _locals(ctx).hours);
        if (23 < _locals(ctx).hours)
            _pass(ctx) = false;
    };
    auto const parse_minute_into_locals = [](auto & ctx) {
        parser::parse(_attr(ctx), parser::uint_, _locals(ctx).minutes);
        if (59 < _locals(ctx).minutes)
            _pass(ctx) = false;
    };
    auto const parse_second_and_assign = [](auto & ctx) {
        double s = 0.0;
        parser::parse(_where(ctx), parser::double_, s);
        // Due to the leap second rules, the number of seconds may be up
        // to 60.xxx.
        if (61.0 < _locals(ctx).minutes)
            _pass(ctx) = false;
        unsigned int seconds = s * 1000.0;
        _val(ctx) = time{
            std::chrono::hours(_locals(ctx).hours) +
            std::chrono::minutes(_locals(ctx).minutes) +
            std::chrono::seconds(seconds)};
    };
    auto const parse_minute_and_assign = [](auto & ctx) {
        unsigned int m = 0;
        parser::parse(_where(ctx), parser::uint_, m);
        if (59 < m)
            _pass(ctx) = false;
        _val(ctx) =
            std::chrono::hours(_locals(ctx).hours) + std::chrono::minutes(m);
    };
    auto const construct_full_date = [](auto & ctx) {
        auto tup = _attr(ctx);
        using ::boost::parser::literals;
        _val(ctx) = year_month_day(
            year_t(tup[0_c]), month_t(tup[1_c]), day_t(tup[2_c]));
    };
    auto const construct_full_time = [](auto & ctx) {
        using ::boost::parser::literals;
        auto val = _attr(ctx)[0_c];
        val.tz_offset = _attr(ctx)[1_c];
        _val(ctx) = val;
    };
    auto const construct_date_time = [](auto & ctx) {
        using ::boost::parser::literals;
        _val(ctx) = date_time(_attr(ctx)[0_c], _attr(ctx)[1_c]);
    };

    inline auto const date_time_def =
        offset_date_time | local_date_time | local_date | local_time;

    inline parser::uint_parser<unsigned int, 10, 4, 4> const date_fullyear;
    inline parser::uint_parser<unsigned int, 10, 2, 2> const date_month_def;
    inline parser::uint_parser<unsigned int, 10, 2, 2> const date_mday_def;
    inline parser::uint_parser<unsigned int, 10, 2, 2> const time_hour_def;
    inline parser::uint_parser<unsigned int, 10, 2, 2> const time_minute_def;
    inline parser::uint_parser<unsigned int, 10, 2, 2> const time_second_def;

    inline auto const partial_time_def =
        time_hour[parse_hour_into_locals] >> ':' >>
        time_minute[parse_minute_into_locals] >> ':' >>
        (time_second >>
         -('.' >> +parser::char_('0', '9')))[parse_second_and_assign];

    inline auto const time_offset_def =
        ((parser::lit("Z") | "z") >>
         parser::attr(std::chrono::milliseconds(0))) |
        parser::raw[(parser::lit('+') | '-') >> time_hour]
                   [parse_hour_into_locals] >>
            ':' >> time_minute[parse_minute_and_assign];

    inline auto const full_date_def =
        (date_fullyear >> '-' >> date_month >> '-' >>
         date_mday)[construct_full_date];

    inline auto const full_time_def =
        (partial_time >> time_offset)[construct_full_time];

    inline auto const offset_date_time_def =
        (full_date >> (parser::lit('T') | 't' | ' ') >>
         full_time)[construct_date_time];
    inline auto const local_date_time_def =
        (full_date >> (parser::lit('T') | 't' | ' ') >>
         partial_time)[construct_date_time];
    inline auto const local_date_def = full_date;
    inline auto const local_time_def = partial_time;

    // Array

    inline auto const array_def = '[' >> (ws_comment_newline >> val) %
                                             (ws_comment_newline >> ',') >>
                                  ws_comment_newline >> ']';

    inline auto const ws_comment_newline_def = *(ws_char | -comment >> newline);

    // Table

    inline auto const table_def = std_table | array_table;
    inline auto const std_table_def = '[' >> ws >> key >> ws >> ']';

    // Inline table

    inline auto const inline_table_def = '{' >> ws >>
                                         (key_val % (ws >> ',' >> ws)) >> ws >>
                                         '}';

    // Array table

    inline auto const array_table_def = "[[" >> ws >> key >> ws >> "]]";


    // TODO    BOOST_PARSER_DEFINE_RULES(toml);

#if 0 // TODO
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
