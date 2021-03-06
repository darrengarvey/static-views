//          Copyright Tom Westerhout 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/// \file boost/static_views/raw_view.hpp
///
/// \brief Implementation of #raw_view.

#ifndef BOOST_STATIC_VIEWS_RAW_VIEW_HPP
#define BOOST_STATIC_VIEWS_RAW_VIEW_HPP

#include "detail/config.hpp"
#include "errors.hpp"
#include "sequence_traits.hpp"
#include <type_traits>

BOOST_STATIC_VIEWS_BEGIN_NAMESPACE

template <std::size_t N>
constexpr auto capacity = std::integral_constant<std::size_t, N>{};

namespace detail {

template <class S
    BOOST_STATIC_VIEWS_REQUIRES(Sequence<S>)
struct raw_view_impl {

  private:
    using sequence_type = S; // std::remove_cv_t<S>;
    using traits_type = sequence_traits<sequence_type>;
    S* _xs;

  public:
    using value_type      = typename traits_type::value_type;
    using reference       = typename traits_type::reference;
#if 0
    using const_reference = typename sequence_traits<
        std::add_const_t<sequence_type>>::reference;
#endif
    using size_type       = typename traits_type::size_type;
    using index_type      = typename traits_type::index_type;
    using difference_type = typename traits_type::difference_type;

    constexpr raw_view_impl() noexcept = default;

    BOOST_STATIC_VIEWS_CONSTEXPR
    explicit raw_view_impl(S& xs) noexcept : _xs{&xs} {}

    BOOST_STATIC_VIEWS_CONSTEXPR
    raw_view_impl(raw_view_impl const&) noexcept = default;

    BOOST_STATIC_VIEWS_CONSTEXPR
    raw_view_impl(raw_view_impl&&) noexcept = default;

    BOOST_STATIC_VIEWS_CONSTEXPR
    raw_view_impl& operator=(raw_view_impl const&) noexcept = default;

    BOOST_STATIC_VIEWS_CONSTEXPR
    raw_view_impl& operator=(raw_view_impl&&) noexcept = default;

    static constexpr auto extent() noexcept
    {
        return sequence_traits<sequence_type>::extent();
    }

    constexpr auto size() const noexcept
    {
        return sequence_traits<sequence_type>::size(*_xs);
    }

    BOOST_STATIC_VIEWS_FORCEINLINE
    BOOST_STATIC_VIEWS_CONSTEXPR
    auto unsafe_at(index_type const i) const
        // clang-format off
    BOOST_STATIC_VIEWS_DECLTYPE_NOEXCEPT_RETURN
    (
        sequence_traits<sequence_type>::at(*_xs, i)
    );
    // clang-format on

    BOOST_STATIC_VIEWS_FORCEINLINE
    BOOST_STATIC_VIEWS_CONSTEXPR
    BOOST_STATIC_VIEWS_DECLTYPE_AUTO operator[](index_type const i) const
    {
        if (BOOST_STATIC_VIEWS_UNLIKELY(
                0 > i || static_cast<size_type>(i) >= size())) {
            make_out_of_bound_error(
                "Precondition `0 <= i < size()` not satisfied in "
                "boost::static_views::raw_view::operator[].");
        }
        return unsafe_at(i);
    }
};

/// \cond
struct raw_view_fn {
  public:
    template <class S
        BOOST_STATIC_VIEWS_REQUIRES(Sequence<S>)
    BOOST_STATIC_VIEWS_CONSTEXPR auto operator()(S& sequence) const
        noexcept
    {
        return raw_view_impl<S>{sequence};
    }
};
/// \endcond

} // namespace detail

/// \verbatim embed:rst:leading-slashes
/// :math:`\mathtt{raw\_view} : \mathtt{Sequence} \to \mathtt{View}`
/// (i.e.  takes a :ref:`sequence <sequence-concept>` and returns a
/// :ref:`view <view-concept>`). It will trigger ``static_assert``
/// failures if passed a type not modeling the :ref:`sequence
/// <sequence-concept>` concept.
/// \endverbatim
BOOST_STATIC_VIEWS_INLINE_VARIABLE(detail::raw_view_fn, raw_view)

BOOST_STATIC_VIEWS_END_NAMESPACE

#endif // BOOST_STATIC_VIEWS_RAW_VIEW_HPP
