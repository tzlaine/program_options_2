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

#include <algorithm>
#include <iterator>

#if defined(_MSC_VER)
#include <cctype>
#endif


namespace boost { namespace program_options_2 {

    namespace detail {
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
    }

    /** TODO */
    template<typename Char>
    struct arg_view : stl_interfaces::view_interface<arg_view<Char>>
    {
        using iterator = detail::arg_iter<Char>;

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

#if defined(BOOST_PROGRAM_OPTIONS_2_DOXYGEN) || defined(_MSC_VER)

    namespace detial {
        template<typename Char>
        struct winmain_arg_iter : stl_interfaces::proxy_iterator_interface<
                                      winmain_arg_iter<Char>,
                                      std::forward_iterator_tag,
                                      std::basic_string_view<Char>,
                                      std::basic_string_view<Char>,
                                      detail::string_view_arrow_result<Char>>
        {
            winmain_arg_iter() = default;

            explicit winmain_arg_iter(
                Char const * ptr, std::basc_string<Char> & current) :
                ptr_(ptr), current_(current)
            {
                BOOST_ASSERT(ptr);
                operator++();
            }

            std::basic_string_view<Char> operator*() const { return *current_; }

            winmain_arg_iter & operator++()
            {
                current_->clear();

                text::null_sentinel last;

                ptr_ = std::ranges::find_if(ptr_, last, [](unsigned char c) {
                    return !std::isspace(c);
                });

                if (ptr_ == last)
                    return *this;

                bool in_quotes = false;
                int backslashes = 0;

                for (; ptr_ != last; ++ptr_) {
                    if (*ptr_ == '"') {
                        if (backslashes % 2 == 0) {
                            current_->append(backslashes / 2, '\\');
                            in_quotes = !in_quotes;
                        } else {
                            current_->append(backslashes / 2, '\\');
                            *current_ += '"';
                        }
                        backslashes = 0;
                    } else if (*ptr_ == '\\') {
                        ++backslashes;
                    } else {
                        if (backslashes) {
                            current_->append(backslashes, '\\');
                            backslashes = 0;
                        }
                        if (std::isspace((unsigned char)*ptr_) && !in_quotes)
                            return *this;
                        else
                            *current_ += *ptr_;
                    }
                }

                if (backslashes)
                    current_->append(backslashes, '\\');

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
            Char const * ptr_ = nullptr;
            std::basc_string<Char> * current_ = nullptr;
        };
    }

    /** TODO */
    template<typename Char>
    struct winmain_arg_view
        : stl_interfaces::view_interface<winmain_arg_view<Char>>
    {
        using iterator = winmain_arg_iter<Char>;

        winmain_arg_view() = default;
        winmain_arg_view(Char const * args) : current_(), first_(args, current_)
        {}

        iterator begin() const { return first_; }
        text::null_sentinel end() const { return text::null_sentinel{}; }

    private:
        std::basc_string<Char> current_;
        detail::iterator first_;
    };

#endif

    namespace detail {
        template<typename IStreamIter>
        struct response_file_arg_iter
            : stl_interfaces::proxy_iterator_interface<
                  response_file_arg_iter<IStreamIter>,
                  std::input_iterator_tag,
                  std::string_view,
                  std::string_view,
                  detail::string_view_arrow_result<char>>
        {
            response_file_arg_iter() = default;

            explicit response_file_arg_iter(
                IStreamIter it, std::string & current) :
                it_(it), current_(&current)
            {
                operator++();
            }

            std::string_view operator*() const { return *current_; }

            response_file_arg_iter & operator++()
            {
                current_->clear();

                IStreamIter last;
                auto skip_comment = [&] {
                    it_ = std::ranges::find_if(it_, last, [&](unsigned char c) {
                        return 0xa <= c && c <= 0xd;
                    });
                    if (*it_ == 0xa)
                        ++it_;
                };
                auto skip_ws = [&] {
                    it_ = std::ranges::find_if(it_, last, [&](unsigned char c) {
                        return !std::isspace(c);
                    });
                };
                auto skip = [&] {
                    while (it_ != last) {
                        char const c = *it_;
                        if (std::isspace(c))
                            skip_ws();
                        else if (c == '#')
                            skip_comment();
                        else
                            break;
                    }
                };
                auto final_business = [&] {
                    skip();
                    if (2u <= current_->size() && current_->front() == '"' &&
                        current_->back() == '"') {
                        *current_ = std::string_view(
                            current_->data() + 1, current_->size() - 2);
                    }
                    if (it_ == last)
                        just_before_end_ = true;
                };

                skip();
                if (it_ == last) {
                    just_before_end_ = false;
                    return *this;
                }

                bool in_quotes = false;

                while (it_ != last) {
                    char const c = *it_++;
                    if (c == '"') { // unescaped quote
                        *current_ += c;
                        in_quotes = !in_quotes;
                        if (!in_quotes) {
                            final_business();
                            return *this;
                        }
                    } else if (in_quotes && c == '\\') {
                        if (it_ == last) {
                            *current_ += c;
                        } else {
                            char const next_c = *it_++;
                            if (next_c != '\\' && next_c != '"')
                                *current_ += c;
                            *current_ += next_c;
                        }
                    } else if (std::isspace((unsigned char)c) && !in_quotes) {
                        final_business();
                        return *this;
                    } else if (c == '#' && !in_quotes) {
                        final_business();
                        return *this;
                    } else {
                        *current_ += c;
                    }
                }

                final_business();
                return *this;
            }

            friend bool operator==(
                response_file_arg_iter lhs, response_file_arg_iter rhs) noexcept
            {
                return lhs.just_before_end_ == rhs.just_before_end_ &&
                       lhs.it_ == rhs.it_;
            }

            using base_type = stl_interfaces::proxy_iterator_interface<
                response_file_arg_iter<IStreamIter>,
                std::input_iterator_tag,
                std::string_view,
                std::string_view,
                string_view_arrow_result<char>>;
            using base_type::operator++;

        private:
            IStreamIter it_;
            std::string * current_ = nullptr;
            bool just_before_end_ = false;
        };

        struct response_file_arg_view
            : stl_interfaces::view_interface<response_file_arg_view>
        {
            using iterator =
                response_file_arg_iter<std::istream_iterator<char>>;

            response_file_arg_view() = default;
            response_file_arg_view(std::istream & is) :
                current_(), first_(std::istream_iterator<char>(is), current_)
            {}

            iterator begin() const { return first_; }
            iterator end() const { return {}; }

        private:
            std::string current_;
            iterator first_;
        };
    }

}}

#endif
