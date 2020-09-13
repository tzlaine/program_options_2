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
using sv = std::string_view;

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
            po2::positional<std::vector<int>>("foo", 2),
            true);
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

struct my_int
{
    my_int() = default;
    my_int(int i) : value_(i) {}
    int value_;
};

TEST(printing, print_option_arguments)
{
    // no defaults
    {
        auto const arg = po2::argument<int>("-b,--blah");
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b B]");
    }
    {
        auto const arg = po2::argument<std::vector<int>>("-b,--blah", 2);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b B B]");
    }
    {
        auto const arg = po2::argument<int>("--blah", po2::zero_or_one);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [--blah [BLAH]]");
    }
    {
        auto const arg =
            po2::argument<std::vector<int>>("-b,--blah", po2::zero_or_more);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b [B ...]]");
    }
    {
        auto const arg =
            po2::argument<std::vector<int>>("-b,--blah", po2::one_or_more);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b B ...]");
    }
    {
        auto const arg =
            po2::argument<std::vector<int>>("-b,--blah", po2::remainder);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b [B ...]]");
    }

    // unprintable choice type
    {
        auto const arg = po2::argument<my_int>(
            "-b,--blah", 1, my_int{1}, my_int{2}, my_int{3});
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b B]");
    }
    // printable choices
    {
        auto const arg = po2::argument<int>("-b,--blah", 1, 1, 2, 3);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b {1,2,3}]");
    }
    {
        auto const arg =
            po2::argument<std::vector<int>>("-b,--blah", 2, 1, 2, 3);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b {1,2,3} {1,2,3}]");
    }
    {
        auto const arg =
            po2::argument<int>("-b,--blah", po2::zero_or_one, 1, 2, 3);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b [{1,2,3}]]");
    }
    {
        auto const arg = po2::argument<std::vector<int>>(
            "-b,--blah", po2::zero_or_more, 1, 2, 3);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b [{1,2,3} ...]]");
    }
    {
        auto const arg = po2::argument<std::vector<int>>(
            "-b,--blah", po2::one_or_more, 1, 2, 3);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b {1,2,3} ...]");
    }
    {
        auto const arg = po2::argument<std::vector<int>>(
            "-b,--blah", po2::remainder, 1, 2, 3);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b [{1,2,3} ...]]");
    }

    // with defaults
    {
        auto const arg = po2::with_default(po2::argument<int>("-b,--blah"), 42);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b B]");
    }
    {
        auto const arg = po2::with_default(
            po2::argument<int>("-b,--blah", po2::zero_or_one), 42);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b [B]]");
    }
    {
        auto const arg = po2::with_default(
            po2::argument<std::vector<int>>("-b,--blah", 2),
            std::vector<int>({42}));
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b B B]");
    }
    {
        auto const arg = po2::with_default(
            po2::argument<std::vector<int>>("-b,--blah", 2), 42);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b B B]");
    }
    {
        auto const arg =
            po2::with_default(po2::argument<int>("-b,--blah", 1, 1, 2, 3), 3);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b {1,2,3}]");
    }

    // add a display name
    {
        auto const arg =
            po2::with_display_name(po2::argument<int>("-b,--blah"), "blerg");
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [-b blerg]");
    }
}

TEST(printing, print_option_positionals)
{
    // no defaults
    {
        auto const arg = po2::positional<int>("blah");
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " BLAH");
    }
    {
        auto const arg = po2::positional<std::vector<int>>("blah", 2);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " BLAH BLAH");
    }
    {
        auto const arg = po2::positional<int>("blah", po2::zero_or_one);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [BLAH]");
    }
    {
        auto const arg =
            po2::positional<std::vector<int>>("blah", po2::zero_or_more);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [BLAH ...]");
    }
    {
        auto const arg =
            po2::positional<std::vector<int>>("blah", po2::one_or_more);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " BLAH ...");
    }
    {
        auto const arg =
            po2::positional<std::vector<int>>("blah", po2::remainder);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [BLAH ...]");
    }

    // unprintable choice type
    {
        auto const arg =
            po2::positional<my_int>("blah", 1, my_int{1}, my_int{2}, my_int{3});
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " BLAH");
    }
    // printable choices
    {
        auto const arg = po2::positional<int>("blah", 1, 1, 2, 3);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " {1,2,3}");
    }
    {
        auto const arg = po2::positional<std::vector<int>>("blah", 2, 1, 2, 3);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " {1,2,3} {1,2,3}");
    }
    {
        auto const arg =
            po2::positional<int>("blah", po2::zero_or_one, 1, 2, 3);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [{1,2,3}]");
    }
    {
        auto const arg = po2::positional<std::vector<int>>(
            "blah", po2::zero_or_more, 1, 2, 3);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [{1,2,3} ...]");
    }
    {
        auto const arg = po2::positional<std::vector<int>>(
            "blah", po2::one_or_more, 1, 2, 3);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " {1,2,3} ...");
    }
    {
        auto const arg =
            po2::positional<std::vector<int>>("blah", po2::remainder, 1, 2, 3);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [{1,2,3} ...]");
    }

    // with defaults
    {
        auto const arg = po2::with_default(po2::positional<int>("blah"), 42);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " BLAH");
    }
    {
        auto const arg = po2::with_default(
            po2::positional<int>("blah", po2::zero_or_one), 42);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " [BLAH]");
    }
    {
        auto const arg = po2::with_default(
            po2::positional<std::vector<int>>("blah", 2),
            std::vector<int>({42}));
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " BLAH BLAH");
    }
    {
        auto const arg =
            po2::with_default(po2::positional<std::vector<int>>("blah", 2), 42);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " BLAH BLAH");
    }
    {
        auto const arg =
            po2::with_default(po2::positional<int>("blah", 1, 1, 2, 3), 3);
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " {1,2,3}");
    }

    // add a display name
    {
        auto const arg =
            po2::with_display_name(po2::positional<int>("blah"), "blerg");
        std::ostringstream os;
        po2::detail::print_option<boost::text::format::utf8>(os, arg, 8, 8);
        EXPECT_EQ(os.str(), " blerg");
    }
}

TEST(printing, detail_print_help_synopsis)
{
    std::string const exe = std::string("foo") + po2::detail::fs_sep + "bar";

    {
        std::ostringstream os;
        po2::detail::print_help_synopsis<boost::text::format::utf8>(
            std::cout,
            sv(exe),
            sv("A program that does things."),
            po2::positional<int>("foo"));
        // EXPECT_EQ(os.str(), " FOO");
    }

    {
        std::ostringstream os;
        po2::detail::print_help_synopsis<boost::text::format::utf8>(
            std::cout,
            sv(exe),
            sv("A program that does things."),
            po2::positional<std::vector<int>>("foo", 30));
        // EXPECT_EQ(os.str(), " FOO");
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
        // EXPECT_EQ(os.str(), " FOO");
    }
}

// TODO
