/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/
#ifndef SCORE_LANGUAGE_SAFECPP_STRING_VIEW_ZSPAN_H
#define SCORE_LANGUAGE_SAFECPP_STRING_VIEW_ZSPAN_H

#include "score/language/safecpp/string_view/null_termination_violation_policies.h"

#if defined(__has_include) && __has_include(<span>)
#include <span>
#endif
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <type_traits>
#include <utility>

// NOLINTBEGIN(readability-identifier-naming): STL-style notation is intended here
// in order to facilitate interoperability with other STL-like containters/algorithms

namespace score::safecpp::details
{
constexpr inline auto kNullByte = '\0';

///
/// @brief view type over contiguous sequence of objects which is guaranteed to be null-terminated
/// @details The null-termination guarantee for the underlying sequence gets preserved by every
///          member function offered by `zspan`. That’s why the underlying sequence can only
///          get accessed via a `const_pointer` as returned by `data()`.
///
template <typename T>
class zspan
{
    static_assert(std::is_same_v<unsigned char, std::make_unsigned_t<std::remove_const_t<T>>>,
                  "`safecpp::zspan` is not permitted to be instantiated for non-character types");

    /// @brief type trait for checking whether type \p U; is a `safecpp::zspan`
    template <typename U>
    class is_zspan : public std::false_type
    {
    };
    template <typename U>
    class is_zspan<zspan<U>> : public std::true_type
    {
    };

    /// @brief type trait for checking whether type \p R; is a range over elements which can get compared to kNullByte
    template <typename R, typename = void>
    class is_compatible_range : public std::false_type
    {
    };
    template <typename R>
    class is_compatible_range<
        R,
        std::void_t<decltype(std::data(std::declval<R>())), decltype(std::size(std::declval<R>()))>>
        // NOLINTNEXTLINE(score-banned-function) use of `std::data()` for sole type trait purpose here is legitimate
        : public std::is_convertible<decltype(*std::data(std::declval<R>())), decltype(details::kNullByte)>
    {
      public:
        // NOLINTNEXTLINE(score-banned-function) use of `std::data()` for sole type trait purpose here is legitimate
        using value_type = std::remove_reference_t<decltype(*std::data(std::declval<R>()))>;
    };

    /// @brief type trait for checking whether type \p R; is a range over a guaranteed null-terminated sequence
    template <typename R, typename = void>
    class is_guaranteed_null_terminated : public std::false_type
    {
    };
    template <typename R>
    class is_guaranteed_null_terminated<
        R,
        std::void_t<decltype(std::declval<R>().c_str()), decltype(std::declval<R>().size())>> : public std::true_type
    {
      public:
        using value_type = std::remove_reference_t<decltype(*std::declval<R>().c_str())>;
    };

    /// @brief type trait for checking whether a ptr to \p RangeValueType;'s is convertible to one of \p ZSpanValueType;
    template <typename RangeValueType, typename ZSpanValueType>
    class is_pointer_convertible
        : public std::is_convertible<std::add_pointer_t<RangeValueType>, std::add_pointer_t<ZSpanValueType>>
    {
    };

  public:
    using value_type = T;
    using size_type = std::size_t;

    using pointer = std::add_pointer_t<value_type>;
    using reference = std::add_lvalue_reference_t<value_type>;
    using const_pointer = std::add_pointer_t<std::add_const_t<value_type>>;
    using const_reference = std::add_lvalue_reference_t<std::add_const_t<value_type>>;
    using violation_policies = null_termination_violation_policies;

    ///
    /// @brief helper type for encapsulating references to single elements of the `zspan`'s underlying sequence
    /// @details Rationale is to facilitate element access while hiding the non-const pointer to the underlying sequence
    ///          and at the same time protecting against write accesses to adjacent elements. This helps a) to maintain
    ///          full control over which elements exactly shall get modified and, as a result, b) protects the
    ///          trailing null-terminator of the underyling sequence from getting overwritten.
    ///
    class element_accessor
    {
      public:
        /// @brief constructs an `element_accessor` for provided \p data; at position \p index;
        constexpr explicit element_accessor(pointer data, size_type index) noexcept : element_{&data[index]} {}

        /// @brief given that value types are assignable, assigns provided \p value;
        template <typename ValueType, std::enable_if_t<std::is_assignable_v<reference, ValueType>, bool> = true>
        constexpr element_accessor& operator=(ValueType value) noexcept(
            std::is_nothrow_assignable_v<reference, ValueType>)
        {
            *element_ = std::move(value);
            return *this;
        }

        /// @brief value assignments on const `element_accessor` type are prohibited
        template <typename ValueType,
                  std::enable_if_t<std::negation_v<std::is_same<ValueType, element_accessor>>, bool> = true>
        constexpr element_accessor& operator=(ValueType) const noexcept = delete;

