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
    auto const arg2 =
        po2::argument<double>("-b,--branch", "Number of branchs", 1, 1, 2, 3);
    auto const arg3 = po2::argument<short>("-e", "Number of e's", 1, 1, 2, 3);
    auto const arg4 = po2::argument<short>("-f", "Number of f's", 1, 1, 2, 3);

    auto exclusive =
        po2::exclusive(arg1, po2::group(po2::exclusive(arg2, arg3)), arg4);
}
