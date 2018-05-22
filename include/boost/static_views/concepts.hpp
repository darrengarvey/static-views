//          Copyright Tom Westerhout 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_STATIC_VIEWS_CONCEPTS_HPP
#define BOOST_STATIC_VIEWS_CONCEPTS_HPP

#include "detail/config.hpp"
#include "detail/utils.hpp"
#include "detail/invoke.hpp"
#include <type_traits>

BOOST_STATIC_VIEWS_BEGIN_NAMESPACE

/// \typedef void_t
/// \verbatim embed:rst:leading-slashes
/// It is an alias to :cpp:`std::void_t` if available. Otherwise it is
/// defined as:
///
/// .. code-block:: cpp
///
///   template <class...>
///   using void_t = void;
/// \endverbatim
#if defined(__cpp_lib_void_t) && __cpp_lib_void_t >= 201411
using std::void_t;
#else
template <class...>
using void_t = void;
#endif

#if defined(__has_include) && __has_include(<experimental/type_traits>)
BOOST_STATIC_VIEWS_END_NAMESPACE
#include <experimental/type_traits>
BOOST_STATIC_VIEWS_BEGIN_NAMESPACE

using nonesuch = std::experimental::nonesuch;

template <template <class...> class Op, class... Args>
using is_detected = std::experimental::is_detected<Op, Args...>;

template <template <class...> class Op, class... Args>
using detected_t = std::experimental::detected_t<Op, Args...>;

#else

/// \verbatim embed:rst:leading-slashes
/// NOP type. It is an alias to :cpp:`std::experimental::nonesuch` if
/// available. Otherwise it is defined as:
///
/// .. code-block:: cpp
///
///   struct nonesuch {
///       nonesuch()                = delete;
///       nonesuch(nonesuch const&) = delete;
///       nonesuch(nonesuch&&)      = delete;
///       nonesuch& operator=(nonesuch const&) = delete;
///       nonesuch& operator=(nonesuch&&) = delete;
///   };
/// \endverbatim
struct nonesuch {
    nonesuch()                = delete;
    nonesuch(nonesuch const&) = delete;
    nonesuch(nonesuch&&)      = delete;
    nonesuch& operator=(nonesuch const&) = delete;
    nonesuch& operator=(nonesuch&&) = delete;
};

namespace detail {
// clang-format off
template < class Default
         , class
         , template <class...> class Op
         , class... Args
         >
struct detector {
    using value_t = std::false_type;
    using type    = Default;
};

template < class Default
         , template <class...> class Op
         , class... Args
         >
struct detector<Default, void_t<Op<Args...>>, Op, Args...> {
    using value_t = std::true_type;
    using type    = Op<Args...>;
};
// clang-format on
} // namespace detail


/// \verbatim embed:rst:leading-slashes
/// An alias to :cpp:`std::experimental::is_detected` if
/// available. Otherwise it is implemented from scratch.
/// \endverbatim
template <template <class...> class Op, class... Args>
using is_detected =
    typename detail::detector<nonesuch, void, Op, Args...>::value_t;

/// \verbatim embed:rst:leading-slashes
/// An alias to :cpp:`std::experimental::is_detected` if
/// available. Otherwise it is implemented from scratch.
/// \endverbatim
template <template <class...> class Op, class... Args>
using detected_t =
    typename detail::detector<nonesuch, void, Op, Args...>::type;

#endif

#if defined(__cpp_lib_experimental_invocation_type)                  \
    && __cpp_lib_experimental_invocation_type >= 201411
// #if defined(__has_include) && __has_include(<experimental/type_traits>)

template <class Function, class... Args>
using is_invocable = std::is_invocable<Function, Args...>;

template <class Function, class... Args>
using is_nothrow_invocable =
    std::is_nothrow_invocable<Function, Args...>;

template <class Return, class Function, class... Args>
using is_invocable_r = std::is_invocable_r<Return, Function, Args...>;

template <class Return, class Function, class... Args>
using is_nothrow_invocable_r =
    std::is_nothrow_invocable_r<Return, Function, Args...>;

#else

namespace detail {

template <class Function, class... Args>
using invoke_t = decltype(
    invoke(std::declval<Function>(), std::declval<Args>()...));

} // namespace detail

template <class Fn, class... Xs>
using is_invocable = is_detected<detail::invoke_t, Fn, Xs...>;

