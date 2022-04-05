// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/program_options_2/parse_command_line.hpp>
#include <boost/program_options_2/storage.hpp>

#include <gtest/gtest.h>

// TODO: Document that the option default type (std::string_view) causes
// dangling when loading from files.

// TODO: Need tests (and fixes!) for saving and loading groups, especially
// commands.


namespace po2 = boost::program_options_2;

#define ARGUMENTS(T, choice0, choice1, choice2)                                \
    po2::argument<T>("-a,--abacus", "The abacus."),                            \
        po2::argument<T>("-b,--bobcat", "The bobcat."),                        \
        po2::argument<std::vector<T>>("-c,--cataphract", "The cataphract", 2), \
        po2::argument<T>(                                                      \
            "-d,--dolemite", "*The* Dolemite.", 1, choice0, choice1, choice2), \
        po2::argument<std::vector<T>>(                                         \
            "-z,--zero-plus", "None is fine; so is more.", po2::zero_or_more), \
        po2::argument<std::set<T>>(                                            \
            "-o,--one-plus", "One is fine; so is more.", po2::one_or_more)

#define MIXED(T, choice0, choice1, choice2, default_)                          \
    po2::argument<T>("-a,--abacus", "The abacus."),                            \
        po2::with_default(                                                     \
            po2::argument<T>("-b,--bobcat", "The bobcat."), default_),         \
        po2::positional<std::vector<T>>("cataphract", "The cataphract", 2),    \
        po2::argument<T>(                                                      \
            "-d,--dolemite", "*The* Dolemite.", 1, choice0, choice1, choice2), \
        po2::remainder<std::vector<std::string>>(                              \
            "args", "other args at the end")

