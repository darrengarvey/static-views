//          Copyright Tom Westerhout 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_STATIC_VIEWS_HASHED_HPP
#define BOOST_STATIC_VIEWS_HASHED_HPP

#include <type_traits>
#include <limits>
#include <boost/static_views/detail/config.hpp>
#include <boost/static_views/detail/find_first.hpp>
#include <boost/static_views/detail/invoke.hpp>
#include <boost/static_views/algorithm_base.hpp>
#include <boost/static_views/raw_view.hpp>
#include <boost/static_views/slice.hpp>
#include <boost/static_views/through.hpp>
#include <boost/static_views/view_base.hpp>

BOOST_STATIC_VIEWS_BEGIN_NAMESPACE

namespace detail {

template <class View>
struct hasher_compat_checker {
    template <class Hasher>
    using call_t = decltype( invoke(
        std::declval<Hasher const&>(),
        std::declval<View const&>()[std::declval<std::size_t>()]) );

    template <class Hasher>
    static constexpr auto is_compatible() noexcept -> bool
    {
        static_assert(concepts::is_View<View>(),
            "[INTERNAL] Invalid use of hasher_compat_checker.");
        return std::is_convertible<detected_t<call_t, Hasher>,
            std::size_t>::value;
    }

    template <class Hasher>
    static constexpr auto assert_compatible() noexcept -> bool
    {
        constexpr bool x = is_compatible<Hasher>();
        static_assert(x,
            "`Hasher` must be a function that takes an element of "
            "`View` and returns a type convertible to size_t.");
        return x;
    }
};

template <std::size_t BucketSize, std::size_t... Is>
struct hashed_init_impl {
    std::size_t storage[sizeof...(Is)];

    // clang-format off
    template <class View, class Hasher>
    BOOST_STATIC_VIEWS_FORCEINLINE
    BOOST_STATIC_VIEWS_CONSTEXPR
    explicit hashed_init_impl(View const& xs, Hasher const& h)
        : storage{((void)Is, hashed_init_impl::capacity())...}
    {
        static_assert(hasher_compat_checker<View>::template is_compatible<Hasher>(),
            "[INTERNAL] Invalid use of hashed_init_impl.");

        auto const size = xs.size();
        for (std::size_t i = 0; i < size; ++i) {
            insert(i, bucket_size() * (invoke(h, xs[i]) % bucket_count()));
        }
    }
    // clang-format on

  private:
    static constexpr auto capacity() noexcept -> std::size_t
    {
        static_assert(sizeof...(Is) > 0,
            "[INTERNAL] Invalid use of hashed_init_impl.");
        return sizeof...(Is);
    }

    static constexpr auto bucket_size() noexcept -> std::size_t
    {
        static_assert(BucketSize > 0,
            "[INTERNAL] Invalid use of hashed_init_impl.");
        return BucketSize;
    }

    static constexpr auto bucket_count() noexcept -> std::size_t
    {
        static_assert(sizeof...(Is) % bucket_size() == 0,
            "[INTERNAL] Invalid use of hashed_init_impl.");
        return capacity() / bucket_size();
    }

