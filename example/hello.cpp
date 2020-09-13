// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/program_options_2/parse_command_line.hpp>

#include <iostream>


namespace po2 = boost::program_options_2;

int main(int argc, char const * argv[])
{
    // TODO: Catch hana::tuple; use below.
    po2::parse_command_line(
        argc,
        argv,
        std::cout,
        po2::positional<std::string_view>(
            "addressee", "The person to whom to say hello."));

    std::cout << "Hello, TODO!\n";
}
