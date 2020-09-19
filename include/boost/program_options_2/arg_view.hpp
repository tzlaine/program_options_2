// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PROGRAM_OPTIONS_2_ARG_VIEW_HPP
#define BOOST_PROGRAM_OPTIONS_2_ARG_VIEW_HPP

#include <boost/program_options_2/config.hpp>

#include <boost/assert.hpp>
#include <boost/stl_interfaces/iterator_interface.hpp>
#include <boost/stl_interfaces/view_interface.hpp>

#include <iterator>

#if defined(_MSC_VER)
#include <cctype>
#endif


namespace boost { namespace program_options_2 {

    /** TODO */
    template<typename Char>
    struct arg_iter : stl_interfaces::proxy_iterator_interface<
                          arg_iter<Char>,
                          std::random_access_iterator_tag,
                          std::basic_string_view<Char>>
    {
        arg_iter() = default;
        arg_iter(Char const ** ptr) : it_(ptr) { BOOST_ASSERT(ptr); }

        std::basic_string_view<Char> operator*() const { return {*it_}; }

    private:
        friend stl_interfaces::access;
        Char const **& base_reference() noexcept { return it_; }
        Char const ** base_reference() const noexcept { return it_; }
        Char const ** it_;
    };

    /** TODO */
    template<typename Char>
    struct arg_view : stl_interfaces::view_interface<arg_view<Char>>
    {
        using iterator = arg_iter<Char>;

        arg_view() = default;
        arg_view(int argc, Char const ** argv) :
            first_(argv), last_(argv + argc)
        {}

        iterator begin() const { return first_; }
        iterator end() const { return last_; }

    private:
        iterator first_;
        iterator last_;
    };

#if defined(BOOST_PROGRAM_OPTIONS_2_DOXYGEN) || defined(_MSC_VER)

    namespace detail {
        template<typename Char>
        struct string_view_arrow_result
        {
            using string_view = std::basic_string_view<Char>;

            string_view_arrow_result(std::basic_string<Char> const & str) :
                value_(str)
            {}

            string_view const * operator->() const noexcept { return &value_; }
            string_view * operator->() noexcept { return &value_; }

        private:
            string_view value_;
        };
    }

    /** TODO */
    template<typename Char>
    struct winmain_arg_iter : stl_interfaces::proxy_iterator_interface<
                                  winmain_arg_iter<Char>,
                                  std::forward_iterator_tag,
                                  std::basic_string_view<Char>,
                                  std::basic_string_view<Char>,
                                  detail::string_view_arrow_result<Char>>
    {
        winmain_arg_iter() = default;

        explicit winmain_arg_iter(Char const * ptr) : ptr_(ptr)
        {
            BOOST_ASSERT(ptr);
            operator++();
        }

        std::basic_string_view<Char> operator*() const { return current_; }

        winmain_arg_iter & operator++()
        {
            current_.clear();

            text::null_sentinel last;

            ptr_ = std::ranges::find_if(
                ptr_, last, [](unsigned char c) { !std::isspace(c); });

            if (ptr_ == last)
                return *this;

            bool in_quotes = false;
            int backslashes = 0;

            for (; ptr_ != last; ++ptr_) {
                if (*ptr_ == '"') {
                    if (backslashes % 2 == 0) {
                        current_.append(backslashes / 2, '\\');
                        in_quotes = !in_quotes;
                    } else {
                        current_.append(backslashes / 2, '\\');
                        current_ += '"';
                    }
                    backslashes = 0;
                } else if (*ptr_ == '\\') {
                    ++backslashes;
                } else {
                    if (backslashes) {
                        current_.append(backslashes, '\\');
                        backslashes = 0;
                    }
                    if (std::isspace((unsigned char)*ptr_) && !in_quotes)
                        return *this;
                    else
                        current_ += *ptr_;
                }
            }

            if (backslashes)
                current_.append(backslashes, '\\');

            return *this;
        }

        friend bool
        operator==(winmain_arg_iter lhs, text::null_sentinel rhs) noexcept
        {
            return lhs.ptr_ == rhs;
        }

        using base_type = stl_interfaces::proxy_iterator_interface<
            winmain_arg_iter<Char>,
            std::forward_iterator_tag,
            std::basic_string_view<Char>,
            std::basic_string_view<Char>,
            string_view_arrow_result<Char>>;
        using base_type::operator++;

    private:
        Char const * ptr_;
        std::basc_string<Char> current_;
    };

    /** TODO */
    template<typename Char>
    struct winmain_arg_view
        : stl_interfaces::view_interface<winmain_arg_view<Char>>
    {
        using iterator = winmain_arg_iter<Char>;

        winmain_arg_view() = default;
        winmain_arg_view(Char const * args) : first_(args) {}

        iterator begin() const { return first_; }
        text::null_sentinel end() const { return text::null_sentinel{}; }

    private:
        iterator first_;
    };

#endif

}}

#endif
