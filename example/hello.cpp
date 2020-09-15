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
    auto const result = po2::parse_command_line(
        argc,
        argv,
        "A program that prints hello.",
        std::cout,
        po2::positional("addressee", "The person to say hello to."));

    using namespace boost::hana::literals;
    std::cout << "Hello, " << result[0_c] << "!\n";
}
