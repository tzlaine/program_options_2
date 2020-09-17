// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#define BOOST_PROGRAM_OPTIONS_2_TESTING
#include <boost/program_options_2/parse_command_line.hpp>

#include "ill_formed.hpp"

#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>

#include <gtest/gtest.h>


namespace po2 = boost::program_options_2;
using boost::is_same;

template<typename Option, typename Default>
using add_default = decltype(
    po2::with_default(std::declval<Option>(), std::declval<Default>()));

template<typename T>
using argument_with_mixed_choices =
    decltype(po2::argument<T>("-b,--blah", 1, std::vector<int>{}, 2, 3));
static_assert(ill_formed<argument_with_mixed_choices, int>::value);
static_assert(ill_formed<argument_with_mixed_choices, std::vector<int>>::value);

TEST(options, arguments)
{
    // no defaults
    {
        auto const arg = po2::argument<int>("-b,--blah", "bleurgh");
        EXPECT_EQ(arg.names, "-b,--blah");
        EXPECT_EQ(arg.help_text, "bleurgh");
        EXPECT_EQ(arg.args, 1);
        EXPECT_EQ(arg.action, po2::detail::action_kind::assign);
        BOOST_MPL_ASSERT((is_same<decltype(arg.default_value), po2::no_value>));
        EXPECT_EQ(arg.choices.size(), 0);
        EXPECT_EQ(arg.arg_display_name, "");
    }
    {
        auto const arg = po2::argument<std::optional<int>>(
            "-b,--blah", "bleurgh", po2::zero_or_one);
        EXPECT_EQ(arg.names, "-b,--blah");
        EXPECT_EQ(arg.help_text, "bleurgh");
        EXPECT_EQ(arg.args, po2::zero_or_one);
        EXPECT_EQ(arg.action, po2::detail::action_kind::assign);
        BOOST_MPL_ASSERT((is_same<decltype(arg.default_value), po2::no_value>));
        EXPECT_EQ(arg.choices.size(), 0);
        EXPECT_EQ(arg.arg_display_name, "");
    }
    {
        auto const arg =
            po2::argument<std::vector<int>>("-b,--blah", "bleurgh", 2);
        EXPECT_EQ(arg.names, "-b,--blah");
        EXPECT_EQ(arg.help_text, "bleurgh");
        EXPECT_EQ(arg.args, 2);
        EXPECT_EQ(arg.action, po2::detail::action_kind::insert);
        BOOST_MPL_ASSERT((is_same<decltype(arg.default_value), po2::no_value>));
        EXPECT_EQ(arg.choices.size(), 0);
        EXPECT_EQ(arg.arg_display_name, "");
    }
    {
        auto const arg = po2::argument<int>("-b,--blah", "bleurgh", 1, 1, 2, 3);
        EXPECT_EQ(arg.names, "-b,--blah");
        EXPECT_EQ(arg.help_text, "bleurgh");
        EXPECT_EQ(arg.args, 1);
        EXPECT_EQ(arg.action, po2::detail::action_kind::assign);
        BOOST_MPL_ASSERT((is_same<decltype(arg.default_value), po2::no_value>));
        EXPECT_EQ(arg.choices, (std::array<int, 3>{{1, 2, 3}}));
        EXPECT_EQ(arg.arg_display_name, "");
    }

    // with defaults
    {
        auto const arg =
            po2::with_default(po2::argument<int>("-b,--blah", "bleurgh"), 42);
        EXPECT_EQ(arg.names, "-b,--blah");
        EXPECT_EQ(arg.help_text, "bleurgh");
        EXPECT_EQ(arg.args, 1);
        EXPECT_EQ(arg.action, po2::detail::action_kind::assign);
        EXPECT_EQ(arg.default_value, 42);
        EXPECT_EQ(arg.choices.size(), 0);
        EXPECT_EQ(arg.arg_display_name, "");
    }
    {
        auto const arg = po2::with_default(
            po2::argument<std::optional<int>>(
                "-b,--blah", "bleurgh", po2::zero_or_one),
            42);
        EXPECT_EQ(arg.names, "-b,--blah");
        EXPECT_EQ(arg.help_text, "bleurgh");
        EXPECT_EQ(arg.args, po2::zero_or_one);
        EXPECT_EQ(arg.action, po2::detail::action_kind::assign);
        EXPECT_EQ(arg.default_value, 42);
        EXPECT_EQ(arg.choices.size(), 0);
        EXPECT_EQ(arg.arg_display_name, "");
    }
    {
        auto const arg = po2::with_default(
            po2::argument<std::vector<int>>("-b,--blah", "bleurgh", 2),
            std::vector<int>({42}));
        EXPECT_EQ(arg.names, "-b,--blah");
        EXPECT_EQ(arg.help_text, "bleurgh");
        EXPECT_EQ(arg.args, 2);
        EXPECT_EQ(arg.action, po2::detail::action_kind::insert);
        EXPECT_EQ(arg.default_value, std::vector<int>({42}));
        EXPECT_EQ(arg.choices.size(), 0);
        EXPECT_EQ(arg.arg_display_name, "");
    }
    {
        auto const arg = po2::with_default(
            po2::argument<std::vector<int>>("-b,--blah", "bleurgh", 2), 42);
        EXPECT_EQ(arg.names, "-b,--blah");
        EXPECT_EQ(arg.help_text, "bleurgh");
        EXPECT_EQ(arg.args, 2);
        EXPECT_EQ(arg.action, po2::detail::action_kind::insert);
        EXPECT_EQ(arg.default_value, 42);
        EXPECT_EQ(arg.choices.size(), 0);
        EXPECT_EQ(arg.arg_display_name, "");
    }
    {
        auto const arg = po2::with_default(
            po2::argument<int>("-b,--blah", "bleurgh", 1, 1, 2, 3), 3);
        EXPECT_EQ(arg.names, "-b,--blah");
        EXPECT_EQ(arg.help_text, "bleurgh");
        EXPECT_EQ(arg.args, 1);
        EXPECT_EQ(arg.action, po2::detail::action_kind::assign);
        EXPECT_EQ(arg.default_value, 3);
        EXPECT_EQ(arg.choices, (std::array<int, 3>{{1, 2, 3}}));
        EXPECT_EQ(arg.arg_display_name, "");
    }

    // add a display name
    {
        auto const arg = po2::with_display_name(
            po2::argument<int>("-b,--blah", "bleurgh"), "blerg");
        EXPECT_EQ(arg.names, "-b,--blah");
        EXPECT_EQ(arg.help_text, "bleurgh");
        EXPECT_EQ(arg.args, 1);
        EXPECT_EQ(arg.action, po2::detail::action_kind::assign);
        BOOST_MPL_ASSERT((is_same<decltype(arg.default_value), po2::no_value>));
        EXPECT_EQ(arg.choices.size(), 0);
        EXPECT_EQ(arg.arg_display_name, "blerg");
    }
}