template <class Fn, class... Xs>
using is_nothrow_invocable = std::conditional_t<
    detail::utils::all(
        is_detected<detail::invoke_t, Fn, Xs...>::value,
        noexcept(invoke(std::declval<Fn>(), std::declval<Xs>()...))),
    std::true_type, std::false_type>;

template <class R, class Fn, class... Xs>
using is_invocable_r = std::conditional_t<
    detail::utils::all(is_invocable<Fn, Xs...>::value,
        std::is_convertible<detected_t<detail::invoke_t, Fn, Xs...>,
            R>::value),
    std::true_type, std::false_type>;

template <class R, class Fn, class... Xs>
using is_nothrow_invocable_r = std::conditional_t<
    detail::utils::all(is_nothrow_invocable<Fn, Xs...>::value,
        std::is_convertible<detected_t<detail::invoke_t, Fn, Xs...>,
            R>::value),
    std::true_type, std::false_type>;

#endif




#if 0

#define BOOST_STATIC_VIEWS_DEFINE_CHECK(name, T, expr, msg)          \
    struct name {                                                    \
        template <class T>                                           \
        static constexpr auto test() noexcept -> bool                \
        {                                                            \
            return expr;                                             \
        }                                                            \
                                                                     \
        template <class T>                                           \
        static constexpr auto check() noexcept -> bool               \
        {                                                            \
            constexpr bool x = test<T>();                            \
            static_assert(x, msg);                                   \
            return x;                                                \
        }                                                            \
    } /**/

#define BOOST_STATIC_VIEWS_DEFINE_CHECK2(name, T, V, expr, msg)      \
    struct name {                                                    \
        template <class T, class V>                                  \
        static constexpr auto test() noexcept -> bool                \
        {                                                            \
            return expr;                                             \
        }                                                            \
                                                                     \
        template <class T, class V>                                  \
        static constexpr auto check() noexcept -> bool               \
        {                                                            \
            constexpr bool x = test<T, V>();                         \
            static_assert(x, msg);                                   \
            return x;                                                \
        }                                                            \
    } /**/

BOOST_STATIC_VIEWS_DEFINE_CHECK(move_constructible, T,
    (std::is_move_constructible<T>::value
        /*&& std::is_move_assignable<T>::value*/),
    "T does not model the MoveConstructible concept.");

struct member_size {
  private:
    template <class T>
    using has_size_t = decltype(std::declval<T>().size());

  public:
    BOOST_STATIC_VIEWS_DEFINE_CHECK(Has, T,
        (is_detected<has_size_t, T>::value),
        "T has no `size()` member function.");

    BOOST_STATIC_VIEWS_DEFINE_CHECK(Noexcept, T,
        (noexcept(std::declval<T>().size())),
        "`T::size()` is not noexcept.");

    BOOST_STATIC_VIEWS_DEFINE_CHECK(Return, T,
        (std::is_unsigned<detected_t<has_size_t, T>>::value),
        "`T::size()` must return an unsigned integral.");

  private:
    using type = and_<Has, all_<Return, Noexcept>>;

  public:
    template <class T>
    using return_t = detected_t<has_size_t, T>;

    template <class T>
    static constexpr auto test() noexcept -> bool
    {
        return type::test<T>();
    }

    template <class T>
    static constexpr auto check() noexcept -> bool
    {
        return type::check<T>();
    }
};

struct member_extent {
  private:
    template <class T>
    using has_extent_t = decltype(T::extent());

    template <class F, int = F{}()>
    static constexpr auto is_constexpr_impl(
        F /*unused*/, int /*unused*/) noexcept -> bool
    {
        return true;
    }

    template <class F>
    static constexpr auto is_constexpr_impl(
        F /*unused*/, ... /*unused*/) noexcept -> bool
    {
        return false;
    }

    template <class F>
    static constexpr auto is_constexpr(F /*unused*/) noexcept -> bool
    {
        return is_constexpr_impl(F{}, int{});
    }

    template <class T>
    struct call_extent {
        constexpr auto operator()() noexcept -> int
        {
            T::extent();
            return {};
        }
    };

  public:
    BOOST_STATIC_VIEWS_DEFINE_CHECK(Has, T,
        (is_detected<has_extent_t, T>::value),
        "T has no statit `extent()` member function.");

    BOOST_STATIC_VIEWS_DEFINE_CHECK(Constexpr, T,
        (is_constexpr(call_extent<T>{})),
        "`T::extent()` is not constexpr.");

    BOOST_STATIC_VIEWS_DEFINE_CHECK(Noexcept, T,
        (noexcept(T::extent())),
        "`T::extent()` is not noexcept.");

