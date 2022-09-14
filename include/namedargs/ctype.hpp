/// @file ctype.hpp
#pragma once

namespace namedargs {
  constexpr bool isspace(char c) {
    return ('\t' <= c and c <= '\r') or c == ' ';
  }

  constexpr bool isdigit(char c) { return '0' <= c and c <= '9'; }

  constexpr bool isupper(char c) { return 'A' <= c and c <= 'Z'; }

  constexpr bool islower(char c) { return 'a' <= c and c <= 'z'; }

  constexpr bool isident1(char c) {
    return isupper(c) or islower(c) or c == '_';
  }

  constexpr bool isident2(char c) { return isident1(c) or isdigit(c); }

  constexpr bool ispunct(char c) {
    return ('!' <= c and c <= '/') or (':' <= c and c <= '@')
           or ('[' <= c and c <= '`') or ('{' <= c and c <= '~');
  }
} // namespace namedargs
