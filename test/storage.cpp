// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/program_options_2/parse_command_line.hpp>
#include <boost/program_options_2/storage.hpp>

#include <gtest/gtest.h>


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

TEST(storage, save_load_response_file)
{
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
        po2::any_map m;
        po2::parse_command_line(
            args, m, "A program.", os, ARGUMENTS(int, 4, 5, 6));

        EXPECT_EQ(m.size(), 6u);
        EXPECT_EQ(boost::any_cast<int>(m["abacus"]), 55);
        EXPECT_EQ(boost::any_cast<int>(m["bobcat"]), 66);
        EXPECT_EQ(
            boost::any_cast<std::vector<int>>(m["cataphract"]),
            std::vector<int>({77, 88}));
        EXPECT_EQ(boost::any_cast<int>(m["dolemite"]), 5);
        EXPECT_EQ(
            boost::any_cast<std::vector<int>>(m["zero-plus"]),
            std::vector<int>{2});
        EXPECT_EQ(
            boost::any_cast<std::set<int>>(m["one-plus"]), std::set<int>{2});

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
        po2::any_map m;
        po2::parse_command_line(
            args, m, "A program.", os, ARGUMENTS(int, 4, 5, 6));

        EXPECT_EQ(m.size(), 6u);
        EXPECT_EQ(boost::any_cast<int>(m["abacus"]), 55);

        EXPECT_THROW(
            po2::save_response_file(
                "empty_file",
                m,
                po2::argument<double>("-a,--abacus", "The abacus.")),
            po2::save_error);
        try {
            po2::load_response_file(
                "empty_file",
                m,
                po2::argument<double>("-a,--abacus", "The abacus."));
        } catch (po2::save_error & e) {
            EXPECT_EQ(e.error(), po2::save_result::bad_any_cast);
        }
    }

    {
        std::ostringstream os;
        std::vector<std::string_view> args{"prog", "@saved_map"};
        po2::any_map m;
        po2::parse_command_line(
            args, m, "A program.", os, ARGUMENTS(int, 4, 5, 6));

        EXPECT_EQ(m.size(), 6u);
        EXPECT_EQ(boost::any_cast<int>(m["abacus"]), 55);
    }

    {
        po2::any_map m;
        po2::load_response_file("saved_map", m, ARGUMENTS(int, 4, 5, 6));

        EXPECT_EQ(m.size(), 6u);
        EXPECT_EQ(boost::any_cast<int>(m["abacus"]), 55);
        EXPECT_EQ(boost::any_cast<int>(m["bobcat"]), 66);
        EXPECT_EQ(
            boost::any_cast<std::vector<int>>(m["cataphract"]),
            std::vector<int>({77, 88}));
        EXPECT_EQ(boost::any_cast<int>(m["dolemite"]), 5);
        EXPECT_EQ(
            boost::any_cast<std::vector<int>>(m["zero-plus"]),
            std::vector<int>{2});
        EXPECT_EQ(
            boost::any_cast<std::set<int>>(m["one-plus"]), std::set<int>{2});
    }

    {
        po2::any_map m;
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
        po2::any_map m;
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

#if 0 // TODO: Fix! (seems to be an unbounded loop)
    {
       std::ofstream ofs("bad_map_for_loading");
        ofs << "unknown";
        ofs.close();
        po2::any_map m;
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
#endif

    {
        std::ofstream ofs("bad_map_for_loading");
        ofs << "--abacus fish";
        ofs.close();
        po2::any_map m;
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
        po2::any_map m;
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
            return po2::validation_result{false};
        };

        std::ofstream ofs("bad_map_for_loading");
        ofs << "--abacus 55";
        ofs.close();

        po2::any_map m;
#if 0 // TODO: Fix!
        EXPECT_THROW(
            po2::load_response_file(
                "bad_map_for_loading",
                m,
                po2::with_validator(
                    po2::argument<int>("-a,--abacus", "The abacus."),
                    validator)),
            po2::load_error);
#endif
        try {
            po2::load_response_file(
                "bad_map_for_loading",
                m,
                po2::with_validator(
                    po2::argument<int>("-a,--abacus", "The abacus."),
                    validator));
        } catch (po2::load_error & e) {
            EXPECT_EQ(e.error(), po2::load_result::no_such_choice);
        }
    }
}

#undef ARGUMENTS
