// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#define BOOST_PROGRAM_OPTIONS_2_TESTING
#include <boost/program_options_2/parse_command_line.hpp>

#include "ill_formed.hpp"

#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>

#include <gtest/gtest.h>

#if defined(UNIX_BUILD)
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#endif


namespace po2 = boost::program_options_2;
using boost::is_same;
using boost::hana::tuple;
template<typename T>
using opt = std::optional<T>;
using namespace boost::hana::literals;

po2::customizable_strings user_strings()
{
    po2::customizable_strings retval;
    retval.usage_text = "USAGE: ";
    retval.positional_section_text = "POSITIONAL arguments:";
    retval.optional_section_text = "OPTIONAL arguments:";
    retval.help_names = "-r,--redacted";
    retval.help_description = "Nothing to see here.";
    return retval;
}

TEST(parse_command_line, api)
{
    std::string arg_string = std::string("foo") + po2::detail::fs_sep + "bar";
    arg_string += '\0';
    arg_string += "jj";
    arg_string += '\0';

    int const argc = 2;
    char const * argv[2] = {
        arg_string.c_str(), arg_string.c_str() + arg_string.size() - 3};

#if defined(_MSC_VER)
    std::wstring warg_string =
        std::wstring(u"foo") + wchar_t(po2::detail::fs_sep) + L"bar";
    warg_string += L'\0';
    warg_string += L"jj";
    warg_string += L'\0';

    int const wargc = 2;
    wchar_t const * wargv[2] = {
        warg_string.c_str(), warg_string.c_str() + warg_string.size() - 3};
#endif

    // arg-view, no user strings
    {
        std::ostringstream os;
        auto result = po2::parse_command_line(
            po2::arg_view(argc, argv),
            "A program.",
            os,
            po2::positional("pos", "Positional."));
        BOOST_MPL_ASSERT((is_same<decltype(result), tuple<std::string_view>>));
        EXPECT_EQ(result[0_c], "jj");
    }
    {
        std::ostringstream os;
        auto const arg_view = po2::arg_view(argc, argv);
        std::vector<std::string_view> args(arg_view.begin(), arg_view.end());
        auto result = po2::parse_command_line(
            args, "A program.", os, po2::positional("pos", "Positional."));
        BOOST_MPL_ASSERT((is_same<decltype(result), tuple<std::string_view>>));
        EXPECT_EQ(result[0_c], "jj");
    }
#if defined(_MSC_VER)
    {
        std::wostringstream os;
        auto result = po2::parse_command_line(
            po2::arg_view(wargc, wargv),
            "A program.",
            os,
            po2::positional("pos", "Positional."));
        BOOST_MPL_ASSERT((is_same<decltype(result), tuple<std::wstring_view>>));
        EXPECT_EQ(result[0_c], L"jj");
    }
#endif

    // arg-view, with user strings
    {
        std::ostringstream os;
        auto result = po2::parse_command_line(
            po2::arg_view(argc, argv),
            "A program.",
            os,
            user_strings(),
            po2::positional("pos", "Positional."));
        BOOST_MPL_ASSERT((is_same<decltype(result), tuple<std::string_view>>));
        EXPECT_EQ(result[0_c], "jj");
    }
    {
        std::ostringstream os;
        auto const arg_view = po2::arg_view(argc, argv);
        std::vector<std::string_view> args(arg_view.begin(), arg_view.end());
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            user_strings(),
            po2::positional("pos", "Positional."));
        BOOST_MPL_ASSERT((is_same<decltype(result), tuple<std::string_view>>));
        EXPECT_EQ(result[0_c], "jj");
    }
#if defined(_MSC_VER)
    {
        std::wostringstream os;
        auto result = po2::parse_command_line(
            po2::arg_view(wargc, wargv),
            "A program.",
            os,
            user_strings(),
            po2::positional("pos", "Positional."));
        BOOST_MPL_ASSERT((is_same<decltype(result), tuple<std::wstring_view>>));
        EXPECT_EQ(result[0_c], L"jj");
    }
#endif

    // argc/argv, no user strings
    {
        std::ostringstream os;
        auto result = po2::parse_command_line(
            argc,
            argv,
            "A program.",
            os,
            po2::positional("pos", "Positional."));
        BOOST_MPL_ASSERT((is_same<decltype(result), tuple<std::string_view>>));
        EXPECT_EQ(result[0_c], "jj");
    }
#if defined(_MSC_VER)
    {
        std::wostringstream os;
        auto result = po2::parse_command_line(
            wargc,
            wargv,
            "A program.",
            os,
            po2::positional("pos", "Positional."));
        BOOST_MPL_ASSERT((is_same<decltype(result), tuple<std::wstring_view>>));
        EXPECT_EQ(result[0_c], L"jj");
    }
#endif

    // argc/argv, with user strings
    {
        std::ostringstream os;
        auto result = po2::parse_command_line(
            argc,
            argv,
            "A program.",
            os,
            user_strings(),
            po2::positional("pos", "Positional."));
        BOOST_MPL_ASSERT((is_same<decltype(result), tuple<std::string_view>>));
        EXPECT_EQ(result[0_c], "jj");
    }
