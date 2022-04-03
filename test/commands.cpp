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
auto const arg4 = po2::argument<short>("-f", "Number of f's", 1, 1, 2, 3);

// TODO: Test printing for all of the below (in the printing test).

// TODO: Printing of commands has an extra line break in the synopsis.

TEST(commands, api)
{
    // interior commands
    {
        auto command_with_subcommand = po2::command(
            "cmd",
            "A top-level command.",
            pos1,
            po2::command(
                [](auto) {},
                "subcmd",
                "Sub-command for cmd.",
                arg1,
                arg2,
                arg3));
        (void)command_with_subcommand;
    }

    {
        auto command_with_subcommand = po2::command(
            "cmd",
            pos1,
            po2::command(
                [](auto) {},
                "subcmd",
                "Sub-command for cmd.",
                arg1,
                arg2,
                arg3));
        (void)command_with_subcommand;
    }

    // leaf commands
    {
        auto command =
            po2::command([](auto) {}, "cmd", "A command.", arg1, arg2, arg3);
        (void)command;
    }

    {
        auto command = po2::command([](auto) {}, "cmd", arg1, arg2, arg3);
        (void)command;
    }
}

TEST(commands, command)
{
    // TODO: Move to printing.cpp?
    // printing
    {
        auto command =
            po2::command([](auto) {}, "cmd", "A command.", arg1, arg2, arg3);
        auto group = po2::group(pos1, command);

        EXPECT_TRUE(po2::detail::contains_commands<decltype(command)>());
        EXPECT_TRUE(po2::detail::contains_commands<decltype(group)>());

        {
            std::vector<std::string_view> args{"prog", "-h"};
            std::ostringstream os;
            std::map<std::string_view, std::any> result;
            try {
                po2::parse_command_line(
                    args, result, "A program.", os, group);
            } catch (int) {
                // TODO: Remove this try once parsing is implemented.
                std::cout << "command line parse with commands failed; "
                             "os.str() sez:\n"
                          << os.str() << "\n";
            }
        }

        {
            auto command_with_subcommand = po2::command(
                "cmd",
                "A top-level command.",
                pos1,
                po2::command(
                    [](auto) {},
                    "subcmd",
                    "Sub-command for cmd.",
                    arg1,
                    arg2,
                    arg3));

            {
                std::vector<std::string_view> args{"prog", "-h"};
                std::ostringstream os;
                std::map<std::string_view, std::any> result;
                try {
                    po2::parse_command_line(
                        args,
                        result,
                        "A program.",
                        os,
                        command_with_subcommand);
                } catch (int) {
                    // TODO: Remove this try once parsing is implemented.
                    std::cout << "command line parse with commands failed; "
                                 "os.str() sez:\n"
                              << os.str() << "\n";
                }
            }
            {
                std::vector<std::string_view> args{"prog", "cmd", "-h"};
                std::ostringstream os;
                std::map<std::string_view, std::any> result;
                try {
                    po2::parse_command_line(
                        args,
                        result,
                        "A program.",
                        os,
                        command_with_subcommand);
                } catch (int) {
                    // TODO: Remove this try once parsing is implemented.
                    std::cout << "command line parse with commands failed; "
                                 "os.str() sez:\n"
                              << os.str() << "\n";
                }
            }
            {
                std::vector<std::string_view> args{
                    "prog", "cmd", "subcmd", "-h"};
                std::ostringstream os;
                std::map<std::string_view, std::any> result;
                try {
                    po2::parse_command_line(
                        args,
                        result,
                        "A program.",
                        os,
                        command_with_subcommand);
                } catch (int) {
                    // TODO: Remove this try once parsing is implemented.
                    std::cout << "command line parse with commands failed; "
                                 "os.str() sez:\n"
                              << os.str() << "\n";
                }
            }
        }
    }

    {
        auto command = po2::command([](auto) {}, "cmd", arg1, arg2, arg3);
        std::vector<std::string_view> args{"prog", "-h"};

        std::ostringstream os;
        std::map<std::string_view, std::any> result;
        try {
            po2::parse_command_line(args, result, "A program.", os, command);
        } catch (int) {
            // TODO: Remove this try once parsing is implemented.
            std::cout
                << "command line parse with commands failed; os.str() sez:\n"
                << os.str() << "\n";
        }
    }

    {
        auto command = po2::command([](auto) {}, "cmd", arg1, arg2, arg3);
        std::vector<std::string_view> args{
            "prog", "cmd", "-a", "55", "--branch", "2", "-e", "3"};

        std::ostringstream os;
        std::map<std::string_view, std::any> result;
        try {
            po2::parse_command_line(args, result, "A program.", os, command);
        } catch (int) {
            // TODO: Remove this try once parsing is implemented.
            std::cout
                << "command line parse with commands failed; os.str() sez:\n"
                << os.str() << "\n";
        }
    }

    // TODO: Move to printing.cpp?
    // multiple commands
    {
        std::string const expected_help = R"(usage:  prog [-h] COMMAND

A program.

commands:
  cmd1        What command 1 does...
  cmd2        What command 2 does...
  cmd3        What command 3 does...

Use 'prog CMD -h' for help on command CMD.
)";
        auto command1 =
            po2::command([](auto) {}, "cmd1", "What command 1 does...", arg1);
        auto command2 =
            po2::command([](auto) {}, "cmd2", "What command 2 does...", arg2);
        auto command3 =
            po2::command([](auto) {}, "cmd3", "What command 3 does...", arg3);
        std::vector<std::string_view> args{"prog", "-h"};

        std::ostringstream os;
        std::map<std::string_view, std::any> result;

        os = std::ostringstream();
        try {
            po2::parse_command_line(
                args, result, "A program.", os, command1, command2, command3);
        } catch (int) {
        }
        EXPECT_EQ(os.str(), expected_help);

        os = std::ostringstream();
        try {
            po2::parse_command_line(
                args,
                result,
                "A program.",
                os,
                po2::group(command1, command2, command3));
        } catch (int) {
        }
        EXPECT_EQ(os.str(), expected_help);

        os = std::ostringstream();
        try {
            po2::parse_command_line(
                args,
                result,
                "A program.",
                os,
                po2::group(pos1, po2::group(command1, command2, command3)));
        } catch (int) {
        }
        EXPECT_EQ(os.str(), expected_help);

        os = std::ostringstream();
        try {
            po2::parse_command_line(
                args,
                result,
                "A program.",
                os,
                po2::group(command1, command2),
                command3);
        } catch (int) {
        }
        EXPECT_EQ(os.str(), expected_help);

        os = std::ostringstream();
        try {
            po2::parse_command_line(
                args,
                result,
                "A program.",
                os,
                command1,
                po2::group(command2, command3));
        } catch (int) {
        }
        EXPECT_EQ(os.str(), expected_help);
    }

    // TODO: Subcommands (3 deep)

    // TODO: Named groups of commands

    // TODO: Move to printing.cpp?
    // named groups "nested" within commands
    {
        auto command = po2::command(
            [](auto) {},
            "cmd",
            "What the command does...",
            po2::group("A group", "Its function", arg1));
        std::vector<std::string_view> args;

        std::ostringstream os;
        std::map<std::string_view, std::any> result;

        args = {"prog", "-h"};
        os = std::ostringstream();
        try {
            po2::parse_command_line(
                args,
                result,
                "A program.",
                os,
                po2::group("A group of commands", "Its function", command));
        } catch (int) {
        }
        EXPECT_EQ(os.str(), R"(usage:  prog [-h] COMMAND

A program.

optional arguments:
  -h, --help  Print this help message and exit

A group of commands:
  Its function

  cmd         What the command does...

response files:
  Use '@file' to load a file containing command line arguments.
)");

#if 0 // TODO: Borken.
        args = {"prog", "cmd", "-h"};
        os = std::ostringstream();
        try {
            po2::parse_command_line(
                args,
                result,
                "A program.",
                os,
                po2::group("A group of commands", "Its function", command));
        } catch (int) {
        }
        EXPECT_EQ(os.str(), R"()");
#endif
    }
}
