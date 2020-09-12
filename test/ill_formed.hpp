// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Copyright (C) 2019 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PARSER_ILL_FORMED_HPP
#define BOOST_PARSER_ILL_FORMED_HPP


namespace test_detail {

    template<typename...>
    struct void_
    {
        using type = void;
        static constexpr bool value = true;
    };

    template<typename... T>
    using void_t = typename void_<T...>::type;

    template<typename T>
    using remove_cv_ref_t =
        typename std::remove_cv<typename std::remove_reference<T>::type>::type;

    struct nonesuch
    {};

    template<
        typename Default,
        typename AlwaysVoid,
        template<typename...>
        class Template,
        typename... Args>
    struct detector
    {
        using value_t = std::false_type;
        using type = Default;
    };

    template<
        typename Default,
        template<typename...>
        class Template,
        typename... Args>
    struct detector<Default, void_t<Template<Args...>>, Template, Args...>
    {
        using value_t = std::true_type;
        using type = Template<Args...>;
    };

    template<template<typename...> class Template, typename... Args>
    using is_detected =
        typename detector<nonesuch, void, Template, Args...>::value_t;

    template<template<typename...> class Template, typename... Args>
    using detected_t =
        typename detector<nonesuch, void, Template, Args...>::type;

    template<
        typename Default,
        template<typename...>
        class Template,
        typename... Args>
    using detected_or =
        typename detector<Default, void, Template, Args...>::type;

}

template<template<class...> class Template, typename... Args>
using ill_formed = std::integral_constant<
    bool,
    !test_detail::is_detected<Template, Args...>::value>;

template<template<class...> class Template, typename... Args>
using well_formed = std::
    integral_constant<bool, test_detail::is_detected<Template, Args...>::value>;


#if 0 // TODO: Seems like too much work....
#include <boost/hana.hpp>

template<typename... T>
struct pack
{};

template<
    template<class...>
    class Template,
    template<class... Args>
    class Pack,
    typename = boost::hana::when<true>>
struct ill_formed_detector : std::false_type
{};
template<template<class...> class Template, template<class... Args> class Pack>
struct ill_formed_detector<
    Template,
    Pack,
    boost::hana::when_valid<Template<Args...>>> : std::true_type
{};

template<template<class...> class Template, typename... Args>
constexpr bool ill_formed = ill_formed_detector<Template, pack<Args...>>::value;

template<template<class...> class Template, typename... Args>
constexpr bool well_formed =
    !ill_formed_detector<Template, pack<Args...>>::value;
#endif

#endif
