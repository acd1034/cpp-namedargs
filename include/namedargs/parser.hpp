/// @file fundamental.hpp
#pragma once
#include <functional> // std::invoke
#include <stdexcept>  // std::runtime_error
#include <string>
#include <string_view>
#include <vector>
#include <namedargs/fundamental.hpp>

namespace namedargs {
  enum class TokenKind {
    punct, // Punctuators
    num,   // Numeric literals
    str,   // String literals
    ident, // Identifiers
    eof,   // End-of-file markers
  };

  struct Token {
    TokenKind kind;
    std::string_view sv;
    std::int64_t num; // Used if TokenKind::num
  };

  struct parse_error : std::runtime_error {
    explicit parse_error(const std::string& msg) : std::runtime_error(msg) {}
    explicit parse_error(const char* msg) : std::runtime_error(msg) {}
    parse_error(const parse_error&) noexcept = default;
    ~parse_error() noexcept override = default;
  };

  constexpr bool isspace(char c) {
    constexpr std::string_view space = " \f\n\r\t\v";
    return space.find(c) != std::string_view::npos;
  }

  constexpr bool isdigit(char c) { return '0' <= c and c <= '9'; }

  constexpr bool isident1(char c) {
    return ('a' <= c and c <= 'z') or ('A' <= c and c <= 'Z') or c == '_';
  }

  constexpr bool isident2(char c) {
    return isident1(c) or ('0' <= c and c <= '9');
  }

  template <class Pred>
  constexpr std::size_t find_if_not(std::string_view sv, Pred pred,
                                    std::size_t pos = 0) {
    for (; pos < sv.size(); ++pos)
      if (not std::invoke(pred, sv[pos]))
        return pos;
    return std::string_view::npos;
  }

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
      // TODO: throw parse_error
      const std::size_t size = icast<std::size_t>(ptr - start);
      tokens_.push_back({TokenKind::num, sv.substr(0, size), num});
      return sv.substr(size);
    }

    constexpr std::string_view tokenize_string_literal(std::string_view sv) {
      const std::size_t pos = sv.find_first_not_of('\'', 1);
      if (pos == std::string_view::npos)
        throw parse_error("unclosed string literal");
      tokens_.push_back({TokenKind::str, sv.substr(1, pos), {}});
      return sv.substr(pos + 1);
    }

    constexpr std::string_view tokenize_identifier(std::string_view sv) {
      const std::size_t pos = find_if_not(sv, isident2, 1);
      tokens_.push_back({TokenKind::ident, sv.substr(0, pos), {}});
      return sv.substr(pos);
    }

    constexpr std::string_view tokenize() {
      std::string_view sv = input_;
      while (not sv.empty()) {
        // Skip whitespace characters.
        sv = skip_whitespaces(sv);

        // Numeric literal
        if (namedargs::isdigit(sv.front())) {
          sv = tokenize_number(sv);
          continue;
        }

        // String literal
        if (sv.starts_with('\'')) {
          sv = tokenize_string_literal(sv);
          continue;
        }

        // Identifier
        if (isident1(sv.front())) {
          sv = tokenize_identifier(sv);
          continue;
        }
      }
      return sv;
    }
  };
} // namespace namedargs