TEST(storage, save_load_response_file)
{
    // Just arguments

    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog",
            "-a",
            "55",
            "--bobcat",
            "66",
            "-o",
            "2",
            "-z",
            "2",
            "-c",
            "77",
            "88",
            "--dolemite",
            "5"};
        po2::string_any_map m;
        po2::parse_command_line(
            args, m, "A program.", os, ARGUMENTS(int, 4, 5, 6));

        EXPECT_EQ(m.size(), 6u);
        EXPECT_EQ(std::any_cast<int>(m["abacus"]), 55);
        EXPECT_EQ(std::any_cast<int>(m["bobcat"]), 66);
        EXPECT_EQ(
            std::any_cast<std::vector<int>>(m["cataphract"]),
            std::vector<int>({77, 88}));
        EXPECT_EQ(std::any_cast<int>(m["dolemite"]), 5);
        EXPECT_EQ(
            std::any_cast<std::vector<int>>(m["zero-plus"]),
            std::vector<int>{2});
        EXPECT_EQ(
            std::any_cast<std::set<int>>(m["one-plus"]), std::set<int>{2});

        po2::save_response_file("saved_map", m, ARGUMENTS(int, 4, 5, 6));
    }
    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog",
            "-a",
            "55",
            "--bobcat",
            "66",
            "-o",
            "2",
            "-z",
            "2",
            "-c",
            "77",
            "88",
            "--dolemite",
            "5"};
        po2::string_any_map m;
        po2::parse_command_line(
            args, m, "A program.", os, ARGUMENTS(int, 4, 5, 6));

        EXPECT_EQ(m.size(), 6u);
        EXPECT_EQ(std::any_cast<int>(m["abacus"]), 55);

        EXPECT_THROW(
            po2::save_response_file(
                "dummy_file",
                m,
                po2::argument<double>("-a,--abacus", "The abacus.")),
            po2::save_error);
        try {
            po2::load_response_file(
                "dummy_file",
                m,
                po2::argument<double>("-a,--abacus", "The abacus."));
        } catch (po2::save_error & e) {
            EXPECT_EQ(e.error(), po2::save_result::bad_any_cast);
        }
    }

    {
        std::ostringstream os;
        std::vector<std::string_view> args{"prog", "@saved_map"};
        po2::string_any_map m;
        po2::parse_command_line(
            args, m, "A program.", os, ARGUMENTS(int, 4, 5, 6));

        EXPECT_EQ(m.size(), 6u);
        EXPECT_EQ(std::any_cast<int>(m["abacus"]), 55);
    }

    {
        po2::string_any_map m;
        po2::load_response_file("saved_map", m, ARGUMENTS(int, 4, 5, 6));

        EXPECT_EQ(m.size(), 6u);
        EXPECT_EQ(std::any_cast<int>(m["abacus"]), 55);
        EXPECT_EQ(std::any_cast<int>(m["bobcat"]), 66);
        EXPECT_EQ(
            std::any_cast<std::vector<int>>(m["cataphract"]),
            std::vector<int>({77, 88}));
        EXPECT_EQ(std::any_cast<int>(m["dolemite"]), 5);
        EXPECT_EQ(
            std::any_cast<std::vector<int>>(m["zero-plus"]),
            std::vector<int>{2});
        EXPECT_EQ(
            std::any_cast<std::set<int>>(m["one-plus"]), std::set<int>{2});
    }

    // Mixed arguments and positionals

    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog",
            "-a",
            "55",
            "--bobcat",
            "66",
            "77",
            "88",
            "--dolemite",
            "5",
            "\\2\""};
        po2::string_any_map m;
        po2::parse_command_line(
            args, m, "A program.", os, MIXED(int, 4, 5, 6, 42));

        EXPECT_EQ(m.size(), 5u);
        EXPECT_EQ(std::any_cast<int>(m["abacus"]), 55);
        EXPECT_EQ(std::any_cast<int>(m["bobcat"]), 66);
        EXPECT_EQ(
            std::any_cast<std::vector<int>>(m["cataphract"]),
            std::vector<int>({77, 88}));
        EXPECT_EQ(std::any_cast<int>(m["dolemite"]), 5);
        EXPECT_EQ(
            std::any_cast<std::vector<std::string>>(m["args"]),
            std::vector<std::string>({"\\2\""}));

        po2::save_response_file("saved_mixed_map", m, MIXED(int, 4, 5, 6, 42));

        {
            std::ifstream ifs("saved_mixed_map");
            std::string const contents_with_comments =
                "#comments are ignored\n" + po2::detail::file_slurp(ifs) +
                "#   more comments";
            ifs.close();
            std::ofstream ofs("saved_mixed_map");
            ofs << contents_with_comments;
        }
    }

    {
        std::ostringstream os;
        std::vector<std::string_view> args{"prog", "@saved_mixed_map"};
        po2::string_any_map m;
        po2::parse_command_line(
            args, m, "A program.", std::cout, MIXED(int, 4, 5, 6, 42));

        EXPECT_EQ(m.size(), 5u);
        EXPECT_EQ(std::any_cast<int>(m["abacus"]), 55);
    }

    {
        po2::string_any_map m;
        po2::load_response_file("saved_mixed_map", m, MIXED(int, 4, 5, 6, 42));

        EXPECT_EQ(m.size(), 5u);
        EXPECT_EQ(std::any_cast<int>(m["abacus"]), 55);
        EXPECT_EQ(std::any_cast<int>(m["bobcat"]), 66);
        EXPECT_EQ(
            std::any_cast<std::vector<int>>(m["cataphract"]),
            std::vector<int>({77, 88}));
        EXPECT_EQ(std::any_cast<int>(m["dolemite"]), 5);
        EXPECT_EQ(
            std::any_cast<std::vector<std::string>>(m["args"]),
            std::vector<std::string>({"\\2\""}));
    }

    // Error cases.

    {
        po2::string_any_map m;
        EXPECT_THROW(
            po2::load_response_file(
                "nonexistent_file", m, ARGUMENTS(int, 4, 5, 6)),
            po2::load_error);
        try {
            po2::load_response_file(
                "nonexistent_file", m, ARGUMENTS(int, 4, 5, 6));
        } catch (po2::load_error & e) {
            EXPECT_EQ(
                e.error(), po2::load_result::could_not_open_file_for_reading);
        }
    }

    {
        std::ofstream ofs("bad_map_for_loading");
        ofs << "--cataphract 5";
        ofs.close();
        po2::string_any_map m;
        EXPECT_THROW(
            po2::load_response_file(
                "bad_map_for_loading", m, ARGUMENTS(int, 4, 5, 6)),
            po2::load_error);
        try {
            po2::load_response_file(
                "bad_map_for_loading", m, ARGUMENTS(int, 4, 5, 6));
        } catch (po2::load_error & e) {
            EXPECT_EQ(e.error(), po2::load_result::wrong_number_of_args);
        }
    }

    {
        std::ofstream ofs("bad_map_for_loading");
        ofs << "--unknown";
        ofs.close();
        po2::string_any_map m;
        EXPECT_THROW(
            po2::load_response_file(
                "bad_map_for_loading", m, ARGUMENTS(int, 4, 5, 6)),
            po2::load_error);
        try {
            po2::load_response_file(
                "bad_map_for_loading", m, ARGUMENTS(int, 4, 5, 6));
        } catch (po2::load_error & e) {
            EXPECT_EQ(e.error(), po2::load_result::unknown_arg);
        }
    }

    {
        std::ofstream ofs("bad_map_for_loading");
        ofs << "unknown";
        ofs.close();
        po2::string_any_map m;
        EXPECT_THROW(
            po2::load_response_file(
                "bad_map_for_loading", m, ARGUMENTS(int, 4, 5, 6)),
            po2::load_error);
        try {
            po2::load_response_file(
                "bad_map_for_loading", m, ARGUMENTS(int, 4, 5, 6));
        } catch (po2::load_error & e) {
            EXPECT_EQ(e.error(), po2::load_result::unknown_arg);
        }
    }

    {
        std::ofstream ofs("bad_map_for_loading");
        ofs << "--abacus fish";
        ofs.close();
        po2::string_any_map m;
        EXPECT_THROW(
            po2::load_response_file(
                "bad_map_for_loading", m, ARGUMENTS(int, 4, 5, 6)),
            po2::load_error);
        try {
            po2::load_response_file(
                "bad_map_for_loading", m, ARGUMENTS(int, 4, 5, 6));
        } catch (po2::load_error & e) {
            EXPECT_EQ(e.error(), po2::load_result::cannot_parse_arg);
        }
    }

    {
        std::ofstream ofs("bad_map_for_loading");
        ofs << "--dolemite 555";
        ofs.close();
        po2::string_any_map m;
        EXPECT_THROW(
            po2::load_response_file(
                "bad_map_for_loading", m, ARGUMENTS(int, 4, 5, 6)),
            po2::load_error);
        try {
            po2::load_response_file(
                "bad_map_for_loading", m, ARGUMENTS(int, 4, 5, 6));
        } catch (po2::load_error & e) {
            EXPECT_EQ(e.error(), po2::load_result::no_such_choice);
        }
    }

    {
        auto validator = [](auto const &) {
            return po2::validation_result{false, "bad"};
        };

        std::ofstream ofs("bad_map_for_loading");
        ofs << "--abacus 55";
        ofs.close();

        po2::string_any_map m;
        EXPECT_THROW(
            po2::load_response_file(
                "bad_map_for_loading",
                m,
                po2::with_validator(
                    po2::argument<int>("-a,--abacus", "The abacus."),
                    validator)),
            po2::load_error);
        try {
            po2::load_response_file(
                "bad_map_for_loading",
                m,
                po2::with_validator(
                    po2::argument<int>("-a,--abacus", "The abacus."),
                    validator));
        } catch (po2::load_error & e) {
            EXPECT_EQ(e.error(), po2::load_result::validation_error);
        }
    }
}