#if defined(_MSC_VER)
    {
        std::wostringstream os;
        auto result = po2::parse_command_line(
            wargc,
            wargv,
            "A program.",
            os,
            user_strings(),
            po2::positional("pos", "Positional."));
        BOOST_MPL_ASSERT((is_same<decltype(result), tuple<std::wstring_view>>));
        EXPECT_EQ(result[0_c], L"jj");
    }
#endif
}

TEST(parse_command_line, errors)
{
    // unknown arg
    {
        std::ostringstream os;
        try {
            std::vector<std::string_view> args{"prog", "-b"};
            po2::parse_command_line(
                args, "A program.", os, po2::argument("-a", "Arg."));
        } catch (int) {
        }
        EXPECT_EQ(os.str(), R"(error: unrecognized argument '-b'

usage:  prog [-h] [-a A]

A program.

optional arguments:
  -h, --help  Print this help message and exit
  -a          Arg.

response files:
  Write '@file' to load a file containing command line arguments.
)");
    }

    // wrong number of args
    {
        std::ostringstream os;
        try {
            std::vector<std::string_view> args{"prog", "-a", "too", "many"};
            po2::parse_command_line(
                args,
                "A program.",
                os,
                po2::argument("-a", "Arg."));
        } catch (int) {
        }
        EXPECT_EQ(os.str(), R"(error: unrecognized argument 'many'

usage:  prog [-h] [-a A]

A program.

optional arguments:
  -h, --help  Print this help message and exit
  -a          Arg.

response files:
  Write '@file' to load a file containing command line arguments.
)");
    }

    // cannot parse arg
    {
        std::ostringstream os;
        try {
            std::vector<std::string_view> args{"prog", "-a", "3.0"};
            po2::parse_command_line(
                args,
                "A program.",
                os,
                po2::argument<int>("-a", "Arg."));
        } catch (int) {
        }
        EXPECT_EQ(os.str(), R"(error: cannot parse argument '3.0'

usage:  prog [-h] [-a A]

A program.

optional arguments:
  -h, --help  Print this help message and exit
  -a          Arg.

response files:
  Write '@file' to load a file containing command line arguments.
)");
    }

    // no such choice
    {
        std::ostringstream os;
        try {
            std::vector<std::string_view> args{"prog", "-a", "baz"};
            po2::parse_command_line(
                args,
                "A program.",
                os,
                po2::argument("-a", "Arg.", 1, "foo", "bar"));
        } catch (int) {
        }
        EXPECT_EQ(os.str(), R"(error: 'baz' is not one of the allowed choices

usage:  prog [-h] [-a {foo,bar}]

A program.

optional arguments:
  -h, --help  Print this help message and exit
  -a          Arg.

response files:
  Write '@file' to load a file containing command line arguments.
)");
    }

    // extra positional
    {
        std::ostringstream os;
        try {
            std::vector<std::string_view> args{"prog", "unknown"};
            po2::parse_command_line(
                args,
                "A program.",
                os,
                po2::argument("-a", "Arg."));
        } catch (int) {
        }
        EXPECT_EQ(os.str(), R"(error: unrecognized argument 'unknown'

usage:  prog [-h] [-a A]

A program.

optional arguments:
  -h, --help  Print this help message and exit
  -a          Arg.

response files:
  Write '@file' to load a file containing command line arguments.
)");
    }

    // missing positional
    {
        std::ostringstream os;
        try {
            std::vector<std::string_view> args{"prog"};
            po2::parse_command_line(
                args,
                "A program.",
                os,
                po2::positional("pos", "Pos."));
        } catch (int) {
        }
        EXPECT_EQ(os.str(), R"(error: one or more missing positional arguments, starting with 'POS'

usage:  prog [-h] POS

A program.

positional arguments:
  pos         Pos.

optional arguments:
  -h, --help  Print this help message and exit

response files:
  Write '@file' to load a file containing command line arguments.
)");
    }
}

struct cleaner
{
    cleaner(int fd = -1) : fd_(fd) {}
    ~cleaner() {
#if defined(UNIX_BUILD)
        BOOST_ASSERT(fd_ != -1);
        close(fd_);
#endif
#if BOOST_PROGRAM_OPTIONS_2_USE_STD_FILESYSTEM
        namespace fs = std::filesystem;
#else
        namespace fs = boost::filesystem;
#endif
        fs::remove_all("test_dir");
    }

private:
    int fd_;
};

