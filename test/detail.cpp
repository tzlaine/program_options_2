// Copyright (C) 2018 T. Zachary Laine
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
    // TODO
}
