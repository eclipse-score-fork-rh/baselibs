///
/// \file
/// \copyright Copyright (C) 2015-2020, Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
///

// IWYU pragma: private

#ifndef SCORE_LANGUAGE_FUTURECPP_PRIVATE_TYPE_TRAITS_IS_SPAN_HPP
#define SCORE_LANGUAGE_FUTURECPP_PRIVATE_TYPE_TRAITS_IS_SPAN_HPP

#include <type_traits>

namespace score::cpp
{

template <typename T>
class span;

/// \brief Check whether the type is a span type
/// \tparam T: type to be checked
template <typename T>
struct is_span : std::false_type
{
};

/// \brief Check whether the type is a span type
/// \tparam T: type to be checked
template <typename T>
struct is_span<span<T>> : std::true_type
{
};

} // namespace score::cpp

#endif // SCORE_LANGUAGE_FUTURECPP_PRIVATE_TYPE_TRAITS_IS_SPAN_HPP
