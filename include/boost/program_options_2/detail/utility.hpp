// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PROGRAM_OPTIONS_2_DETAIL_UTILITY_HPP
#define BOOST_PROGRAM_OPTIONS_2_DETAIL_UTILITY_HPP

#include <boost/program_options_2/fwd.hpp>

#include <boost/stl_interfaces/iterator_interface.hpp>
#include <boost/stl_interfaces/view_interface.hpp>

#if BOOST_PROGRAM_OPTIONS_2_USE_STD_FILESYSTEM
#include <filesystem>
#else
#include <boost/filesystem.hpp>
#endif


namespace boost { namespace program_options_2 { namespace detail {

    template<typename BidiIter, typename T>
    BidiIter find_last(BidiIter first, BidiIter last, T const & x)
    {
        auto it = last;
        while (it != first) {
            if (*--it == x)
                return it;
        }
        return last;
    }

#if BOOST_PROGRAM_OPTIONS_2_USE_STD_FILESYSTEM
    inline const auto fs_sep = std::filesystem::path::preferred_separator;
    using error_code = std::error_code;
#else
    inline const auto fs_sep = filesystem::path::preferred_separator;
    using error_code = system::error_code;
#endif

    // clang-format off
        template<typename T, typename U>
        concept insertable_from = requires(T t, U u) {
            t.insert(t.end(), u);
        };
    // clang-format on

    template<typename T, typename U, typename = hana::when<true>>
    struct is_insertable_from : std::false_type
    {};
    template<typename T, typename U>
    struct is_insertable_from<
        T,
        U,
        hana::when_valid<decltype(std::declval<T &>().insert(
            std::declval<T &>().end(), std::declval<U &>()))>> : std::true_type
    {};

    template<typename T>
    struct is_optional : std::false_type
    {};
    template<typename T>
    struct is_optional<std::optional<T>> : std::true_type
    {};

    template<
        typename T,
        typename Choice,
        bool AllAssignable,
        bool AllInsertable>
    struct choice_type_impl
    {};
    template<typename T, typename Choice, bool AllInsertable>
    struct choice_type_impl<T, Choice, true, AllInsertable>
    {
        using type = Choice;
    };
    template<typename T, typename Choice>
    struct choice_type_impl<T, Choice, false, true>
    {
        using type = std::ranges::range_value_t<T>;
    };
    template<typename T, typename Choice = T, typename... Choices>
    struct choice_type : choice_type_impl<
                             T,
                             Choice,
                             std::is_assignable_v<T &, Choice> &&
                                 std::is_constructible_v<T, Choice> &&
                                 ((std::is_assignable_v<T &, Choices> &&
                                   std::is_constructible_v<T, Choices>)&&...),
                             is_insertable_from<T, Choice>::value &&
                                 (is_insertable_from<T, Choices>::value && ...)>
    {};
    template<typename T, typename... Choices>
    using choice_type_t = typename choice_type<T, Choices...>::type;

    struct names_iter : stl_interfaces::proxy_iterator_interface<
                            names_iter,
                            std::forward_iterator_tag,
                            std::string_view>
    {
        using sv_iterator = std::string_view::iterator;

        names_iter() = default;
        names_iter(sv_iterator it) : it_(it), last_(it) {}
        names_iter(sv_iterator it, sv_iterator last) : it_(it), last_(last) {}

        std::string_view operator*() const
        {
            sv_iterator last = std::find(it_, last_, ',');
            BOOST_ASSERT(it_ != last_);
            return {&*it_, std::size_t(last - it_)};
        }
        names_iter & operator++()
        {
            it_ = std::find(it_, last_, ',');
            if (it_ != last_)
                ++it_;
            return *this;
        }
        friend bool operator==(names_iter lhs, names_iter rhs) noexcept
        {
            return lhs.it_ == rhs.it_;
        }

        using base_type = stl_interfaces::proxy_iterator_interface<
            names_iter,
            std::forward_iterator_tag,
            std::string_view>;
        using base_type::operator++;

    private:
        sv_iterator it_;
        sv_iterator last_;
    };

    struct names_view
    {
        using iterator = names_iter;

        names_view() = default;
        names_view(std::string_view names) :
            first_(names_iter(names.begin(), names.end())),
            last_(names_iter(names.end()))
        {}

        iterator begin() const { return first_; }
        iterator end() const { return last_; }

    private:
        iterator first_;
        iterator last_;
    };

    inline bool positional(std::string_view name)
    {
        for (auto name : detail::names_view(name)) {
            if (name[0] == '-')
                return false;
        }
        return true;
    }

    template<typename Pred>
    std::string_view first_name_prefer(std::string_view names, Pred pred)
    {
        std::string_view prev_name;
        for (auto sv : names_view(names)) {
            if (pred(sv))
                return sv;
            if (prev_name.empty())
                prev_name = sv;
        }
        return prev_name;
    }

    inline std::string_view first_short_name(std::string_view names)
    {
        return detail::first_name_prefer(
            names, [](std::string_view sv) { return detail::short_(sv); });
    }

    inline std::string_view first_long_name(std::string_view names)
    {
        return detail::first_name_prefer(
            names, [](std::string_view sv) { return detail::long_(sv); });
    }

    inline std::string_view trim_leading_dashes(std::string_view name)
    {
        char const * first = name.data();
        char const * const last = first + name.size();
        while (first != last && *first == '-') {
            ++first;
        }
        return {first, std::size_t(last - first)};
    }

    template<typename... Options>
    auto to_ref_tuple(hana::tuple<Options...> const & opts)
    {
        using opts_as_ref_tuple_type = hana::tuple<Options const &...>;
        return hana::unpack(opts, [](Options const &... o) {
            return opts_as_ref_tuple_type{o...};
        });
    }

    template<typename... Options>
    auto make_opt_tuple(hana::tuple<Options const &...> const & opts)
    {
        auto unflattened = hana::transform(opts, [](auto const & opt) {
            using opt_type = std::remove_cvref_t<decltype(opt)>;
            if constexpr (is_group<opt_type>::value) {
                if constexpr (
                    !opt_type::mutually_exclusive && !opt_type::subcommand) {
                    return detail::make_opt_tuple(
                        detail::to_ref_tuple(opt.options));
                } else {
                    return hana::make_tuple(opt);
                }
            } else {
                return hana::make_tuple(opt);
            }
        });
        return hana::flatten(unflattened);
    }

    template<typename... Options>
    auto make_opt_tuple(Options const &... opts)
    {
        using opts_as_tuple_type = hana::tuple<Options const &...>;
        return detail::make_opt_tuple(opts_as_tuple_type{opts...});
    }

}}}

#endif