    BOOST_STATIC_VIEWS_FORCEINLINE
    BOOST_STATIC_VIEWS_CONSTEXPR
    auto insert(std::size_t const i, std::size_t const guess) -> void
    {
#if defined(__cpp_constexpr) && __cpp_constexpr >= 201603
        // We have C++17 constexpr lambdas
        constexpr auto is_empty = [](auto const x) noexcept
        {
            return x == hashed_init_impl::capacity();
        };
#else
        struct is_empty_impl {
            BOOST_STATIC_VIEWS_CONSTEXPR
            auto operator()(std::size_t const x) const noexcept
            {
                return x == hashed_init_impl::capacity();
            }
        };
        constexpr is_empty_impl is_empty{};
#endif

        auto const bucket =
            slice(guess, guess + bucket_size())(raw_view(storage));
        auto const p = find_first_i(bucket, is_empty);

        if (p != bucket_size()) {
            storage[guess + p] = i;
        }
        else {
            make_full_bucket_error("Bucket is full.");
        }
    }
};

template <std::size_t BucketSize, class V, class H, std::size_t... Is>
BOOST_STATIC_VIEWS_CONSTEXPR auto make_hashed_init_impl(
    V const& xs, H const& h, std::index_sequence<Is...> /*unused*/)
{
    return hashed_init_impl<BucketSize, Is...>{xs, h};
}

template <std::size_t BucketCount, std::size_t BucketSize, class V,
    class H>
BOOST_STATIC_VIEWS_CONSTEXPR auto make_hashed_init_impl(
    V const& xs, H const& h)
{
    static_assert(
        std::numeric_limits<std::size_t>::max() / BucketCount
            >= BucketSize,
        "[INTERNAL] Overflow detected.");
    return make_hashed_init_impl<BucketSize>(
        xs, h, std::make_index_sequence<BucketCount * BucketSize>{});
}

template <std::size_t BucketCount, std::size_t BucketSize, class View,
    class Hasher>
struct hashed_impl
    : view_adaptor_base<
          hashed_impl<BucketCount, BucketSize, View, Hasher>, View> {

  private:
    static_assert(
        BucketCount > 0, "[INTERNAL] Invalid use of hashed_impl.");
    static_assert(
        BucketSize > 0, "[INTERNAL] Invalid use of hashed_impl.");

    static_assert(
        is_wrapper<View>(), "[INTERNAL] Invalid use of hashed_impl.");
    using view_type = typename View::type;
    static_assert(concepts::is_View<view_type>(),
        "[INTERNAL] Invalid use of hashed_impl.");

    static_assert(is_wrapper<Hasher>(),
        "[INTERNAL] Invalid use of hashed_impl.");
    using hasher_type = typename Hasher::type;
    static_assert(
        hasher_compat_checker<view_type>::template is_compatible<
            hasher_type>(),
        "[INTERNAL] Invalid use of hashed_impl.");

  public:
    /// \brief Returns the number buckets.
    static constexpr auto bucket_count() noexcept
    {
        return BucketCount;
    }

    /// \brief Returns the capacity of each bucket.
    static constexpr auto bucket_size() noexcept
    {
        return BucketSize;
    }

    /// \brief Constructs a hashed view of \p xs using \p hf as a hash
    /// function.

    /// \tparam BucketCount
    /// Number of buckets.
    /// \tparam BucketSize
    /// Capacity of each bucket.
    /// \tparam View
    /// \verbatim embed:rst:leading-slashes
    /// Wrapper around a view, i.e. ``typename View::type`` is a view
    /// and
    /// must model the :ref:`view <view-concept>` concept.
    /// \endverbatim
    /// \param xs      Rvalue reference to a wrapper around a view.
    /// \param hf      Hash function to use.
    /// \param storage Array of indices initialised by
    /// #hashed_init_impl.
    ///
    ///
    /// \verbatim embed:rst:leading-slashes
    /// .. note::
    ///   This function requires an initialiser storage as an argument
    ///   which
    ///   is an implementation detail. This constructor is thus not
    ///   meant to
    ///   be used explicitly, use the :cpp:var:`hashed` factory
    ///   function instead.
    /// \endverbatim
    template <std::size_t... Is>
    BOOST_STATIC_VIEWS_CONSTEXPR hashed_impl(View&& xs, Hasher&& hf,
        std::size_t (&storage)[hashed_impl::bucket_count()
                               * hashed_impl::bucket_size()],
        std::index_sequence<Is...> /*unused*/)
        : hashed_impl::view_adaptor_base_type{std::move(xs)}
        , _hf{std::move(hf)}
        , _storage{storage[Is]...}
    {
    }


    /// \brief Returns the capacity of the view.

    /// \verbatim embed:rst:leading-slashes
    /// This function is required by the :ref:`view <view-concept>`
    /// concept. Hashed view is a view of buckets rather than
    /// individual elements. This function is thus equivalent to
    /// :cpp:func:`bucket_count()
    /// <detail::hashed_impl::bucket_count()>`.
    /// \endverbatim
    static constexpr auto capacity() noexcept
    {
        return bucket_count();
    }

    /// \brief Returns a reference to the hash function.
    BOOST_STATIC_VIEWS_CONSTEXPR
    BOOST_STATIC_VIEWS_DECLTYPE_AUTO hash_function() const noexcept
    {
        static_assert(noexcept(_hf.get()),
            "hashed_impl assumes that _hf has a noexcept get().");
        return _hf.get();
    }

    // clang-format off
    template <class HashFunction>
    BOOST_STATIC_VIEWS_CONSTEXPR
    BOOST_STATIC_VIEWS_DECLTYPE_AUTO hash_function(HashFunction&& hf)
    {
        _hf = make_wrapper(std::forward<HashFunction>(hf));
    }
    // clang-format on

    /// \brief Returns the bucket corresponding to \p hash.

    /// \verbatim embed:rst:leading-slashes
    /// This function is required by the :ref:`view <view-concept>`
    /// concept. It returns a view of elements that have hash
    /// ``hash``. If there are no such elements, the returned view
    /// will have size zero. \endverbatim
    BOOST_STATIC_VIEWS_FORCEINLINE
    BOOST_STATIC_VIEWS_CONSTEXPR
    auto operator[](std::size_t const hash) const
    // noexcept /* TODO add specifiers */
    {
#if defined(__cpp_constexpr) && __cpp_constexpr >= 201603
        constexpr auto is_empty = [](auto const x) noexcept
        {
            return x == bucket_count() * bucket_size();
        };
#else
        struct is_empty_impl {
            BOOST_STATIC_VIEWS_CONSTEXPR
            auto operator()(std::size_t const x) const noexcept
            {
                return x == bucket_count() * bucket_size();
            }
        };
        constexpr is_empty_impl is_empty{};
#endif
        auto const i = bucket_size() * (hash % bucket_count());
        auto const n = find_first_i(
            slice(i, i + bucket_size())(raw_view(_storage)),
            is_empty);
        return through(slice(i, i + n)(raw_view(_storage)))(
            this->parent());
    }

  private:
    Hasher      _hf;
    std::size_t _storage[bucket_count() * bucket_size()];
};

template <std::size_t BucketCount, std::size_t BucketSize>
struct make_hashed_impl {

