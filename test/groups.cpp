// Copyright (C) 2022 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#define BOOST_PROGRAM_OPTIONS_2_TESTING
#include <boost/program_options_2/option_groups.hpp>
#include <boost/program_options_2/decorators.hpp>
#include <boost/program_options_2/parse_command_line.hpp>

#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>

#include <gtest/gtest.h>


namespace po2 = boost::program_options_2;
using boost::is_same;
using boost::hana::tuple;
template<typename T>
using opt = std::optional<T>;
using namespace boost::hana::literals;

auto const arg1 = po2::argument<int>("-a,--apple", "Number of apples");
auto const arg2 =
    po2::argument<double>("-b,--branch", "Number of branchs", 1, 1, 2, 3);
auto const pos1 = po2::positional<float>("cats", "Number of cats");
auto const pos2 = po2::with_display_name(
    po2::positional<std::string>("dog", "Name of dog"), "Doggo-name");
auto const arg3 = po2::argument<short>("-e", "Number of e's", 1, 1, 2, 3);

// TODO: Test printing for all of the below (in the printing test).

TEST(groups, exclusive)
{
    auto exclusive = po2::exclusive(arg1, arg2, arg3);

#if 0 // TODO: Test that this fails.
    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog", "-a", "55", "--branch", "2", "-e", "3"};
        auto result =
            po2::parse_command_line(args, "A program.", os, exclusive);
        BOOST_MPL_ASSERT((is_same<
                          decltype(result),
                          tuple<std::variant<int, double, short>>>));
    }
#endif
    {
        std::ostringstream os;
        std::vector<std::string_view> args{"prog", "-e", "3"};
        auto result =
            po2::parse_command_line(args, "A program.", os, exclusive);
        BOOST_MPL_ASSERT((is_same<
                          decltype(result),
                          tuple<std::variant<int, double, short>>>));
        EXPECT_EQ(result[0_c].index(), 2);
        EXPECT_EQ(std::get<short>(result[0_c]), 3);
    }
}

#if 0
TEST(groups, command)
{
    {
        auto command = po2::command("cmd", arg1, arg2, arg3);
        std::vector<std::string_view> args{
            "prog", "cmd", "-a", "55", "--branch", "2", "-e", "3"};
#if 0
        std::ostringstream os;
        auto result = po2::parse_command_line(args, "A program.", os, command);
#endif
    }
}
#endif

