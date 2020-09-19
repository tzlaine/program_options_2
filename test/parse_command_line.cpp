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
using boost::hana::tuple;
template<typename T>
using opt = std::optional<T>;
using namespace boost::hana::literals;

po2::customizable_strings user_strings()
{
    po2::customizable_strings retval;
    retval.usage_text = "USAGE: ";
    retval.positional_section_text = "POSITIONAL arguments:";
    retval.optional_section_text = "OPTIONAL arguments:";
    retval.help_names = "-r,--redacted";
    retval.help_description = "Nothing to see here.";
    return retval;
}

TEST(parse_command_line, api)
{
    std::string arg_string = std::string("foo") + po2::detail::fs_sep + "bar";
    arg_string += '\0';
    arg_string += "jj";
    arg_string += '\0';

    int const argc = 2;
    char const * argv[2] = {
        arg_string.c_str(), arg_string.c_str() + arg_string.size() - 3};

#if defined(_MSC_VER)
    std::wstring warg_string =
        std::wstring(u"foo") + wchar_t(po2::detail::fs_sep) + L"bar";
    warg_string += L'\0';
    warg_string += L"jj";
    warg_string += L'\0';

    int const wargc = 2;
    wchar_t const * wargv[2] = {
        warg_string.c_str(), warg_string.c_str() + warg_string.size() - 3};
#endif

    // arg-view, no user strings
    {
        std::ostringstream os;
        auto result = po2::parse_command_line(
            po2::arg_view(argc, argv),
            "A program.",
            os,
            po2::positional("pos", "Positional."));
        BOOST_MPL_ASSERT((is_same<decltype(result), tuple<std::string_view>>));
        EXPECT_EQ(result[0_c], "jj");
    }
    {
        std::ostringstream os;
        auto const arg_view = po2::arg_view(argc, argv);
        std::vector<std::string_view> args(arg_view.begin(), arg_view.end());
        auto result = po2::parse_command_line(
            args, "A program.", os, po2::positional("pos", "Positional."));
        BOOST_MPL_ASSERT((is_same<decltype(result), tuple<std::string_view>>));
        EXPECT_EQ(result[0_c], "jj");
    }
#if defined(_MSC_VER)
    {
        std::wostringstream os;
        auto result = po2::parse_command_line(
            po2::arg_view(wargc, wargv),
            "A program.",
            os,
            po2::positional("pos", "Positional."));
        BOOST_MPL_ASSERT((is_same<decltype(result), tuple<std::wstring_view>>));
        EXPECT_EQ(result[0_c], L"jj");
    }
#endif

    // arg-view, with user strings
    {
        std::ostringstream os;
        auto result = po2::parse_command_line(
            po2::arg_view(argc, argv),
            "A program.",
            os,
            user_strings(),
            po2::positional("pos", "Positional."));
        BOOST_MPL_ASSERT((is_same<decltype(result), tuple<std::string_view>>));
        EXPECT_EQ(result[0_c], "jj");
    }
    {
        std::ostringstream os;
        auto const arg_view = po2::arg_view(argc, argv);
        std::vector<std::string_view> args(arg_view.begin(), arg_view.end());
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            user_strings(),
            po2::positional("pos", "Positional."));
        BOOST_MPL_ASSERT((is_same<decltype(result), tuple<std::string_view>>));
        EXPECT_EQ(result[0_c], "jj");
    }
#if defined(_MSC_VER)
    {
        std::wostringstream os;
        auto result = po2::parse_command_line(
            po2::arg_view(wargc, wargv),
            "A program.",
            os,
            user_strings(),
            po2::positional("pos", "Positional."));
        BOOST_MPL_ASSERT((is_same<decltype(result), tuple<std::wstring_view>>));
        EXPECT_EQ(result[0_c], L"jj");
    }
#endif

    // argc/argv, no user strings
    {
        std::ostringstream os;
        auto result = po2::parse_command_line(
            argc,
            argv,
            "A program.",
            os,
            po2::positional("pos", "Positional."));
        BOOST_MPL_ASSERT((is_same<decltype(result), tuple<std::string_view>>));
        EXPECT_EQ(result[0_c], "jj");
    }
#if defined(_MSC_VER)
    {
        std::wostringstream os;
        auto result = po2::parse_command_line(
            wargc,
            wargv,
            "A program.",
            os,
            po2::positional("pos", "Positional."));
        BOOST_MPL_ASSERT((is_same<decltype(result), tuple<std::wstring_view>>));
        EXPECT_EQ(result[0_c], L"jj");
    }
