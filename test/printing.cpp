// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#define BOOST_PROGRAM_OPTIONS_2_TESTING
#include <boost/program_options_2/options.hpp>
#include <boost/program_options_2/decorators.hpp>
#include <boost/program_options_2/parse_command_line.hpp>

#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>

#include <gtest/gtest.h>


namespace po2 = boost::program_options_2;
using sv = std::string_view;

TEST(printing, detail_print_uppercase)
{
    {
        std::ostringstream os;
        po2::detail::print_uppercase(
            os, std::string_view("this should end up screaming."));
        EXPECT_EQ(os.str(), "THIS SHOULD END UP SCREAMING.");
    }
    {
        std::ostringstream os;
        po2::detail::print_uppercase(
            os,
            std::string_view(
                "This should scream, too, but it needs to be really, really "
                "long so that it overflows the side-buffer size, and so has to "
                "be done in multiple chunks.  Fingers crossed!"));
        EXPECT_EQ(
            os.str(),
            "THIS SHOULD SCREAM, TOO, BUT IT NEEDS TO BE REALLY, REALLY "
            "LONG SO THAT IT OVERFLOWS THE SIDE-BUFFER SIZE, AND SO HAS TO "
            "BE DONE IN MULTIPLE CHUNKS.  FINGERS CROSSED!");
    }
}

TEST(printing, detail_print_args)
{
    {
        std::ostringstream os;
        po2::detail::print_args(
            os, sv("foo"), po2::positional<int>("foo", ""), true);
        EXPECT_EQ(os.str(), " FOO");
    }
    {
        std::ostringstream os;
        po2::detail::print_args(
            os,
            sv("foo"),
            po2::positional<std::vector<int>>("foo", "", 2),
            true);
        EXPECT_EQ(os.str(), " FOO FOO");
    }
    {
        std::ostringstream os;
        po2::detail::print_args(
            os,
            sv("foo"),
            po2::positional<std::vector<int>>("foo", "", po2::one_or_more),
            false);
        EXPECT_EQ(os.str(), "FOO ...");
    }
    {
        std::ostringstream os;
        po2::detail::print_args(
            os,
            sv("foo"),
            po2::with_display_name(
                po2::positional<std::vector<int>>("foo", "", 2), "bar"),
            true);
        EXPECT_EQ(os.str(), " bar bar");
    }
    {
        std::ostringstream os;
        po2::detail::print_args(
            os,
            sv("foo"),
            po2::with_display_name(
                po2::positional<std::vector<int>>("foo", "", po2::one_or_more),
                "bar"),
            false);
        EXPECT_EQ(os.str(), "bar ...");
    }
}

struct my_int
{
    my_int() = default;
    my_int(int i) : value_(i) {}
    int value_;
};