TEST(parse_command_line, validators)
{
#if BOOST_PROGRAM_OPTIONS_2_USE_STD_FILESYSTEM
    namespace fs = std::filesystem;
#else
    namespace fs = boost::filesystem;
#endif
    fs::remove_all("test_dir");
    fs::create_directory("test_dir");
    fs::create_directory("test_dir/dir");
    std::ofstream("test_dir/file"); // "touch file"

#if defined(UNIX_BUILD)
    mkfifo("test_dir/pipe", 0644);
    sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    std::strcpy(addr.sun_path, "test_dir/socket");
    int fd = socket(PF_UNIX, SOCK_STREAM, 0);
    bind(fd, (sockaddr *)&addr, sizeof(addr));
    fs::create_symlink("file", "test_dir/symlink");

    cleaner _(fd);
#else
    cleaner _;
#endif

    // path()
    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog", "-a", "test_dir/dir", "-b", "test_dir/dir"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::argument("-a", "A string."),
            po2::path(
                po2::argument("-b", "A string which must refer to a path.")));
        BOOST_MPL_ASSERT(
            (is_same<
                decltype(result),
                tuple<opt<std::string_view>, opt<std::string_view>>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(result[0_c], "test_dir/dir");
        EXPECT_TRUE(result[1_c]);
        EXPECT_EQ(result[1_c], "test_dir/dir");
    }
    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog", "-a", "test_dir/file", "-b", "test_dir/file"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::argument("-a", "A string."),
            po2::path(
                po2::argument("-b", "A string which must refer to a path.")));
        BOOST_MPL_ASSERT(
            (is_same<
                decltype(result),
                tuple<opt<std::string_view>, opt<std::string_view>>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(result[0_c], "test_dir/file");
        EXPECT_TRUE(result[1_c]);
        EXPECT_EQ(result[1_c], "test_dir/file");
    }
#if defined(UNIX_BUILD)
    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog", "-a", "test_dir/pipe", "-b", "test_dir/pipe"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::argument("-a", "A string."),
            po2::path(
                po2::argument("-b", "A string which must refer to a path.")));
        BOOST_MPL_ASSERT(
            (is_same<
                decltype(result),
                tuple<opt<std::string_view>, opt<std::string_view>>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(result[0_c], "test_dir/pipe");
        EXPECT_TRUE(result[1_c]);
        EXPECT_EQ(result[1_c], "test_dir/pipe");
    }
    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog", "-a", "test_dir/socket", "-b", "test_dir/socket"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::argument("-a", "A string."),
            po2::path(
                po2::argument("-b", "A string which must refer to a path.")));
        BOOST_MPL_ASSERT(
            (is_same<
                decltype(result),
                tuple<opt<std::string_view>, opt<std::string_view>>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(result[0_c], "test_dir/socket");
        EXPECT_TRUE(result[1_c]);
        EXPECT_EQ(result[1_c], "test_dir/socket");
    }
    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog", "-a", "test_dir/symlink", "-b", "test_dir/symlink"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::argument("-a", "A string."),
            po2::path(
                po2::argument("-b", "A string which must refer to a path.")));
        BOOST_MPL_ASSERT(
            (is_same<
                decltype(result),
                tuple<opt<std::string_view>, opt<std::string_view>>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(result[0_c], "test_dir/symlink");
        EXPECT_TRUE(result[1_c]);
        EXPECT_EQ(result[1_c], "test_dir/symlink");
    }
    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog", "-a", "/dev/null", "-b", "/dev/null"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::argument("-a", "A string."),
            po2::path(
                po2::argument("-b", "A string which must refer to a path.")));
        BOOST_MPL_ASSERT(
            (is_same<
                decltype(result),
                tuple<opt<std::string_view>, opt<std::string_view>>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(result[0_c], "/dev/null");
        EXPECT_TRUE(result[1_c]);
        EXPECT_EQ(result[1_c], "/dev/null");
    }
#endif

    // dir()
    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog", "-a", "test_dir/dir", "-b", "test_dir/dir"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::argument("-a", "A string."),
            po2::directory(po2::argument(
                "-b", "A string which must refer to a directory.")));
        BOOST_MPL_ASSERT(
            (is_same<
                decltype(result),
                tuple<opt<std::string_view>, opt<std::string_view>>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(result[0_c], "test_dir/dir");
        EXPECT_TRUE(result[1_c]);
        EXPECT_EQ(result[1_c], "test_dir/dir");
    }

    // file()
    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog", "-a", "test_dir/file", "-b", "test_dir/file"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::argument("-a", "A string."),
            po2::file(po2::argument(
                "-b", "A string which must refer to a file.")));
        BOOST_MPL_ASSERT(
            (is_same<
                decltype(result),
                tuple<opt<std::string_view>, opt<std::string_view>>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(result[0_c], "test_dir/file");
        EXPECT_TRUE(result[1_c]);
        EXPECT_EQ(result[1_c], "test_dir/file");
    }
#if defined(UNIX_BUILD)
    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog", "-a", "test_dir/pipe", "-b", "test_dir/pipe"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::argument("-a", "A string."),
            po2::file(
                po2::argument("-b", "A string which must refer to a path.")));
        BOOST_MPL_ASSERT(
            (is_same<
                decltype(result),
                tuple<opt<std::string_view>, opt<std::string_view>>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(result[0_c], "test_dir/pipe");
        EXPECT_TRUE(result[1_c]);
        EXPECT_EQ(result[1_c], "test_dir/pipe");
    }
    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog", "-a", "test_dir/socket", "-b", "test_dir/socket"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::argument("-a", "A string."),
            po2::file(
                po2::argument("-b", "A string which must refer to a path.")));
        BOOST_MPL_ASSERT(
            (is_same<
                decltype(result),
                tuple<opt<std::string_view>, opt<std::string_view>>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(result[0_c], "test_dir/socket");
        EXPECT_TRUE(result[1_c]);
        EXPECT_EQ(result[1_c], "test_dir/socket");
    }
    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog", "-a", "test_dir/symlink", "-b", "test_dir/symlink"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::argument("-a", "A string."),
            po2::file(
                po2::argument("-b", "A string which must refer to a path.")));
        BOOST_MPL_ASSERT(
            (is_same<
                decltype(result),
                tuple<opt<std::string_view>, opt<std::string_view>>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(result[0_c], "test_dir/symlink");
        EXPECT_TRUE(result[1_c]);
        EXPECT_EQ(result[1_c], "test_dir/symlink");
    }
    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog", "-a", "/dev/null", "-b", "/dev/null"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::argument("-a", "A string."),
            po2::file(
                po2::argument("-b", "A string which must refer to a path.")));
        BOOST_MPL_ASSERT(
            (is_same<
                decltype(result),
                tuple<opt<std::string_view>, opt<std::string_view>>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(result[0_c], "/dev/null");
        EXPECT_TRUE(result[1_c]);
        EXPECT_EQ(result[1_c], "/dev/null");
    }
#endif

    // validation error: path not found
    {
        std::ostringstream os;
        try {
            std::vector<std::string_view> args{
                "prog", "-a", "test_dir/nonesuch"};
            po2::parse_command_line(
                args, "A program.", os, po2::path(po2::argument("-a", "Arg.")));
        } catch (int) {
        }
        EXPECT_EQ(os.str(), R"(error: path 'test_dir/nonesuch' not found

usage:  prog [-h] [-a A]

A program.

optional arguments:
  -h, --help  Print this help message and exit
  -a          Arg.

response files:
  Write '@file' to load a file containing command line arguments.
)");
    }

    // validation error: directory not found
    {
        std::ostringstream os;
        try {
            std::vector<std::string_view> args{
                "prog", "-a", "test_dir/nonesuch"};
            po2::parse_command_line(
                args,
                "A program.",
                os,
                po2::directory(po2::argument("-a", "Arg.")));
        } catch (int) {
        }
        EXPECT_EQ(os.str(), R"(error: directory 'test_dir/nonesuch' not found

usage:  prog [-h] [-a A]

A program.

optional arguments:
  -h, --help  Print this help message and exit
  -a          Arg.

response files:
  Write '@file' to load a file containing command line arguments.
)");
    }

    // validation error: file not found
    {
        std::ostringstream os;
        try {
            std::vector<std::string_view> args{
                "prog", "-a", "test_dir/nonesuch"};
            po2::parse_command_line(
                args, "A program.", os, po2::file(po2::argument("-a", "Arg.")));
        } catch (int) {
        }
        EXPECT_EQ(os.str(), R"(error: file 'test_dir/nonesuch' not found

usage:  prog [-h] [-a A]

A program.

optional arguments:
  -h, --help  Print this help message and exit
  -a          Arg.

response files:
  Write '@file' to load a file containing command line arguments.
)");
    }

    // validation error: found file not directory
    {
        std::ostringstream os;
        try {
            std::vector<std::string_view> args{"prog", "-a", "test_dir/file"};
            po2::parse_command_line(
                args,
                "A program.",
                os,
                po2::directory(po2::argument("-a", "Arg.")));
        } catch (int) {
        }
        EXPECT_EQ(os.str(), R"(error: 'test_dir/file' is a file, but a directory was expected

usage:  prog [-h] [-a A]

A program.

optional arguments:
  -h, --help  Print this help message and exit
  -a          Arg.

response files:
  Write '@file' to load a file containing command line arguments.
)");
    }
#if defined(UNIX_BUILD)
    {
        std::ostringstream os;
        try {
            std::vector<std::string_view> args{"prog", "-a", "test_dir/pipe"};
            po2::parse_command_line(
                args,
                "A program.",
                os,
                po2::directory(po2::argument("-a", "Arg.")));
        } catch (int) {
        }
        EXPECT_EQ(os.str(), R"(error: 'test_dir/pipe' is a file, but a directory was expected

usage:  prog [-h] [-a A]

A program.

optional arguments:
  -h, --help  Print this help message and exit
  -a          Arg.

response files:
  Write '@file' to load a file containing command line arguments.
)");
    }
    {
        std::ostringstream os;
        try {
            std::vector<std::string_view> args{"prog", "-a", "test_dir/socket"};
            po2::parse_command_line(
                args,
                "A program.",
                os,
                po2::directory(po2::argument("-a", "Arg.")));
        } catch (int) {
        }
        EXPECT_EQ(os.str(), R"(error: 'test_dir/socket' is a file, but a directory was expected

usage:  prog [-h] [-a A]

A program.

optional arguments:
  -h, --help  Print this help message and exit
  -a          Arg.

response files:
  Write '@file' to load a file containing command line arguments.
)");
    }
    {
        std::ostringstream os;
        try {
            std::vector<std::string_view> args{"prog", "-a", "test_dir/symlink"};
            po2::parse_command_line(
                args,
                "A program.",
                os,
                po2::directory(po2::argument("-a", "Arg.")));
        } catch (int) {
        }
        EXPECT_EQ(os.str(), R"(error: 'test_dir/symlink' is a file, but a directory was expected

usage:  prog [-h] [-a A]

A program.

optional arguments:
  -h, --help  Print this help message and exit
  -a          Arg.

response files:
  Write '@file' to load a file containing command line arguments.
)");
    }
#endif

    // validation error: found directory not file
    {
        std::ostringstream os;
        try {
            std::vector<std::string_view> args{"prog", "-a", "test_dir/dir"};
            po2::parse_command_line(
                args, "A program.", os, po2::file(po2::argument("-a", "Arg.")));
        } catch (int) {
        }
        EXPECT_EQ(os.str(), R"(error: 'test_dir/dir' is a directory, but a file was expected

usage:  prog [-h] [-a A]

A program.

optional arguments:
  -h, --help  Print this help message and exit
  -a          Arg.

response files:
  Write '@file' to load a file containing command line arguments.
)");
    }
}

