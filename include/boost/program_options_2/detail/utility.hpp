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
    constexpr BidiIter find_last(BidiIter first, BidiIter last, T const & x)
    {
        auto it = last;
        while (it != first) {
            if (*--it == x)
                return it;
        }
        return last;
    }

    // This unfortunately necessary to use in some places.  This is because a
    // std::basic_string<CharT> cannot be passed to a function template
    // (templated on CharT) that takes a std::basic_string_view<CharT> -- the
    // deduction does not work.
    template<typename String>
    auto make_string_view(String const & s)
    {
        auto ptr = std::to_address(std::begin(s));
        using char_type = std::remove_cvref_t<decltype(*ptr)>;
        return std::basic_string_view<char_type>(ptr, ptr + std::size(s));
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

    template<typename... Options>
    void get_help_option(
        std::optional<names_view> & retval, Options const &... opts);

    template<typename Option>
    void
    get_help_option_impl(std::optional<names_view> & retval, Option const & opt)
    {
        if constexpr (group_<Option>) {
            return hana::unpack(opt.options, [&](auto const &... opts) {
                return detail::get_help_option(retval, opts...);
            });
        } else {
            if (opt.action == detail::action_kind::help)
                retval = names_view(opt.names);
        }
    }

    template<typename... Options>
    void
    get_help_option(std::optional<names_view> & retval, Options const &... opts)
    {
        auto dummy = (detail::get_help_option_impl(retval, opts), ..., 0);
        (void)dummy;
    }

    template<typename... Options>
    std::optional<names_view> help_option(Options const &... opts)
    {
        std::optional<names_view> retval;
        detail::get_help_option(retval, opts...);
        return retval;
    }

    template<typename R>
    bool contains_ws(R const & r)
    {
        auto const last = r.end();
        for (auto first = r.begin(); first != last; ++first) {
            auto const cp = *first;
            // space, tab through lf
            if (cp == 0x0020 || (0x0009 <= cp && cp <= 0x000d))
                return true;
            if (cp == 0x0085 || cp == 0x00a0 || cp == 0x1680 ||
                (0x2000 <= cp && cp <= 0x200a) || cp == 0x2028 ||
                cp == 0x2029 || cp == 0x202F || cp == 0x205F || cp == 0x3000) {
                return true;
            }
        }
        return false;
    }

    inline bool
    positional(std::string_view name, customizable_strings const & strings)
    {
        if (detail::contains_ws(name))
            return false;
        for (auto name : detail::names_view(name)) {
            if (detail::transcoded_starts_with(
                    name, strings.short_option_prefix) ||
                detail::transcoded_starts_with(
                    name, strings.long_option_prefix)) {
                return false;
            }
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

    inline std::string_view first_short_name(
        std::string_view names, customizable_strings const & strings)
    {
        return detail::first_name_prefer(
            names, [&strings](std::string_view sv) {
                return detail::short_(sv, strings);
            });
    }

    inline std::string_view first_long_name(
        std::string_view names, customizable_strings const & strings)
    {
        return detail::first_name_prefer(
            names, [&strings](std::string_view sv) {
                return detail::long_(sv, strings);
            });
    }

    inline std::string_view trim_leading_dashes(
        std::string_view name, customizable_strings const & strings)
    {
        if (detail::transcoded_starts_with(name, strings.long_option_prefix))
            return name.substr(strings.long_option_prefix.size());
        if (detail::transcoded_starts_with(name, strings.short_option_prefix))
            return name.substr(strings.short_option_prefix.size());
        return name;
    }

    template<typename... Options>
    auto to_ref_tuple(hana::tuple<Options...> const & opts)
    {
        using opts_as_ref_tuple_type = hana::tuple<Options const &...>;
        return hana::unpack(opts, [](Options const &... o) {
            return opts_as_ref_tuple_type{o...};
        });
    }

    template<bool ForPrinting, bool ForGroupConstruction, typename... Options>
    auto make_opt_tuple_impl(hana::tuple<Options...> && opts)
    {
        auto unflattened = hana::transform(opts, [](auto const & opt) {
            using opt_type = std::remove_cvref_t<decltype(opt)>;
            if constexpr (is_group<opt_type>::value) {
                constexpr bool regular_group =
                    !opt_type::mutually_exclusive && !opt_type::subcommand;
                constexpr bool collapsible_group =
                    !ForGroupConstruction || !opt.named_group;
                if constexpr (
                    !ForPrinting && regular_group && collapsible_group) {
                    return detail::
                        make_opt_tuple_impl<false, ForGroupConstruction>(
                            detail::to_ref_tuple(opt.options));
                } else if constexpr (
                    ForPrinting && opt.flatten_during_printing) {
                    return detail::make_opt_tuple_impl<true, false>(
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
    auto make_opt_tuple(hana::tuple<Options const &...> && opts)
    {
        return detail::make_opt_tuple_impl<false, false>(std::move(opts));
    }

    template<typename... Options>
    auto make_opt_tuple(Options const &... opts)
    {
        return detail::make_opt_tuple(hana::tuple<Options const &...>{opts...});
    }

    template<typename Option>
    struct contains_commands_impl
    {
        constexpr static bool call() { return false; }
    };

    template<
        exclusive_t MutuallyExclusive,
        subcommand_t Subcommand,
        required_t Required,
        named_group_t NamedGroup,
        typename Func,
        typename... Options>
    struct contains_commands_impl<option_group<
        MutuallyExclusive,
        Subcommand,
        Required,
        NamedGroup,
        Func,
        Options...>>
    {
        constexpr static bool call()
        {
            if constexpr (Subcommand == subcommand_t::yes)
                return true;
            else
                return (detail::contains_commands_impl<Options>::call() || ...);
        }
    };

    template<typename... Options>
    constexpr bool contains_commands()
    {
        return (
            detail::contains_commands_impl<
                std::remove_cvref_t<Options>>::call() ||
            ...);
    }

    template<typename... Options>
    constexpr bool contains_commands(hana::tuple<Options...> const &)
    {
        return (
            detail::contains_commands_impl<
                std::remove_cvref_t<Options>>::call() ||
            ...);
    }

}}}

#endif