TEST(printing, print_option_arguments)
{
    po2::customizable_strings const strings;

    // no defaults
    {
        auto const arg = po2::argument<int>("-b,--blah", "");
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b B]");
    }
    {
        auto const arg = po2::argument<std::vector<int>>("-b,--blah", "", 2);
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b B B]");
    }
    {
        auto const arg =
            po2::argument<std::optional<int>>("--blah", "", po2::zero_or_one);
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [--blah [BLAH]]");
    }
    {
        auto const arg =
            po2::argument<std::vector<int>>("-b,--blah", "", po2::zero_or_more);
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b [B ...]]");
    }
    {
        auto const arg =
            po2::argument<std::vector<int>>("-b,--blah", "", po2::one_or_more);
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b B ...]");
    }

    // unprintable choice type
    {
        auto const arg = po2::argument<my_int>(
            "-b,--blah", "", 1, my_int{1}, my_int{2}, my_int{3});
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b B]");
    }
    // printable choices
    {
        auto const arg = po2::argument<int>("-b,--blah", "", 1, 1, 2, 3);
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b {1,2,3}]");
    }
    {
        auto const arg =
            po2::argument<std::vector<int>>("-b,--blah", "", 2, 1, 2, 3);
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b {1,2,3} {1,2,3}]");
    }
    {
        auto const arg = po2::argument<std::optional<int>>(
            "-b,--blah", "", po2::zero_or_one, 1, 2, 3);
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b [{1,2,3}]]");
    }
    {
        auto const arg = po2::argument<std::vector<int>>(
            "-b,--blah", "", po2::zero_or_more, 1, 2, 3);
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b [{1,2,3} ...]]");
    }
    {
        auto const arg = po2::argument<std::vector<int>>(
            "-b,--blah", "", po2::one_or_more, 1, 2, 3);
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b {1,2,3} ...]");
    }
    {
        auto const arg = po2::argument<std::vector<int>>(
            "-b,--blah", "", po2::zero_or_more, 1, 2, 3);
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b [{1,2,3} ...]]");
    }

    // with defaults
    {
        auto const arg =
            po2::with_default(po2::argument<int>("-b,--blah", ""), 42);
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b B]");
    }
    {
        auto const arg = po2::with_default(
            po2::argument<std::optional<int>>(
                "-b,--blah", "", po2::zero_or_one),
            42);
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b [B]]");
    }
    {
        auto const arg = po2::with_default(
            po2::argument<std::vector<int>>("-b,--blah", "", 2),
            std::vector<int>({42}));
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b B B]");
    }
    {
        auto const arg = po2::with_default(
            po2::argument<std::vector<int>>("-b,--blah", "", 2), 42);
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b B B]");
    }
    {
        auto const arg = po2::with_default(
            po2::argument<int>("-b,--blah", "", 1, 1, 2, 3), 3);
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b {1,2,3}]");
    }

    // add a display name
    {
        auto const arg = po2::with_display_name(
            po2::argument<int>("-b,--blah", ""), "blerg");
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b blerg]");
    }
}

TEST(printing, print_option_positionals)
{
    po2::customizable_strings const strings;

    // no defaults
    {
        auto const arg = po2::positional<int>("blah", "");
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " BLAH");
    }
    {
        auto const arg = po2::positional<std::vector<int>>("blah", "", 2);
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " BLAH BLAH");
    }
    {
        auto const arg =
            po2::positional<std::vector<int>>("blah", "", po2::one_or_more);
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " BLAH ...");
    }
    {
        auto const arg = po2::remainder<std::vector<int>>("blah", "");
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [BLAH ...]");
    }

    // unprintable choice type
    {
        auto const arg = po2::positional<my_int>(
            "blah", "", 1, my_int{1}, my_int{2}, my_int{3});
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " BLAH");
    }
    // printable choices
    {
        auto const arg = po2::positional<int>("blah", "", 1, 1, 2, 3);
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " {1,2,3}");
    }
    {
        auto const arg =
            po2::positional<std::vector<int>>("blah", "", 2, 1, 2, 3);
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " {1,2,3} {1,2,3}");
    }
    {
        auto const arg = po2::positional<std::vector<int>>(
            "blah", "", po2::one_or_more, 1, 2, 3);
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " {1,2,3} ...");
    }

    // add a display name
    {
        auto const arg =
            po2::with_display_name(po2::positional<int>("blah", ""), "blerg");
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " blerg");
    }
}

TEST(printing, print_option_other)
{
    po2::customizable_strings const strings;

    {
        auto const arg = po2::flag("--foo", "");
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [--foo]");
    }
    {
        auto const arg = po2::inverted_flag("--foo", "");
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [--foo]");
    }
    {
        auto const arg = po2::counted_flag("--foo,-f", "");
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-f[f...]]");
    }
    {
        auto const arg = po2::version("3.2.1");
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-v]");
    }
    {
        auto const arg = po2::help("-a,--ayuda", "");
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-a]");
    }
    {
        auto help_str = [] { return sv("Help message goes here.", ""); };
        auto const arg = po2::help(help_str);
        std::ostringstream os;
        po2::detail::print_option(strings, os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-h]");
    }
}

