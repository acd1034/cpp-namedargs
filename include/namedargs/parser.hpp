/// @file fundamental.hpp
#pragma once
#include <charconv> // std::from_chars
#include <string>
#include <string_view>
#include <vector>
#include <namedargs/fundamental.hpp>

namespace namedargs {
  enum class TokenKind {
    ident, // Identifiers
    punct, // Punctuators
    num,   // Numeric literals
    eof,   // End-of-file markers
  };

  struct Token {
    TokenKind kind;
    std::int64_t num;
  };

  constexpr bool isspace(char c) {
    constexpr std::string_view space = " \f\n\r\t\v";
    return space.find(c) != std::string_view::npos;
  }

  constexpr bool isdigit(char c) { return '0' <= c and c <= '9'; }

  constexpr std::pair<std::int64_t, const char*>
  from_chars(const char* first, const char* last, std::int64_t num = 0) {
    return first != last and namedargs::isdigit(*first)
             ? namedargs::from_chars(first + 1, last, 10 * num + (*first - '0'))
             : std::make_pair(num, first);
  }

  struct ArgParser {
  private:
    std::string_view input_{};
    std::vector<Token> tokens_{};

  public:
    constexpr explicit ArgParser(std::string_view input)
      : input_(std::move(input)) {}

    constexpr std::string_view skip_whitespaces(std::string_view sv) {
      std::size_t i = 0;
      while (namedargs::isspace(sv[i]))
        ++i;
      return sv.substr(i);
    }

    constexpr std::string_view tokenize_number(std::string_view sv) {
      const char* start = sv.data();
      const auto [num, ptr] = namedargs::from_chars(start, start + sv.size());
      tokens_.push_back({TokenKind::num, num});
      return sv.substr(ptr - start);
    }

    constexpr std::string_view tokenize_impl(std::string_view sv) {
      // Skip whitespace characters.
      sv = skip_whitespaces(sv);

      // Numeric literal
      if (namedargs::isdigit(sv.front()))
        return tokenize_number(sv);
    }

    constexpr std::string_view tokenize() {
      std::string_view sv = input_;
      while (not sv.empty())
        sv = tokenize_impl(sv);
      return sv;
    }
  };
} // namespace namedargs