        /// @brief provides read-only access to the underlying element
        /// @note for write access to elements, above-defined `operator=` shall be used
        // NOLINTNEXTLINE(google-explicit-constructor) allow implicit conversion to `const_reference`
        constexpr operator const_reference() const noexcept
        {
            return *element_;
        }

      private:
        pointer element_;
    };

  public:
    ///
    /// @brief Constructs `zspan` as view over a guaranteed null-terminated \p range;.
    ///
    template <typename RangeType,
              typename ZSpanValueType = T,
              typename RangeValueType = std::enable_if_t<is_guaranteed_null_terminated<RangeType>::value,
                                                         typename is_guaranteed_null_terminated<RangeType>::value_type>,
              std::enable_if_t<is_pointer_convertible<RangeValueType, ZSpanValueType>::value, bool> = true>
    // NOLINTNEXTLINE(google-explicit-constructor) allow implicit conversions from guaranteed null-terminated ranges
    constexpr zspan(RangeType&& range) noexcept(noexcept(range.c_str()) && noexcept(range.size()))
        : data_{range.c_str()}, size_{range.size()}
    {
    }

    ///
    /// @brief Constructs `zspan` as view over arbitrary \p range;.
    /// @details The provided \p violation_policy; will get invoked in case the range is not null-terminated.
    ///
    template <typename RangeType,
              typename ZSpanValueType = T,
              typename RangeValueType = std::enable_if_t<is_compatible_range<RangeType>::value,
                                                         typename is_compatible_range<RangeType>::value_type>,
              typename RT = std::remove_cv_t<std::remove_reference_t<RangeType>>,
              typename ViolationPolicy = violation_policies::default_policy,
              std::enable_if_t<violation_policies::is_valid_one<ViolationPolicy>(), bool> = true,
              std::enable_if_t<is_pointer_convertible<RangeValueType, ZSpanValueType>::value, bool> = true,
              std::enable_if_t<
                  std::conjunction_v<std::negation<is_zspan<RT>>, std::negation<is_guaranteed_null_terminated<RT>>>,
                  bool> = true>
    // NOLINTBEGIN(cppcoreguidelines-pro-type-member-init) false positive, this constructor delegates to another one
    constexpr explicit zspan(RangeType&& range, ViolationPolicy violation_policy = {}) noexcept(
        // NOLINTNEXTLINE(score-banned-function) using `std::data()` is valid here since used in conj. with `std::size()`
        noexcept(std::data(range)) && noexcept(std::size(range)) && noexcept(std::invoke(violation_policy, "reason")))
        // NOLINTNEXTLINE(score-banned-function) using `std::data()` is valid here since used in conj. with `std::size()`
        : zspan(std::data(range), std::size(range), std::move(violation_policy))
    // NOLINTEND(cppcoreguidelines-pro-type-member-init)
    {
    }

    ///
    /// @brief Constructs a `zspan` as view over null-terminated sequence \p data; with provided \p size;.
    /// @details The provided \p violation_policy; will get invoked in case the sequence is not null-terminated.
    ///
    template <typename ViolationPolicy = violation_policies::default_policy,
              std::enable_if_t<violation_policies::is_valid_one<ViolationPolicy>(), bool> = true>
    constexpr explicit zspan(pointer data, size_type size, ViolationPolicy violation_policy = {}) noexcept(
        noexcept(std::invoke(violation_policy, "reason")))
        : data_{data}, size_{size}
    {
        if ((data_ == nullptr) || (size_ == 0U) || (back() != details::kNullByte))
        {
            data_ = nullptr;
            size_ = 0U;
            std::invoke(violation_policy, "score::safecpp::zspan: provided range is not null-terminated");
        }
        if (size_ > 0U)
        {
            size_ -= 1U;
        }
    }

    ///
    /// @brief Constructs a `zspan` from another `zspan` of compatible type.
    ///
    template <typename U,
              std::enable_if_t<std::conjunction_v<std::negation<std::is_same<U, T>>,
                                                  std::is_convertible<typename zspan<U>::pointer, pointer>>,
                               bool> = true>
    // NOLINTNEXTLINE(google-explicit-constructor) allow implicit conversions from compatible other `zspan`
    constexpr zspan(zspan<U> other) noexcept : data_{other.data()}, size_{other.size()}
    {
    }

    ///
    /// @brief Default constructs a `zspan`.
    ///
    constexpr zspan() noexcept = default;

    ///
    /// @brief Obtains pointer to the null-terminated underlying sequence.
    ///
    [[nodiscard]] constexpr const_pointer data() const noexcept
    {
        return data_;
    }

    ///
    /// @brief Obtains number of `value_type` elements in the view (not accounting the trailing null-terminator).
    ///
    [[nodiscard]] constexpr size_type size() const noexcept
    {
        return size_;
    }

    ///
    /// @brief Returns whether the span's range is empty or not.
    ///
    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return (data_ == nullptr || size_ == 0U);
    }

