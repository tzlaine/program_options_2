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

TEST(printing, detail_print_uppercase)
{
    {
        std::ostringstream os;
        po2::detail::print_uppercase<boost::text::format::utf8>(
            os, std::string_view("this should end up screaming."));
        EXPECT_EQ(os.str(), "THIS SHOULD END UP SCREAMING.");
    }
    {
        std::ostringstream os;
        po2::detail::print_uppercase<boost::text::format::utf8>(
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
        po2::detail::print_args<boost::text::format::utf8>(
            os, std::string_view("foo"), po2::positional<int>("foo"), true);
        EXPECT_EQ(os.str(), " FOO");
    }
    {
        std::ostringstream os;
        po2::detail::print_args<boost::text::format::utf8>(
            os,
            std::string_view("foo"),
            po2::positional<std::vector<int>>("foo", 2), true);
        EXPECT_EQ(os.str(), " FOO FOO");
    }
    {
        std::ostringstream os;
        po2::detail::print_args<boost::text::format::utf8>(
            os,
            std::string_view("foo"),
            po2::positional<std::vector<int>>("foo", po2::one_or_more),
            false);
        EXPECT_EQ(os.str(), "FOO ...");
    }
    {
        std::ostringstream os;
        po2::detail::print_args<boost::text::format::utf8>(
            os,
            std::string_view("foo"),
            po2::with_display_name(
                po2::positional<std::vector<int>>("foo", 2), "bar"),
            true);
        EXPECT_EQ(os.str(), " bar bar");
    }
    {
        std::ostringstream os;
        po2::detail::print_args<boost::text::format::utf8>(
            os,
            std::string_view("foo"),
            po2::with_display_name(
                po2::positional<std::vector<int>>("foo", po2::one_or_more),
                "bar"),
            false);
        EXPECT_EQ(os.str(), "bar ...");
    }
}

TEST(printing, detail_print_help_synopsis)
{
    std::string const exe = std::string("foo") + po2::detail::fs_sep + "bar";
    using sv = std::string_view;

    {
        std::ostringstream os;
        po2::detail::print_help_synopsis<boost::text::format::utf8>(
            std::cout,
            sv(exe),
            sv("A program that does things."),
            po2::positional<int>("foo"));
        //EXPECT_EQ(os.str(), " FOO");
    }

    {
        std::ostringstream os;
        po2::detail::print_help_synopsis<boost::text::format::utf8>(
            std::cout,
            sv(exe),
            sv("A program that does things."),
            po2::positional<std::vector<int>>("foo", 30));
        //EXPECT_EQ(os.str(), " FOO");
    }

    {
        std::string const long_exe = std::string("foo") + po2::detail::fs_sep +
                                     "barrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr";
        std::ostringstream os;
        po2::detail::print_help_synopsis<boost::text::format::utf8>(
            std::cout,
            sv(long_exe),
            sv("A program that does things."),
            po2::positional<std::vector<int>>("foo", 30));
        //EXPECT_EQ(os.str(), " FOO");
    }
}

// TODO