TEST(storage, save_load_json_file)
{
    // Mixed arguments only

    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog",
            "-a",
            "55",
            "--bobcat",
            "66",
            "-o",
            "2",
            "-z",
            "2",
            "-c",
            "77",
            "88",
            "--dolemite",
            "5"};
        po2::string_any_map m;
        po2::parse_command_line(
            args, m, "A program.", os, ARGUMENTS(int, 4, 5, 6));

        EXPECT_EQ(m.size(), 6u);
        EXPECT_EQ(std::any_cast<int>(m["abacus"]), 55);
        EXPECT_EQ(std::any_cast<int>(m["bobcat"]), 66);
        EXPECT_EQ(
            std::any_cast<std::vector<int>>(m["cataphract"]),
            std::vector<int>({77, 88}));
        EXPECT_EQ(std::any_cast<int>(m["dolemite"]), 5);
        EXPECT_EQ(
            std::any_cast<std::vector<int>>(m["zero-plus"]),
            std::vector<int>{2});
        EXPECT_EQ(
            std::any_cast<std::set<int>>(m["one-plus"]), std::set<int>{2});

        po2::save_json_file("saved_json_map", m, ARGUMENTS(int, 4, 5, 6));

        {
            std::ifstream ifs("saved_json_map");
            std::string const contents_with_comments =
                "#comments are ignored\n" + po2::detail::file_slurp(ifs) +
                "#   more comments";
            ifs.close();
            std::ofstream ofs("saved_json_map");
            ofs << contents_with_comments;
        }
    }
    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog",
            "-a",
            "55",
            "--bobcat",
            "66",
            "-o",
            "2",
            "-z",
            "2",
            "-c",
            "77",
            "88",
            "--dolemite",
            "5"};
        po2::string_any_map m;
        po2::parse_command_line(
            args, m, "A program.", os, ARGUMENTS(int, 4, 5, 6));

        EXPECT_THROW(
            po2::save_json_file(
                "dummy_file",
                m,
                po2::argument<double>("-a,--abacus", "The abacus.")),
            po2::save_error);
        try {
            po2::save_json_file(
                "dummy_file",
                m,
                po2::argument<double>("-a,--abacus", "The abacus."));
        } catch (po2::save_error & e) {
            EXPECT_EQ(e.error(), po2::save_result::bad_any_cast);
        }
    }
    {
        po2::string_any_map m;
        EXPECT_THROW(
            po2::load_json_file("dummy_file", m, ARGUMENTS(int, 4, 5, 6)),
            po2::load_error);
        try {
            po2::load_json_file("dummy_file", m, ARGUMENTS(int, 4, 5, 6));
        } catch (po2::load_error & e) {
            EXPECT_EQ(e.error(), po2::load_result::malformed_json);
            EXPECT_EQ(e.str(), R"(dummy_file:2:0: error: Expected '}' here (end of input):

^

Note: The file is expected to use a subset of JSON that contains only strings,
arrays, and objects.  JSON types null, boolean, and number are not supported,
and character escapes besides '\\' and '\"' are not supported.
)");
        }
    }

    {
        po2::string_any_map m;
        po2::load_json_file("saved_json_map", m, ARGUMENTS(int, 4, 5, 6));

        EXPECT_EQ(m.size(), 6u);
        EXPECT_EQ(std::any_cast<int>(m["abacus"]), 55);
        EXPECT_EQ(std::any_cast<int>(m["bobcat"]), 66);
        EXPECT_EQ(
            std::any_cast<std::vector<int>>(m["cataphract"]),
            std::vector<int>({77, 88}));
        EXPECT_EQ(std::any_cast<int>(m["dolemite"]), 5);
        EXPECT_EQ(
            std::any_cast<std::vector<int>>(m["zero-plus"]),
            std::vector<int>{2});
        EXPECT_EQ(
            std::any_cast<std::set<int>>(m["one-plus"]), std::set<int>{2});
    }


    // Mixed arguments and positionals

    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog",
            "-a",
            "55",
            "--bobcat",
            "66",
            "77",
            "88",
            "--dolemite",
            "5",
            "\\2\""};
        po2::string_any_map m;
        po2::parse_command_line(
            args, m, "A program.", os, MIXED(int, 4, 5, 6, 42));

        EXPECT_EQ(m.size(), 5u);
        EXPECT_EQ(std::any_cast<int>(m["abacus"]), 55);
        EXPECT_EQ(std::any_cast<int>(m["bobcat"]), 66);
        EXPECT_EQ(
            std::any_cast<std::vector<int>>(m["cataphract"]),
            std::vector<int>({77, 88}));
        EXPECT_EQ(std::any_cast<int>(m["dolemite"]), 5);
        EXPECT_EQ(
            std::any_cast<std::vector<std::string>>(m["args"]),
            std::vector<std::string>({"\\2\""}));

        po2::save_response_file(
            "saved_mixed_json_map", m, MIXED(int, 4, 5, 6, 42));

        {
            std::ifstream ifs("saved_mixed_json_map");
            std::string const contents_with_comments =
                "#comments are ignored\n" + po2::detail::file_slurp(ifs) +
                "#   more comments";
            ifs.close();
            std::ofstream ofs("saved_mixed_json_map");
            ofs << contents_with_comments;
        }
    }

    {
        po2::string_any_map m;
        po2::load_response_file(
            "saved_mixed_json_map", m, MIXED(int, 4, 5, 6, 42));

        EXPECT_EQ(m.size(), 5u);
        EXPECT_EQ(std::any_cast<int>(m["abacus"]), 55);
        EXPECT_EQ(std::any_cast<int>(m["bobcat"]), 66);
        EXPECT_EQ(
            std::any_cast<std::vector<int>>(m["cataphract"]),
            std::vector<int>({77, 88}));
        EXPECT_EQ(std::any_cast<int>(m["dolemite"]), 5);
        EXPECT_EQ(
            std::any_cast<std::vector<std::string>>(m["args"]),
            std::vector<std::string>({"\\2\""}));
    }


    // Error cases

    {
        std::ofstream ofs("bad_json_map_for_loading");
        ofs << "{ \"--unknown\": \"unknown\" }";
        ofs.close();
        po2::string_any_map m;
        EXPECT_THROW(
            po2::load_response_file(
                "bad_json_map_for_loading", m, ARGUMENTS(int, 4, 5, 6)),
            po2::load_error);
        try {
            po2::load_response_file(
                "bad_json_map_for_loading", m, ARGUMENTS(int, 4, 5, 6));
        } catch (po2::load_error & e) {
            EXPECT_EQ(e.error(), po2::load_result::unknown_arg);
        }
    }
}

#undef ARGUMENTS
#undef MIXED
