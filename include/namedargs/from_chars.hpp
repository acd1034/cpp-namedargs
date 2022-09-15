/// @file from_chars.hpp
// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#pragma once
#include <cassert>
#include <cstddef>
#include <iterator> // std::size
#include <system_error>
#include <type_traits>

namespace namedargs {
  // clang-format off

  // from_chars_result

  struct from_chars_result {
    const char* ptr;
    std::errc ec;
  };

  // _Digit_from_char

  constexpr unsigned char _Digit_from_char(const char _Ch) noexcept {
    // convert ['0', '9'] ['A', 'Z'] ['a', 'z'] to [0, 35], everything else to 255
    constexpr unsigned char _Digit_from_byte[] = {
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   255, 255,
      255, 255, 255, 255, 255, 10,  11,  12,  13,  14,  15,  16,  17,  18,  19,
      20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,
      35,  255, 255, 255, 255, 255, 255, 10,  11,  12,  13,  14,  15,  16,  17,
      18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,
      33,  34,  35,  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255};
    static_assert(std::size(_Digit_from_byte) == 256);

    return _Digit_from_byte[static_cast<unsigned char>(_Ch)];
  }

  // _Integer_from_chars

  template <class _RawTy>
  constexpr from_chars_result
  _Integer_from_chars(const char* const _First, const char* const _Last,
                      _RawTy& _Raw_value, const int _Base) noexcept {
    assert(_First <= _Last);
    assert(_Base >= 2 && _Base <= 36 && "invalid base in from_chars()");

    bool _Minus_sign = false;

    const char* _Next = _First;

    if constexpr (std::is_signed_v<_RawTy>) {
      if (_Next != _Last && *_Next == '-') {
        _Minus_sign = true;
        ++_Next;
      }
    }

    using _Unsigned = std::make_unsigned_t<_RawTy>;

    constexpr _Unsigned _Uint_max = static_cast<_Unsigned>(-1);
    constexpr _Unsigned _Int_max = static_cast<_Unsigned>(_Uint_max >> 1);
    constexpr _Unsigned _Abs_int_min = static_cast<_Unsigned>(_Int_max + 1);

    _Unsigned _Risky_val;
    _Unsigned _Max_digit;

    if constexpr (std::is_signed_v<_RawTy>) {
      if (_Minus_sign) {
        _Risky_val = static_cast<_Unsigned>(_Abs_int_min / _Base);
        _Max_digit = static_cast<_Unsigned>(_Abs_int_min % _Base);
      } else {
        _Risky_val = static_cast<_Unsigned>(_Int_max / _Base);
        _Max_digit = static_cast<_Unsigned>(_Int_max % _Base);
      }
    } else {
      _Risky_val = static_cast<_Unsigned>(_Uint_max / _Base);
      _Max_digit = static_cast<_Unsigned>(_Uint_max % _Base);
    }

    _Unsigned _Value = 0;

    bool _Overflowed = false;

    for (; _Next != _Last; ++_Next) {
      const unsigned char _Digit = _Digit_from_char(*_Next);

      if (_Digit >= _Base) {
        break;
      }

      if (_Value < _Risky_val // never overflows
          || (_Value == _Risky_val && _Digit <= _Max_digit)) { // overflows for certain digits
        _Value = static_cast<_Unsigned>(_Value * _Base + _Digit);
      } else { // _Value > _Risky_val always overflows
        _Overflowed = true; // keep going, _Next still needs to be updated, _Value is now irrelevant
      }
    }

    if (_Next - _First == static_cast<std::ptrdiff_t>(_Minus_sign)) {
      return {_First, std::errc::invalid_argument};
    }

    if (_Overflowed) {
      return {_Next, std::errc::result_out_of_range};
    }

    if constexpr (std::is_signed_v<_RawTy>) {
      if (_Minus_sign) {
        _Value = static_cast<_Unsigned>(0 - _Value);
      }
    }

    _Raw_value = static_cast<_RawTy>(_Value); // implementation-defined for negative, N4713 7.8 [conv.integral]/3

    return {_Next, std::errc{}};
  }

  // from_chars

  template <class _Ty>
  constexpr from_chars_result from_chars(const char* const _First,
                                         const char* const _Last, _Ty& _Value,
                                         const int _Base = 10) noexcept {
    return _Integer_from_chars(_First, _Last, _Value, _Base);
  }

  from_chars_result from_chars(const char* _First, const char* _Last,
                               bool& _Value, const int _Base = 10) = delete;

  // clang-format on
} // namespace namedargs