    static_assert(BucketCount > 0,
        "`boost::static_views::hashed<BucketCount, BucketSize>` "
        "requires BucketCount to be greater than zero.");
    static_assert(BucketCount > 0,
        "`boost::static_views::hashed<BucketCount, BucketSize>` "
        "requires BucketSize to be greater than zero.");

    // clang-format off
    template <class View, class Hasher>
    BOOST_STATIC_VIEWS_FORCEINLINE
    BOOST_STATIC_VIEWS_CONSTEXPR
    auto operator()(View&& xs, Hasher&& hf) const
    {
        using view_type = typename std::decay_t<View>::type;
        detail::hasher_compat_checker<view_type>::template
            assert_compatible<std::decay_t<Hasher>>();
        auto init = detail::make_hashed_init_impl<
            BucketCount, BucketSize>(xs.get(), hf);
        auto hasher = make_wrapper(std::forward<Hasher>(hf));
        return detail::hashed_impl<BucketCount, BucketSize,
            std::decay_t<View>, decltype(hasher)>{
            std::forward<View>(xs), std::move(hasher),
            init.storage,
            std::make_index_sequence<BucketCount * BucketSize>{}};
    }
    // clang-format on
};
} // end namespace detail

/// \brief A functor for creating "hashed views"

/// \f[
/// \text{hashed} : (\text{T} \to \mathbb{N}) \to \text{View} \to
/// \text{View} \f]
///
/// Given a hash function `hf` and a view `xs`, creates a view of `xs`
/// which uses hashes as indices to access elements.
///
/// So, say, \f$ \text{hf} : \text{T} \to \mathbb{N} \f$ is a hash
/// function, `xs` is a view of elements of type `T`, and `ys =`
/// #hashed `(hf)(xs)`. Then
///
/// \f[
/// \{ x \in \text{xs} \;\;|\;\;
///     \text{hf}(x) = h \,\%\, \text{ys.bucket_count}() \}
///     = \text{ys}[h] \;, \forall h \in \mathbb{N}\;.
/// \f]
///
/// \verbatim embed:rst:leading-slashes
/// .. note::
///   Haskell notation is used here, i.e. the function is curried and
///   :math:`\text{hashed}(\text{hf}) : \text{View} \to \text{View}`
///   models the :ref:`algorithm <algorithm-concept>` concept.
/// \endverbatim
#if defined(DOXYGEN_IN_HOUSE)
template <std::size_t BucketCount, std::size_t BucketSize>
constexpr auto hashed = implementation detail;
#else
inline namespace {
template <std::size_t BucketCount, std::size_t BucketSize>
BOOST_STATIC_VIEWS_CONSTEXPR auto const& hashed =
    ::BOOST_STATIC_VIEWS_NAMESPACE::_static_const<
        ::BOOST_STATIC_VIEWS_NAMESPACE::detail::make_algorithm_impl<
            detail::make_hashed_impl<BucketCount, BucketSize>>>;
} // anonymous namespace
#endif

BOOST_STATIC_VIEWS_END_NAMESPACE

#endif // BOOST_STATIC_VIEWS_HASHED_HPP