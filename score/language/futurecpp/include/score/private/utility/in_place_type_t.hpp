/********************************************************************************
 * Copyright (c) 2022 Contributors to the Eclipse Foundation
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

///
/// \file
/// \copyright Copyright (c) 2022 Contributors to the Eclipse Foundation
///

// IWYU pragma: private

#ifndef SCORE_LANGUAGE_FUTURECPP_PRIVATE_UTILITY_IN_PLACE_TYPE_T_HPP
#define SCORE_LANGUAGE_FUTURECPP_PRIVATE_UTILITY_IN_PLACE_TYPE_T_HPP

#include <cstddef>

namespace score::cpp
{

/// Dispatch type used to construct a variant or any using direct-initialization.
template <typename T>
struct in_place_type_t
{
    explicit in_place_type_t() = default;
};

/// Instance of \a in_place_type_t for use with \a variant or any.
template <typename T>
constexpr in_place_type_t<T> in_place_type{};

/// Dispatch type used to construct a variant using index-based initialization.
template <std::size_t I>
struct in_place_index_t
{
    explicit in_place_index_t() = default;
};

/// Instance of \a in_place_index_t for use with \a variant.
template <std::size_t I>
constexpr in_place_index_t<I> in_place_index{};

} // namespace score::cpp

#endif // SCORE_LANGUAGE_FUTURECPP_PRIVATE_UTILITY_IN_PLACE_TYPE_T_HPP
