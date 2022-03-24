// Copyright (C) 2022 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/program_options_2/option_groups.hpp>
#include <boost/program_options_2/parse_command_line.hpp>

namespace po2 = boost::program_options_2;

int main()
{
    auto const arg1 = po2::argument<int>("-a,--apple", "Number of apples");
    auto command = po2::command("cmd", arg1);

    std::vector<std::string_view> args{"prog", "cmd", "-a"};
    std::ostringstream os;
    auto result = po2::parse_command_line(args, "A program.", os, command);
}