#endif

    // argc/argv, with user strings
    {
        std::ostringstream os;
        auto result = po2::parse_command_line(
            argc,
            argv,
            "A program.",
            os,
            user_strings(),
            po2::positional("pos", "Positional."));
        BOOST_MPL_ASSERT((is_same<decltype(result), tuple<std::string_view>>));
        EXPECT_EQ(result[0_c], "jj");
    }
#if defined(_MSC_VER)
    {
        std::wostringstream os;
        auto result = po2::parse_command_line(
            wargc,
            wargv,
            "A program.",
            os,
            user_strings(),
            po2::positional("pos", "Positional."));
        BOOST_MPL_ASSERT((is_same<decltype(result), tuple<std::wstring_view>>));
        EXPECT_EQ(result[0_c], L"jj");
    }
#endif
}

#define ARGUMENTS(T, choice0, choice1, choice2)                                \
    po2::argument<T>("-a,--abacus", "The abacus."),                            \
        po2::argument<std::optional<T>>(                                       \
            "-b,--bobcat", "The bobcat.", po2::zero_or_one),                   \
        po2::argument<std::vector<T>>("-c,--cataphract", "The cataphract", 2), \
        po2::argument<T>(                                                      \
            "-d,--dolemite", "*The* Dolemite.", 1, choice0, choice1, choice2)

#define ARGUMENTS_WITH_DEFAULTS(T, choice0, choice1, choice2, default_)        \
    po2::with_default(                                                         \
        po2::argument<T>("-a,--abacus", "The abacus."), default_),             \
        po2::with_default(                                                     \
            po2::argument<std::optional<T>>(                                   \
                "-b,--bobcat", "The bobcat.", po2::zero_or_one),               \
            default_),                                                         \
        po2::with_default(                                                     \
            po2::argument<std::vector<T>>(                                     \
                "-c,--cataphract", "The cataphract", 2),                       \
            std::vector<T>({default_})),                                       \
        po2::with_default(                                                     \
            po2::argument<T>(                                                  \
                "-d,--dolemite",                                               \
                "*The* Dolemite.",                                             \
                1,                                                             \
                choice0,                                                       \
                choice1,                                                       \
                choice2),                                                      \
            choice2)

TEST(parse_command_line, arguments)
{
    // TODO
    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog",
            "-a",
            "55",
            "--bobcat",
            "66",
            "-c",
            "77",
            "88",
            "--dolemite",
            "5"};
        auto result = po2::parse_command_line(
            args, "A program.", os, ARGUMENTS(int, 4, 5, 6));
        BOOST_MPL_ASSERT((is_same<
                          decltype(result),
                          tuple<
                              opt<int>,
                              opt<opt<int>>,
                              opt<std::vector<int>>,
                              opt<int>>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(*result[0_c], 55);
        EXPECT_TRUE(result[1_c]);
        EXPECT_EQ(*result[1_c], 66);
        EXPECT_TRUE(result[2_c]);
        EXPECT_EQ(*result[2_c], std::vector<int>({77, 88}));
        EXPECT_TRUE(result[3_c]);
        EXPECT_EQ(*result[3_c], 5);
    }
    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog", "--bobcat", "66", "-c", "77", "88", "--dolemite", "5"};
        auto result = po2::parse_command_line(
            args, "A program.", os, ARGUMENTS_WITH_DEFAULTS(int, 4, 5, 6, 42));
        BOOST_MPL_ASSERT((is_same<
                          decltype(result),
                          tuple<
                              opt<int>,
                              opt<opt<int>>,
                              opt<std::vector<int>>,
                              opt<int>>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(*result[0_c], 42);
        EXPECT_TRUE(result[1_c]);
        EXPECT_EQ(*result[1_c], 66);
        EXPECT_TRUE(result[2_c]);
        EXPECT_EQ(*result[2_c], std::vector<int>({77, 88}));
        EXPECT_TRUE(result[3_c]);
        EXPECT_EQ(*result[3_c], 5);
    }
}

#undef ARGUMENTS
#undef ARGUMENTS_WITH_DEFAULT

TEST(parse_command_line, positionals)
{
    // TODO
}

TEST(parse_command_line, mixed)
{
    // TODO
}