    ///
    /// @brief Returns a modifiable `element_accessor` to the first element of the span.
    /// @note aborts program execution in case the `zspan` is empty
    ///
    [[nodiscard]] constexpr element_accessor front() noexcept
    {
        if (empty())
#if __cplusplus >= 202002L  // C++20
            [[unlikely]]
#endif
        {
            std::invoke(violation_policies::abort{}, "score::safecpp::zspan::front(): zspan is empty");
        }
        return element_accessor{data_, 0U};
    }

    ///
    /// @brief Returns a non-modifiable `element_accessor` to the first element of the span.
    /// @note aborts program execution in case the `zspan` is empty
    ///
    [[nodiscard]] constexpr const element_accessor front() const noexcept
    {
        if (empty())
#if __cplusplus >= 202002L  // C++20
            [[unlikely]]
#endif
        {
            std::invoke(violation_policies::abort{}, "score::safecpp::zspan::front(): zspan is empty");
        }
        return element_accessor{data_, 0U};
    }

    ///
    /// @brief Returns a modifiable `element_accessor` to the last element of the span.
    /// @note aborts program execution in case the `zspan` is empty
    ///
    [[nodiscard]] constexpr element_accessor back() noexcept
    {
        if (empty())
#if __cplusplus >= 202002L  // C++20
            [[unlikely]]
#endif
        {
            std::invoke(violation_policies::abort{}, "score::safecpp::zspan::back(): zspan is empty");
        }
        return element_accessor{data_, size_ - 1U};
    }

    ///
    /// @brief Returns a non-modifiable `element_accessor` to the last element of the span.
    /// @note aborts program execution in case the `zspan` is empty
    ///
    [[nodiscard]] constexpr const element_accessor back() const noexcept
    {
        if (empty())
#if __cplusplus >= 202002L  // C++20
            [[unlikely]]
#endif
        {
            std::invoke(violation_policies::abort{}, "score::safecpp::zspan::back(): zspan is empty");
        }
        return element_accessor{data_, size_ - 1U};
    }

    ///
    /// @brief element access operator (non-const overload)
    /// @param index index of the element to get accessed
    /// @note throws `std::out_of_range` in case \p index; is out of the `zspan`'s range
    ///
    [[nodiscard]] constexpr element_accessor at(size_type index)
    {
        if (index >= size_)
#if __cplusplus >= 202002L  // C++20
            [[unlikely]]
#endif
        {
            std::invoke(violation_policies::throw_exception<std::out_of_range>{},
                        "score::safecpp::zspan::at(): index out of bounds");
        }
        return element_accessor{data_, index};
    }

    ///
    /// @brief element access operator (const overload)
    /// @param index index of the element to get accessed
    /// @note throws `std::out_of_range` in case \p index; is out of the `zspan`'s range
    ///
    [[nodiscard]] constexpr const element_accessor at(size_type index) const
    {
        if (index >= size_)
#if __cplusplus >= 202002L  // C++20
            [[unlikely]]
#endif
        {
            std::invoke(violation_policies::throw_exception<std::out_of_range>{},
                        "score::safecpp::zspan::at(): index out of bounds");
        }
        return element_accessor{data_, index};
    }

    ///
    /// @brief element access operator (non-const overload)
    /// @param index index of the element to get accessed
    /// @note aborts program execution in case \p index; is out of the `zspan`'s range
    ///
    [[nodiscard]] constexpr element_accessor operator[](size_type index) noexcept
    {
        if (index >= size_)
#if __cplusplus >= 202002L  // C++20
            [[unlikely]]
#endif
        {
            std::invoke(violation_policies::abort{}, "score::safecpp::zspan::operator[]: index out of bounds");
        }
        return element_accessor{data_, index};
    }

    ///
    /// @brief element access operator (const overload)
    /// @param index index of the element to get accessed
    /// @note aborts program execution in case \p index; is out of the `zspan`'s range
    ///
    [[nodiscard]] constexpr const element_accessor operator[](size_type index) const noexcept
    {
        if (index >= size_)
#if __cplusplus >= 202002L  // C++20
            [[unlikely]]
#endif
        {
            std::invoke(violation_policies::abort{}, "score::safecpp::zspan::operator[]: index out of bounds");
        }
        return element_accessor{data_, index};
    }

    /// @brief Disallow implicit conversions back to `std::span` since its `data()` method exposes a non-const pointer.
    /// @details Reason: a non-const pointer to the underlying sequence could get used to overwrite the null-terminator.
#if defined(__cpp_lib_span)
    operator std::span<std::remove_const_t<CharType>>() const = delete;
    operator std::span<std::add_const_t<CharType>>() const = delete;
#endif

  private:
    pointer data_{};
    size_type size_{};
};

}  // namespace score::safecpp::details

// NOLINTEND(readability-identifier-naming)

#endif  // SCORE_LANGUAGE_SAFECPP_STRING_VIEW_ZSPAN_H
