// Copyright (C) 2022 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/program_options_2/option_groups.hpp>

namespace po2 = boost::program_options_2;

int main()
{
    auto const arg1 = po2::argument<int>("-a,--apple", "Number of apples");
    auto leaf_command = po2::command([](auto) {}, "cmd", arg1);
    auto interior_command = po2::command([](auto) {}, "cmd", leaf_command);
}