TEST(printing, detail_print_help_synopsis)
{
    std::string const exe = std::string("foo") + po2::detail::fs_sep + "bar";

    po2::detail::parse_contexts_vec<char> const parse_contexts;

    {
        std::ostringstream os;
        po2::detail::print_help_synopsis(
            po2::customizable_strings{},
            os,
            sv(exe),
            sv("A program that does things."),
            parse_contexts,
            po2::positional<int>("foo", ""));
        EXPECT_EQ(os.str(), R"(usage:  foo/bar FOO

A program that does things.
)");
    }

    {
        std::ostringstream os;
        po2::detail::print_help_synopsis(
            po2::customizable_strings{},
            os,
            sv(exe),
            sv("A program that does things."),
            parse_contexts,
            po2::positional<std::vector<int>>("foo", "", 30));
        EXPECT_EQ(os.str(), R"(usage:  foo/bar
                FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO

A program that does things.
)");
    }

    {
        std::string const long_exe = std::string("foo") + po2::detail::fs_sep +
                                     "barrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr";
        std::ostringstream os;
        po2::detail::print_help_synopsis(
            po2::customizable_strings{},
            os,
            sv(long_exe),
            sv("A program that does things."),
            parse_contexts,
            po2::positional<std::vector<int>>("foo", "", 30));
        EXPECT_EQ(
            os.str(), R"(usage:  foo/barrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr
        FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO

A program that does things.
)");
    }
}

po2::customizable_strings user_strings()
{
    po2::customizable_strings retval;
    retval.usage_text = "USAGE: ";
    retval.positional_section_text = "POSITIONAL arguments:";
    retval.optional_section_text = "OPTIONAL arguments:";
    retval.default_help_names = "-r,--redacted";
    retval.help_description = "Nothing to see here.";
    return retval;
}

TEST(printing, parse_command_line_help)
{
    std::string strings = std::string("foo") + po2::detail::fs_sep + "bar";
    strings += '\0';
    strings += "-h";
    strings += '\0';

    int const argc = 2;
    char const * argv[2] = {
        strings.c_str(), strings.c_str() + strings.size() - 3};

    {
        std::ostringstream os;
        try {
            po2::parse_command_line(
                argc,
                argv,
                "A test program to see how things work.",
                os,
                po2::argument("--non-positional", "A non-positional argument."),
                po2::positional("positional", "A positional argument."),
                po2::positional(
                    "a-very-very-very-obnoxiously-long-positional",
                    "Another positional argument."),
                po2::argument(
                    "--non-pos-2", "A second non-positional argument."));
        } catch (int) {
        }
        EXPECT_EQ(
            os.str(),
            R"(usage:  bar [-h] [--non-positional NON-POSITIONAL] POSITIONAL
            A-VERY-VERY-VERY-OBNOXIOUSLY-LONG-POSITIONAL [--non-pos-2 NON-POS-2]

A test program to see how things work.

positional arguments:
  positional            A positional argument.
  a-very-very-very-obnoxiously-long-positional
                        Another positional argument.

optional arguments:
  -h, --help            Print this help message and exit
  --non-positional      A non-positional argument.
  --non-pos-2           A second non-positional argument.

response files:
  Use '@file' to load a file containing command line arguments.
)");
    }

    {
        std::ostringstream os;
        try {
            po2::parse_command_line(
                argc,
                argv,
                "A test program to see how things work.",
                os,
                po2::argument("--non-positional", "A non-positional argument."),
                po2::positional("positional", "A positional argument."),
                po2::argument(
                    "--an,--arg,--with,--many-names,--only,-a,--single,--short,"
                    "--one",
                    "Another non-positional argument."),
                po2::argument(
                    "--non-pos-2",
                    "A second non-positional argument.  This one has a "
                    "particularly long description, just so we can see what "
                    "the "
                    "column-wrapping looks like."));
        } catch (int) {
        }
        EXPECT_EQ(
            os.str(),
            R"(usage:  bar [-h] [--non-positional NON-POSITIONAL] POSITIONAL [-a A]
            [--non-pos-2 NON-POS-2]

A test program to see how things work.

positional arguments:
  positional            A positional argument.

optional arguments:
  -h, --help            Print this help message and exit
  --non-positional      A non-positional argument.
  --an, --arg, --with, --many-names, --only, -a, --single, --short, --one
                        Another non-positional argument.
  --non-pos-2           A second non-positional argument.  This one has a 
                        particularly long description, just so we can see what 
                        the column-wrapping looks like.

response files:
  Use '@file' to load a file containing command line arguments.
)");
    }

    {
        std::string strings = std::string("foo") + po2::detail::fs_sep + "bar";
        strings += '\0';
        strings += "-r";
        strings += '\0';

        int const argc = 2;
        char const * argv[2] = {
            strings.c_str(), strings.c_str() + strings.size() - 3};

        std::ostringstream os;
        try {
            po2::parse_command_line(
                argc,
                argv,
                "A test program to see how things work.",
                os,
                user_strings(),
                po2::argument("--non-positional", "A non-positional argument."),
                po2::positional("positional", "A positional argument."),
                po2::argument(
                    "--an,--arg,--with,--many-names,--only,-a,--single,--short,"
                    "--one",
                    "Another non-positional argument."),
                po2::argument(
                    "--non-pos-2",
                    "A second non-positional argument.  This one has a "
                    "particularly long description, just so we can see what "
                    "the "
                    "column-wrapping looks like."));
        } catch (int) {
        }
        EXPECT_EQ(
            os.str(),
            R"(USAGE:  bar [-r] [--non-positional NON-POSITIONAL] POSITIONAL [-a A]
            [--non-pos-2 NON-POS-2]

A test program to see how things work.

POSITIONAL arguments:
  positional            A positional argument.

OPTIONAL arguments:
  -r, --redacted        Nothing to see here.
  --non-positional      A non-positional argument.
  --an, --arg, --with, --many-names, --only, -a, --single, --short, --one
                        Another non-positional argument.
  --non-pos-2           A second non-positional argument.  This one has a 
                        particularly long description, just so we can see what 
                        the column-wrapping looks like.

response files:
  Use '@file' to load a file containing command line arguments.
)");
    }
}