#define ARGUMENTS(T, choice0, choice1, choice2)                                \
    po2::argument<T>("-a,--abacus", "The abacus."),                            \
        po2::argument<std::optional<T>>(                                       \
            "-b,--bobcat", "The bobcat.", po2::zero_or_one),                   \
        po2::argument<std::vector<T>>("-c,--cataphract", "The cataphract", 2), \
        po2::argument<T>(                                                      \
            "-d,--dolemite", "*The* Dolemite.", 1, choice0, choice1, choice2), \
        po2::argument<std::vector<T>>(                                         \
            "-z,--zero-plus", "None is fine; so is more.", po2::zero_or_more), \
        po2::argument<std::set<T>>(                                            \
            "-o,--one-plus", "One is fine; so is more.", po2::one_or_more)

#define ARGUMENTS_WITH_DEFAULTS(T, choice0, choice1, choice2, default_)        \
    po2::with_default(                                                         \
        po2::argument<T>("-a,--abacus", "The abacus."), default_),             \
        po2::with_default(                                                     \
            po2::argument<std::optional<T>>(                                   \
                "-b,--bobcat", "The bobcat.", po2::zero_or_one),               \
            default_),                                                         \
        po2::with_default(                                                     \
            po2::argument<std::vector<T>>(                                     \
                "-c,--cataphract", "The cataphract", 2),                       \
            std::vector<T>({default_})),                                       \
        po2::with_default(                                                     \
            po2::argument<T>(                                                  \
                "-d,--dolemite",                                               \
                "*The* Dolemite.",                                             \
                1,                                                             \
                choice0,                                                       \
                choice1,                                                       \
                choice2),                                                      \
            choice2)