    BOOST_STATIC_VIEWS_DEFINE_CHECK(Return, T,
        (std::is_integral<detected_t<has_extent_t, T>>::value
            && (T::extent() >= 0 || T::extent() == dynamic_extent)),
        "`T::extent()` must return a non-negative number or "
        "dynamic_extent.");

    BOOST_STATIC_VIEWS_DEFINE_CHECK(Static, T,
        (T::extent() >= 0),
        "`T::extent()` must be non-negative.");

  private:
    using type = and_<Has, Constexpr, all_<Return, Noexcept>>;

  public:
    template <class T>
    using return_t = detected_t<has_extent_t, T>;

    template <class T>
    static constexpr auto test() noexcept -> bool
    {
        return type::test<T>();
    }

    template <class T>
    static constexpr auto check() noexcept -> bool
    {
        return type::check<T>();
    }
};

struct member_parent {
  private:
    template <class T>
    using has_parent_t = decltype(std::declval<T>().parent());

  public:
    BOOST_STATIC_VIEWS_DEFINE_CHECK(Has, T,
        (is_detected<has_parent_t, T>::value),
        "T has no `parent()` member function.");

    BOOST_STATIC_VIEWS_DEFINE_CHECK(Noexcept, T,
        (noexcept(std::declval<T>().parent())),
        "`T::parent()` is not noexcept.");

  private:
    using type = and_<Has, Noexcept>;

  public:
    template <class T>
    using return_t = detected_t<has_parent_t, T>;

    template <class T>
    static constexpr auto test() noexcept -> bool
    {
        return type::test<T>();
    }

    template <class T>
    static constexpr auto check() noexcept -> bool
    {
        return type::check<T>();
    }
};

struct member_map {
  private:
    template <class T>
    using has_map_t =
        decltype(std::declval<T>().map(std::declval<std::size_t>()));

  public:
    BOOST_STATIC_VIEWS_DEFINE_CHECK(Has, T,
        (is_detected<has_map_t, T>::value),
        "T has no `map(size_t)` member function.");

    BOOST_STATIC_VIEWS_DEFINE_CHECK(Noexcept, T,
        (noexcept(std::declval<T>().map(std::declval<std::size_t>()))),
        "`T::map(size_t)` is not noexcept.");

    BOOST_STATIC_VIEWS_DEFINE_CHECK(Return, T,
        (std::is_unsigned<detected_t<has_map_t, T>>::value),
        "`T::map(size_t)` must return an unsigned integral.");

  private:
    using type = and_<Has, all_<Return, Noexcept>>;

  public:
    template <class T>
    using return_t = detected_t<has_map_t, T>;

    template <class T>
    static constexpr auto test() noexcept -> bool
    {
        return type::test<T>();
    }

    template <class T>
    static constexpr auto check() noexcept -> bool
    {
        return type::check<T>();
    }
};

struct member_unsafe_at {
  private:
    template <class T>
    using has_unsafe_at_t =
        decltype(std::declval<T>().unsafe_at(std::declval<std::size_t>()));

  public:
    BOOST_STATIC_VIEWS_DEFINE_CHECK(Has, T,
        (is_detected<has_unsafe_at_t, T>::value),
        "T has no `unsafe_at(size_t)` member function.");

    BOOST_STATIC_VIEWS_DEFINE_CHECK(Noexcept, T,
        (noexcept(std::declval<T>().unsafe_at(std::declval<std::size_t>()))),
        "`T::unsafe_at(size_t)` is not noexcept.");

  private:
    using type = and_<Has, Noexcept>;

  public:
    template <class T>
    using return_t = detected_t<has_unsafe_at_t, T>;

    template <class T>
    static constexpr auto test() noexcept -> bool
    {
        return type::test<T>();
    }

    template <class T>
    static constexpr auto check() noexcept -> bool
    {
        return type::check<T>();
    }
};

struct member_index_operator {
  private:
    template <class T>
    using has_index_operator_t =
        decltype(std::declval<T>()[std::declval<std::size_t>()]);

  public:
    BOOST_STATIC_VIEWS_DEFINE_CHECK(Has, T,
        (is_detected<has_index_operator_t, T>::value),
        "T has no `operator[](size_t)` member function.");

    BOOST_STATIC_VIEWS_DEFINE_CHECK(Noexcept, T,
        (noexcept(std::declval<T>()[std::declval<std::size_t>()])),
        "`T::operator[](size_t)` is not noexcept.");