TEST(options, positionals)
{
    // TODO
}

TEST(options, help)
{
    auto help_func = [] { return ""; };

    {
        auto const arg = po2::help(help_func);
        EXPECT_EQ(arg.names, "--help,-h");
        EXPECT_EQ(arg.help_text, "Print this help message and exit");
        EXPECT_EQ(arg.args, 0);
        EXPECT_EQ(arg.action, po2::detail::action_kind::help);
        BOOST_MPL_ASSERT_NOT((is_same<decltype(arg.default_value), po2::no_value>));
        EXPECT_EQ(arg.choices.size(), 0);
        EXPECT_EQ(arg.arg_display_name, "");
    }
    {
        auto const arg = po2::help(help_func, "-h");
        EXPECT_EQ(arg.names, "-h");
        EXPECT_EQ(arg.help_text, "Print this help message and exit");
        EXPECT_EQ(arg.args, 0);
        EXPECT_EQ(arg.action, po2::detail::action_kind::help);
        BOOST_MPL_ASSERT_NOT((is_same<decltype(arg.default_value), po2::no_value>));
        EXPECT_EQ(arg.choices.size(), 0);
        EXPECT_EQ(arg.arg_display_name, "");
    }
    {
        auto const arg =
            po2::help(help_func, "-m", "Print this melp message and mexit");
        EXPECT_EQ(arg.names, "-m");
        EXPECT_EQ(arg.help_text, "Print this melp message and mexit");
        EXPECT_EQ(arg.args, 0);
        EXPECT_EQ(arg.action, po2::detail::action_kind::help);
        BOOST_MPL_ASSERT_NOT((is_same<decltype(arg.default_value), po2::no_value>));
        EXPECT_EQ(arg.choices.size(), 0);
        EXPECT_EQ(arg.arg_display_name, "");
    }

    {
        auto const arg = po2::help("-f");
        EXPECT_EQ(arg.names, "-f");
        EXPECT_EQ(arg.help_text, "Print this help message and exit");
        EXPECT_EQ(arg.args, 0);
        EXPECT_EQ(arg.action, po2::detail::action_kind::help);
        BOOST_MPL_ASSERT((is_same<decltype(arg.default_value), po2::no_value>));
        EXPECT_EQ(arg.choices.size(), 0);
        EXPECT_EQ(arg.arg_display_name, "");
    }
    {
        auto const arg = po2::help("-m", "Print this melp message and mexit");
        EXPECT_EQ(arg.names, "-m");
        EXPECT_EQ(arg.help_text, "Print this melp message and mexit");
        EXPECT_EQ(arg.args, 0);
        EXPECT_EQ(arg.action, po2::detail::action_kind::help);
        BOOST_MPL_ASSERT((is_same<decltype(arg.default_value), po2::no_value>));
        EXPECT_EQ(arg.choices.size(), 0);
        EXPECT_EQ(arg.arg_display_name, "");
    }

    {
        std::string strings = std::string("foo") + po2::detail::fs_sep + "bar";
        strings += '\0';
        strings += "-f";
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
                po2::help("-f,--foo", "Print this foo message and exit"),
                po2::argument("--non-positional", "A non-positional argument."),
                po2::argument(
                    "--non-pos-2", "A second non-positional argument."));
        } catch (int) {
        }
        EXPECT_EQ(
            os.str(),
            R"(usage:  bar [-f] [--non-positional NON-POSITIONAL] [--non-pos-2 NON-POS-2]

A test program to see how things work.

optional arguments:
  -f, --foo         Print this foo message and exit
  --non-positional  A non-positional argument.
  --non-pos-2       A second non-positional argument.
)");
    }

    {
        std::string strings = std::string("foo") + po2::detail::fs_sep + "bar";
        strings += '\0';
        strings += "-f";
        strings += '\0';

        int const argc = 2;
        char const * argv[2] = {
            strings.c_str(), strings.c_str() + strings.size() - 3};

        std::ostringstream os;
        auto help_func = [&] { return "Custom help text goes here.\n"; };
        try {
            po2::parse_command_line(
                argc,
                argv,
                "A test program to see how things work.",
                os,
                po2::help(
                    help_func, "-f,--foo", "Print this foo message and exit"),
                po2::argument("--non-positional", "A non-positional argument."),
                po2::argument(
                    "--non-pos-2", "A second non-positional argument."));
        } catch (int) {
        }
        EXPECT_EQ(os.str(), "Custom help text goes here.\n");
    }
}

