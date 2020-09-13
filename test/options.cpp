// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
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
        auto const arg = po2::argument<int>("-b,--blah");
        EXPECT_EQ(arg.names, "-b,--blah");
        EXPECT_EQ(arg.args, 1);
        EXPECT_EQ(arg.action, po2::detail::action_kind::assign);
        BOOST_MPL_ASSERT((is_same<decltype(arg.value), po2::detail::no_value>));
        EXPECT_EQ(arg.choices.size(), 0);
        EXPECT_EQ(arg.arg_display_name, "");
    }
    {
        auto const arg = po2::argument<int>("-b,--blah", po2::zero_or_one);
        EXPECT_EQ(arg.names, "-b,--blah");
        EXPECT_EQ(arg.args, po2::zero_or_one);
        EXPECT_EQ(arg.action, po2::detail::action_kind::assign);
        BOOST_MPL_ASSERT((is_same<decltype(arg.value), po2::detail::no_value>));
        EXPECT_EQ(arg.choices.size(), 0);
        EXPECT_EQ(arg.arg_display_name, "");
    }
    {
        auto const arg = po2::argument<std::vector<int>>("-b,--blah", 2);
        EXPECT_EQ(arg.names, "-b,--blah");
        EXPECT_EQ(arg.args, 2);
        EXPECT_EQ(arg.action, po2::detail::action_kind::insert);
        BOOST_MPL_ASSERT((is_same<decltype(arg.value), po2::detail::no_value>));
        EXPECT_EQ(arg.choices.size(), 0);
        EXPECT_EQ(arg.arg_display_name, "");
    }
    {
        auto const arg = po2::argument<int>("-b,--blah", 1, 1, 2, 3);
        EXPECT_EQ(arg.names, "-b,--blah");
        EXPECT_EQ(arg.args, 1);
        EXPECT_EQ(arg.action, po2::detail::action_kind::assign);
        BOOST_MPL_ASSERT((is_same<decltype(arg.value), po2::detail::no_value>));
        EXPECT_EQ(arg.choices, (std::array<int, 3>{{1, 2, 3}}));
        EXPECT_EQ(arg.arg_display_name, "");
    }

    // with defaults
    {
        auto const arg = po2::with_default(po2::argument<int>("-b,--blah"), 42);
        EXPECT_EQ(arg.names, "-b,--blah");
        EXPECT_EQ(arg.args, 1);
        EXPECT_EQ(arg.action, po2::detail::action_kind::assign);
        EXPECT_EQ(arg.value, 42);
        EXPECT_EQ(arg.choices.size(), 0);
        EXPECT_EQ(arg.arg_display_name, "");
    }
    {
        auto const arg = po2::with_default(
            po2::argument<int>("-b,--blah", po2::zero_or_one), 42);
        EXPECT_EQ(arg.names, "-b,--blah");
        EXPECT_EQ(arg.args, po2::zero_or_one);
        EXPECT_EQ(arg.action, po2::detail::action_kind::assign);
        EXPECT_EQ(arg.value, 42);
        EXPECT_EQ(arg.choices.size(), 0);
        EXPECT_EQ(arg.arg_display_name, "");
    }
    {
        auto const arg = po2::with_default(
            po2::argument<std::vector<int>>("-b,--blah", 2),
            std::vector<int>({42}));
        EXPECT_EQ(arg.names, "-b,--blah");
        EXPECT_EQ(arg.args, 2);
        EXPECT_EQ(arg.action, po2::detail::action_kind::insert);
        EXPECT_EQ(arg.value, std::vector<int>({42}));
        EXPECT_EQ(arg.choices.size(), 0);
        EXPECT_EQ(arg.arg_display_name, "");
    }
    {
        auto const arg = po2::with_default(
            po2::argument<std::vector<int>>("-b,--blah", 2), 42);
        EXPECT_EQ(arg.names, "-b,--blah");
        EXPECT_EQ(arg.args, 2);
        EXPECT_EQ(arg.action, po2::detail::action_kind::insert);
        EXPECT_EQ(arg.value, 42);
        EXPECT_EQ(arg.choices.size(), 0);
        EXPECT_EQ(arg.arg_display_name, "");
    }
    {
        auto const arg =
            po2::with_default(po2::argument<int>("-b,--blah", 1, 1, 2, 3), 3);
        EXPECT_EQ(arg.names, "-b,--blah");
        EXPECT_EQ(arg.args, 1);
        EXPECT_EQ(arg.action, po2::detail::action_kind::assign);
        EXPECT_EQ(arg.value, 3);
        EXPECT_EQ(arg.choices, (std::array<int, 3>{{1, 2, 3}}));
        EXPECT_EQ(arg.arg_display_name, "");
    }

    // add a display name
    {
        auto const arg =
            po2::with_display_name(po2::argument<int>("-b,--blah"), "blerg");
        EXPECT_EQ(arg.names, "-b,--blah");
        EXPECT_EQ(arg.args, 1);
        EXPECT_EQ(arg.action, po2::detail::action_kind::assign);
        BOOST_MPL_ASSERT((is_same<decltype(arg.value), po2::detail::no_value>));
        EXPECT_EQ(arg.choices.size(), 0);
        EXPECT_EQ(arg.arg_display_name, "blerg");
    }
}

TEST(options, positionals)
{
    // TODO
}