TEST(printing, response_file)
{
    {
        std::ostringstream os;
        try {
            std::vector<std::string_view> args = {"prog"};
            po2::parse_command_line(
                args,
                "A program.",
                os,
                po2::response_file("--config"),
                po2::positional("positional", "A positional argument."));
        } catch (int) {
        }
        EXPECT_EQ(
            os.str(),
            R"(error: one or more missing positional arguments, starting with 'POSITIONAL'

usage:  prog [-h] [--config CONFIG] POSITIONAL

A program.

positional arguments:
  positional  A positional argument.

optional arguments:
  -h, --help  Print this help message and exit
  --config    Load the given response file, and parse the options it contains; 
              '@file' works as well
)");
    }
    {
        std::ostringstream os;
        try {
            std::vector<std::string_view> args = {"prog"};
            po2::parse_command_line(
                args,
                "A program.",
                os,
                po2::positional("positional", "A positional argument."));
        } catch (int) {
        }
        EXPECT_EQ(
            os.str(),
            R"(error: one or more missing positional arguments, starting with 'POSITIONAL'

usage:  prog [-h] POSITIONAL

A program.

positional arguments:
  positional  A positional argument.

optional arguments:
  -h, --help  Print this help message and exit

response files:
  Use '@file' to load a file containing command line arguments.
)");
    }
    {
        po2::customizable_strings strings;
        strings.response_file_note = "";

        std::ostringstream os;
        try {
            std::vector<std::string_view> args = {"prog"};
            po2::parse_command_line(
                args,
                "A program.",
                os,
                strings,
                po2::positional("positional", "A positional argument."));
        } catch (int) {
        }
        EXPECT_EQ(
            os.str(),
            R"(error: one or more missing positional arguments, starting with 'POSITIONAL'

usage:  prog [-h] POSITIONAL

A program.

positional arguments:
  positional  A positional argument.

optional arguments:
  -h, --help  Print this help message and exit
)");
    }
}