  private:
    using type = Has;

  public:
    template <class T>
    using return_t = detected_t<has_index_operator_t, T>;

    template <class T>
    static constexpr auto test() noexcept -> bool
    {
        return type::test<T>();
    }

    template <class T>
    static constexpr auto check() noexcept -> bool
    {
        return type::check<T>();
    }
};



} // namespace concepts

#endif


#if defined(__cpp_concepts) && __cpp_concepts >= 201507
#define BOOST_STATIC_VIEWS_CONCEPTS
#endif

#if defined(BOOST_STATIC_VIEWS_CONCEPTS)
#define BOOST_STATIC_VIEWS_REQUIRES(...) > requires __VA_ARGS__
#else
#define BOOST_STATIC_VIEWS_REQUIRES(...)                             \
    , class = std::enable_if_t<__VA_ARGS__>>
#endif


#if defined(BOOST_STATIC_VIEWS_CONCEPTS)
#if defined(__has_include) && __has_include(<concepts>)
#include <concepts>
#elif defined(__has_include) && __has_include(<experimental/concepts>)
#include <experimental/concepts>
#else
template <class T, class U>
concept bool Same = std::is_same<T, U>::value;

template <class From, class To>
concept bool ConvertibleTo = std::is_convertible<From, To>::value
    && requires(From (&f)()) { static_cast<To>(f()); };

template <class T>
concept bool Destructible = std::is_nothrow_destructible<T>::value;

template <class T, class... Args>
concept bool Constructible =
    Destructible<T>&& std::is_constructible<T, Args...>::value;

template <class T>
concept bool MoveConstructible =
    Constructible<T, T>&& ConvertibleTo<T, T>;

template <class T, class Dummy>
concept bool _MoveConstructible = MoveConstructible<T>;
#endif
#else
namespace detail {
template <class From, class To>
using funny_extra_test_in_convertible_t =
    decltype(static_cast<To>(std::declval<From (&)()>()()));
} // namespace detail

template <class T, class U>
constexpr bool Same = std::is_same<T, U>::value;

template <class From, class To>
constexpr bool ConvertibleTo =
    std::is_convertible<From, To>::value&& is_detected<
        detail::funny_extra_test_in_convertible_t, From, To>::value;

template <class T>
constexpr bool Destructible = std::is_nothrow_destructible<T>::value;

template <class T, class... Args>
constexpr bool Constructible =
    Destructible<T> && std::is_constructible<T, Args...>::value;

template <class T>
constexpr bool MoveConstructible =
    Constructible<T, T>&& ConvertibleTo<T, T>;

template <class T, class Dummy>
constexpr bool _MoveConstructible = MoveConstructible<T>;
#endif

namespace detail {
template <class T, int = (T::extent(), 0)>
static constexpr auto has_extent_impl(int) noexcept -> std::true_type
{
    return {};
}

template <class T>
static constexpr auto has_extent_impl(...) noexcept -> std::false_type
{
    return {};
}

template <class T>
using has_size_t = decltype(std::declval<T const&>().size());

template <class T, class IndexType>
using has_at_t =
    decltype(std::declval<T>().at(std::declval<IndexType>()));

template <class T, class IndexType>
using has_index_operator_t =
    decltype(std::declval<T>()[std::declval<IndexType>()]);
} // namespace detail

#if defined(BOOST_STATIC_VIEWS_CONCEPTS)
template <class T>
concept bool HasExtent = requires() {
    { T::extent() } noexcept;
};
template <class T>
concept bool HasSize = requires(T const& xs) {
    { xs.size() } noexcept;
};
template <class T, class IndexType = std::size_t>
concept bool HasAt = requires(T xs, IndexType i) {
    xs.at(i);
};
template <class T, class IndexType = std::size_t>
concept bool HasIndexOperator = requires(T xs, IndexType i) {
    xs[i];
};
#else
template <class T>
constexpr bool HasExtent = detail::has_extent_impl<T>(int{})();
template <class T>
constexpr bool HasSize = is_detected<detail::has_size_t, T>::value;
template <class T, class IndexType = std::size_t>
constexpr bool HasAt =
    is_detected<detail::has_at_t, T, IndexType>::value;
template <class T, class IndexType = std::size_t>
constexpr bool HasIndexOperator =
    is_detected<detail::has_index_operator_t, T, IndexType>::value;
#endif

BOOST_STATIC_VIEWS_END_NAMESPACE

#endif // BOOST_STATIC_VIEWS_CONCEPTS_HPP