TEST(parse_command_line, arguments)
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
        auto result = po2::parse_command_line(
            args, "A program.", os, ARGUMENTS(int, 4, 5, 6));
        BOOST_MPL_ASSERT((is_same<
                          decltype(result),
                          tuple<
                              opt<int>,
                              opt<opt<int>>,
                              opt<std::vector<int>>,
                              opt<int>,
                              opt<std::vector<int>>,
                              opt<std::set<int>>>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(*result[0_c], 55);
        EXPECT_TRUE(result[1_c]);
        EXPECT_EQ(*result[1_c], 66);
        EXPECT_TRUE(result[2_c]);
        EXPECT_EQ(*result[2_c], std::vector<int>({77, 88}));
        EXPECT_TRUE(result[3_c]);
        EXPECT_EQ(*result[3_c], 5);
        EXPECT_TRUE(result[4_c]);
        EXPECT_EQ(*result[4_c], std::vector<int>({2}));
        EXPECT_TRUE(result[5_c]);
        EXPECT_EQ(*result[5_c], std::set<int>({2}));
    }
    {
        std::ostringstream os;
        std::vector<std::string_view> args{"prog", "-b", "-z"};
        auto result = po2::parse_command_line(
            args, "A program.", os, ARGUMENTS(int, 4, 5, 6));
        BOOST_MPL_ASSERT((is_same<
                          decltype(result),
                          tuple<
                              opt<int>,
                              opt<opt<int>>,
                              opt<std::vector<int>>,
                              opt<int>,
                              opt<std::vector<int>>,
                              opt<std::set<int>>>>));
        EXPECT_FALSE(result[0_c]);
        EXPECT_TRUE(result[1_c]);
        EXPECT_EQ(*result[1_c], std::nullopt);
        EXPECT_FALSE(result[2_c]);
        EXPECT_FALSE(result[3_c]);
        EXPECT_TRUE(result[4_c]);
        EXPECT_EQ(*result[4_c], std::vector<int>({}));
        EXPECT_FALSE(result[5_c]);
    }
    {
        std::ostringstream os;
        std::vector<std::string_view> args{"prog", "-b", "-z"};
        auto result = po2::parse_command_line(
            args, "A program.", os, ARGUMENTS(double, 4, 5, 6));
        BOOST_MPL_ASSERT((is_same<
                          decltype(result),
                          tuple<
                              opt<double>,
                              opt<opt<double>>,
                              opt<std::vector<double>>,
                              opt<double>,
                              opt<std::vector<double>>,
                              opt<std::set<double>>>>));
        EXPECT_FALSE(result[0_c]);
        EXPECT_TRUE(result[1_c]);
        EXPECT_EQ(*result[1_c], std::nullopt);
        EXPECT_FALSE(result[2_c]);
        EXPECT_FALSE(result[3_c]);
        EXPECT_TRUE(result[4_c]);
        EXPECT_EQ(*result[4_c], std::vector<double>({}));
        EXPECT_FALSE(result[5_c]);
    }
    {
        std::ostringstream os;
        std::vector<std::string_view> args{"prog"};
        auto result = po2::parse_command_line(
            args, "A program.", os, ARGUMENTS_WITH_DEFAULTS(int, 4, 5, 6, 42));
        BOOST_MPL_ASSERT((is_same<
                          decltype(result),
                          tuple<
                              opt<int>,
                              opt<opt<int>>,
                              opt<std::vector<int>>,
                              opt<int>>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(*result[0_c], 42);
        EXPECT_TRUE(result[1_c]);
        EXPECT_EQ(*result[1_c], 42);
        EXPECT_TRUE(result[2_c]);
        EXPECT_EQ(*result[2_c], std::vector<int>({42}));
        EXPECT_TRUE(result[3_c]);
        EXPECT_EQ(*result[3_c], 6);
    }
    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog", "--dolemite", "5", "--bobcat", "66", "-c", "77", "88"};
        auto result = po2::parse_command_line(
            args, "A program.", os, ARGUMENTS_WITH_DEFAULTS(int, 4, 5, 6, 42));
        BOOST_MPL_ASSERT((is_same<
                          decltype(result),
                          tuple<
                              opt<int>,
                              opt<opt<int>>,
                              opt<std::vector<int>>,
                              opt<int>>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(*result[0_c], 42);
        EXPECT_TRUE(result[1_c]);
        EXPECT_EQ(*result[1_c], 66);
        EXPECT_TRUE(result[2_c]);
        EXPECT_EQ(*result[2_c], std::vector<int>({77, 88}));
        EXPECT_TRUE(result[3_c]);
        EXPECT_EQ(*result[3_c], 5);
    }

    {
        std::ostringstream os;
        std::vector<std::string_view> args{"prog", "-z", "3"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::argument<std::vector<int>>(
                "-z,--zero-plus",
                "None is fine; so is more.",
                po2::zero_or_more));
        BOOST_MPL_ASSERT(
            (is_same<decltype(result), tuple<opt<std::vector<int>>>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(*result[0_c], std::vector<int>({3}));
    }
    {
        std::ostringstream os;
        std::vector<std::string_view> args{"prog", "-z", "3.0"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::argument<std::vector<double>>(
                "-z,--zero-plus",
                "None is fine; so is more.",
                po2::zero_or_more));
        BOOST_MPL_ASSERT(
            (is_same<decltype(result), tuple<opt<std::vector<double>>>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(*result[0_c], std::vector<double>({3.0}));
    }

    {
        std::ostringstream os;
        std::vector<std::string_view> args{"prog", "-c", "77", "88.8"};
        try {
            po2::parse_command_line(
                args, "A program.", os, ARGUMENTS(int, 4, 5, 6));
        } catch (int) {
        }
        EXPECT_EQ(os.str(), R"(error: cannot parse argument '88.8'

usage:  prog [-h] [-a A] [-b [B]] [-c C C] [-d {4,5,6}] [-z [Z ...]] [-o O ...]

A program.

optional arguments:
  -h, --help        Print this help message and exit
  -a, --abacus      The abacus.
  -b, --bobcat      The bobcat.
  -c, --cataphract  The cataphract
  -d, --dolemite    *The* Dolemite.
  -z, --zero-plus   None is fine; so is more.
  -o, --one-plus    One is fine; so is more.

response files:
  Write '@file' to load a file containing command line arguments.
)");
    }
}

#undef ARGUMENTS
#undef ARGUMENTS_WITH_DEFAULT

#define POSITIONALS(T, choice0, choice1, choice2)                              \
    po2::positional<T>("abacus", "The abacus."),                               \
        po2::positional<std::optional<T>>("bobcat", "The bobcat."),            \
        po2::positional<std::vector<T>>("cataphract", "The cataphract", 2),    \
        po2::positional<T>(                                                    \
            "dolemite", "*The* Dolemite.", 1, choice0, choice1, choice2),      \
        po2::positional<std::set<T>>(                                          \
            "one-plus", "One is fine; so is more.", po2::one_or_more)

TEST(parse_command_line, positionals)
{
    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog", "55", "66", "77", "88", "5", "2"};
        auto result = po2::parse_command_line(
            args, "A program.", os, POSITIONALS(int, 4, 5, 6));
        BOOST_MPL_ASSERT(
            (is_same<
                decltype(result),
                tuple<int, opt<int>, std::vector<int>, int, std::set<int>>>));
        EXPECT_EQ(result[0_c], 55);
        EXPECT_TRUE(result[1_c]);
        EXPECT_EQ(*result[1_c], 66);
        EXPECT_EQ(result[2_c], std::vector<int>({77, 88}));
        EXPECT_EQ(result[3_c], 5);
        EXPECT_EQ(result[4_c], std::set<int>({2}));
    }
    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog", "55", "66", "77", "88", "5", "2"};
        auto result = po2::parse_command_line(
            args, "A program.", os, POSITIONALS(double, 4, 5, 6));
        BOOST_MPL_ASSERT((is_same<
                          decltype(result),
                          tuple<
                              double,
                              opt<double>,
                              std::vector<double>,
                              double,
                              std::set<double>>>));
        EXPECT_EQ(result[0_c], 55);
        EXPECT_TRUE(result[1_c]);
        EXPECT_EQ(*result[1_c], 66);
        EXPECT_EQ(result[2_c], std::vector<double>({77, 88}));
        EXPECT_EQ(result[3_c], 5);
        EXPECT_EQ(result[4_c], std::set<double>({2}));
    }
    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog", "55", "66", "77", "88", "5", "2"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::remainder("args", "other args at the end"));
        BOOST_MPL_ASSERT(
            (is_same<decltype(result), tuple<std::vector<std::string_view>>>));
        EXPECT_EQ(
            result[0_c],
            std::vector<std::string_view>({"55", "66", "77", "88", "5", "2"}));
    }
    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog", "55", "66", "77", "88", "5", "2"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::remainder<std::vector<int>>("args", "other args at the end"));
        BOOST_MPL_ASSERT((is_same<decltype(result), tuple<std::vector<int>>>));
        EXPECT_EQ(result[0_c], std::vector<int>({55, 66, 77, 88, 5, 2}));
    }
}

#undef POSITIONALS

#define MIXED(T, choice0, choice1, choice2, default_)                          \
    po2::argument<T>("-a,--abacus", "The abacus."),                            \
        po2::with_default(                                                     \
            po2::argument<std::optional<T>>(                                   \
                "-b,--bobcat", "The bobcat.", po2::zero_or_one),               \
            default_),                                                         \
        po2::positional<std::vector<T>>("cataphract", "The cataphract", 2),    \
        po2::argument<T>(                                                      \
            "-d,--dolemite", "*The* Dolemite.", 1, choice0, choice1, choice2), \
        po2::argument<std::vector<T>>(                                         \
            "-z,--zero-plus", "None is fine; so is more.", po2::zero_or_more), \
        po2::remainder("args", "other args at the end")

TEST(parse_command_line, mixed)
{
    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog", "-a", "55", "-b", "66", "77", "88", "-d", "5", "2"};
        auto result = po2::parse_command_line(
            args, "A program.", os, MIXED(int, 4, 5, 6, 42));
        BOOST_MPL_ASSERT((is_same<
                          decltype(result),
                          tuple<
                              opt<int>,
                              opt<opt<int>>,
                              std::vector<int>,
                              opt<int>,
                              opt<std::vector<int>>,
                              std::vector<std::string_view>>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(*result[0_c], 55);
        EXPECT_TRUE(result[1_c]);
        EXPECT_EQ(*result[1_c], std::optional<int>{66});
        EXPECT_EQ(result[2_c], std::vector<int>({77, 88}));
        EXPECT_TRUE(result[3_c]);
        EXPECT_EQ(*result[3_c], 5);
        EXPECT_FALSE(result[4_c]);
        EXPECT_EQ(result[5_c], std::vector<std::string_view>({"2"}));
    }
    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog", "-a", "55", "-b", "66", "77", "88", "-d", "5", "-z", "2"};
        auto result = po2::parse_command_line(
            args, "A program.", os, MIXED(int, 4, 5, 6, 42));
        BOOST_MPL_ASSERT((is_same<
                          decltype(result),
                          tuple<
                              opt<int>,
                              opt<opt<int>>,
                              std::vector<int>,
                              opt<int>,
                              opt<std::vector<int>>,
                              std::vector<std::string_view>>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(*result[0_c], 55);
        EXPECT_TRUE(result[1_c]);
        EXPECT_EQ(*result[1_c], std::optional<int>{66});
        EXPECT_EQ(result[2_c], std::vector<int>({77, 88}));
        EXPECT_TRUE(result[3_c]);
        EXPECT_EQ(*result[3_c], 5);
        EXPECT_TRUE(result[4_c]);
        EXPECT_EQ(*result[4_c], std::vector<int>({2}));
        EXPECT_EQ(result[5_c], std::vector<std::string_view>({}));
    }

    {
        std::ostringstream os;
        std::vector<std::string_view> args{"prog", "-3.0"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::remainder<std::vector<double>>(
                "args", "other args at the end"));
        BOOST_MPL_ASSERT((is_same<
                          decltype(result),
                          tuple<std::vector<double>>>));
        EXPECT_EQ(result[0_c], std::vector<double>({-3.0}));
    }
    {
        std::ostringstream os;
        std::vector<std::string_view> args{"prog", "-z", "2", "3.0"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::argument<std::vector<int>>(
                "-z,--zero-plus",
                "None is fine; so is more.",
                po2::zero_or_more),
            po2::remainder<std::vector<double>>(
                "args", "other args at the end"));
        BOOST_MPL_ASSERT((is_same<
                          decltype(result),
                          tuple<opt<std::vector<int>>, std::vector<double>>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(*result[0_c], std::vector<int>({2}));
        EXPECT_EQ(result[1_c], std::vector<double>({3.0}));
    }
}

#undef MIXED

TEST(parse_command_line, flags)
{
    {
        std::ostringstream os;
        std::vector<std::string_view> args{"prog"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::flag("-f", "F."),
            po2::inverted_flag("-g", "G."));
        BOOST_MPL_ASSERT((is_same<decltype(result), tuple<bool, bool>>));
        EXPECT_EQ(result[0_c], false);
        EXPECT_EQ(result[1_c], true);
    }
    {
        std::ostringstream os;
        std::vector<std::string_view> args{"prog", "-f", "-g"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::flag("-f", "F."),
            po2::inverted_flag("-g", "G."));
        BOOST_MPL_ASSERT((is_same<decltype(result), tuple<bool, bool>>));
        EXPECT_EQ(result[0_c], true);
        EXPECT_EQ(result[1_c], false);
    }
}

TEST(parse_command_line, counted_flags)
{
    {
        std::ostringstream os;
        std::vector<std::string_view> args{"prog"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::counted_flag("-v,--verbose", "Verbosity level."));
        BOOST_MPL_ASSERT((is_same<decltype(result), tuple<opt<int>>>));
        EXPECT_FALSE(result[0_c]);
    }
    {
        std::ostringstream os;
        std::vector<std::string_view> args{"prog", "-v"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::counted_flag("-v,--verbose", "Verbosity level."));
        BOOST_MPL_ASSERT((is_same<decltype(result), tuple<opt<int>>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(*result[0_c], 1);
    }
    {
        std::ostringstream os;
        std::vector<std::string_view> args{"prog", "--verbose"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::counted_flag("-v,--verbose", "Verbosity level."));
        BOOST_MPL_ASSERT((is_same<decltype(result), tuple<opt<int>>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(*result[0_c], 1);
    }
    {
        std::ostringstream os;
        std::vector<std::string_view> args{"prog", "-vvvv"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::counted_flag("-v,--verbose", "Verbosity level."));
        BOOST_MPL_ASSERT((is_same<decltype(result), tuple<opt<int>>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(*result[0_c], 4);
    }
}

TEST(parse_command_line, numeric_named_argument)
{
    {
        std::ostringstream os;
        std::vector<std::string_view> args{"prog", "-a", "-1", "-2"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::argument<std::vector<int>>("-a", "A.", po2::one_or_more));
        BOOST_MPL_ASSERT(
            (is_same<decltype(result), tuple<opt<std::vector<int>>>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(*result[0_c], std::vector<int>({-1, -2}));
    }
    {
        std::ostringstream os;
        std::vector<std::string_view> args{"prog", "-a", "-1", "-2"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::argument<std::vector<int>>("-a", "A.", po2::one_or_more),
            po2::flag("-2", "Two."));
        BOOST_MPL_ASSERT(
            (is_same<decltype(result), tuple<opt<std::vector<int>>, bool>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(*result[0_c], std::vector<int>({-1}));
        EXPECT_TRUE(result[1_c]);
    }
    {
        std::ostringstream os;
        std::vector<std::string_view> args{"prog", "-a", "-2"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::argument<std::vector<int>>("-a", "A.", po2::zero_or_more),
            po2::flag("-2", "Two."));
        BOOST_MPL_ASSERT(
            (is_same<decltype(result), tuple<opt<std::vector<int>>, bool>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(*result[0_c], std::vector<int>({}));
        EXPECT_TRUE(result[1_c]);
    }
}

TEST(parse_command_line, response_files)
{
    {
        std::ofstream ofs("simple_response_file");
        ofs << " -a\n-1 \n";
    }
    {
        std::ofstream ofs("other_simple_response_file");
        ofs << "@simple_response_file";
    }

    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog", "-a", "-1", "-2", "@simple_response_file"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::argument<std::vector<int>>("-a", "A.", po2::one_or_more));
        BOOST_MPL_ASSERT(
            (is_same<decltype(result), tuple<opt<std::vector<int>>>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(*result[0_c], std::vector<int>({-1, -2, -1}));
    }

    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog", "@simple_response_file", "-a", "-1", "-2"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::argument<std::vector<int>>("-a", "A.", po2::one_or_more));
        BOOST_MPL_ASSERT(
            (is_same<decltype(result), tuple<opt<std::vector<int>>>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(*result[0_c], std::vector<int>({-1, -1, -2}));
    }

    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog", "--config", "simple_response_file", "-a", "-1", "-2"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::argument<std::vector<int>>("-a", "A.", po2::one_or_more),
            po2::response_file("--config"));
        BOOST_MPL_ASSERT((is_same<
                          decltype(result),
                          tuple<opt<std::vector<int>>, po2::no_value>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(*result[0_c], std::vector<int>({-1, -1, -2}));
    }

    {
        std::ostringstream os;
        std::vector<std::string_view> args{
            "prog", "--config", "other_simple_response_file", "-a", "-1", "-2"};
        auto result = po2::parse_command_line(
            args,
            "A program.",
            os,
            po2::argument<std::vector<int>>("-a", "A.", po2::one_or_more),
            po2::response_file("--config"));
        BOOST_MPL_ASSERT((is_same<
                          decltype(result),
                          tuple<opt<std::vector<int>>, po2::no_value>>));
        EXPECT_TRUE(result[0_c]);
        EXPECT_EQ(*result[0_c], std::vector<int>({-1, -1, -2}));
    }

    {
        po2::customizable_strings strings;
        strings.response_file_description = "";

        std::ostringstream os;
        try {
            std::vector<std::string_view> args{
                "prog", "@simple_response_file", "-a", "-1", "-2"};
            auto result = po2::parse_command_line(
                args,
                "A program.",
                os,
                strings,
                po2::argument<std::vector<int>>("-a", "A.", po2::one_or_more));
            BOOST_MPL_ASSERT(
                (is_same<decltype(result), tuple<opt<std::vector<int>>>>));
        } catch (int) {
        }
        EXPECT_EQ(os.str(), R"(error: unrecognized argument '@simple_response_file'

usage:  prog [-h] [-a A ...]

A program.

optional arguments:
  -h, --help  Print this help message and exit
  -a          A.
)");
    }
}
