// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/program_options_2/parse_command_line.hpp>

#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>

#include <gtest/gtest.h>


namespace po2 = boost::program_options_2;

TEST(detail, program_name)
{
    using sv = std::string_view;

    EXPECT_EQ(po2::detail::program_name(sv("prog.exe")), "prog.exe");

    auto const sep = po2::detail::fs_sep;

    {
        std::string path = std::string("foo") + sep;
        EXPECT_EQ(po2::detail::program_name(sv(path)), "");
    }

    {
        std::string path =
            std::string("foo") + sep + "bar" + sep + "baz" + sep + "prog.exe";
        EXPECT_EQ(po2::detail::program_name(sv(path)), "prog.exe");
    }
}

TEST(detail, names_view)
{
    {
        auto const v = po2::detail::names_view("foo");
        std::vector<std::string_view> names(v.begin(), v.end());
        EXPECT_EQ(names.size(), 1u);
        EXPECT_EQ(names[0], "foo");
    }
    {
        auto const v = po2::detail::names_view(",foo,bar, baz,");
        std::vector<std::string_view> names(v.begin(), v.end());
        EXPECT_EQ(names.size(), 4u);
        EXPECT_EQ(names[0], "");
        EXPECT_EQ(names[1], "foo");
        EXPECT_EQ(names[2], "bar");
        EXPECT_EQ(names[3], " baz");
    }
}

namespace user_namespace {
    struct tag
    {};
    po2::customizable_strings help_text_customizable_strings(tag)
    {
        po2::customizable_strings retval;
        retval.help_names = "-r,--redacted";
        return retval;
    }
}

TEST(detail, misc)
{
    // first_shortest_name
    {
        EXPECT_EQ(po2::detail::first_shortest_name(",foo,bar, baz,"), " baz");
    }
    {
        EXPECT_EQ(po2::detail::first_shortest_name("--bar,-f,-b"), "-f");
    }

    // valid_nonpositional_names
    {
        EXPECT_TRUE(po2::detail::valid_nonpositional_names("-f,--bar,-b"));
    }
    {
        EXPECT_TRUE(po2::detail::valid_nonpositional_names("-f,--bar,-b"));
    }
    {
        EXPECT_FALSE(po2::detail::valid_nonpositional_names("-f,bar,-b"));
    }

    // argv_contains_default_help_flag
    {
        char const * argv[] = {"", "help"};
        EXPECT_FALSE(po2::detail::argv_contains_default_help_flag(
            po2::detail::default_strings_tag{},
            std::begin(argv),
            std::end(argv)));
    }
    {
        char const * argv[] = {"foo", "-h"};
        EXPECT_TRUE(po2::detail::argv_contains_default_help_flag(
            po2::detail::default_strings_tag{},
            std::begin(argv),
            std::end(argv)));
    }
    {
        char const * argv[] = {"foo", "--help"};
        EXPECT_TRUE(po2::detail::argv_contains_default_help_flag(
            po2::detail::default_strings_tag{},
            std::begin(argv),
            std::end(argv)));
    }
    // user-customized strings
    {
        char const * argv[] = {"", "redacted"};
        EXPECT_FALSE(po2::detail::argv_contains_default_help_flag(
            user_namespace::tag{}, std::begin(argv), std::end(argv)));
    }
    {
        char const * argv[] = {"foo", "-r"};
        EXPECT_TRUE(po2::detail::argv_contains_default_help_flag(
            user_namespace::tag{}, std::begin(argv), std::end(argv)));
    }
    {
        char const * argv[] = {"foo", "--redacted"};
        EXPECT_TRUE(po2::detail::argv_contains_default_help_flag(
            user_namespace::tag{}, std::begin(argv), std::end(argv)));
    }
}
