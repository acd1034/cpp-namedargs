/// @file fundamental.hpp
#pragma once
#include <cassert>
#include <compare>
#include <concepts>
#include <cstddef> // std::size_t, std::ptrdiff_t, std::nullptr_t
#include <cstdint> // std::int32_t
#include <initializer_list>
#include <tuple>
#include <type_traits>
#include <utility> // std::move, std::forward, std::swap, std::exchange

namespace namedargs {
  /// squared
  template <class X>
  constexpr auto squared(const X& x) noexcept(noexcept(x* x)) -> decltype(x * x) {
    return x * x;
  }

  /// similar_to
  template <class T, class U>
  concept similar_to = std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;
} // namespace namedargs