TEST(groups, group)
{
    {
        auto group = po2::group(pos1, pos2, arg1, arg2, arg3);

        auto opt_tuple = po2::detail::make_opt_tuple(group);
        BOOST_MPL_ASSERT((is_same<
                          decltype(opt_tuple),
                          tuple<
                              std::decay_t<decltype(pos1)>,
                              std::decay_t<decltype(pos2)>,
                              std::decay_t<decltype(arg1)>,
                              std::decay_t<decltype(arg2)>,
                              std::decay_t<decltype(arg3)>>>));

        std::vector<std::string_view> args{
            "prog", "7", "11", "-a", "55", "--branch", "2", "-e", "3"};

        {
            std::ostringstream os;
            auto result =
                po2::parse_command_line(args, "A program.", os, group);
            BOOST_MPL_ASSERT((is_same<
                              decltype(result),
                              tuple<
                                  float,
                                  std::string,
                                  opt<int>,
                                  opt<double>,
                                  opt<short>>>));

            EXPECT_EQ(result[0_c], 7.0f);
            EXPECT_EQ(result[1_c], "11");
            EXPECT_TRUE(result[2_c]);
            EXPECT_EQ(result[2_c], 55);
            EXPECT_TRUE(result[3_c]);
            EXPECT_EQ(result[3_c], 2.0);
            EXPECT_TRUE(result[4_c]);
            EXPECT_EQ(result[4_c], 3);
        }
        {
            std::ostringstream os;
            po2::string_view_any_map result;
            po2::parse_command_line(args, result, "A program.", os, group);
            EXPECT_EQ(std::any_cast<float>(result["cats"]), 7.0f);
            EXPECT_EQ(std::any_cast<std::string>(result["dog"]), "11");
            EXPECT_EQ(std::any_cast<int>(result["apple"]), 55);
            EXPECT_EQ(std::any_cast<double>(result["branch"]), 2.0);
            EXPECT_EQ(std::any_cast<short>(result["e"]), 3);
        }
    }
    {
        auto subgroup1 = po2::group(pos1, pos2);
        auto subgroup2 = po2::group(arg1, arg2, arg3);
        auto group = po2::group(subgroup1, subgroup2);

        auto opt_tuple = po2::detail::make_opt_tuple(group);
        BOOST_MPL_ASSERT((is_same<
                          decltype(opt_tuple),
                          tuple<
                              std::decay_t<decltype(pos1)>,
                              std::decay_t<decltype(pos2)>,
                              std::decay_t<decltype(arg1)>,
                              std::decay_t<decltype(arg2)>,
                              std::decay_t<decltype(arg3)>>>));

        std::vector<std::string_view> args{
            "prog", "7", "11", "-a", "55", "--branch", "2", "-e", "3"};
        {
            std::ostringstream os;
            auto result =
                po2::parse_command_line(args, "A program.", os, group);
            BOOST_MPL_ASSERT((is_same<
                              decltype(result),
                              tuple<
                                  float,
                                  std::string,
                                  opt<int>,
                                  opt<double>,
                                  opt<short>>>));

            EXPECT_EQ(result[0_c], 7.0f);
            EXPECT_EQ(result[1_c], "11");
            EXPECT_TRUE(result[2_c]);
            EXPECT_EQ(result[2_c], 55);
            EXPECT_TRUE(result[3_c]);
            EXPECT_EQ(result[3_c], 2.0);
            EXPECT_TRUE(result[4_c]);
            EXPECT_EQ(result[4_c], 3);
        }
        {
            std::ostringstream os;
            po2::string_view_any_map result;
            po2::parse_command_line(args, result, "A program.", os, group);
            EXPECT_EQ(std::any_cast<float>(result["cats"]), 7.0f);
            EXPECT_EQ(std::any_cast<std::string>(result["dog"]), "11");
            EXPECT_EQ(std::any_cast<int>(result["apple"]), 55);
            EXPECT_EQ(std::any_cast<double>(result["branch"]), 2.0);
            EXPECT_EQ(std::any_cast<short>(result["e"]), 3);
        }
    }
    {
        auto subgroup = po2::group(pos1, pos2);
        auto group = po2::group(subgroup, arg1);

        auto opt_tuple = po2::detail::make_opt_tuple(group);
        BOOST_MPL_ASSERT((is_same<
                          decltype(opt_tuple),
                          tuple<
                              std::decay_t<decltype(pos1)>,
                              std::decay_t<decltype(pos2)>,
                              std::decay_t<decltype(arg1)>>>));

        std::vector<std::string_view> args{"prog", "7", "11", "-a", "55"};
        {
            std::ostringstream os;
            auto result =
                po2::parse_command_line(args, "A program.", os, group);
            BOOST_MPL_ASSERT((is_same<
                              decltype(result),
                              tuple<float, std::string, opt<int>>>));

            EXPECT_EQ(result[0_c], 7.0f);
            EXPECT_EQ(result[1_c], "11");
            EXPECT_TRUE(result[2_c]);
            EXPECT_EQ(result[2_c], 55);
        }
        {
            std::ostringstream os;
            po2::string_view_any_map result;
            po2::parse_command_line(args, result, "A program.", os, group);
            EXPECT_EQ(std::any_cast<float>(result["cats"]), 7.0f);
            EXPECT_EQ(std::any_cast<std::string>(result["dog"]), "11");
            EXPECT_EQ(std::any_cast<int>(result["apple"]), 55);
        }
    }
    {
        auto group = po2::group(pos1, pos2);

        auto opt_tuple = po2::detail::make_opt_tuple(group, arg1);
        BOOST_MPL_ASSERT((is_same<
                          decltype(opt_tuple),
                          tuple<
                              std::decay_t<decltype(pos1)>,
                              std::decay_t<decltype(pos2)>,
                              std::decay_t<decltype(arg1)>>>));

        std::vector<std::string_view> args{"prog", "7", "11", "-a", "55"};
        {
            std::ostringstream os;
            auto result =
                po2::parse_command_line(args, "A program.", os, group, arg1);
            BOOST_MPL_ASSERT((is_same<
                              decltype(result),
                              tuple<float, std::string, opt<int>>>));

            EXPECT_EQ(result[0_c], 7.0f);
            EXPECT_EQ(result[1_c], "11");
            EXPECT_TRUE(result[2_c]);
            EXPECT_EQ(result[2_c], 55);
        }
        {
            std::ostringstream os;
            po2::string_view_any_map result;
            po2::parse_command_line(
                args, result, "A program.", os, group, arg1);
            EXPECT_EQ(std::any_cast<float>(result["cats"]), 7.0f);
            EXPECT_EQ(std::any_cast<std::string>(result["dog"]), "11");
            EXPECT_EQ(std::any_cast<int>(result["apple"]), 55);
        }
    }
}