TEST(options, version)
{
    {
        auto const arg = po2::version("Foo version 1.2.3.\n");
        EXPECT_EQ(arg.names, "--version,-v");
        EXPECT_EQ(arg.help_text, "Print the version and exit");
        EXPECT_EQ(arg.args, 0);
        EXPECT_EQ(arg.action, po2::detail::action_kind::version);
        EXPECT_EQ(arg.default_value, "Foo version 1.2.3.\n");
        EXPECT_EQ(arg.choices.size(), 0);
        EXPECT_EQ(arg.arg_display_name, "");
    }
    {
        auto const arg = po2::version("Foo version 1.2.3.\n", "-f");
        EXPECT_EQ(arg.names, "-f");
        EXPECT_EQ(arg.help_text, "Print the version and exit");
        EXPECT_EQ(arg.args, 0);
        EXPECT_EQ(arg.action, po2::detail::action_kind::version);
        EXPECT_EQ(arg.default_value, "Foo version 1.2.3.\n");
        EXPECT_EQ(arg.choices.size(), 0);
        EXPECT_EQ(arg.arg_display_name, "");
    }
    {
        auto const arg = po2::version(
            "Foo version 1.2.3.\n", "-f", "Print the fersion and fexit");
        EXPECT_EQ(arg.names, "-f");
        EXPECT_EQ(arg.help_text, "Print the fersion and fexit");
        EXPECT_EQ(arg.args, 0);
        EXPECT_EQ(arg.action, po2::detail::action_kind::version);
        EXPECT_EQ(arg.default_value, "Foo version 1.2.3.\n");
        EXPECT_EQ(arg.choices.size(), 0);
        EXPECT_EQ(arg.arg_display_name, "");
    }

    {
        std::string strings = std::string("foo") + po2::detail::fs_sep + "bar";
        strings += '\0';
        strings += "-v";
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
                po2::version("Foo version 1.2.3.\n"),
                po2::argument("--non-positional", "A non-positional argument."),
                po2::argument(
                    "--non-pos-2", "A second non-positional argument."));
        } catch (int) {
        }
        EXPECT_EQ(os.str(), "Foo version 1.2.3.\n");
    }
}

TEST(options, others)
{
    // TODO
}
